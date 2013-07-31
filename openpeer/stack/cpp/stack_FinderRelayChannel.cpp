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

#include <openpeer/stack/internal/stack_FinderRelayChannel.h>
#include <openpeer/stack/internal/stack_Helper.h>
#include <openpeer/stack/internal/stack_Account.h>
#include <openpeer/stack/internal/stack_Stack.h>

#include <openpeer/stack/IPeerFiles.h>
#include <openpeer/stack/IPeerFilePrivate.h>
#include <openpeer/stack/IPeerFilePublic.h>
#include <openpeer/stack/IPeer.h>

#include <openpeer/services/IRSAPublicKey.h>
#include <openpeer/services/IHelper.h>
#include <openpeer/services/IHTTP.h>

#include <zsLib/Log.h>
#include <zsLib/helpers.h>

#include <zsLib/Stringize.h>

namespace openpeer { namespace stack { ZS_DECLARE_SUBSYSTEM(openpeer_stack) } }

namespace openpeer
{
  namespace stack
  {
    using zsLib::Stringize;
    using stack::message::IMessageHelper;

    namespace internal
    {
      using services::IHelper;

//      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderRelayChannel
      #pragma mark

      //-----------------------------------------------------------------------
      FinderRelayChannel::FinderRelayChannel(
                                             IMessageQueuePtr queue,
                                             IFinderRelayChannelDelegatePtr delegate,
                                             AccountPtr account,
                                             ITransportStreamPtr receiveStream,
                                             ITransportStreamPtr sendStream
                                             ) :
        zsLib::MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mAccount(account),
        mIncoming(false),
        mOuterReceiveStream(receiveStream),
        mOuterSendStream(sendStream)
      {
        ZS_LOG_DEBUG(log("created"))
        mDefaultSubscription = mSubscriptions.subscribe(delegate, IStackForInternal::queueDelegate());
      }

      //-----------------------------------------------------------------------
      void FinderRelayChannel::init()
      {
        AutoRecursiveLock lock(getLock());

        mTCPReceiveStream = ITransportStream::create();
        mTCPSendStream = ITransportStream::create();

        mMLSChannel = mMLSChannel->create(mThisWeak.lock(), mTCPReceiveStream, mOuterReceiveStream, mOuterSendStream, mTCPSendStream);

        step();
      }

      //-----------------------------------------------------------------------
      FinderRelayChannel::~FinderRelayChannel()
      {
        ZS_LOG_DEBUG(log("destroyed"))
        mThisWeak.reset();
        cancel();
      }

      //-----------------------------------------------------------------------
      FinderRelayChannelPtr FinderRelayChannel::convert(IFinderRelayChannelPtr channel)
      {
        return boost::dynamic_pointer_cast<FinderRelayChannel>(channel);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderRelayChannel => IFinderRelayChannel
      #pragma mark

      //-----------------------------------------------------------------------
      String FinderRelayChannel::toDebugString(IFinderRelayChannelPtr channel, bool includeCommaPrefix)
      {
        if (!channel) return String(includeCommaPrefix ? ", message incoming=(null)" : "message incoming=(null)");

        FinderRelayChannelPtr pThis = FinderRelayChannel::convert(channel);
        return pThis->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      FinderRelayChannelPtr FinderRelayChannel::connect(
                                                        IFinderRelayChannelDelegatePtr delegate,
                                                        AccountPtr account,
                                                        ITransportStreamPtr receiveStream,
                                                        ITransportStreamPtr sendStream,
                                                        IPAddress remoteFinderIP,
                                                        const char *localContextID,
                                                        const char *relayAccessToken,
                                                        const char *relayAccessSecretProof,
                                                        const char *encryptDataUsingEncodingPassphrase
                                                        )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!account)
        ZS_THROW_INVALID_ARGUMENT_IF(!receiveStream)
        ZS_THROW_INVALID_ARGUMENT_IF(!sendStream)
        ZS_THROW_INVALID_ARGUMENT_IF(!relayAccessToken)
        ZS_THROW_INVALID_ARGUMENT_IF(!relayAccessSecretProof)
        ZS_THROW_INVALID_ARGUMENT_IF(!encryptDataUsingEncodingPassphrase)

        FinderRelayChannelPtr pThis(new FinderRelayChannel(IStackForInternal::queueStack(), delegate, account, receiveStream, sendStream));
        pThis->mThisWeak = pThis;

        pThis->mConnectInfo.mFinderIP = remoteFinderIP;
        pThis->mConnectInfo.mLocalContextID = String(localContextID);
        pThis->mConnectInfo.mRelayAccessToken = String(relayAccessToken);
        pThis->mConnectInfo.mRelayAccessSecretProof = String(relayAccessSecretProof);
        pThis->mConnectInfo.mEncryptionPassphrase = String(encryptDataUsingEncodingPassphrase);

        pThis->init();

        return pThis;
      }

