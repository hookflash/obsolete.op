/*

 Copyright (c) 2013, SMB Phone Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 The views and conclusions contained in the software and documentation are those
 of the authors and should not be interpreted as representing official policies,
 either expressed or implied, of the FreeBSD Project.

 */

#include <openpeer/services/internal/services_RUDPMessaging.h>
#include <openpeer/services/internal/services_Helper.h>

#include <openpeer/services/IRUDPListener.h>
#include <openpeer/services/IRUDPICESocketSession.h>

#include <cryptopp/queue.h>

#include <zsLib/Exception.h>
#include <zsLib/helpers.h>
#include <zsLib/Log.h>
#include <zsLib/Stringize.h>

#define OPENPEER_SERVICES_RUDPMESSAGING_RECYCLE_BUFFER_SIZE ((1 << (sizeof(WORD)*8)) + sizeof(DWORD))
#define OPENPEER_SERVICES_RUDPMESSAGING_MAX_RECYLCE_BUFFERS (100)

namespace openpeer { namespace services { ZS_DECLARE_SUBSYSTEM(openpeer_services) } }

namespace openpeer
{
  namespace services
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPMessaging => IRUDPMessaging
      #pragma mark

      //-----------------------------------------------------------------------
      RUDPMessaging::RUDPMessaging(
                                   IMessageQueuePtr queue,
                                   IRUDPMessagingDelegatePtr delegate,
                                   ITransportStreamPtr receiveStream,
                                   ITransportStreamPtr sendStream,
                                   ULONG maxMessageSizeInBytes
                                   ) :
        MessageQueueAssociator(queue),
        mCurrentState(RUDPMessagingState_Connecting),
        mDelegate(IRUDPMessagingDelegateProxy::createWeak(queue, delegate)),
        mNulTerminateBuffers(true),
        mMaxMessageSizeInBytes(maxMessageSizeInBytes),
        mOuterReceiveStream(receiveStream->getWriter()),
        mOuterSendStream(sendStream->getReader()),
        mWireReceiveStream(ITransportStream::create()->getReader()),
        mWireSendStream(ITransportStream::create()->getWriter())
      {
        ZS_LOG_BASIC(log("created"))
      }

      //-----------------------------------------------------------------------
      void RUDPMessaging::init()
      {
        AutoRecursiveLock lock(mLock);
        mWireReceiveStreamSubscription = mWireReceiveStream->subscribe(mThisWeak.lock());
        mWireSendStreamSubscription = mWireSendStream->subscribe(mThisWeak.lock());

        mOuterReceiveStreamSubscription = mOuterReceiveStream->subscribe(mThisWeak.lock());
        mOuterSendStreamSubscription = mOuterSendStream->subscribe(mThisWeak.lock());
      }

      //-----------------------------------------------------------------------
      RUDPMessaging::~RUDPMessaging()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      RUDPMessagingPtr RUDPMessaging::convert(IRUDPMessagingPtr messaging)
      {
        return boost::dynamic_pointer_cast<RUDPMessaging>(messaging);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPMessaging => IRUDPMessaging
      #pragma mark

      //-----------------------------------------------------------------------
      String RUDPMessaging::toDebugString(IRUDPMessagingPtr messaging, bool includeCommaPrefix)
      {
        if (!messaging) return String(includeCommaPrefix ? ", rudp channel stream=(null)" : "rudp channel stream=(null)");

        RUDPMessagingPtr pThis = RUDPMessaging::convert(messaging);
        return pThis->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      RUDPMessagingPtr RUDPMessaging::acceptChannel(
                                                    IMessageQueuePtr queue,
                                                    IRUDPListenerPtr listener,
                                                    IRUDPMessagingDelegatePtr delegate,
                                                    ITransportStreamPtr receiveStream,
                                                    ITransportStreamPtr sendStream,
                                                    ULONG maxMessageSizeInBytes
                                                    )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!listener)
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!receiveStream)
        ZS_THROW_INVALID_ARGUMENT_IF(!sendStream)

