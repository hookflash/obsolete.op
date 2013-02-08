/*
 
 Copyright (c) 2012, SMB Phone Inc.
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

#include <hookflash/services/internal/services_RUDPMessaging.h>
#include <hookflash/services/IRUDPListener.h>
#include <hookflash/services/IRUDPICESocketSession.h>
#include <zsLib/Exception.h>
#include <zsLib/zsHelpers.h>
#include <zsLib/Log.h>
#include <zsLib/Stringize.h>

#define HOOKFLASH_SERVICES_RUDPMESSAGING_RECYCLE_BUFFER_SIZE ((1 << (sizeof(WORD)*8)) + sizeof(DWORD))
#define HOOKFLASH_SERVICES_RUDPMESSAGING_MAX_RECYLCE_BUFFERS (100)

namespace hookflash { namespace services { ZS_DECLARE_SUBSYSTEM(hookflash_services) } }

namespace hookflash
{
  namespace services
  {
    namespace internal
    {
      using zsLib::Stringize;

      typedef zsLib::WORD WORD;
      typedef zsLib::ULONG ULONG;
      typedef zsLib::String String;
      typedef zsLib::IPAddress IPAddress;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef RUDPMessaging::MessageBuffer MessageBuffer;

      //-----------------------------------------------------------------------
      RUDPMessaging::RUDPMessaging(
                                   IMessageQueuePtr queue,
                                   IRUDPMessagingDelegatePtr delegate,
                                   ULONG maxMessageSizeInBytes
                                   ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(IRUDPMessagingDelegateProxy::createWeak(queue, delegate)),
        mCurrentState(RUDPMessagingState_Connecting),
        mShutdownReason(RUDPMessagingShutdownReason_None),
        mMaxMessageSizeInBytes(maxMessageSizeInBytes),
        mNextMessageSizeInBytes(0),
        mInformedReadReady(false),
        mInformedWriteReady(false)
      {
        ZS_LOG_BASIC(log("created"))
      }

      //-------------------------------------------------------------------------
      void RUDPMessaging::init()
      {
      }

      //-------------------------------------------------------------------------
      RUDPMessaging::~RUDPMessaging()
      {
        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //-------------------------------------------------------------------------
      RUDPMessagingPtr RUDPMessaging::acceptChannel(
                                                    IMessageQueuePtr queue,
                                                    IRUDPListenerPtr listener,
                                                    IRUDPMessagingDelegatePtr delegate,
                                                    ULONG maxMessageSizeInBytes
                                                    )
      {
        ZS_THROW_INVALID_USAGE_IF(!listener)
        ZS_THROW_INVALID_USAGE_IF(!delegate)
        RUDPMessagingPtr pThis(new RUDPMessaging(queue, delegate, maxMessageSizeInBytes));
        pThis->mThisWeak = pThis;
        pThis->mChannel = listener->acceptChannel(pThis);
        pThis->init();
        if (!pThis->mChannel) {
          ZS_LOG_ERROR(Detail, pThis->log("listener failed to accept channel"))
          pThis->setShutdownReason(RUDPMessagingShutdownReason_OpenFailure);
          pThis->cancel();
          return RUDPMessagingPtr();
        }
        ZS_LOG_DEBUG(pThis->log("listener channel accepted"))
        return pThis;
      }

      //-------------------------------------------------------------------------
      RUDPMessagingPtr RUDPMessaging::acceptChannel(
                                                    IMessageQueuePtr queue,
                                                    IRUDPICESocketSessionPtr session,
                                                    IRUDPMessagingDelegatePtr delegate,
                                                    ULONG maxMessageSizeInBytes
                                                    )
      {
        ZS_THROW_INVALID_USAGE_IF(!session)
        ZS_THROW_INVALID_USAGE_IF(!delegate)
        RUDPMessagingPtr pThis(new RUDPMessaging(queue, delegate, maxMessageSizeInBytes));
        pThis->mThisWeak = pThis;
        pThis->mChannel = session->acceptChannel(pThis);
        pThis->init();
        if (!pThis->mChannel) {
          ZS_LOG_ERROR(Detail, pThis->log("session failed to accept channel"))
          pThis->setShutdownReason(RUDPMessagingShutdownReason_OpenFailure);
          pThis->cancel();
          return RUDPMessagingPtr();
        }
        ZS_LOG_DEBUG(pThis->log("session channel accepted"))
        return pThis;
      }

      //-------------------------------------------------------------------------
      RUDPMessagingPtr RUDPMessaging::openChannel(
                                                  IMessageQueuePtr queue,
                                                  IRUDPICESocketSessionPtr session,
                                                  IRUDPMessagingDelegatePtr delegate,
                                                  const char *connectionInfo,
                                                  ULONG maxMessageSizeInBytes
                                                  )
      {
        ZS_THROW_INVALID_USAGE_IF(!session)
        ZS_THROW_INVALID_USAGE_IF(!delegate)
        RUDPMessagingPtr pThis(new RUDPMessaging(queue, delegate, maxMessageSizeInBytes));
        pThis->mThisWeak = pThis;
        pThis->mChannel = session->openChannel(pThis, connectionInfo);
        pThis->init();
        if (!pThis->mChannel) {
          ZS_LOG_ERROR(Detail, pThis->log("session failed to open channel"))
          pThis->setShutdownReason(RUDPMessagingShutdownReason_OpenFailure);
          pThis->cancel();
          return RUDPMessagingPtr();
        }
        ZS_LOG_DEBUG(pThis->log("session channel openned"))
        return pThis;
      }

      //-----------------------------------------------------------------------
      IRUDPMessaging::RUDPMessagingStates RUDPMessaging::getState() const
      {
        AutoRecursiveLock lock(mLock);
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      IRUDPMessaging::RUDPMessagingShutdownReasons RUDPMessaging::getShutdownReason() const
      {
        AutoRecursiveLock lock(mLock);
        if (RUDPMessagingShutdownReason_None != mShutdownReason) return mShutdownReason;
        if (mChannel) {
          IRUDPChannel::RUDPChannelShutdownReasons reason = mChannel->getShutdownReason();
          if (IRUDPChannel::RUDPChannelShutdownReason_None != reason) {
            RUDPMessaging *pThis = const_cast<RUDPMessaging *>(this);
            pThis->mShutdownReason = ((RUDPMessagingShutdownReasons)reason);
          }
        }
        return mShutdownReason;
      }

      //-------------------------------------------------------------------------
      void RUDPMessaging::shutdown()
      {
        AutoRecursiveLock lock(mLock);
        setShutdownReason(RUDPMessagingShutdownReason_Closed);
        cancel();
      }

      //-------------------------------------------------------------------------
      void RUDPMessaging::shutdownDirection(Shutdown state)
      {
        IRUDPChannelPtr channel = getChannel();
        if (!channel) return;
        channel->shutdownDirection(state);
      }

      //-------------------------------------------------------------------------
      bool RUDPMessaging::send(
                               const BYTE *message,
                               ULONG messsageLengthInBytes
                               )
      {
        {
          AutoRecursiveLock lock(mLock);
          mInformedWriteReady = false;  // if the send was called in response to a write-ready event then the write-ready flag must be cleared so the event can be fired again
          ZS_LOG_DEBUG(log("send called") + ", message length=" + Stringize<ULONG>(messsageLengthInBytes).string())
        }

        if ((NULL == message) ||
            (0 == messsageLengthInBytes)) return true;

        IRUDPChannelPtr channel = getChannel();
        if (!channel) return false;

        boost::shared_array<BYTE> recycleBuffer;
        boost::shared_array<BYTE> buffer;

        AutoRecycleBuffer recycle(*this, recycleBuffer);

        if (messsageLengthInBytes < HOOKFLASH_SERVICES_RUDPMESSAGING_RECYCLE_BUFFER_SIZE - sizeof(DWORD)) {
          // this buffer can fit inside the recycle buffer, great!
          getBuffer(buffer);
          recycleBuffer = buffer;
        } else {
          // does not fit, so allocate a custom size
          buffer = boost::shared_array<BYTE>(new BYTE[messsageLengthInBytes + sizeof(DWORD)]);
        }

        // put the size of the message at the front
        BYTE *dest = buffer.get();
        ((DWORD *)dest)[0] = htonl(messsageLengthInBytes);
        memcpy(&(dest[sizeof(DWORD)]), message, messsageLengthInBytes);

        return channel->send(dest, sizeof(DWORD) + messsageLengthInBytes);
      }

      MessageBuffer RUDPMessaging::getBufferLargeEnoughForNextMessage()
      {
        ULONG size = getNextReceivedMessageSizeInBytes();
        if (0 == size) {
          ZS_LOG_DEBUG(log("no data available thus get buffer large enough is returning NULL buffer"))
          return boost::shared_array<BYTE>();
        }

        boost::shared_array<BYTE> buffer(new BYTE[size+sizeof(char)]);
        memset(buffer.get(), 0, size+sizeof(char)); // force the memory to be NUL filled (including one extra char at the end)

        return buffer;
      }

      //-------------------------------------------------------------------------
      void RUDPMessaging::setMaxMessageSizeInBytes(ULONG maxMessageSizeInBytes)
      {
        AutoRecursiveLock lock(mLock);
        mMaxMessageSizeInBytes = maxMessageSizeInBytes;
      }

      //-------------------------------------------------------------------------
      ULONG RUDPMessaging::getNextReceivedMessageSizeInBytes()
      {
        AutoRecursiveLock lock(mLock);
        if (!mChannel) {
          ZS_LOG_WARNING(Debug, log("no channel associated with messaging thus returning message size of 0"))
          return 0;
        }

        obtainNextMessageSize();
        if (0 == mNextMessageSizeInBytes) {
          ZS_LOG_DEBUG(log("no data available at this time"))
          return 0;
        }

        ULONG available = mChannel->getReceiveSizeAvailableInBytes();
        if (available < mNextMessageSizeInBytes) {
          ZS_LOG_DEBUG(log("not enough data received to deliver message to delegate") + ", available= " + Stringize<ULONG>(available).string() + ", next message size=" + Stringize<DWORD>(mNextMessageSizeInBytes).string())
          return 0;
        }

        notifyReadReady();

        return mNextMessageSizeInBytes;
      }

      //-------------------------------------------------------------------------
      ULONG RUDPMessaging::receive(BYTE *outBuffer)
      {
        AutoRecursiveLock lock(mLock);

        if (!mChannel) return 0;
        ULONG nextMessageSize = getNextReceivedMessageSizeInBytes();
        if (0 == nextMessageSize) return 0;

        ULONG received = mChannel->receive(outBuffer, nextMessageSize);
        ZS_LOG_DEBUG(log("channel data received") + ", size=" + Stringize<ULONG>(received).string())

        if (0 == received) return 0;
        mInformedReadReady = false; // if the receive was called in response to a read-ready event then the read-ready event flag must be cleared so the event can fire again

        if (received != nextMessageSize) {
          ZS_LOG_ERROR(Detail, log("failed to obtain data from the channel in the exact size as indicated in the stream"))
          // this should not happen
          mChannel->shutdown();
          mChannel.reset();

          setShutdownReason(RUDPMessagingShutdownReason_IllegalStreamState);
          cancel();
          return 0;
        }

        // reset the next message size since we have received it
        mNextMessageSizeInBytes = 0;

        // cause a read ready notification if there is enough data available for a message
        getNextReceivedMessageSizeInBytes();
        return received;
      }

      //-------------------------------------------------------------------------
      IPAddress RUDPMessaging::getConnectedRemoteIP()
      {
        IRUDPChannelPtr channel = getChannel();
        if (!channel) return IPAddress();
        return channel->getConnectedRemoteIP();
      }

      //-------------------------------------------------------------------------
      String RUDPMessaging::getRemoteConnectionInfo()
      {
        IRUDPChannelPtr channel = getChannel();
        if (!channel) return String();
        return channel->getRemoteConnectionInfo();
      }

      //-------------------------------------------------------------------------
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
          {
            getShutdownReason();
            cancel();
            break;
          }
          case IRUDPChannel::RUDPChannelState_Shutdown:
          {
            getShutdownReason();
            mChannel.reset();
            cancel();
            break;
          }
        }
      }

      //-------------------------------------------------------------------------
      void RUDPMessaging::onRUDPChannelReadReady(IRUDPChannelPtr session)
      {
        AutoRecursiveLock lock(mLock);
        ZS_LOG_DEBUG(log("notify ready to read more data"))
        // cause a read ready notification if there is enough data available for a message
        getNextReceivedMessageSizeInBytes();
      }

      //-------------------------------------------------------------------------
      void RUDPMessaging::onRUDPChannelWriteReady(IRUDPChannelPtr session)
      {
        AutoRecursiveLock lock(mLock);
        if (!mDelegate) return;
        if (mInformedWriteReady) return;

        try {
          ZS_LOG_DEBUG(log("notify write ready"))
          mDelegate->onRUDPMessagingWriteReady(mThisWeak.lock());
          mInformedWriteReady = true;
        } catch(IRUDPMessagingDelegateProxy::Exceptions::DelegateGone &) {
          setShutdownReason(RUDPMessagingShutdownReason_DelegateGone);
          cancel();
        }
      }

      String RUDPMessaging::log(const char *message) const
      {
        return String("RUDPMessaging [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-------------------------------------------------------------------------
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

        mNextMessageSizeInBytes = 0;
      }

      //-------------------------------------------------------------------------
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

      //-------------------------------------------------------------------------
      void RUDPMessaging::setShutdownReason(RUDPMessagingShutdownReasons reason)
      {
        AutoRecursiveLock lock(mLock);
        if (mShutdownReason == reason) return;

        getShutdownReason();  // fix shutdown state to the RUDPChannel

        if (RUDPMessagingShutdownReason_None != mShutdownReason) {
          ZS_LOG_WARNING(Detail, log("attempting to set shutdown reason when already have one") + ", current reason=" + toString(mShutdownReason) + ", attempting to set reason=" + toString(reason))
          return;
        }

        ZS_LOG_DEBUG(log("setting shutdown reason") + reason)
        mShutdownReason = reason;
      }

      //-------------------------------------------------------------------------
      IRUDPChannelPtr RUDPMessaging::getChannel() const
      {
        AutoRecursiveLock lock(mLock);
        return mChannel;
      }

      //-------------------------------------------------------------------------
      void RUDPMessaging::obtainNextMessageSize()
      {
        if (!mChannel) return;
        if (0 != mNextMessageSizeInBytes) return;

        ULONG available = mChannel->getReceiveSizeAvailableInBytes();
        if (available < sizeof(DWORD)) return;

        BYTE buffer[sizeof(DWORD)];
        available = mChannel->receive(&(buffer[0]), sizeof(buffer));
        if (0 == available) return;

        if (sizeof(buffer) != available) {
          mChannel->shutdown();
          mChannel.reset();

          setShutdownReason(RUDPMessagingShutdownReason_IllegalStreamState);
          cancel();
          return;
        }
        mNextMessageSizeInBytes = ntohl(*((DWORD *)&(buffer[0])));
        if (mNextMessageSizeInBytes > mMaxMessageSizeInBytes) {
          mChannel->shutdown();
          mChannel.reset();

          setShutdownReason(RUDPMessagingShutdownReason_IllegalStreamState);
          cancel();
          return;
        }
      }

      //-----------------------------------------------------------------------
      void RUDPMessaging::notifyReadReady()
      {
        AutoRecursiveLock lock(mLock);
        if (!mDelegate) {
          ZS_LOG_DEBUG(log("delegate is not attached thus read ready is ignored"))
          return;
        }
        if (mInformedReadReady) {
          ZS_LOG_DEBUG(log("already informed ready ready"))
          return;
        }

        try {
          ZS_LOG_DEBUG(log("notify read ready"))
          mDelegate->onRUDPMessagingReadReady(mThisWeak.lock());
          mInformedReadReady = true;
        } catch(IRUDPMessagingDelegateProxy::Exceptions::DelegateGone &) {
          setShutdownReason(RUDPMessagingShutdownReason_DelegateGone);
          cancel();
        }
      }

      //-----------------------------------------------------------------------
      void RUDPMessaging::getBuffer(RecycledPacketBuffer &outBuffer)
      {
        AutoRecursiveLock lock(mLock);
        if (mRecycledBuffers.size() < 1) {
          outBuffer = RecycledPacketBuffer(new BYTE[HOOKFLASH_SERVICES_RUDPMESSAGING_RECYCLE_BUFFER_SIZE]);
          return;
        }

        outBuffer = mRecycledBuffers.front();
        mRecycledBuffers.pop_front();
      }

      //-----------------------------------------------------------------------
      void RUDPMessaging::recycleBuffer(RecycledPacketBuffer &buffer)
      {
        AutoRecursiveLock lock(mLock);
        if (!buffer) return;

        if (mRecycledBuffers.size() >= HOOKFLASH_SERVICES_RUDPMESSAGING_MAX_RECYLCE_BUFFERS) {
          buffer.reset();
          return;
        }
        mRecycledBuffers.push_back(buffer);
        buffer.reset();
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
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
    IRUDPMessagingPtr IRUDPMessaging::acceptChannel(
                                                    IMessageQueuePtr queue,
                                                    IRUDPListenerPtr listener,
                                                    IRUDPMessagingDelegatePtr delegate,
                                                    ULONG maxMessageSizeInBytes
                                                    )
    {
      return internal::RUDPMessaging::acceptChannel(queue, listener, delegate, maxMessageSizeInBytes);
    }

    //-------------------------------------------------------------------------
    IRUDPMessagingPtr IRUDPMessaging::acceptChannel(
                                                    IMessageQueuePtr queue,
                                                    IRUDPICESocketSessionPtr session,
                                                    IRUDPMessagingDelegatePtr delegate,
                                                    ULONG maxMessageSizeInBytes
                                                    )
    {
      return internal::RUDPMessaging::acceptChannel(queue, session, delegate, maxMessageSizeInBytes);
    }

    //-------------------------------------------------------------------------
    IRUDPMessagingPtr IRUDPMessaging::openChannel(
                                                  IMessageQueuePtr queue,
                                                  IRUDPICESocketSessionPtr session,
                                                  IRUDPMessagingDelegatePtr delegate,
                                                  const char *connectionInfo,
                                                  ULONG maxMessageSizeInBytes
                                                  )
    {
      return internal::RUDPMessaging::openChannel(queue, session, delegate, connectionInfo, maxMessageSizeInBytes);
    }
  }
}