      //-----------------------------------------------------------------------
      FinderRelayChannelPtr FinderRelayChannel::createIncoming(
                                                               IFinderRelayChannelDelegatePtr delegate,
                                                               AccountPtr account,
                                                               ITransportStreamPtr receiveStream,
                                                               ITransportStreamPtr sendStream
                                                               )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!account)
        ZS_THROW_INVALID_ARGUMENT_IF(!receiveStream)
        ZS_THROW_INVALID_ARGUMENT_IF(!sendStream)

        FinderRelayChannelPtr pThis(new FinderRelayChannel(IStackForInternal::queueStack(), delegate, account, receiveStream, sendStream));
        pThis->mThisWeak = pThis;
        pThis->init();

        return pThis;
      }

      //-----------------------------------------------------------------------
      void FinderRelayChannel::cancel()
      {
        ZS_LOG_DEBUG(log("cancel called"))

        AutoRecursiveLock lock(getLock());

        setState(SessionState_Shutdown);

        mAccount.reset();

        if (mMLSChannel) {
          mMLSChannel->cancel();
          mMLSChannel.reset();
        }

        mDefaultSubscription->cancel();
        mSubscriptions.clear();

        ZS_LOG_DEBUG(log("cancel complete"))
      }

      //-----------------------------------------------------------------------
      FinderRelayChannel::SessionStates FinderRelayChannel::getState(
                                                                     WORD *outLastErrorCode,
                                                                     String *outLastErrorReason
                                                                     ) const
      {
        AutoRecursiveLock lock(getLock());
        if (outLastErrorCode) *outLastErrorCode = mLastError;
        if (outLastErrorReason) *outLastErrorReason = mLastErrorReason;
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      IFinderRelayChannelSubscriptionPtr FinderRelayChannel::subscribe(IFinderRelayChannelDelegatePtr originalDelegate)
      {
        ZS_LOG_TRACE(log("subscribe called"))

        AutoRecursiveLock lock(getLock());

        if (!originalDelegate) return mDefaultSubscription;

        IFinderRelayChannelSubscriptionPtr subscription = mSubscriptions.subscribe(originalDelegate, IStackForInternal::queueDelegate());

        IFinderRelayChannelDelegatePtr delegate = mSubscriptions.delegate(subscription);

        if (delegate) {
          FinderRelayChannelPtr pThis(mThisWeak.lock());

          if (SessionState_Pending != mCurrentState) {
            delegate->onFinderRelayChannelStateChanged(pThis, mCurrentState);
          }

          if (isShutdown()) {
            ZS_LOG_WARNING(Detail, log("subscription created after shutdown"))
            return subscription;
          }

          if (mMLSChannel->getRemoteContextID().hasData()) {
            // have remote context ID, but have we set local context ID?
            if (mMLSChannel->getLocalContextID().isEmpty()) {
              delegate->onFinderRelayChannelNeedsContext(pThis);
            }
          }
        }

        return subscription;
      }

      //-----------------------------------------------------------------------
      void FinderRelayChannel::setIncomingContext(
                                                  const char *contextID,
                                                  const char *decryptUsingEncodingPassphrase,
                                                  IPeerPtr remotePeer
                                                  )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!contextID)
        ZS_THROW_INVALID_ARGUMENT_IF(!decryptUsingEncodingPassphrase)
        ZS_THROW_INVALID_ARGUMENT_IF(!remotePeer)

        AutoRecursiveLock lock(getLock());

        if (!mMLSChannel) {
          ZS_LOG_WARNING(Detail, log("MLS channel is gone"))
          return;
        }

        mMLSChannel->setLocalContextID(contextID);
        mMLSChannel->setReceiveKeyingDecoding(decryptUsingEncodingPassphrase);

        IPeerFilePublicPtr peerFilePublic = remotePeer->getPeerFilePublic();
        ZS_THROW_INVALID_ARGUMENT_IF(!peerFilePublic)

        mMLSChannel->setSendKeyingEncoding(peerFilePublic->getPublicKey());

