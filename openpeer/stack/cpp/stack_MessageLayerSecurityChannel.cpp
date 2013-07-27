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

#include <openpeer/stack/internal/stack_MessageLayerSecurityChannel.h>
#include <openpeer/stack/internal/stack_Helper.h>
#include <openpeer/stack/internal/stack_Account.h>
#include <openpeer/stack/internal/stack_Stack.h>
#include <openpeer/stack/message/IMessageHelper.h>
#include <openpeer/stack/IPeerFilePrivate.h>
#include <openpeer/stack/IPeerFilePublic.h>
#include <openpeer/stack/IPeerFiles.h>
#include <openpeer/stack/IRSAPublicKey.h>

#include <zsLib/Log.h>
#include <zsLib/XML.h>
#include <zsLib/helpers.h>

#include <zsLib/Stringize.h>
#include <zsLib/Numeric.h>

#define OPENPEER_STACK_MESSAGE_LAYER_SECURITY_DEFAULT_TOTAL_SEND_KEYS 3

#define OPENPEER_STACK_MLS_DEFAULT_KEYING_EXPIRES_TIME_IN_SECONDS (60*3)

namespace openpeer { namespace stack { ZS_DECLARE_SUBSYSTEM(openpeer_stack) } }

namespace openpeer
{
  namespace stack
  {
    using zsLib::DWORD;
    using zsLib::Stringize;
    using zsLib::Numeric;
    using stack::message::IMessageHelper;

    namespace internal
    {
      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      static SecureByteBlockPtr decryptUsingPassphraseEncoding(
                                                               const String &passphrase,
                                                               const String &nonce,
                                                               const String &value
                                                               )
      {
        typedef IHelper::SplitMap SplitMap;

        //hex(`<salt>`) + ":" + base64(encrypt(`<encrypted-value>`)), where key = hmac(`<external-passphrase>`, "keying:" + `<nonce>`), iv = `<salt>`

        SplitMap values;
        IHelper::split(value, values, ':');

        if (values.size() < 2) {
          ZS_LOG_WARNING(Debug, String("failed to split hex salt from encrypted value") + ", value=" + value)
          return SecureByteBlockPtr();
        }

        String hexSalt = values[0];
        String base64Value = values[1];

        SecureByteBlockPtr iv = IHelper::convertFromHex(hexSalt);
        SecureByteBlockPtr input = IHelper::convertFromBase64(base64Value);

        SecureByteBlockPtr key = IHelper::hmac(*IHelper::hmacKeyFromPassphrase(passphrase), "keying:" + nonce);

        if ((!iv) || (!key) || (!input)) {
          ZS_LOG_WARNING(Debug, String("missing vital information required to be able to decrypt value"))
          return SecureByteBlockPtr();
        }

        return IHelper::decrypt(*key, *iv, *input);
      }
      
      //-----------------------------------------------------------------------
      static String encodeUsingPassphraseEncoding(
                                                  const String &passphrase,
                                                  const String &nonce,
                                                  const SecureByteBlock &value
                                                  )
      {
        typedef IHelper::SplitMap SplitMap;

        //hex(`<salt>`) + ":" + base64(encrypt(`<encrypted-value>`)), where key = hmac(`<external-passphrase>`, "keying:" + `<nonce>`), iv = `<salt>`

        SecureByteBlockPtr iv = IHelper::random(IHelper::getHashDigestSize(IHelper::HashAlgorthm_MD5));
        String hexSalt = IHelper::convertToHex(*iv);

        SecureByteBlockPtr key = IHelper::hmac(*IHelper::hmacKeyFromPassphrase(passphrase), "keying:" + nonce);

        SecureByteBlockPtr output = IHelper::encrypt(*key, *iv, value);

        return IHelper::convertToHex(*iv) + ":" + IHelper::convertToBase64(*output);
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageLayerSecurityChannel
      #pragma mark

      //-----------------------------------------------------------------------
      MessageLayerSecurityChannel::MessageLayerSecurityChannel(
                                                               IMessageQueuePtr queue,
                                                               IMessageLayerSecurityChannelDelegatePtr delegate,
                                                               IPeerFilesPtr localPeerFiles,
                                                               LocalPublicKeyReferenceTypes localPublicKeyReferenceType,
                                                               const char *contextID,
                                                               IAccountPtr account
                                                               ) :
        zsLib::MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(IMessageLayerSecurityChannelDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate)),
        mAccount(Account::convert(account)),
        mCurrentState(SessionState_ReceiveOnly),
        mLastError(0),
        mPeerFiles(localPeerFiles),
        mLocalReferenceType(localPublicKeyReferenceType),
        mLocalContextID(contextID),
        mNextReceiveSequenceNumber(0)
      {
        ZS_LOG_DEBUG(log("created"))
        ZS_THROW_BAD_STATE_IF(!mDelegate)
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::init()
      {
        IMessageLayerSecurityChannelAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      MessageLayerSecurityChannel::~MessageLayerSecurityChannel()
      {
        ZS_LOG_DEBUG(log("destroyed"))
        mThisWeak.reset();
        cancel();
      }

      //-----------------------------------------------------------------------
      MessageLayerSecurityChannelPtr MessageLayerSecurityChannel::convert(IMessageLayerSecurityChannelPtr channel)
      {
        return boost::dynamic_pointer_cast<MessageLayerSecurityChannel>(channel);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageLayerSecurityChannel => IMessageLayerSecurityChannel
      #pragma mark

      //-----------------------------------------------------------------------
      String MessageLayerSecurityChannel::toDebugString(IMessageLayerSecurityChannelPtr channel, bool includeCommaPrefix)
      {
        if (!channel) return String(includeCommaPrefix ? ", message incoming=(null)" : "message incoming=(null)");

        MessageLayerSecurityChannelPtr pThis = MessageLayerSecurityChannel::convert(channel);
        return pThis->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      MessageLayerSecurityChannelPtr MessageLayerSecurityChannel::create(
                                                                         IMessageLayerSecurityChannelDelegatePtr delegate,
                                                                         IPeerFilesPtr localPeerFiles,
                                                                         LocalPublicKeyReferenceTypes localPublicKeyReferenceType,
                                                                         const char *contextID,
                                                                         IAccountPtr account
                                                                         )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!localPeerFiles)

        MessageLayerSecurityChannelPtr pThis(new MessageLayerSecurityChannel(IStackForInternal::queueStack(), delegate, localPeerFiles, localPublicKeyReferenceType, contextID, account));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::cancel()
      {
        ZS_LOG_DEBUG(log("cancel called"))

        AutoRecursiveLock lock(getLock());

        setState(SessionState_Shutdown);

        mDelegate.reset();

        mAccount.reset();

        mPeerFiles.reset();

        mSendingRemotePublicKey.reset();

        mReceivingRemotePublicKey.reset();
        mReceivingRemotePublicPeerFile.reset();

        mMessagesToEncode.clear();
        mPendingBuffersToSendOnWire.clear();

        mMessagesReceived.clear();
        mReceivedBuffersToDecode.clear();

        mReceiveKeys.clear();
        mSendKeys.clear();

        ZS_LOG_DEBUG(log("cancel complete"))
      }

      //-----------------------------------------------------------------------
      IMessageLayerSecurityChannel::SessionStates MessageLayerSecurityChannel::getState(
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
      bool MessageLayerSecurityChannel::send(
                                             const BYTE *buffer,
                                             ULONG bufferSizeInBytes
                                             )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!buffer)
        ZS_THROW_INVALID_ARGUMENT_IF(0 == bufferSizeInBytes)

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("cannot send data as already shutdown"))
          return false;
        }

        SecureByteBlockPtr pendingBuffer = IHelper::convertToBuffer(buffer, bufferSizeInBytes);
        mMessagesToEncode.push_back(pendingBuffer);

        // process it later...
        IMessageLayerSecurityChannelAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
        return true;
      }