        RUDPMessagingPtr pThis(new RUDPMessaging(queue, delegate, receiveStream, sendStream, maxMessageSizeInBytes));
        pThis->mThisWeak = pThis;
        pThis->mChannel = listener->acceptChannel(pThis, pThis->mWireReceiveStream->getStream(), pThis->mWireSendStream->getStream());
        pThis->init();
        if (!pThis->mChannel) {
          ZS_LOG_ERROR(Detail, pThis->log("listener failed to accept channel"))
          pThis->setError(RUDPMessagingShutdownReason_OpenFailure, "channel accept failure");
          pThis->cancel();
          return RUDPMessagingPtr();
        }
        ZS_LOG_DEBUG(pThis->log("listener channel accepted"))
        return pThis;
      }

      //-----------------------------------------------------------------------
      RUDPMessagingPtr RUDPMessaging::acceptChannel(
                                                    IMessageQueuePtr queue,
                                                    IRUDPICESocketSessionPtr session,
                                                    IRUDPMessagingDelegatePtr delegate,
                                                    ITransportStreamPtr receiveStream,
                                                    ITransportStreamPtr sendStream,
                                                    ULONG maxMessageSizeInBytes
                                                    )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!session)
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!receiveStream)
        ZS_THROW_INVALID_ARGUMENT_IF(!sendStream)

        RUDPMessagingPtr pThis(new RUDPMessaging(queue, delegate, receiveStream, sendStream, maxMessageSizeInBytes));
        pThis->mThisWeak = pThis;
        pThis->mChannel = session->acceptChannel(pThis, pThis->mWireReceiveStream->getStream(), pThis->mWireSendStream->getStream());
        pThis->init();
        if (!pThis->mChannel) {
          ZS_LOG_ERROR(Detail, pThis->log("session failed to accept channel"))
          pThis->setError(RUDPMessagingShutdownReason_OpenFailure, "channel accept failure");
          pThis->cancel();
          return RUDPMessagingPtr();
        }
        ZS_LOG_DEBUG(pThis->log("session channel accepted"))
        return pThis;
      }

      //-----------------------------------------------------------------------
      RUDPMessagingPtr RUDPMessaging::openChannel(
                                                  IMessageQueuePtr queue,
                                                  IRUDPICESocketSessionPtr session,
                                                  IRUDPMessagingDelegatePtr delegate,
                                                  const char *connectionInfo,
                                                  ITransportStreamPtr receiveStream,
                                                  ITransportStreamPtr sendStream,
                                                  ULONG maxMessageSizeInBytes
                                                  )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!session)
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!receiveStream)
        ZS_THROW_INVALID_ARGUMENT_IF(!sendStream)

        RUDPMessagingPtr pThis(new RUDPMessaging(queue, delegate, receiveStream, sendStream, maxMessageSizeInBytes));
        pThis->mThisWeak = pThis;
        pThis->mChannel = session->openChannel(pThis, connectionInfo, pThis->mWireReceiveStream->getStream(), pThis->mWireSendStream->getStream());
        pThis->init();
        if (!pThis->mChannel) {
          ZS_LOG_ERROR(Detail, pThis->log("session failed to open channel"))
          pThis->setError(RUDPMessagingShutdownReason_OpenFailure, "channel open failure");
          pThis->cancel();
          return RUDPMessagingPtr();
        }
        ZS_LOG_DEBUG(pThis->log("session channel openned"))
        return pThis;
      }

      //-----------------------------------------------------------------------
      IRUDPMessaging::RUDPMessagingStates RUDPMessaging::getState(
                                                                  WORD *outLastErrorCode,
                                                                  String *outLastErrorReason
                                                                  ) const
      {
        AutoRecursiveLock lock(mLock);
        if (outLastErrorCode) *outLastErrorCode = mLastError;
        if (outLastErrorReason) *outLastErrorReason = mLastErrorReason;
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      void RUDPMessaging::shutdown()
      {
        AutoRecursiveLock lock(mLock);
        cancel();
      }

      //-----------------------------------------------------------------------
      void RUDPMessaging::shutdownDirection(Shutdown state)
      {
        IRUDPChannelPtr channel = getChannel();
        if (!channel) return;
        channel->shutdownDirection(state);
      }

      //-----------------------------------------------------------------------
      void RUDPMessaging::setMaxMessageSizeInBytes(ULONG maxMessageSizeInBytes)
      {
        AutoRecursiveLock lock(mLock);
        mMaxMessageSizeInBytes = maxMessageSizeInBytes;
      }

      //-----------------------------------------------------------------------
      void RUDPMessaging::setAutoNulTerminateReceiveBuffers(bool nulTerminate)
      {
        AutoRecursiveLock lock(mLock);
        mNulTerminateBuffers = nulTerminate;
      }

      //-----------------------------------------------------------------------
      IPAddress RUDPMessaging::getConnectedRemoteIP()
      {
        IRUDPChannelPtr channel = getChannel();
        if (!channel) return IPAddress();
        return channel->getConnectedRemoteIP();
      }

      //-----------------------------------------------------------------------
      String RUDPMessaging::getRemoteConnectionInfo()
      {
        IRUDPChannelPtr channel = getChannel();
        if (!channel) return String();
        return channel->getRemoteConnectionInfo();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPMessaging => IRUDPChannelDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void RUDPMessaging::onRDUPChannelStateChanged(
                                                    IRUDPChannelPtr session,
                                                    RUDPChannelStates state
                                                    )
      {
        AutoRecursiveLock lock(mLock);
        ZS_LOG_DEBUG(log("notified of channel state change"))

        if (session != mChannel) {
          ZS_LOG_WARNING(Debug, log("notified of channel state change for obsolete channel thus ignoring"))
          return;
        }

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("notified of channel state change but already shutdown thus ignoring"))
          return;
        }

        switch (state) {
          case IRUDPChannel::RUDPChannelState_Connecting: break;
          case IRUDPChannel::RUDPChannelState_Connected:  {
            if (isShuttingDown()) return;
            setState(RUDPMessagingState_Connected);
            break;
          }
          case IRUDPChannel::RUDPChannelState_ShuttingDown:
          case IRUDPChannel::RUDPChannelState_Shutdown:
          {
            WORD errorCode = 0;
            String reason;
            mChannel->getState(&errorCode, &reason);
            if (0 != errorCode) {
              setError(errorCode, reason);
            }
            if (IRUDPChannel::RUDPChannelState_Shutdown == state) {
              mChannel.reset();
            }
            cancel();
            break;
          }
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPMessaging => ITransportStreamWriterDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void RUDPMessaging::onTransportStreamWriterReady(ITransportStreamWriterPtr writer)
      {
        AutoRecursiveLock lock(mLock);
        if (writer == mOuterReceiveStream) {
          ZS_LOG_TRACE(log("on transport stream outer receive ready"))
          mWireReceiveStream->notifyReaderReadyToRead();
          get(mInformedOuterReceiveReady) = true;
        } else if (writer == mWireSendStream) {
          ZS_LOG_TRACE(log("on transport stream wire send ready"))
          mOuterSendStream->notifyReaderReadyToRead();
          get(mInformedWireSendReady) = true;
        }
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPMessaging => ITransportStreamReaderDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void RUDPMessaging::onTransportStreamReaderReady(ITransportStreamReaderPtr reader)
      {
        AutoRecursiveLock lock(mLock);
        ZS_LOG_TRACE(log("on transport stream reader ready"))
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPMessaging => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String RUDPMessaging::log(const char *message) const
      {
        return String("RUDPMessaging [") + string(mID) + "] " + message;
      }

      //-----------------------------------------------------------------------
      String RUDPMessaging::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(mLock);
        bool firstTime = !includeCommaPrefix;
        return

        Helper::getDebugValue("rudp messaging ID", string(mID), firstTime) +

        Helper::getDebugValue("state", IRUDPMessaging::toString(mCurrentState), firstTime) +
        Helper::getDebugValue("last error", 0 != mLastError ? string(mLastError) : String(), firstTime) +
        Helper::getDebugValue("last reason", mLastErrorReason, firstTime) +

        Helper::getDebugValue("delegate", mDelegate ? String("true") : String(), firstTime) +

        Helper::getDebugValue("nul terminate", mNulTerminateBuffers ? String("true") : String(), firstTime) +

        Helper::getDebugValue("outer receive stream", mOuterReceiveStream ? String("true") : String(), firstTime) +
        Helper::getDebugValue("outer send stream", mOuterSendStream ? String("true") : String(), firstTime) +

        Helper::getDebugValue("wire receive stream", mWireReceiveStream ? String("true") : String(), firstTime) +
        Helper::getDebugValue("wire send stream", mWireSendStream ? String("true") : String(), firstTime) +

        Helper::getDebugValue("outer receive stream subscription", mOuterReceiveStreamSubscription ? String("true") : String(), firstTime) +
        Helper::getDebugValue("outer send stream subscription", mOuterSendStreamSubscription ? String("true") : String(), firstTime) +

        Helper::getDebugValue("wire receive stream subscription", mWireReceiveStreamSubscription ? String("true") : String(), firstTime) +
        Helper::getDebugValue("wire send stream subscription", mWireSendStreamSubscription ? String("true") : String(), firstTime) +

        Helper::getDebugValue("informed outer receive ready", mInformedOuterReceiveReady ? String("true") : String(), firstTime) +
        Helper::getDebugValue("informed wire send ready", mInformedWireSendReady ? String("true") : String(), firstTime) +

        Helper::getDebugValue("graceful shutdown reference", mGracefulShutdownReference ? String("true") : String(), firstTime) +

        Helper::getDebugValue("channel", mChannel ? String("true") : String(), firstTime) +

        Helper::getDebugValue("next message size (bytes)", 0 != mNextMessageSizeInBytes ? string(mNextMessageSizeInBytes) : String(), firstTime) +

        Helper::getDebugValue("max message size (bytes)", 0 != mMaxMessageSizeInBytes ? string(mNextMessageSizeInBytes) : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      void RUDPMessaging::step()
      {
        if (isShutdown()) {
          ZS_LOG_DEBUG(log("step forwarding to cancel"))
          return;
        }

        ZS_LOG_DEBUG(log("step"))

        if (!stepSendData()) return;
        if (!stepReceiveData()) return;

        ZS_LOG_TRACE(log("step complete"))
      }

      //-----------------------------------------------------------------------
      bool RUDPMessaging::stepSendData()
      {
        if (!mInformedWireSendReady) {
          ZS_LOG_TRACE(log("wire has not informed it's ready to send data"))
          return true;
        }

        while (mOuterSendStream->getTotalReadBuffersAvailable() > 0) {
          SecureByteBlockPtr message = mOuterSendStream->read();

          SecureByteBlockPtr buffer(new SecureByteBlock(message->SizeInBytes() + sizeof(DWORD)));

          // put the size of the message at the front
          BYTE *dest = buffer->BytePtr();
          ((DWORD *)dest)[0] = htonl(message->SizeInBytes());
          memcpy(&(dest[sizeof(DWORD)]), message->BytePtr(), message->SizeInBytes());

          ZS_LOG_TRACE(log("sending buffer") + ", message size=" + string(message->SizeInBytes()))
          mWireSendStream->write(buffer);
        }

        return true;
      }

      //-----------------------------------------------------------------------
      bool RUDPMessaging::stepReceiveData()
      {
        if (!mInformedOuterReceiveReady) {
          ZS_LOG_TRACE(log("outer has not informed it's ready to receive data"))
          return true;
        }

        // read all data available

        while (mWireReceiveStream->getTotalReadBuffersAvailable() > 0) {

          DWORD bufferSize = 0;

          ULONG read = mWireReceiveStream->peek(bufferSize);
          if (read != sizeof(bufferSize)) {
            ZS_LOG_TRACE(log("not enough data available to read"))
            break;
          }

          ULONG available = mWireReceiveStream->getTotalReadSizeAvailableInBytes();

          if (available < sizeof(DWORD) + bufferSize) {
            ZS_LOG_TRACE(log("not enough data available to read") + ", available=" + string(available) + ", buffer size=" + string(bufferSize))
            break;
          }

          mWireReceiveStream->skip(sizeof(DWORD));

          SecureByteBlockPtr message(new SecureByteBlock);
          message->CleanNew(bufferSize + (mNulTerminateBuffers ? sizeof(char) : 0));
          if (bufferSize > 0) {
            mWireReceiveStream->read(message->BytePtr(), bufferSize);
          }

          ZS_LOG_TRACE(log("message is read") + ", size=" + string(bufferSize))

          if (bufferSize > 0) {
            mOuterReceiveStream->write(message);
          }
        }

        return true;
      }

      //-----------------------------------------------------------------------
      void RUDPMessaging::cancel()
      {
        AutoRecursiveLock lock(mLock);  // just in case

        if (isShutdown()) return;

        setState(RUDPMessagingState_ShuttingDown);

        if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

        if (mChannel) {
          mChannel->shutdown();
        }

        if (mGracefulShutdownReference) {
          ZS_LOG_DEBUG(log("shutting down gracefully"))

          if (mChannel) {
            if (IRUDPChannel::RUDPChannelState_Shutdown != mChannel->getState()) {
              // channel is not ready to shutdown just yet
              ZS_LOG_DEBUG(log("waiting for RUDP channel to shutdown"))
              return;
            }
          }
        }

        setState(RUDPMessagingState_Shutdown);

        mDelegate.reset();
        mGracefulShutdownReference.reset();

        mChannel.reset();

        get(mNextMessageSizeInBytes) = 0;
      }

      //-----------------------------------------------------------------------
      void RUDPMessaging::setState(RUDPMessagingStates state)
      {
        if (state == mCurrentState) return;
        ZS_LOG_BASIC(log("state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state))

        mCurrentState = state;

        if (!mDelegate) return;

        RUDPMessagingPtr pThis = mThisWeak.lock();

        if (pThis) {
          try {
            mDelegate->onRUDPMessagingStateChanged(mThisWeak.lock(), state);
          } catch(IRUDPMessagingDelegateProxy::Exceptions::DelegateGone &) {
          }
        }
      }

      //-----------------------------------------------------------------------
      void RUDPMessaging::setError(WORD errorCode, const char *inReason)
      {
        String reason(inReason);
        if (reason.isEmpty()) {
          reason = IHTTP::toString(IHTTP::toStatusCode(errorCode));
        }

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_WARNING(Detail, log("already shutting down thus ignoring new error") + ", new error=" + string(errorCode) + ", new reason=" + reason + getDebugValueString())
          return;
        }

        if (0 != mLastError) {
          ZS_LOG_WARNING(Detail, log("error already set thus ignoring new error") + ", new error=" + string(errorCode) + ", new reason=" + reason + getDebugValueString())
          return;
        }

        get(mLastError) = errorCode;
        mLastErrorReason = reason;

        ZS_LOG_WARNING(Detail, log("error set") + ", code=" + string(mLastError) + ", reason=" + mLastErrorReason + getDebugValueString())
      }

      //-----------------------------------------------------------------------
      IRUDPChannelPtr RUDPMessaging::getChannel() const
      {
        AutoRecursiveLock lock(mLock);
        return mChannel;
      }

    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IRUDPMessaging
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IRUDPMessaging::toString(RUDPMessagingStates state)
    {
      switch (state) {
        case IRUDPMessaging::RUDPMessagingState_Connecting:   return "Connecting";
        case IRUDPMessaging::RUDPMessagingState_Connected:    return "Connected";
        case IRUDPMessaging::RUDPMessagingState_ShuttingDown: return "Shutting down";
        case IRUDPMessaging::RUDPMessagingState_Shutdown:     return "Shutdown";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *IRUDPMessaging::toString(RUDPMessagingShutdownReasons reason)
    {
      return IRUDPChannel::toString((IRUDPChannel::RUDPChannelShutdownReasons)reason);
    }

    //-------------------------------------------------------------------------
    String IRUDPMessaging::toDebugString(IRUDPMessagingPtr messaging, bool includeCommaPrefix)
    {
      return internal::RUDPMessaging::toDebugString(messaging, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    IRUDPMessagingPtr IRUDPMessaging::acceptChannel(
                                                    IMessageQueuePtr queue,
                                                    IRUDPListenerPtr listener,
                                                    IRUDPMessagingDelegatePtr delegate,
                                                    ITransportStreamPtr receiveStream,
                                                    ITransportStreamPtr sendStream,
                                                    ULONG maxMessageSizeInBytes
                                                    )
    {
      return internal::IRUDPMessagingFactory::singleton().acceptChannel(queue, listener, delegate, receiveStream, sendStream, maxMessageSizeInBytes);
    }

    //-------------------------------------------------------------------------
    IRUDPMessagingPtr IRUDPMessaging::acceptChannel(
                                                    IMessageQueuePtr queue,
                                                    IRUDPICESocketSessionPtr session,
                                                    IRUDPMessagingDelegatePtr delegate,
                                                    ITransportStreamPtr receiveStream,
                                                    ITransportStreamPtr sendStream,
                                                    ULONG maxMessageSizeInBytes
                                                    )
    {
      return internal::IRUDPMessagingFactory::singleton().acceptChannel(queue, session, delegate, receiveStream, sendStream, maxMessageSizeInBytes);
    }

    //-------------------------------------------------------------------------
    IRUDPMessagingPtr IRUDPMessaging::openChannel(
                                                  IMessageQueuePtr queue,
                                                  IRUDPICESocketSessionPtr session,
                                                  IRUDPMessagingDelegatePtr delegate,
                                                  const char *connectionInfo,
                                                  ITransportStreamPtr receiveStream,
                                                  ITransportStreamPtr sendStream,
                                                  ULONG maxMessageSizeInBytes
                                                  )
    {
      return internal::IRUDPMessagingFactory::singleton().openChannel(queue, session, delegate, connectionInfo, receiveStream, sendStream, maxMessageSizeInBytes);
    }
  }
}