        step();
      }

      //-----------------------------------------------------------------------
      String FinderRelayChannel::getLocalContextID() const
      {
        AutoRecursiveLock lock(getLock());

        if (!mMLSChannel) {
          ZS_LOG_WARNING(Detail, log("MLS channel is gone"))
          return String();
        }

        return mMLSChannel->getLocalContextID();
      }

      //-----------------------------------------------------------------------
      String FinderRelayChannel::getRemoteContextID() const
      {
        AutoRecursiveLock lock(getLock());

        if (!mMLSChannel) {
          ZS_LOG_WARNING(Detail, log("MLS channel is gone"))
          return String();
        }

        return mMLSChannel->getRemoteContextID();
      }

      //-----------------------------------------------------------------------
      IPeerPtr FinderRelayChannel::getRemotePeer() const
      {
        AutoRecursiveLock lock(getLock());
        return mRemotePeer;
      }

      //-----------------------------------------------------------------------
      IRSAPublicKeyPtr FinderRelayChannel::getRemotePublicKey() const
      {
        AutoRecursiveLock lock(getLock());
        return mRemotePublicKey;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderRelayChannel => IMessageLayerSecurityChannelDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void FinderRelayChannel::onMessageLayerSecurityChannelStateChanged(
                                                                         IMessageLayerSecurityChannelPtr channel,
                                                                         IMessageLayerSecurityChannel::SessionStates state
                                                                         )
      {
        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("already shutdown"))
          return;
        }

        ZS_THROW_BAD_STATE_IF(channel != mMLSChannel)

        AccountPtr account = mAccount.lock();
        if (!account) return;

        IPeerFilesPtr peerFiles = account->forFinderRelay().getPeerFiles();
        if (!peerFiles) return;

        IPeerFilePrivatePtr peerFilePrivate = peerFiles->getPeerFilePrivate();
        IPeerFilePublicPtr peerFilePublic = peerFiles->getPeerFilePublic();

        if (IMessageLayerSecurityChannel::SessionState_WaitingForNeededInformation == state) {
          if (mMLSChannel->needsReceiveKeyingDecodingPrivateKey()) {

            mMLSChannel->setReceiveKeyingDecoding(peerFilePrivate->getPrivateKey(), peerFilePublic->getPublicKey());
          }

          if (mMLSChannel->needsReceiveKeyingMaterialSigningPublicKey()) {
            ElementPtr receiveSignedEl = mMLSChannel->getSignedReceivingKeyingMaterial();
            String peerURI;
            String fullPublicKey;
            stack::IHelper::getSignatureInfo(receiveSignedEl, NULL, &peerURI, NULL, NULL, NULL, &fullPublicKey);

            if (peerURI.hasData()) {
              mRemotePeer = IPeer::create(account, peerURI);
              if (mRemotePeer) {
                IPeerFilePublicPtr remotePeerFilePublic = mRemotePeer->getPeerFilePublic();
                if (remotePeerFilePublic) {
                  mRemotePublicKey = remotePeerFilePublic->getPublicKey();
                }
              }
            }

            if (fullPublicKey) {
              mRemotePublicKey = IRSAPublicKey::load(*IHelper::convertFromBase64(fullPublicKey));
            }

            if (mRemotePublicKey) {
              mMLSChannel->setReceiveKeyingMaterialSigningPublicKey(mRemotePublicKey);
            }
          }

          if (mMLSChannel->needsSendKeyingMaterialToeBeSigned()) {
            DocumentPtr doc;
            ElementPtr signEl;
            mMLSChannel->getSendKeyingMaterialNeedingToBeSigned(doc, signEl);

            if (signEl) {
              peerFilePrivate->signElement(signEl, mIncoming ? IPeerFilePrivate::SignatureType_FullPublicKey : IPeerFilePrivate::SignatureType_PeerURI);
              mMLSChannel->notifySendKeyingMaterialSigned();
            }
          }
        }

        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderRelayChannel  => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &FinderRelayChannel::getLock() const
      {
        return mLock;
      }

      //-----------------------------------------------------------------------
      String FinderRelayChannel::log(const char *message) const
      {
        return String("FinderRelayChannel [" + Stringize<typeof(mID)>(mID).string() + "] " + message);
      }

      //-----------------------------------------------------------------------
      String FinderRelayChannel::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;

        return Helper::getDebugValue("relay channel id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("subscriptions", mSubscriptions.size() > 0 ? Stringize<IFinderRelayChannelDelegateSubscriptions::size_type>(mSubscriptions.size()).string() : String(), firstTime) +
               Helper::getDebugValue("default subscription", mDefaultSubscription ? String("true") : String(), firstTime) +
               Helper::getDebugValue("state", toString(mCurrentState), firstTime) +
               Helper::getDebugValue("last error", 0 != mLastError ? Stringize<typeof(mLastError)>(mLastError).string() : String(), firstTime) +
               Helper::getDebugValue("last reason", mLastErrorReason, firstTime) +
               Helper::getDebugValue("account", mAccount.lock() ? String("true") : String(), firstTime) +
               Helper::getDebugValue("incoming", mIncoming ? String("true") : String(), firstTime) +
               IMessageLayerSecurityChannel::toDebugString(mMLSChannel) +
               IPeer::toDebugString(mRemotePeer) +
               Helper::getDebugValue("remote public key", mRemotePublicKey ? String("true") : String(), firstTime) +
               ", outer recv stream: " + ITransportStream::toDebugString(mOuterReceiveStream, false) +
               ", outer send stream: " + ITransportStream::toDebugString(mOuterSendStream, false) +
               ", tcp recv stream: " + ITransportStream::toDebugString(mTCPReceiveStream, false) +
               ", tcp send stream: " + ITransportStream::toDebugString(mTCPSendStream, false);
      }

      //-----------------------------------------------------------------------
      void FinderRelayChannel::setState(SessionStates state)
      {
        if (state == mCurrentState) return;

        ZS_LOG_DEBUG(log("state changed") + ", state=" + toString(state) + ", old state=" + toString(mCurrentState))
        mCurrentState = state;
        FinderRelayChannelPtr pThis = mThisWeak.lock();

        if (pThis) {
          mSubscriptions.delegate()->onFinderRelayChannelStateChanged(pThis, mCurrentState);
        }
      }

      //-----------------------------------------------------------------------
      void FinderRelayChannel::setError(WORD errorCode, const char *inReason)
      {
        String reason(inReason ? String(inReason) : String());
        if (reason.isEmpty()) {
          reason = IHTTP::toString(IHTTP::toStatusCode(errorCode));
        }

        if (0 != mLastError) {
          ZS_LOG_WARNING(Detail, log("error already set thus ignoring new error") + ", new error=" + Stringize<typeof(errorCode)>(errorCode).string() + ", new reason=" + reason + getDebugValueString())
          return;
        }

        mLastError = errorCode;
        mLastErrorReason = reason;

        ZS_LOG_WARNING(Detail, log("error set") + ", code=" + Stringize<typeof(mLastError)>(mLastError).string() + ", reason=" + mLastErrorReason + getDebugValueString())
      }
      
      //-----------------------------------------------------------------------
      void FinderRelayChannel::step()
      {
        if (isShutdown()) {
          ZS_LOG_DEBUG(log("step continue to shutdown"))
          cancel();
          return;
        }

        ZS_LOG_DEBUG(log("step") + getDebugValueString())

        // TODO

        setState(SessionState_Connected);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageLayerSecurityChannel
      #pragma mark

      //-----------------------------------------------------------------------
      const char *IFinderRelayChannel::toString(SessionStates state)
      {
        switch (state)
        {
          case SessionState_Pending:      return "Pending";
          case SessionState_Connected:    return "Connected";
          case SessionState_Shutdown:     return "Shutdown";
        }
        return "UNDEFINED";
      }

      //-----------------------------------------------------------------------
      IFinderRelayChannelPtr IFinderRelayChannel::connect(
                                                          IFinderRelayChannelDelegatePtr delegate,        // can pass in IFinderRelayChannelDelegatePtr() if not interested in the events
                                                          AccountPtr account,
                                                          ITransportStreamPtr receiveStream,
                                                          ITransportStreamPtr sendStream,
                                                          IPAddress remoteFinderIP,
                                                          const char *localContextID,
                                                          const char *relayAccessToken,
                                                          const char *relayAccessSecretProof,
                                                          const char *encryptDataUsingEncodingPassphrase
                                                          )
      {
        return internal::IFinderRelayChannelFactory::singleton().connect(delegate, account, receiveStream, sendStream, remoteFinderIP, localContextID, relayAccessToken, relayAccessSecretProof, encryptDataUsingEncodingPassphrase);
      }

      //-----------------------------------------------------------------------
      IFinderRelayChannelPtr IFinderRelayChannel::createIncoming(
                                                                 IFinderRelayChannelDelegatePtr delegate, // can pass in IFinderRelayChannelDelegatePtr() if not interested in the events
                                                                 AccountPtr account,
                                                                 ITransportStreamPtr receiveStream,
                                                                 ITransportStreamPtr sendStream
                                                                 )
      {
        return internal::IFinderRelayChannelFactory::singleton().createIncoming(delegate, account, receiveStream, sendStream);
      }
    }
  }
}