      //-----------------------------------------------------------------------
      ULONG MessageLayerSecurityChannel::getTotalIncomingMessages() const
      {
        AutoRecursiveLock lock(getLock());
        return mMessagesReceived.size();
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr MessageLayerSecurityChannel::getNextIncomingMessage()
      {
        AutoRecursiveLock lock(getLock());

        if (mMessagesReceived.size() < 1) return SecureByteBlockPtr();

        SecureByteBlockPtr result = mMessagesReceived.front();
        mMessagesReceived.pop_front();
        return result;
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::setReceiveKeyingDecoding(const char *passphrase)
      {
        AutoRecursiveLock lock(getLock());
        ZS_THROW_INVALID_USAGE_IF(mReceivingPassphrase.hasData())

        mReceivingPassphrase = String(passphrase);

        IMessageLayerSecurityChannelAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::setSendKeyingEncoding(IRSAPublicKeyPtr remotePublicKey)
      {
        AutoRecursiveLock lock(getLock());
        ZS_THROW_INVALID_USAGE_IF(mSendingRemotePublicKey)
        ZS_THROW_INVALID_USAGE_IF(mReceivingPassphrase.hasData())

        mSendingRemotePublicKey = remotePublicKey;

        IMessageLayerSecurityChannelAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::setSendKeyingEncoding(const char *passphrase)
      {
        AutoRecursiveLock lock(getLock());
        ZS_THROW_INVALID_USAGE_IF(mSendingRemotePublicKey)
        ZS_THROW_INVALID_USAGE_IF(mReceivingPassphrase.hasData())

        mSendingPassphrase = String(passphrase);

        IMessageLayerSecurityChannelAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------d------------------------------------------------
      String MessageLayerSecurityChannel::getLocalContextID() const
      {
        AutoRecursiveLock lock(getLock());
        return mLocalContextID;
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::setLocalContextID(const char *contextID)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!contextID)

        AutoRecursiveLock lock(getLock());
        if (mLocalContextID.hasData()) {
          ZS_THROW_INVALID_ARGUMENT_IF(contextID != mLocalContextID)
        }

        ZS_LOG_DEBUG(log("setting local context ID") + ", context ID=" + contextID)

        mLocalContextID = contextID;

        IMessageLayerSecurityChannelAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      String MessageLayerSecurityChannel::getRemoteContextID() const
      {
        AutoRecursiveLock lock(getLock());
        return mRemoteContextID;
      }

      //-----------------------------------------------------------------------
      IMessageLayerSecurityChannel::RemotePublicKeyReferenceTypes MessageLayerSecurityChannel::getRemotePublicKeyReferencedType() const
      {
        AutoRecursiveLock lock(getLock());

        if (mReceivingRemotePublicPeerFile) {
          return IMessageLayerSecurityChannel::RemotePublicKeyReferenceType_PeerURI;
        }
        if (mReceivingRemotePublicKey) {
          return IMessageLayerSecurityChannel::RemotePublicKeyReferenceType_FullPublicKey;
        }
        return RemotePublicKeyReferenceType_Unknown;
      }

      //-----------------------------------------------------------------------
      IRSAPublicKeyPtr MessageLayerSecurityChannel::getRemotePublicKey() const
      {
        AutoRecursiveLock lock(getLock());
        return mReceivingRemotePublicKey;
      }

      //-----------------------------------------------------------------------
      IPeerFilePublicPtr MessageLayerSecurityChannel::getRemoteReferencedPeerFilePublic() const
      {
        AutoRecursiveLock lock(getLock());
        return mReceivingRemotePublicPeerFile;
      }

      //-----------------------------------------------------------------------
      ULONG MessageLayerSecurityChannel::getTotalPendingBuffersToSendOnWire() const
      {
        AutoRecursiveLock lock(getLock());
        return mPendingBuffersToSendOnWire.size();
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr MessageLayerSecurityChannel::getNextPendingBufferToSendOnWire()
      {
        AutoRecursiveLock lock(getLock());

        if (mPendingBuffersToSendOnWire.size() < 1) return SecureByteBlockPtr();

        SecureByteBlockPtr result = mPendingBuffersToSendOnWire.front();
        mPendingBuffersToSendOnWire.pop_front();
        return result;
      }
      
      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::notifyReceivedFromWire(
                                                               const BYTE *buffer,
                                                               ULONG bufferLengthInBytes
                                                               )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!buffer)
        ZS_THROW_INVALID_ARGUMENT_IF(0 != bufferLengthInBytes)

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("cannot receive data as already shutdown"))
          return;
        }

        SecureByteBlockPtr pendingBuffer = IHelper::convertToBuffer(buffer, bufferLengthInBytes);
        mReceivedBuffersToDecode.push_back(pendingBuffer);

        // process it later...
        IMessageLayerSecurityChannelAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageLayerSecurityChannel => IMessageLayerSecurityChannelAsyncDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::onStep()
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("on step"))
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageLayerSecurityChannel  => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &MessageLayerSecurityChannel::getLock() const
      {
        return mLock;
      }

      //-----------------------------------------------------------------------
      String MessageLayerSecurityChannel::log(const char *message) const
      {
        return String("MessageLayerSecurityChannel [" + Stringize<typeof(mID)>(mID).string() + "] " + message);
      }

      //-----------------------------------------------------------------------
      String MessageLayerSecurityChannel::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("mls channel id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("state", toString(mCurrentState), firstTime) +
               Helper::getDebugValue("last error", 0 != mLastError ? Stringize<typeof(mLastError)>(mLastError).string() : String(), firstTime) +
               Helper::getDebugValue("last reason", mLastErrorReason, firstTime) +
               Helper::getDebugValue("peer files", mPeerFiles ? String("true") : String(), firstTime) +
               Helper::getDebugValue("local reference type", toString(mLocalReferenceType), firstTime) +
               Helper::getDebugValue("local context ID", mLocalContextID, firstTime) +
               Helper::getDebugValue("remote context ID", mRemoteContextID, firstTime) +        
               Helper::getDebugValue("sending remote public key", mSendingRemotePublicKey ? String("true") : String(), firstTime) +
               Helper::getDebugValue("sending passphrase", mSendingPassphrase, firstTime) +
               Helper::getDebugValue("receive seq number", Stringize<typeof(mNextReceiveSequenceNumber)>(mNextReceiveSequenceNumber).string(), firstTime) +
               Helper::getDebugValue("receiving remote public key", mReceivingRemotePublicKey ? String("true") : String(), firstTime) +
               Helper::getDebugValue("receiving remote public peer file", mReceivingRemotePublicPeerFile ? mReceivingRemotePublicPeerFile->getPeerURI() : String(), firstTime) +
               Helper::getDebugValue("receiving passphrase", mReceivingPassphrase, firstTime) +
               Helper::getDebugValue("messages to encode/send", mMessagesToEncode.size() > 0 ? Stringize<BufferList::size_type>(mMessagesToEncode.size()).string() : String(), firstTime) +
               Helper::getDebugValue("pending buffers to send on-wire", mPendingBuffersToSendOnWire.size() > 0 ? Stringize<BufferList::size_type>(mPendingBuffersToSendOnWire.size()).string() : String(), firstTime) +
               Helper::getDebugValue("messages received", mMessagesReceived.size() > 0 ? Stringize<BufferList::size_type>(mMessagesReceived.size()).string() : String(), firstTime) +
               Helper::getDebugValue("receive buffers to decode", mReceivedBuffersToDecode.size() > 0 ? Stringize<BufferList::size_type>(mReceivedBuffersToDecode.size()).string() : String(), firstTime) +
               Helper::getDebugValue("receive keys", mReceiveKeys.size() > 0 ? Stringize<KeyMap::size_type>(mReceiveKeys.size()).string() : String(), firstTime) +
               Helper::getDebugValue("send keys", mSendKeys.size() > 0 ? Stringize<KeyMap::size_type>(mSendKeys.size()).string() : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::setState(SessionStates state)
      {
        if (state != mCurrentState) {
          ZS_LOG_DEBUG(log("state changed") + ", state=" + toString(state) + ", old state=" + toString(mCurrentState))
          mCurrentState = state;
        }

        MessageLayerSecurityChannelPtr pThis = mThisWeak.lock();
        if ((pThis) &&
            (mDelegate)) {
          try {
            ZS_LOG_DEBUG(log("attempting to report state to delegate") + getDebugValueString())
            mDelegate->onMessageLayerSecurityChannelStateChanged(pThis, mCurrentState);
          } catch (IMessageLayerSecurityChannelDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Detail, log("delegate gone"))
          }
        }
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::setError(WORD errorCode, const char *inReason)
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
      void MessageLayerSecurityChannel::step()
      {
        if (isShutdown()) {
          ZS_LOG_DEBUG(log("step continue to shutdown"))
          cancel();
          return;
        }

        ZS_LOG_DEBUG(log("step") + getDebugValueString())

        if (!stepReceive()) return;
        if (!stepSendKeying()) return;
        if (!stepSend()) return;
        if (!stepCheckConnected()) return;

        setState(SessionState_Connected);
      }

      //-----------------------------------------------------------------------
      bool MessageLayerSecurityChannel::stepReceive()
      {
        if (mReceivedBuffersToDecode.size() < 1) {
          ZS_LOG_DEBUG(log("nothing to decode"))
          return true;
        }

        if (mRemoteContextID.hasData()) {
          ZS_LOG_DEBUG(log("already decoded at least one packet"))
          if (mReceiveKeys.size() < 1) {
            if (mReceivingPassphrase.isEmpty()) {
              ZS_LOG_DEBUG(log("waiting for receive keying materials"))
              return true;
            }
          }
        }

        while (mReceivedBuffersToDecode.size() > 0) {
          SecureByteBlockPtr listBuffer = mReceivedBuffersToDecode.front();

          // has to be greater than the size of a DWORD
          if (listBuffer->SizeInBytes() <= sizeof(DWORD)) {
            ZS_LOG_ERROR(Detail, log("algorithm bytes missing in protocol") + ", size=" + Stringize<SecureByteBlock::size_type>(listBuffer->SizeInBytes()).string())
            setError(IHTTP::HTTPStatusCode_Unauthorized, "buffer is not decodable");
            cancel();
            return false;
          }

          const BYTE *buffer = listBuffer->BytePtr();

          DWORD algorithm = ntohl(((DWORD *)buffer)[0]);

          const BYTE *source = (const BYTE *)(&(((DWORD *)buffer)[1]));
          SecureByteBlock::size_type remaining = listBuffer->SizeInBytes() - sizeof(DWORD);

          if (0 != algorithm) {
            mReceivedBuffersToDecode.pop_front();

            // attempt to decode now
            if (mReceiveKeys.size() < 1) {
              ZS_LOG_ERROR(Detail, log("attempting to decode a packet where keying material has not been received"))
              setError(IHTTP::HTTPStatusCode_Forbidden, "attempting to decode a packet where keying material has not been received");
              cancel();
              return false;
            }

            KeyMap::iterator found = mReceiveKeys.find(algorithm);
            if (found == mReceiveKeys.end()) {
              ZS_LOG_ERROR(Detail, log("attempting to decode a packet where keying algorithm does not map to a know key") + ", algorithm=" + Stringize<typeof(algorithm)>(algorithm).string())
              setError(IHTTP::HTTPStatusCode_Forbidden, "attempting to decode a packet where keying algorithm does not map to a know key");
              cancel();
              return false;
            }

            // decode the packet
            KeyInfo &keyInfo = (*found).second;

            size_t integritySize = IHelper::getHashDigestSize(IHelper::HashAlgorthm_SHA1);

            // must be greater in size than the hash algorithm
            if (remaining <= integritySize) {
              ZS_LOG_ERROR(Detail, log("algorithm bytes missing in protocol") + ", size=" + Stringize<SecureByteBlock::size_type>(listBuffer->SizeInBytes()).string())
              setError(IHTTP::HTTPStatusCode_Unauthorized, "buffer is not decodable");
              cancel();
              return false;
            }


            SecureByteBlockPtr integrity(new SecureByteBlock(integritySize));
            memcpy(integrity->BytePtr(), source, integritySize);

            source += integritySize;
            remaining -= integritySize;

            SecureByteBlock input(remaining);

            SecureByteBlockPtr output = IHelper::decrypt(*(keyInfo.mSendKey), *(keyInfo.mNextIV), input);

            if (!output) {
              ZS_LOG_ERROR(Detail, log("unable to decrypte buffer"))
              setError(IHTTP::HTTPStatusCode_Unauthorized, "unable to decrypt buffer");
              cancel();
              return false;
            }

            String hexIV = IHelper::convertToHex(*keyInfo.mNextIV);

            SecureByteBlockPtr calculatedIntegrity = IHelper::hmac(*(IHelper::convertToBuffer(keyInfo.mIntegrityPassphrase)), ("integrity:" + IHelper::convertToHex(*IHelper::hash(*output)) + ":" + hexIV).c_str());

            if (0 != IHelper::compare(*calculatedIntegrity, *integrity)) {
              ZS_LOG_ERROR(Debug,log("integrity failed on packet"))
              setError(IHTTP::HTTPStatusCode_Unauthorized, "buffer is not decodable");
              cancel();
              return false;
            }

            // calculate the next IV and remember the integrity field
            keyInfo.mNextIV = IHelper::hash(hexIV + ":" + IHelper::convertToHex(*calculatedIntegrity));
            keyInfo.mLastIntegrity = calculatedIntegrity;

            mMessagesReceived.push_back(output);

            try {
              mDelegate->onMessageLayerSecurityChannelIncomingMessage(mThisWeak.lock());
            } catch (IMessageLayerSecurityChannelDelegateProxy::Exceptions::DelegateGone &) {
              ZS_LOG_WARNING(Detail, log("delegate gone"))
            }

            // process next buffer
            continue;
          }

          // create a NUL terminated JSON string buffer
          SecureByteBlockPtr jsonBuffer =  IHelper::convertToBuffer(source, remaining, true);

          // parse the buffer
          DocumentPtr doc = Document::createFromAutoDetect((const char *)(jsonBuffer->BytePtr()));

          mReceiveKeys.clear(); // all previous keys are being destroyed

          try {
            ElementPtr keyingEl = doc->findFirstChildElementChecked("keyingBundle")->findFirstChildElementChecked("keying");

            bool encodingPKI = true;

            if (!verifyReceiveSignature(keyingEl)) {
              ZS_LOG_ERROR(Detail, log("failed to validate receiving stream signature"))
              return false;
            }

            String sequenceNumber = IMessageHelper::getElementTextAndDecode(keyingEl->findFirstChildElement("sequence"));

            if (sequenceNumber != Stringize<typeof(mNextReceiveSequenceNumber)>(mNextReceiveSequenceNumber).string()) {
              ZS_LOG_ERROR(Detail, log("sequence number mismatch") + ", sequence=" + sequenceNumber + ", expecting=" + Stringize<typeof(mNextReceiveSequenceNumber)>(mNextReceiveSequenceNumber).string())
              setError(IHTTP::HTTPStatusCode_RequestTimeout, "sequence number mismatch");
              cancel();
              return false;
            }

            if (!mRemoteContextID) {
              mRemoteContextID = IMessageHelper::getElementTextAndDecode(keyingEl->findFirstChildElement("context"));
            }

            Time expires = IMessageHelper::stringToTime(IMessageHelper::getElementTextAndDecode(keyingEl->findFirstChildElement("expires")));
            Time tick = zsLib::now();
            if (tick > expires) {
              ZS_LOG_ERROR(Detail, log("signed keying bundle has expired") + ", expires=" + IMessageHelper::timeToString(expires) + ", now=" + IMessageHelper::timeToString(tick))
              setError(IHTTP::HTTPStatusCode_RequestTimeout, "signed keying bundle has expired");
              cancel();
              return false;
            }

            ElementPtr encodingEl = keyingEl->findFirstChildElementChecked("encoding");
            String type = IMessageHelper::getElementTextAndDecode(encodingEl->findFirstChildElementChecked("type"));
            String nonce = IMessageHelper::getElementTextAndDecode(encodingEl->findFirstChildElementChecked("nonce"));

#define WARNING_CHECK_NONCE_NOT_SEEN_BEFORE 1
#define WARNING_CHECK_NONCE_NOT_SEEN_BEFORE 2

            IPeerFilePublicPtr peerFilePublic = mPeerFiles->getPeerFilePublic();
            IPeerFilePrivatePtr peerFilePrivate = mPeerFiles->getPeerFilePrivate();
            ZS_THROW_BAD_STATE_IF(!peerFilePublic)
            ZS_THROW_BAD_STATE_IF(!peerFilePrivate)

            if ("pki" == type) {
              String encodingFingerprint = IMessageHelper::getElementTextAndDecode(encodingEl->findFirstChildElementChecked("fingerprint"));
              String expectingFingerprint = peerFilePublic->getPublicKey()->getFingerprint();
              if (encodingFingerprint != expectingFingerprint) {
                ZS_LOG_ERROR(Detail, log("encoding not using local public key") + ", encoding fingerprint=" + encodingFingerprint + ", expecting fingerprint=" + expectingFingerprint)
                setError(IHTTP::HTTPStatusCode_RequestTimeout, "signed keying bundle has expired");
                cancel();
                return false;
              }
            } else if ("passphrase" == type) {
              encodingPKI = false;
              if (mReceivingPassphrase.isEmpty()) {
                ZS_LOG_DEBUG(log("cannot continue decoding as missing decoding passphrase (will notify delegate)"))

                try {
                  mDelegate->onMessageLayerSecurityChannelNeedDecodingPassphrase(mThisWeak.lock());
                } catch (IMessageLayerSecurityChannelDelegateProxy::Exceptions::DelegateGone &) {
                  ZS_LOG_WARNING(Detail, log("delegate gone"))
                }
                return true;
              }

              // scope: we have a passphrase, see if the proof validates before attempting to decrypt any keys...
              {
                String algorithm = IMessageHelper::getElementTextAndDecode(encodingEl->findFirstChildElementChecked("algorithm"));
                if (OPENPEER_STACK_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM != algorithm) {
                  ZS_LOG_ERROR(Detail, log("keying encoding not using known algorithm") + ", algorithm=" + algorithm + ", expecting=" + OPENPEER_STACK_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM)
                  setError(IHTTP::HTTPStatusCode_ExpectationFailed, "keyhing encoding not using expecting passphrase");
                  cancel();
                  return false;
                }

                String proof = IMessageHelper::getElementTextAndDecode(encodingEl->findFirstChildElementChecked("proof"));

                // hex(hmac(`<external-passphrase>`, "keying:" + `<nonce>`))
                String calculatedProof = IHelper::convertToHex(*IHelper::hmac(*IHelper::hmacKeyFromPassphrase(mReceivingPassphrase), "keying:" + nonce));

                if (proof != calculatedProof) {
                  ZS_LOG_ERROR(Detail, log("keying encoding not using expecting passphrase") + ", encoding proof=" + proof + ", expecting proof=" + calculatedProof + ", using passphrase=" + mReceivingPassphrase)
                  setError(IHTTP::HTTPStatusCode_ExpectationFailed, "keyhing encoding not using expecting passphrase");
                  cancel();
                  return false;
                }
              }
            }

            // scope: santity check on algorithms receiving
            {
              bool found = false;
              ElementPtr algorithmEl = keyingEl->findFirstChildElementChecked("algorithms")->findFirstChildElementChecked("algorithm");
              while (algorithmEl) {
                String algorithm = IMessageHelper::getElementTextAndDecode(algorithmEl);
                if (OPENPEER_STACK_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM == algorithm) {
                  ZS_LOG_TRACE(log("found mandated algorithm"))
                  found = true;
                  break;
                }
                algorithmEl->findNextSiblingElement("algorithm");
              }
              if (!found) {
                ZS_LOG_ERROR(Detail, log("did not find mandated MLS algorithm") + ", expecting=" + OPENPEER_STACK_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM)
                setError(IHTTP::HTTPStatusCode_ExpectationFailed, "did not find mandated MLS algorithm");
                cancel();
                return false;
              }
            }

            // scope: extract out latest keying material
            {
              ElementPtr keyEl = keyingEl->findFirstChildElementChecked("keys")->findFirstChildElementChecked("key");
              while (keyEl) {
                AlgorithmIndex index = 0;
                try {
                    index = Numeric<AlgorithmIndex>(IMessageHelper::getElementTextAndDecode(keyEl->findFirstChildElementChecked("index")));
                } catch(Numeric<AlgorithmIndex>::ValueOutOfRange &) {
                  ZS_LOG_WARNING(Detail, log("algorithm index value out of range"))
                }
                if (0 == index) {
                  ZS_LOG_WARNING(Detail, log("algorithm index value is not valid") + ", index=" + Stringize<typeof(index)>(index).string())
                  continue;
                }

                String algorithm = IMessageHelper::getElementTextAndDecode(keyEl->findFirstChildElementChecked("algorithm"));
                if (OPENPEER_STACK_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM == algorithm) {
                  ZS_LOG_WARNING(Detail, log("unsupported algorithm (thus skipping)") + ", algorithm=" + algorithm)
                  continue;
                }

                ElementPtr inputs = keyEl->findFirstChildElementChecked("inputs");

                KeyInfo key;
                SecureByteBlockPtr integrityPassphrase;
                if (encodingPKI) {
                  // base64(rsa_encrypt(`<remote-public-key>`, `<value>`))
                  key.mSendKey = peerFilePrivate->decrypt(*IHelper::convertFromBase64(IMessageHelper::getElementTextAndDecode(inputs->findFirstChildElementChecked("secret"))));
                  key.mNextIV = peerFilePrivate->decrypt(*IHelper::convertFromBase64(IMessageHelper::getElementTextAndDecode(inputs->findFirstChildElementChecked("iv"))));
                  integrityPassphrase = peerFilePrivate->decrypt(*IHelper::convertFromBase64(IMessageHelper::getElementTextAndDecode(inputs->findFirstChildElementChecked("hmacIntegrityKey"))));
                } else {
                  key.mSendKey = decryptUsingPassphraseEncoding(mReceivingPassphrase, nonce, IMessageHelper::getElementTextAndDecode(inputs->findFirstChildElementChecked("secret")));
                  key.mNextIV = decryptUsingPassphraseEncoding(mReceivingPassphrase, nonce, IMessageHelper::getElementTextAndDecode(inputs->findFirstChildElementChecked("iv")));
                  integrityPassphrase = decryptUsingPassphraseEncoding(mReceivingPassphrase, nonce, IMessageHelper::getElementTextAndDecode(inputs->findFirstChildElementChecked("hmacIntegrityKey")));
                }
                if (integrityPassphrase) {
                  key.mIntegrityPassphrase = IHelper::convertToString(*integrityPassphrase);
                }
                if ((!key.mSendKey) ||
                    (!key.mNextIV) ||
                    (key.mIntegrityPassphrase.isEmpty())) {
                  ZS_LOG_WARNING(Detail, log("algorithm missing vital secret, iv or integrity information") + ", index=" + Stringize<typeof(index)>(index).string())
                  continue;
                }

                ZS_LOG_DEBUG(log("adding algorithm to available keying information") + ", index=" + Stringize<typeof(index)>(index).string())
                mReceiveKeys[index] = key;
              }
            }

          } catch(CheckFailed &) {
            ZS_LOG_ERROR(Detail, log("expecting element in keying bundle that was missing"))
          }

          if (mReceiveKeys.size() < 1) {
            ZS_LOG_ERROR(Detail, log("did not find any key information"))
            setError(IHTTP::HTTPStatusCode_ExpectationFailed, "did not find any key information");
            cancel();
            return false;
          }

          ZS_LOG_DEBUG(log("successfully extracted keying materials to receive data from remote MLS stream"))
          ++mNextReceiveSequenceNumber;
          mReceivedBuffersToDecode.pop_front();
        }

        return true;
      }

      //-----------------------------------------------------------------------
      bool MessageLayerSecurityChannel::stepSendKeying()
      {
        if (mSendKeys.size() > 0) {
          ZS_LOG_DEBUG(log("already sent keying materials"))
          return true;
        }

        if (mLocalContextID.isEmpty()) {
          ZS_LOG_DEBUG(log("missing local context ID thus cannot send data remotely"))
          return false;
        }

        if ((!mSendingRemotePublicKey) &&
            (mSendingPassphrase.isEmpty())) {
          ZS_LOG_DEBUG(log("send keying material is not ready"))
          return false;
        }
        // create initial encryption offer (hint: it won't change)

        ElementPtr keyingBundleEl = Element::create("keyingBundle");

        ElementPtr keyingEl = Element::create("keying");
        
        keyingEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("sequence", "0"));

        String nonce = IHelper::randomString(32);

        keyingEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("nonce", nonce));
        keyingEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("context", mLocalContextID));

        Time expires = zsLib::now() + Seconds(OPENPEER_STACK_MLS_DEFAULT_KEYING_EXPIRES_TIME_IN_SECONDS);

        keyingEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("expires", IMessageHelper::timeToString(expires)));

        ElementPtr encodingEl = Element::create("encoding");
        keyingEl->adoptAsLastChild(encodingEl);

        String encodingPassphrase = IHelper::randomString(32*8/5);

        if (mSendingPassphrase.hasData()) {
          encodingEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("type", "passphrase"));
          encodingEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("algorithm", OPENPEER_STACK_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM));

          String calculatedProof = IHelper::convertToHex(*IHelper::hmac(*IHelper::hmacKeyFromPassphrase(encodingPassphrase), "keying:" + nonce));

          encodingEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("proof", calculatedProof));
        } else {
          ZS_THROW_BAD_STATE_IF(!mSendingRemotePublicKey)
          encodingEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("type", "pki"));
          encodingEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("fingerprint", mSendingRemotePublicKey->getFingerprint()));
        }

        ElementPtr algorithmsEl = Element::create("algorithms");
        algorithmsEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("algorithm", OPENPEER_STACK_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM));

        ElementPtr keysEl = Element::create("keys");

        for (AlgorithmIndex index = 1; index <= OPENPEER_STACK_MESSAGE_LAYER_SECURITY_DEFAULT_TOTAL_SEND_KEYS; ++index) {
          KeyInfo key;

          key.mIntegrityPassphrase = IHelper::randomString((20*8/5));
          key.mSendKey = IHelper::hash(*IHelper::random(32), IHelper::HashAlgorthm_SHA256);
          key.mNextIV = IHelper::hash(*IHelper::random(16), IHelper::HashAlgorthm_MD5);

          ElementPtr keyEl = Element::create("key");
          keyEl->adoptAsLastChild(IMessageHelper::createElementWithNumber("index", Stringize<typeof(index)>(index).string()));
          keyEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("algorithm", OPENPEER_STACK_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM));

          ElementPtr inputsEl = Element::create("inputs");
          inputsEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("secret", encodeUsingPassphraseEncoding(encodingPassphrase, nonce, *key.mSendKey)));
          inputsEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("iv", encodeUsingPassphraseEncoding(encodingPassphrase, nonce, *key.mNextIV)));
          inputsEl->adoptAsLastChild(IMessageHelper::createElementWithTextAndJSONEncode("hmacIntegrityKey", encodeUsingPassphraseEncoding(encodingPassphrase, nonce, *IHelper::convertToBuffer(key.mIntegrityPassphrase, false))));

          keyEl->adoptAsLastChild(inputsEl);

          keysEl->adoptAsLastChild(keyEl);
        }

        keyingEl->adoptAsLastChild(keysEl);

        IPeerFilePrivatePtr peerFilePrivate = mPeerFiles->getPeerFilePrivate();
        ZS_THROW_BAD_STATE_IF(!peerFilePrivate)

        IPeerFilePrivate::SignatureTypes signatureType = IPeerFilePrivate::SignatureType_PeerURI;
        switch (mLocalReferenceType) {
          case LocalPublicKeyReferenceType_FullPublicKey: signatureType = IPeerFilePrivate::SignatureType_FullPublicKey;
          case LocalPublicKeyReferenceType_PeerURI:       signatureType = IPeerFilePrivate::SignatureType_PeerURI;
        }

        peerFilePrivate->signElement(
                                     keyingEl,
                                     signatureType
                                     );
        
        keyingBundleEl->adoptAsLastChild(keyingEl);

        ULONG outputLength = 0;
        GeneratorPtr generator = Generator::createJSONGenerator();
        boost::shared_array<char> output = generator->write(keyingBundleEl, &outputLength);

        SecureByteBlockPtr buffer(new SecureByteBlock(sizeof(DWORD) + (outputLength * sizeof(char))));

        ((DWORD *)(buffer->BytePtr()))[0] = htonl(0);

        BYTE *dest = (buffer->BytePtr() + sizeof(DWORD));

        memcpy(dest, output.get(), sizeof(char)*outputLength);

        mPendingBuffersToSendOnWire.push_back(buffer);

        // now in a position where we can sent (but not necessarily receive yet)
        setState(SessionState_SendOnly);

        try {
          ZS_LOG_DEBUG(log("notify delegate needs to send keying information on-the-wire") + getDebugValueString())
          mDelegate->onMessageLayerSecurityChannelBufferPendingToSendOnTheWire(mThisWeak.lock());
        } catch (IMessageLayerSecurityChannelDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, log("delegate gone"))
        }

        return true;
      }

      //-----------------------------------------------------------------------
      bool MessageLayerSecurityChannel::stepSend()
      {
        if (mSendKeys.size() < 1) {
          ZS_LOG_DEBUG(log("no send keys set, not sending yet..."))
          return false;
        }

        if (mMessagesToEncode.size() < 1) {
          ZS_LOG_DEBUG(log("no data to be sent over the wire"))
          return true;
        }

        while (mMessagesToEncode.size() > 0) {
          SecureByteBlockPtr buffer = mMessagesToEncode.front();
          mMessagesToEncode.pop_front();

          ZS_THROW_BAD_STATE_IF(!buffer)

          // pick an algorithm
          AlgorithmIndex index = IHelper::random(1, mSendKeys.size());

          KeyMap::iterator found = mSendKeys.find(index);
          ZS_THROW_BAD_STATE_IF(found == mSendKeys.end())

          KeyInfo &keyInfo = (*found).second;


          SecureByteBlockPtr encrypted = IHelper::encrypt(*(keyInfo.mSendKey), *(keyInfo.mNextIV), *buffer);

          String hexIV = IHelper::convertToHex(*keyInfo.mNextIV);

          SecureByteBlockPtr calculatedIntegrity = IHelper::hmac(*(IHelper::convertToBuffer(keyInfo.mIntegrityPassphrase)), ("integrity:" + IHelper::convertToHex(*IHelper::hash(*buffer)) + ":" + hexIV).c_str());

          // calculate the next IV and remember the integrity field
          keyInfo.mNextIV = IHelper::hash(hexIV + ":" + IHelper::convertToHex(*calculatedIntegrity));
          keyInfo.mLastIntegrity = calculatedIntegrity;

          SecureByteBlockPtr output(new SecureByteBlock(sizeof(DWORD) + calculatedIntegrity->SizeInBytes() + encrypted->SizeInBytes()));

          ((DWORD *)output->BytePtr())[0] = htonl(index);

          BYTE *integrityPos = (output->BytePtr() + sizeof(DWORD));
          BYTE *outputPos = (integrityPos + calculatedIntegrity->SizeInBytes());

          memcpy(integrityPos, calculatedIntegrity->BytePtr(), calculatedIntegrity->SizeInBytes());
          memcpy(outputPos, encrypted->BytePtr(), encrypted->SizeInBytes());

          mPendingBuffersToSendOnWire.push_back(output);
        }

        return true;
      }

      //-----------------------------------------------------------------------
      bool MessageLayerSecurityChannel::stepCheckConnected()
      {
        if (mSendKeys.size() < 1) {
          ZS_LOG_DEBUG(log("no send keys set, not sending yet..."))
          return false;
        }
        if (mReceiveKeys.size() < 1) {
          ZS_LOG_DEBUG(log("no receive keys set, not sending yet..."))
          return false;
        }
        ZS_LOG_DEBUG(log("connected"))
        return true;
      }

      //-----------------------------------------------------------------------
      bool MessageLayerSecurityChannel::verifyReceiveSignature(ElementPtr keyingEl)
      {
        String peerURI;
        String fullPublicKey;
        String fingerprint;
        IHelper::getSignatureInfo(keyingEl, NULL, &peerURI, NULL, NULL, NULL, &fullPublicKey, &fingerprint);

        // attempt to resolve public key used in signature...

        if (peerURI.hasData()) {

          if (!mReceivingRemotePublicPeerFile) {
            AccountPtr account = mAccount.lock();
            if (account) {
              IPeerPtr peer = IPeer::create(account, peerURI);
              if (peer) {
                mReceivingRemotePublicPeerFile = peer->getPeerFilePublic();
              }
            }
          }

          if (mReceivingRemotePublicPeerFile) {
            if (peerURI != mReceivingRemotePublicPeerFile->getPeerURI()) {
              ZS_LOG_ERROR(Debug,log("signing peer changed mid stream (illegal)") + ", signature peer URI=" + peerURI + ", original signing peer URI=" + mReceivingRemotePublicPeerFile->getPeerURI())
              setError(IHTTP::HTTPStatusCode_Conflict, "signing peer changed mid stream (illegal)");
              cancel();
              return false;
            }
            if (!mReceivingRemotePublicKey) {
              mReceivingRemotePublicKey = mReceivingRemotePublicPeerFile->getPublicKey();
            }
          }
        }

        if (fullPublicKey.hasData()) {
          IRSAPublicKeyPtr publicKey = IRSAPublicKey::load(*IHelper::convertFromBase64(fullPublicKey));
          fingerprint = publicKey->getFingerprint();
          if (!mReceivingRemotePublicKey) {
            mReceivingRemotePublicKey = publicKey;
          }
        }

        if (fingerprint.hasData()) {
          if (mReceivingRemotePublicKey) {
            if (fingerprint != mReceivingRemotePublicKey->getFingerprint()) {
              ZS_LOG_ERROR(Debug,log("signing peer changed mid stream (illegal)") + ", signature fingerprint=" + fingerprint + ", original signing peer URI=" + mReceivingRemotePublicKey->getFingerprint())
              setError(IHTTP::HTTPStatusCode_Conflict, "signing peer stream mid stream (illegal)");
              cancel();
              return false;
            }
          }
        }

        if (!mReceivingRemotePublicKey) {
          ZS_LOG_ERROR(Debug,log("unable to verify signer as do not have public key of signing peer") + ", peer URI=" + peerURI + ", fingerprint=" + fingerprint + ", full key=" + fullPublicKey)
          setError(IHTTP::HTTPStatusCode_Conflict, "unable to verify signer as do not have public key of signing peer");
          cancel();
          return false;
        }

        if (!mReceivingRemotePublicKey->verifySignature(keyingEl)) {
          ZS_LOG_ERROR(Debug,log("signature failed verification") + ", peer URI=" + peerURI + ", fingerprint=" + fingerprint + ", full key=" + fullPublicKey)
          setError(IHTTP::HTTPStatusCode_Conflict, "signature failed verification");
          cancel();
          return false;
        }

        ZS_LOG_DEBUG(log("signature validation passed"))
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
    }

    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    #pragma mark
    #pragma mark IMessageLayerSecurityChannel
    #pragma mark

    //-----------------------------------------------------------------------
    const char *IMessageLayerSecurityChannel::toString(SessionStates state)
    {
      switch (state)
      {
        case SessionState_ReceiveOnly:  return "Send only";
        case SessionState_SendOnly:     return "Send only";
        case SessionState_Connected:    return "Connected";
        case SessionState_Shutdown:     return "Shutdown";
      }
      return "UNDEFINED";
    }

    //-----------------------------------------------------------------------
    const char *IMessageLayerSecurityChannel::toString(LocalPublicKeyReferenceTypes type)
    {
      switch (type) {
        case LocalPublicKeyReferenceType_FullPublicKey: return "Full public key";
        case LocalPublicKeyReferenceType_PeerURI:       return "Peer URI";
      }
      return "UNDEFINED";
    }

    //-----------------------------------------------------------------------
    const char *IMessageLayerSecurityChannel::toString(RemotePublicKeyReferenceTypes type)
    {
      switch (type) {
        case RemotePublicKeyReferenceType_Unknown:                return "Unknown";
        case RemotePublicKeyReferenceType_FullPublicKey:          return "Full public key";
        case RemotePublicKeyReferenceType_PeerURI:                return "Peer URI";
      }
      return "UNDEFINED";
    }

    //-----------------------------------------------------------------------
    IMessageLayerSecurityChannelPtr IMessageLayerSecurityChannel::create(
                                                                         IMessageLayerSecurityChannelDelegatePtr delegate,
                                                                         IPeerFilesPtr localPeerFiles,
                                                                         LocalPublicKeyReferenceTypes localPublicKeyReferenceType,
                                                                         const char *contextID,
                                                                         IAccountPtr account
                                                                         )
    {
      return internal::IMessageLayerSecurityChannelFactory::singleton().create(delegate, localPeerFiles, localPublicKeyReferenceType, contextID, account);
    }
  }
}
