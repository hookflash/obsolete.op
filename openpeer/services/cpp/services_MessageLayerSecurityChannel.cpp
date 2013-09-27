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

#include <openpeer/services/internal/services_MessageLayerSecurityChannel.h>
#include <openpeer/services/internal/services_Helper.h>
#include <openpeer/services/IRSAPrivateKey.h>
#include <openpeer/services/IRSAPublicKey.h>
#include <openpeer/services/IHTTP.h>

#include <zsLib/Log.h>
#include <zsLib/XML.h>
#include <zsLib/helpers.h>

#include <zsLib/Stringize.h>
#include <zsLib/Numeric.h>

#define OPENPEER_SERVICES_MESSAGE_LAYER_SECURITY_DEFAULT_TOTAL_SEND_KEYS 3

#define OPENPEER_SERVICES_MLS_DEFAULT_KEYING_EXPIRES_TIME_IN_SECONDS (60*3)

namespace openpeer { namespace services { ZS_DECLARE_SUBSYSTEM(openpeer_services) } }

namespace openpeer
{
  namespace services
  {
    using zsLib::DWORD;
    using zsLib::Numeric;

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
      static String getElementTextAndDecode(ElementPtr node)
      {
        if (!node) return String();
        return node->getTextDecoded();
      }

      //-----------------------------------------------------------------------
      static ElementPtr createElementWithText(
                                              const String &elName,
                                              const String &textVal
                                              )
      {
        ElementPtr tmp = Element::create(elName);
        if (textVal.isEmpty()) return tmp;

        TextPtr tmpTxt = Text::create();
        tmpTxt->setValueAndJSONEncode(textVal);
        tmp->adoptAsFirstChild(tmpTxt);
        return tmp;
      }

      //-----------------------------------------------------------------------
      static ElementPtr createElementWithNumber(
                                                const String &elName,
                                                const String &numberAsStringValue
                                                )
      {
        ElementPtr tmp = Element::create(elName);

        if (numberAsStringValue.isEmpty()) return tmp;

        TextPtr tmpTxt = Text::create();
        tmpTxt->setValue(numberAsStringValue, Text::Format_JSONNumberEncoded);
        tmp->adoptAsFirstChild(tmpTxt);

        return tmp;
      }
      
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

        SecureByteBlockPtr key = IHelper::hmac(*IHelper::hmacKeyFromPassphrase(passphrase), "keying:" + nonce, IHelper::HashAlgorthm_SHA256);

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

        SecureByteBlockPtr key = IHelper::hmac(*IHelper::hmacKeyFromPassphrase(passphrase), "keying:" + nonce, IHelper::HashAlgorthm_SHA256);

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
      const char *MessageLayerSecurityChannel::toString(DecodingTypes decodingType)
      {
        switch (decodingType)
        {
          case DecodingType_Unknown:    return "Unknown";
          case DecodingType_PrivateKey: return "Private key";
          case DecodingType_Passphrase: return "Passphrase";
        }
        return "UNDEFINED";
      }

      //-----------------------------------------------------------------------
      MessageLayerSecurityChannel::MessageLayerSecurityChannel(
                                                               IMessageQueuePtr queue,
                                                               IMessageLayerSecurityChannelDelegatePtr delegate,
                                                               ITransportStreamPtr receiveStreamEncoded,
                                                               ITransportStreamPtr receiveStreamDecoded,
                                                               ITransportStreamPtr sendStreamDecoded,
                                                               ITransportStreamPtr sendStreamEncoded,
                                                               const char *contextID
                                                               ) :
        zsLib::MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mCurrentState(SessionState_Pending),
        mLastError(0),
        mLocalContextID(contextID),
        mNextReceiveSequenceNumber(0),
        mReceiveDecodingType(DecodingType_Unknown),
        mReceiveStreamEncoded(receiveStreamEncoded->getReader()),
        mReceiveStreamDecoded(receiveStreamDecoded->getWriter()),
        mSendStreamDecoded(sendStreamDecoded->getReader()),
        mSendStreamEncoded(sendStreamEncoded->getWriter())
      {
        ZS_LOG_DEBUG(log("created"))
        mDefaultSubscription = mSubscriptions.subscribe(delegate);
        ZS_THROW_BAD_STATE_IF(!mDefaultSubscription)
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::init()
      {
        AutoRecursiveLock lock(getLock());

        mReceiveStreamEncodedSubscription = mReceiveStreamEncoded->subscribe(mThisWeak.lock());
        mReceiveStreamDecodedSubscription = mReceiveStreamDecoded->subscribe(mThisWeak.lock());
        mSendStreamDecodedSubscription = mSendStreamDecoded->subscribe(mThisWeak.lock());
        mSendStreamEncodedSubscription = mSendStreamEncoded->subscribe(mThisWeak.lock());

        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
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
        if (!channel) return String(includeCommaPrefix ? ", mls channel=(null)" : "mls channel=(null)");

        MessageLayerSecurityChannelPtr pThis = MessageLayerSecurityChannel::convert(channel);
        return pThis->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      MessageLayerSecurityChannelPtr MessageLayerSecurityChannel::create(
                                                                         IMessageLayerSecurityChannelDelegatePtr delegate,
                                                                         ITransportStreamPtr receiveStreamEncoded,
                                                                         ITransportStreamPtr receiveStreamDecoded,
                                                                         ITransportStreamPtr sendStreamDecoded,
                                                                         ITransportStreamPtr sendStreamEncoded,
                                                                         const char *contextID
                                                                         )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!receiveStreamEncoded)
        ZS_THROW_INVALID_ARGUMENT_IF(!receiveStreamDecoded)
        ZS_THROW_INVALID_ARGUMENT_IF(!sendStreamDecoded)
        ZS_THROW_INVALID_ARGUMENT_IF(!sendStreamEncoded)
        MessageLayerSecurityChannelPtr pThis(new MessageLayerSecurityChannel(IHelper::getServiceQueue(), delegate, receiveStreamEncoded, receiveStreamDecoded, sendStreamDecoded, sendStreamEncoded, contextID));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      IMessageLayerSecurityChannelSubscriptionPtr MessageLayerSecurityChannel::subscribe(IMessageLayerSecurityChannelDelegatePtr originalDelegate)
      {
        AutoRecursiveLock lock(getLock());
        if (!originalDelegate) return mDefaultSubscription;

        IMessageLayerSecurityChannelSubscriptionPtr subscription = mSubscriptions.subscribe(originalDelegate);

        IMessageLayerSecurityChannelDelegatePtr delegate = mSubscriptions.delegate(subscription);

        if (delegate) {
          MessageLayerSecurityChannelPtr pThis = mThisWeak.lock();
          
          if (SessionState_Pending != mCurrentState) {
            delegate->onMessageLayerSecurityChannelStateChanged(pThis, mCurrentState);
          }
        }

        if (isShutdown()) {
          mSubscriptions.clear();
        }

        return subscription;
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::cancel()
      {
        ZS_LOG_DEBUG(log("cancel called"))

        AutoRecursiveLock lock(getLock());

        setState(SessionState_Shutdown);

        mSubscriptions.clear();

        mSendingEncodingRemotePublicKey.reset();

        mSendKeyingNeedingToSignDoc.reset();
        mSendKeyingNeedToSignEl.reset();

        mReceiveDecodingPrivateKey.reset();
        mReceiveDecodingPublicKey.reset();

        mReceiveSigningPublicKey.reset();
        mReceiveKeyingSignedDoc.reset();
        mReceiveKeyingSignedEl.reset();

        mReceiveStreamEncoded->cancel();
        mReceiveStreamDecoded->cancel();

        mSendStreamDecoded->cancel();
        mSendStreamEncoded->cancel();

        mReceiveStreamEncodedSubscription->cancel();
        mSendStreamDecodedSubscription->cancel();

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
      bool MessageLayerSecurityChannel::needsLocalContextID() const
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("cannot need information as already shutdown"))
          return false;
        }

        return mLocalContextID.isEmpty();
      }

      //-----------------------------------------------------------------------
      bool MessageLayerSecurityChannel::needsReceiveKeyingDecodingPrivateKey(
                                                                             String *outFingerprint
                                                                             ) const
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("cannot need information as already shutdown"))
          return false;
        }

        if (outFingerprint) {
          *outFingerprint = mReceiveDecodingPublicKeyFingerprint;
        }

        return DecodingType_PrivateKey == mReceiveDecodingType;
      }

      //-----------------------------------------------------------------------
      bool MessageLayerSecurityChannel::needsReceiveKeyingDecodingPassphrase() const
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("cannot need information as already shutdown"))
          return false;
        }
        return DecodingType_Passphrase == mReceiveDecodingType;
      }

      //-----------------------------------------------------------------------
      bool MessageLayerSecurityChannel::needsReceiveKeyingMaterialSigningPublicKey() const
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("cannot need information as already shutdown"))
          return false;
        }

        return mRemoteContextID.hasData() && (!((bool)(mReceiveSigningPublicKey)));
      }

      //-----------------------------------------------------------------------
      bool MessageLayerSecurityChannel::needsSendKeyingEncodingMaterial() const
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("cannot need information as already shutdown"))
          return false;
        }
        return (!((bool)(mSendingEncodingRemotePublicKey))) &&
               (mSendingEncodingPassphrase.isEmpty());
      }

      //-----------------------------------------------------------------------
      bool MessageLayerSecurityChannel::needsSendKeyingMaterialToeBeSigned() const
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("cannot need information as already shutdown"))
          return false;
        }

        return (mSendKeyingNeedingToSignDoc) && (mSendKeyingNeedToSignEl);
      }

      //-----------------------------------------------------------------------
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

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("already shutdown"))
          return;
        }

        if (mLocalContextID.hasData()) {
          ZS_THROW_INVALID_ARGUMENT_IF(contextID != mLocalContextID)
        }

        ZS_LOG_DEBUG(log("setting local context ID") + ", context ID=" + contextID)

        mLocalContextID = contextID;

        setState(SessionState_Pending);

        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
      }

      //-----------------------------------------------------------------------
      String MessageLayerSecurityChannel::getRemoteContextID() const
      {
        AutoRecursiveLock lock(getLock());
        return mRemoteContextID;
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::setReceiveKeyingDecoding(
                                                                 IRSAPrivateKeyPtr decodingPrivateKey,
                                                                 IRSAPublicKeyPtr decodingPublicKey
                                                                 )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!decodingPrivateKey)
        ZS_THROW_INVALID_ARGUMENT_IF(!decodingPublicKey)

        ZS_LOG_DEBUG(log("set receive keying decoding private/public key") + ", decoding public key fingerprint=" + decodingPublicKey->getFingerprint())

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("already shutdown"))
          return;
        }

        ZS_THROW_INVALID_USAGE_IF(mReceivingDecodingPassphrase.hasData()) // mutually exclusive, can't use both

        if (mReceiveDecodingPrivateKey) {
          ZS_THROW_INVALID_ARGUMENT_IF(mReceiveDecodingPrivateKey != decodingPrivateKey)
        }
        if (mReceiveDecodingPublicKey) {
          ZS_THROW_INVALID_ARGUMENT_IF(mReceiveDecodingPublicKey != decodingPublicKey)
        }

        mReceiveDecodingPrivateKey = decodingPrivateKey;
        mReceiveDecodingPublicKey = decodingPublicKey;

        setState(SessionState_Pending);

        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::setReceiveKeyingDecoding(const char *passphrase)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!passphrase)

        ZS_LOG_DEBUG(log("set receive keying decoding passphrase") + ", passphrase=" + passphrase)

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("already shutdown"))
          return;
        }

        ZS_THROW_INVALID_USAGE_IF(mReceiveDecodingPrivateKey) // mutually exclusive, can't use both

        if (mReceivingDecodingPassphrase.hasData()) {
          ZS_THROW_INVALID_ARGUMENT_IF(passphrase != mReceivingDecodingPassphrase)
        }
        mReceivingDecodingPassphrase = String(passphrase);

        setState(SessionState_Pending);

        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
      }

      //-----------------------------------------------------------------------
      ElementPtr MessageLayerSecurityChannel::getSignedReceivingKeyingMaterial() const
      {
        AutoRecursiveLock lock(getLock());
        return mReceiveKeyingSignedEl;
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::setReceiveKeyingMaterialSigningPublicKey(IRSAPublicKeyPtr remotePublicKey)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!remotePublicKey)

        ZS_LOG_DEBUG(log("receive key signing public key") + ", public key fingerprint=" + remotePublicKey->getFingerprint())

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("already shutdown"))
          return;
        }

        if (mReceiveSigningPublicKey) {
          ZS_THROW_INVALID_ARGUMENT_IF(remotePublicKey != mReceiveSigningPublicKey)
        }

        mReceiveSigningPublicKey = remotePublicKey;

        setState(SessionState_Pending);

        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::setSendKeyingEncoding(IRSAPublicKeyPtr remotePublicKey)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!remotePublicKey)

        ZS_LOG_DEBUG(log("send encoding public key") + ", public key fingerprint=" + remotePublicKey->getFingerprint())

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("already shutdown"))
          return;
        }

        ZS_THROW_INVALID_USAGE_IF(mSendingEncodingPassphrase.hasData())

        if (mSendingEncodingRemotePublicKey) {
          ZS_THROW_INVALID_ARGUMENT_IF(remotePublicKey != mSendingEncodingRemotePublicKey)
        }

        mSendingEncodingRemotePublicKey = remotePublicKey;

        setState(SessionState_Pending);

        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::setSendKeyingEncoding(const char *passphrase)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!passphrase)

        ZS_LOG_DEBUG(log("send keying encoding") + ", passphrase=" + passphrase)

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("already shutdown"))
          return;
        }

        ZS_THROW_INVALID_USAGE_IF(mSendingEncodingRemotePublicKey)

        if (mSendingEncodingPassphrase.hasData()) {
          ZS_THROW_INVALID_ARGUMENT_IF(passphrase != mSendingEncodingPassphrase)
        }

        mSendingEncodingPassphrase = String(passphrase);

        setState(SessionState_Pending);

        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::getSendKeyingMaterialNeedingToBeSigned(
                                                                               DocumentPtr &outDocumentContainedElementToSign,
                                                                               ElementPtr &outElementToSign
                                                                               ) const
      {
        AutoRecursiveLock lock(getLock());

        if ((!mSendKeyingNeedingToSignDoc) ||
            (!mSendKeyingNeedToSignEl)) {
          ZS_LOG_WARNING(Detail, log("no keying material available needing to be signed"))
          return;
        }

        outDocumentContainedElementToSign = mSendKeyingNeedingToSignDoc;
        outElementToSign = mSendKeyingNeedToSignEl;
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::notifySendKeyingMaterialSigned()
      {
        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("already shutdown"))
          return;
        }

        // by clearing out the receive key needing to be signed (but leaving the paired doc), it signals the "step" that the signing process was complete
        mSendKeyingNeedToSignEl.reset();

        setState(SessionState_Pending);

        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageLayerSecurityChannel => ITransportStreamReaderDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::onTransportStreamReaderReady(ITransportStreamReaderPtr reader)
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("transport stream reader ready"))
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageLayerSecurityChannel => ITransportStreamWriterDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::onTransportStreamWriterReady(ITransportStreamWriterPtr writer)
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("transport stream writer ready"))

        if (writer == mReceiveStreamDecoded) {
          get(mReceiveStreamDecodedWriteReady) = true;

          // event typically fires when "outer" notifies it's ready to send data thus need to inform the wire that it can send data now
          mReceiveStreamEncoded->notifyReaderReadyToRead();
        }
        if (writer == mSendStreamEncoded) {
          get(mSendStreamEncodedWriteReady) = true;

          // typically happens when the wire notifies that it's ready to read data thus need to notify outer layer that it can send data
          mSendStreamDecoded->notifyReaderReadyToRead();
        }
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageLayerSecurityChannel => IWakeDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::onWake()
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("on wake"))
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
        return String("MessageLayerSecurityChannel [" + string(mID) + "] " + message);
      }

      //-----------------------------------------------------------------------
      String MessageLayerSecurityChannel::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("mls channel id", string(mID), firstTime) +
               Helper::getDebugValue("subscriptions", mSubscriptions.size() > 0 ? string(mSubscriptions.size()) : String(), firstTime) +
               Helper::getDebugValue("state", IMessageLayerSecurityChannel::toString(mCurrentState), firstTime) +
               Helper::getDebugValue("last error", 0 != mLastError ? string(mLastError) : String(), firstTime) +
               Helper::getDebugValue("last reason", mLastErrorReason, firstTime) +
               Helper::getDebugValue("local context ID", mLocalContextID, firstTime) +
               Helper::getDebugValue("remote context ID", mRemoteContextID, firstTime) +
               Helper::getDebugValue("sending remote public key", mSendingEncodingRemotePublicKey ? String("true") : String(), firstTime) +
               Helper::getDebugValue("sending passphrase", mSendingEncodingPassphrase, firstTime) +
               Helper::getDebugValue("sending keying needs sign doc", mSendKeyingNeedingToSignDoc ? String("true") : String(), firstTime) +
               Helper::getDebugValue("sending keying needs sign element", mSendKeyingNeedToSignEl ? String("true") : String(), firstTime) +
               Helper::getDebugValue("receive seq number", string(mNextReceiveSequenceNumber), firstTime) +
               Helper::getDebugValue("decoding type", toString(mReceiveDecodingType), firstTime) +
               Helper::getDebugValue("decoding public key fingerprint", mReceiveDecodingPublicKeyFingerprint, firstTime) +
               Helper::getDebugValue("receive decoding private key", mReceiveDecodingPrivateKey ? String("true") : String(), firstTime) +
               Helper::getDebugValue("receive decoding public key", mReceiveDecodingPublicKey ? String("true") : String(), firstTime) +
               Helper::getDebugValue("receive decoding passphrase", mReceivingDecodingPassphrase, firstTime) +
               Helper::getDebugValue("receive signing public key", mReceiveSigningPublicKey ? String("true") : String(), firstTime) +
               Helper::getDebugValue("receive keying signed doc", mReceiveKeyingSignedDoc ? String("true") : String(), firstTime) +
               Helper::getDebugValue("receive keying signed element", mReceiveKeyingSignedEl ? String("true") : String(), firstTime) +
               ", receive stream encoded: " + ITransportStream::toDebugString(mReceiveStreamEncoded->getStream(), false) +
               ", receive stream decode: " + ITransportStream::toDebugString(mReceiveStreamDecoded->getStream(), false) +
               ", send stream decoded: " + ITransportStream::toDebugString(mSendStreamDecoded->getStream(), false) +
               ", send stream encoded: " + ITransportStream::toDebugString(mSendStreamEncoded->getStream(), false) +
               Helper::getDebugValue("receive stream encoded subscription", mReceiveStreamEncodedSubscription ? String("true") : String(), firstTime) +
               Helper::getDebugValue("receive stream decoded subscription", mReceiveStreamDecodedSubscription ? String("true") : String(), firstTime) +
               Helper::getDebugValue("send stream decoded subscription", mSendStreamDecodedSubscription ? String("true") : String(), firstTime) +
               Helper::getDebugValue("send stream encoded subscription", mSendStreamEncodedSubscription ? String("true") : String(), firstTime) +
               Helper::getDebugValue("receive stream decoded write ready", mReceiveStreamDecodedWriteReady ? String("true") : String(), firstTime) +
               Helper::getDebugValue("send stream encoded write ready", mSendStreamEncodedWriteReady ? String("true") : String(), firstTime) +
               Helper::getDebugValue("receive keys", mReceiveKeys.size() > 0 ? string(mReceiveKeys.size()) : String(), firstTime) +
               Helper::getDebugValue("send keys", mSendKeys.size() > 0 ? string(mSendKeys.size()) : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      void MessageLayerSecurityChannel::setState(SessionStates state)
      {
        if (state == mCurrentState) return;

        ZS_LOG_DEBUG(log("state changed") + ", state=" + IMessageLayerSecurityChannel::toString(state) + ", old state=" + IMessageLayerSecurityChannel::toString(mCurrentState))
        mCurrentState = state;

        MessageLayerSecurityChannelPtr pThis = mThisWeak.lock();
        if (pThis) {
          ZS_LOG_DEBUG(log("attempting to report state to delegate") + getDebugValueString())
          mSubscriptions.delegate()->onMessageLayerSecurityChannelStateChanged(pThis, mCurrentState);
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
          ZS_LOG_WARNING(Detail, log("error already set thus ignoring new error") + ", new error=" + string(errorCode) + ", new reason=" + reason + getDebugValueString())
          return;
        }

        mLastError = errorCode;
        mLastErrorReason = reason;

        ZS_LOG_WARNING(Detail, log("error set") + ", code=" + string(mLastError) + ", reason=" + mLastErrorReason + getDebugValueString())
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
        if (!mReceiveStreamDecodedWriteReady) {
          ZS_LOG_DEBUG(log("cannot read encoded stream until notified that it's okay to write to decoded stream"))
          return true;
        }

        if ((mReceiveKeyingSignedDoc) &&
            (mReceiveKeyingSignedEl)) {

          if (!mReceiveSigningPublicKey) {
            ZS_LOG_DEBUG(log("waiting for receive keying materials"))
            setState(SessionState_WaitingForNeededInformation);
            return true;
          }

          bool returnResult = false;
          if (!stepProcessReceiveKeying(returnResult)) {
            ZS_LOG_DEBUG(log("receive keying did not complete"))
            return returnResult;
          }

          ZS_LOG_DEBUG(log("receive keying completed"))
        }

        if (mReceiveStreamEncoded->getTotalReadBuffersAvailable() < 1) {
          ZS_LOG_DEBUG(log("nothing to decode"))
          return true;
        }

        if (mRemoteContextID.hasData()) {
          ZS_LOG_DEBUG(log("already decoded at least one packet"))
          if (mReceiveKeys.size() < 1) {
            bool hasReceiveInformation = true;

            switch(mReceiveDecodingType) {
              case DecodingType_Unknown:    break;
              case DecodingType_PrivateKey: hasReceiveInformation = hasReceiveInformation && (mReceiveDecodingPrivateKey); break;
              case DecodingType_Passphrase: hasReceiveInformation = hasReceiveInformation && (mReceivingDecodingPassphrase.hasData()); break;
            }

            hasReceiveInformation = hasReceiveInformation && (mReceiveSigningPublicKey);

            if (!hasReceiveInformation) {
              ZS_LOG_DEBUG(log("waiting for receive keying materials"))
              setState(SessionState_WaitingForNeededInformation);
              return true;
            }
          }
        }

        while (mReceiveStreamEncoded->getTotalReadBuffersAvailable() > 0) {
          ITransportStream::StreamHeaderPtr streamHeader;
          SecureByteBlockPtr streamBuffer = mReceiveStreamEncoded->read(&streamHeader);

          // has to be greater than the size of a DWORD
          if (streamBuffer->SizeInBytes() <= sizeof(DWORD)) {
            ZS_LOG_ERROR(Detail, log("algorithm bytes missing in protocol") + ", size=" + string(streamBuffer->SizeInBytes()))
            setError(IHTTP::HTTPStatusCode_Unauthorized, "buffer is not decodable");
            cancel();
            return false;
          }

          const BYTE *buffer = streamBuffer->BytePtr();

          DWORD algorithm = ntohl(((DWORD *)buffer)[0]);

          const BYTE *source = (const BYTE *)(&(((DWORD *)buffer)[1]));
          SecureByteBlock::size_type remaining = streamBuffer->SizeInBytes() - sizeof(DWORD);

          if (0 != algorithm) {
            // attempt to decode now
            if (mReceiveKeys.size() < 1) {
              ZS_LOG_ERROR(Detail, log("attempting to decode a packet where keying material has not been received"))
              setError(IHTTP::HTTPStatusCode_Forbidden, "attempting to decode a packet where keying material has not been received");
              cancel();
              return false;
            }

            KeyMap::iterator found = mReceiveKeys.find(algorithm);
            if (found == mReceiveKeys.end()) {
              ZS_LOG_ERROR(Detail, log("attempting to decode a packet where keying algorithm does not map to a know key") + ", algorithm=" + string(algorithm))
              setError(IHTTP::HTTPStatusCode_Forbidden, "attempting to decode a packet where keying algorithm does not map to a know key");
              cancel();
              return false;
            }

            // decode the packet
            KeyInfo &keyInfo = (*found).second;

            size_t integritySize = IHelper::getHashDigestSize(IHelper::HashAlgorthm_SHA1);

            // must be greater in size than the hash algorithm
            if (remaining <= integritySize) {
              ZS_LOG_ERROR(Detail, log("algorithm bytes missing in protocol") + ", size=" + string(streamBuffer->SizeInBytes()))
              setError(IHTTP::HTTPStatusCode_Unauthorized, "buffer is not decodable");
              cancel();
              return false;
            }


            SecureByteBlockPtr integrity(new SecureByteBlock(integritySize));
            memcpy(integrity->BytePtr(), source, integritySize);

            source += integritySize;
            remaining -= integritySize;

            SecureByteBlock input(remaining);
            memcpy(input.BytePtr(), source, remaining);

            SecureByteBlockPtr output = IHelper::decrypt(*(keyInfo.mSendKey), *(keyInfo.mNextIV), input);

            if (!output) {
              ZS_LOG_ERROR(Detail, log("unable to decrypte buffer"))
              setError(IHTTP::HTTPStatusCode_Unauthorized, "unable to decrypt buffer");
              cancel();
              return false;
            }

            String hexIV = IHelper::convertToHex(*keyInfo.mNextIV);

            SecureByteBlockPtr calculatedIntegrity = IHelper::hmac(*(IHelper::convertToBuffer(keyInfo.mIntegrityPassphrase)), ("integrity:" + IHelper::convertToHex(*IHelper::hash(*output)) + ":" + hexIV).c_str());

            ZS_LOG_DEBUG(log("received data from wire") + ", buffer size=" + string(streamBuffer->SizeInBytes()) + ", encrypted size=" + string(input.SizeInBytes()) + ", decrypted size=" + string(output->SizeInBytes()) + ", key=" + IHelper::convertToHex(*(keyInfo.mSendKey)) + ", iv=" + hexIV + ", calculated integrity=" + IHelper::convertToHex(*calculatedIntegrity) + ", received integrity=" + IHelper::convertToHex(*integrity))

            if (0 != IHelper::compare(*calculatedIntegrity, *integrity)) {
              ZS_LOG_ERROR(Debug,log("integrity failed on packet"))
              setError(IHTTP::HTTPStatusCode_Unauthorized, "buffer is not decodable");
              cancel();
              return false;
            }

            // calculate the next IV and remember the integrity field
            keyInfo.mNextIV = IHelper::hash(hexIV + ":" + IHelper::convertToHex(*calculatedIntegrity));
            keyInfo.mLastIntegrity = calculatedIntegrity;

            mReceiveStreamDecoded->write(output, streamHeader);

            // process next buffer
            continue;
          }

          bool returnResult = false;
          if (!stepProcessReceiveKeying(returnResult, streamBuffer)) {
            ZS_LOG_DEBUG(log("receive keying did not complete"))
            return returnResult;
          }
        }

        return true;
      }

      //-----------------------------------------------------------------------
      bool MessageLayerSecurityChannel::stepProcessReceiveKeying(
                                                                 bool &outReturnResult,
                                                                 SecureByteBlockPtr keying
                                                                 )
      {
        outReturnResult = true;

        // parse the buffer
        DocumentPtr doc;

        if (mReceiveKeyingSignedDoc) {
          // reuse the signing doc if one exists (to avoid decoding the first packet twice)
          doc = mReceiveKeyingSignedDoc;

          mReceiveKeyingSignedDoc.reset();
          mReceiveKeyingSignedEl.reset();
        }

        if (!doc) {
          const BYTE *buffer = keying->BytePtr();

          const BYTE *source = (const BYTE *)(&(((DWORD *)buffer)[1]));
          SecureByteBlock::size_type remaining = keying->SizeInBytes() - sizeof(DWORD);

          // create a NUL terminated JSON string buffer
          SecureByteBlockPtr jsonBuffer =  IHelper::convertToBuffer(source, remaining);

          ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
          ZS_LOG_DETAIL(log("[ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ ["))
          ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
          ZS_LOG_DETAIL(log("MLS RECEIVE") + "=" + "\n" + ((CSTR)(jsonBuffer->BytePtr())) + "\n")
          ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
          ZS_LOG_DETAIL(log("[ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ [ ["))
          ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))

          // clear out the receive signed document since it's validated
          doc = Document::createFromAutoDetect((const char *)(jsonBuffer->BytePtr()));
        }

        mReceiveKeys.clear(); // all previous keys are being destroyed

        try {
          ElementPtr keyingEl = doc->findFirstChildElementChecked("keyingBundle")->findFirstChildElementChecked("keying");

          if (mRemoteContextID.isEmpty()) {
            mRemoteContextID = getElementTextAndDecode(keyingEl->findFirstChildElement("context"));
          }

          if (!mReceiveSigningPublicKey) {
            ZS_LOG_DEBUG(log("waiting for receive material signing public key"))

            mReceiveKeyingSignedDoc = doc;
            mReceiveKeyingSignedEl = keyingEl;

            setState(SessionState_WaitingForNeededInformation);
            return false;
          }

          if (!mReceiveSigningPublicKey->verifySignature(keyingEl)) {
            ZS_LOG_ERROR(Detail, log("failed to validate receiving stream signature"))
            setError(IHTTP::HTTPStatusCode_Forbidden, "keying encoding not using expecting passphrase");
            cancel();
            outReturnResult = false;
            return false;
          }

          String sequenceNumber = getElementTextAndDecode(keyingEl->findFirstChildElement("sequence"));

          if (sequenceNumber != string(mNextReceiveSequenceNumber)) {
            ZS_LOG_ERROR(Detail, log("sequence number mismatch") + ", sequence=" + sequenceNumber + ", expecting=" + string(mNextReceiveSequenceNumber))
            setError(IHTTP::HTTPStatusCode_RequestTimeout, "sequence number mismatch");
            cancel();
            outReturnResult = false;
            return false;
          }

          Time expires = IHelper::stringToTime(getElementTextAndDecode(keyingEl->findFirstChildElement("expires")));
          Time tick = zsLib::now();
          if (tick > expires) {
            ZS_LOG_ERROR(Detail, log("signed keying bundle has expired") + ", expires=" + IHelper::timeToString(expires) + ", now=" + IHelper::timeToString(tick))
            setError(IHTTP::HTTPStatusCode_RequestTimeout, "signed keying bundle has expired");
            cancel();
            outReturnResult = false;
            return false;
          }

          String nonce = getElementTextAndDecode(keyingEl->findFirstChildElementChecked("nonce"));

          ElementPtr encodingEl = keyingEl->findFirstChildElementChecked("encoding");
          String type = getElementTextAndDecode(encodingEl->findFirstChildElementChecked("type"));

          if ("pki" == type) {
            mReceiveDecodingType = DecodingType_PrivateKey;
            String encodingFingerprint = getElementTextAndDecode(encodingEl->findFirstChildElementChecked("fingerprint"));

            if (!mReceiveDecodingPublicKeyFingerprint.hasData()) {
              mReceiveDecodingPublicKeyFingerprint = encodingFingerprint;
            }

            if (!mReceiveDecodingPrivateKey) {
              ZS_LOG_DEBUG(log("waiting for keying materials"))

              mReceiveKeyingSignedDoc = doc;
              mReceiveKeyingSignedEl = keyingEl;

              setState(SessionState_WaitingForNeededInformation);

              outReturnResult = false;
              return false;
            }

            ZS_THROW_BAD_STATE_IF(!mReceiveDecodingPublicKey)

            String expectingFingerprint = mReceiveDecodingPublicKey->getFingerprint();
            if (encodingFingerprint != expectingFingerprint) {
              ZS_LOG_ERROR(Detail, log("encoding not using local public key") + ", encoding fingerprint=" + encodingFingerprint + ", expecting fingerprint=" + expectingFingerprint)
              setError(IHTTP::HTTPStatusCode_RequestTimeout, "signed keying bundle has expired");
              cancel();
              outReturnResult = false;
              return false;
            }
          } else if ("passphrase" == type) {
            mReceiveDecodingType = DecodingType_Passphrase;

            if (mReceivingDecodingPassphrase.isEmpty()) {
              ZS_LOG_DEBUG(log("cannot continue decoding as missing decoding passphrase (will notify delegate)"))

              mReceiveKeyingSignedDoc = doc;
              mReceiveKeyingSignedEl = keyingEl;

              setState(SessionState_WaitingForNeededInformation);

              outReturnResult = false;
              return false;
            }

            // scope: we have a passphrase, see if the proof validates before attempting to decrypt any keys...
            {
              String algorithm = getElementTextAndDecode(encodingEl->findFirstChildElementChecked("algorithm"));
              if (OPENPEER_SERVICES_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM != algorithm) {
                ZS_LOG_ERROR(Detail, log("keying encoding not using known algorithm") + ", algorithm=" + algorithm + ", expecting=" + OPENPEER_SERVICES_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM)
                setError(IHTTP::HTTPStatusCode_ExpectationFailed, "keyhing encoding not using expecting passphrase");
                cancel();
                outReturnResult = false;
                return false;
              }

              String proof = getElementTextAndDecode(encodingEl->findFirstChildElementChecked("proof"));

              // hex(hmac(`<external-passphrase>`, "keying:" + `<nonce>`))
              String calculatedProof = IHelper::convertToHex(*IHelper::hmac(*IHelper::hmacKeyFromPassphrase(mReceivingDecodingPassphrase), "keying:" + nonce));

              if (proof != calculatedProof) {
                ZS_LOG_ERROR(Detail, log("keying encoding not using expecting passphrase") + ", encoding proof=" + proof + ", expecting proof=" + calculatedProof + ", using passphrase=" + mReceivingDecodingPassphrase)
                setError(IHTTP::HTTPStatusCode_ExpectationFailed, "keyhing encoding not using expecting passphrase");
                cancel();
                outReturnResult = false;
                return false;
              }
            }
          }

#define WARNING_CHECK_NONCE_NOT_SEEN_BEFORE 1
#define WARNING_CHECK_NONCE_NOT_SEEN_BEFORE 2

          // scope: santity check on algorithms receiving
          {
            bool found = false;
            ElementPtr algorithmEl = keyingEl->findFirstChildElementChecked("algorithms")->findFirstChildElementChecked("algorithm");
            while (algorithmEl) {
              String algorithm = getElementTextAndDecode(algorithmEl);
              if (OPENPEER_SERVICES_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM == algorithm) {
                ZS_LOG_TRACE(log("found mandated algorithm"))
                found = true;
                break;
              }
              algorithmEl->findNextSiblingElement("algorithm");
            }
            if (!found) {
              ZS_LOG_ERROR(Detail, log("did not find mandated MLS algorithm") + ", expecting=" + OPENPEER_SERVICES_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM)
              setError(IHTTP::HTTPStatusCode_ExpectationFailed, "did not find mandated MLS algorithm");
              cancel();
              outReturnResult = false;
              return false;
            }
          }

          // scope: extract out latest keying material
          {
            ElementPtr keyEl = keyingEl->findFirstChildElementChecked("keys")->findFirstChildElementChecked("key");
            while (keyEl) {
              // scope: decode key
              {
                AlgorithmIndex index = 0;
                try {
                  index = Numeric<AlgorithmIndex>(getElementTextAndDecode(keyEl->findFirstChildElementChecked("index")));
                } catch(Numeric<AlgorithmIndex>::ValueOutOfRange &) {
                  ZS_LOG_WARNING(Detail, log("algorithm index value out of range"))
                }

                if (0 == index) {
                  ZS_LOG_WARNING(Detail, log("algorithm index value is not valid") + ", index=" + string(index))
                  goto next_key;
                }

                String algorithm = getElementTextAndDecode(keyEl->findFirstChildElementChecked("algorithm"));
                if (OPENPEER_SERVICES_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM != algorithm) {
                  ZS_LOG_WARNING(Detail, log("unsupported algorithm (thus skipping)") + ", algorithm=" + algorithm)
                  goto next_key;
                }

                ElementPtr inputs = keyEl->findFirstChildElementChecked("inputs");

                KeyInfo key;
                SecureByteBlockPtr integrityPassphrase;
                if (DecodingType_PrivateKey == mReceiveDecodingType) {
                  // base64(rsa_encrypt(`<remote-public-key>`, `<value>`))
                  key.mSendKey = mReceiveDecodingPrivateKey->decrypt(*IHelper::convertFromBase64(getElementTextAndDecode(inputs->findFirstChildElementChecked("secret"))));
                  key.mNextIV = mReceiveDecodingPrivateKey->decrypt(*IHelper::convertFromBase64(getElementTextAndDecode(inputs->findFirstChildElementChecked("iv"))));
                  integrityPassphrase = mReceiveDecodingPrivateKey->decrypt(*IHelper::convertFromBase64(getElementTextAndDecode(inputs->findFirstChildElementChecked("hmacIntegrityKey"))));
                } else {
                  key.mSendKey = decryptUsingPassphraseEncoding(mReceivingDecodingPassphrase, nonce, getElementTextAndDecode(inputs->findFirstChildElementChecked("secret")));
                  key.mNextIV = decryptUsingPassphraseEncoding(mReceivingDecodingPassphrase, nonce, getElementTextAndDecode(inputs->findFirstChildElementChecked("iv")));
                  integrityPassphrase = decryptUsingPassphraseEncoding(mReceivingDecodingPassphrase, nonce, getElementTextAndDecode(inputs->findFirstChildElementChecked("hmacIntegrityKey")));
                }
                if (integrityPassphrase) {
                  key.mIntegrityPassphrase = IHelper::convertToString(*integrityPassphrase);
                }
                if ((!key.mSendKey) ||
                    (!key.mNextIV) ||
                    (key.mIntegrityPassphrase.isEmpty())) {
                  ZS_LOG_WARNING(Detail, log("algorithm missing vital secret, iv or integrity information") + ", index=" + string(index))
                  goto next_key;
                }

                ZS_LOG_DEBUG(log("adding algorithm to available keying information") + ", index=" + string(index))
                mReceiveKeys[index] = key;
              }

            next_key:
              keyEl = keyEl->findNextSiblingElement("key");
            }
          }

        } catch(CheckFailed &) {
          ZS_LOG_ERROR(Detail, log("expecting element in keying bundle that was missing"))
        }

        if (mReceiveKeys.size() < 1) {
          ZS_LOG_ERROR(Detail, log("did not find any key information"))
          setError(IHTTP::HTTPStatusCode_ExpectationFailed, "did not find any key information");
          cancel();
          outReturnResult = false;
          return false;
        }

        ZS_LOG_DEBUG(log("successfully extracted keying materials to receive data from remote MLS stream"))
        ++mNextReceiveSequenceNumber;

        return true;
      }

      //-----------------------------------------------------------------------
      bool MessageLayerSecurityChannel::stepSendKeying()
      {
        if (!mSendStreamEncodedWriteReady) {
          ZS_LOG_DEBUG(log("cannot send encoded stream until lower layer transport (typically 'wire' transport) indicates it is ready to send data"))
          return false;
        }

        if (mLocalContextID.isEmpty()) {
          ZS_LOG_DEBUG(log("missing local context ID thus cannot send data remotely"))
          return false;
        }

        if ((!mSendingEncodingRemotePublicKey) &&
            (mSendingEncodingPassphrase.isEmpty())) {
          ZS_LOG_DEBUG(log("send keying material is not ready"))
          setState(SessionState_WaitingForNeededInformation);
          return false;
        }

        if (mSendKeyingNeedingToSignDoc) {
          if (mSendKeyingNeedToSignEl) {
            ZS_LOG_DEBUG(log("send signature not created"))
            return false;
          }

          ElementPtr keyingEl;
          try {
            keyingEl = mSendKeyingNeedingToSignDoc->findFirstChildElementChecked("keyingBundle")->findFirstChildElementChecked("keying");
          } catch(CheckFailed &) {
          }

          if (!keyingEl) {
            ZS_LOG_ERROR(Detail, log("failed to obtain signed keying element"))
            setError(IHTTP::HTTPStatusCode_BadRequest, "failed to obtain signed keying element");
            cancel();
            return false;
          }

          // developer using this class should have signed this bundle if enters this spot
          ElementPtr signatureEl;
          IHelper::getSignatureInfo(keyingEl, &signatureEl);
          ZS_THROW_INVALID_USAGE_IF(!signatureEl)

          // signature has been applied
          ULONG outputLength = 0;
          GeneratorPtr generator = Generator::createJSONGenerator();
          boost::shared_array<char> output = generator->write(mSendKeyingNeedingToSignDoc, &outputLength);

          SecureByteBlockPtr buffer(new SecureByteBlock(sizeof(DWORD) + (outputLength * sizeof(char))));

          ((DWORD *)(buffer->BytePtr()))[0] = htonl(0);

          BYTE *dest = (buffer->BytePtr() + sizeof(DWORD));

          memcpy(dest, output.get(), sizeof(char)*outputLength);

          if (ZS_IS_LOGGING(Trace)) {
            ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
            ZS_LOG_DETAIL(log("] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ]"))
            ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
            ZS_LOG_DETAIL(log("MLS SENDING") + "=" + "\n" + ((CSTR)(output.get())) + "\n")
            ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
            ZS_LOG_DETAIL(log("] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ] ]"))
            ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
          }

          mSendStreamEncoded->write(buffer);

          mSendKeyingNeedingToSignDoc.reset();

          return true;
        }

        if (mSendKeys.size() > 0) {
          ZS_LOG_DEBUG(log("already sent keying materials"))
          return true;
        }

        // create initial encryption offer (hint: it won't change)

        ElementPtr keyingBundleEl = Element::create("keyingBundle");

        ElementPtr keyingEl = Element::create("keying");
        
        keyingEl->adoptAsLastChild(createElementWithNumber("sequence", "0"));

        String nonce = IHelper::randomString(32);

        keyingEl->adoptAsLastChild(createElementWithText("nonce", nonce));
        keyingEl->adoptAsLastChild(createElementWithText("context", mLocalContextID));

        Time expires = zsLib::now() + Seconds(OPENPEER_SERVICES_MLS_DEFAULT_KEYING_EXPIRES_TIME_IN_SECONDS);

        keyingEl->adoptAsLastChild(createElementWithNumber("expires", IHelper::timeToString(expires)));

        ElementPtr encodingEl = Element::create("encoding");
        keyingEl->adoptAsLastChild(encodingEl);

        if (mSendingEncodingPassphrase.hasData()) {
          encodingEl->adoptAsLastChild(createElementWithText("type", "passphrase"));
          encodingEl->adoptAsLastChild(createElementWithText("algorithm", OPENPEER_SERVICES_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM));

          String calculatedProof = IHelper::convertToHex(*IHelper::hmac(*IHelper::hmacKeyFromPassphrase(mSendingEncodingPassphrase), "keying:" + nonce));

          encodingEl->adoptAsLastChild(createElementWithText("proof", calculatedProof));
        } else {
          ZS_THROW_INVALID_ASSUMPTION_IF(!mSendingEncodingRemotePublicKey)
          encodingEl->adoptAsLastChild(createElementWithText("type", "pki"));
          encodingEl->adoptAsLastChild(createElementWithText("fingerprint", mSendingEncodingRemotePublicKey->getFingerprint()));
        }

        ElementPtr algorithmsEl = Element::create("algorithms");
        algorithmsEl->adoptAsLastChild(createElementWithText("algorithm", OPENPEER_SERVICES_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM));

        ElementPtr keysEl = Element::create("keys");

        for (AlgorithmIndex index = 1; index <= OPENPEER_SERVICES_MESSAGE_LAYER_SECURITY_DEFAULT_TOTAL_SEND_KEYS; ++index) {
          KeyInfo key;

          key.mIntegrityPassphrase = IHelper::randomString((20*8/5));
          key.mSendKey = IHelper::hash(*IHelper::random(32), IHelper::HashAlgorthm_SHA256);
          key.mNextIV = IHelper::hash(*IHelper::random(16), IHelper::HashAlgorthm_MD5);

          ElementPtr keyEl = Element::create("key");
          keyEl->adoptAsLastChild(createElementWithNumber("index", string(index)));
          keyEl->adoptAsLastChild(createElementWithText("algorithm", OPENPEER_SERVICES_MESSAGE_LAYER_SECURITY_DEFAULT_CRYPTO_ALGORITHM));

          ElementPtr inputsEl = Element::create("inputs");
          if (mSendingEncodingPassphrase.hasData()) {
            inputsEl->adoptAsLastChild(createElementWithText("secret", encodeUsingPassphraseEncoding(mSendingEncodingPassphrase, nonce, *key.mSendKey)));
            inputsEl->adoptAsLastChild(createElementWithText("iv", encodeUsingPassphraseEncoding(mSendingEncodingPassphrase, nonce, *key.mNextIV)));
            inputsEl->adoptAsLastChild(createElementWithText("hmacIntegrityKey", encodeUsingPassphraseEncoding(mSendingEncodingPassphrase, nonce, *IHelper::convertToBuffer(key.mIntegrityPassphrase))));
          } else {
            ZS_THROW_INVALID_ASSUMPTION_IF(!mSendingEncodingRemotePublicKey)
            inputsEl->adoptAsLastChild(createElementWithText("secret", IHelper::convertToBase64(*mSendingEncodingRemotePublicKey->encrypt(*key.mSendKey))));
            inputsEl->adoptAsLastChild(createElementWithText("iv", IHelper::convertToBase64(*mSendingEncodingRemotePublicKey->encrypt(*key.mNextIV))));
            inputsEl->adoptAsLastChild(createElementWithText("hmacIntegrityKey", IHelper::convertToBase64(*mSendingEncodingRemotePublicKey->encrypt(*IHelper::convertToBuffer(key.mIntegrityPassphrase)))));
          }

          keyEl->adoptAsLastChild(inputsEl);

          keysEl->adoptAsLastChild(keyEl);

          mSendKeys[index] = key;
        }

        keyingEl->adoptAsLastChild(algorithmsEl);
        keyingEl->adoptAsLastChild(keysEl);
        keyingBundleEl->adoptAsLastChild(keyingEl);

        mSendKeyingNeedingToSignDoc = Document::create();
        mSendKeyingNeedingToSignDoc->adoptAsLastChild(keyingBundleEl);
        mSendKeyingNeedToSignEl = keyingEl;

        ZS_LOG_DEBUG(log("waiting for sending keying information to be signed locally"))

        setState(SessionState_WaitingForNeededInformation);
        return false;
      }

      //-----------------------------------------------------------------------
      bool MessageLayerSecurityChannel::stepSend()
      {
        if (mSendKeys.size() < 1) {
          ZS_LOG_DEBUG(log("no send keys set, not sending yet..."))
          return false;
        }

        if (mSendStreamDecoded->getTotalReadBuffersAvailable() < 1) {
          ZS_LOG_DEBUG(log("no data to be sent over the wire"))
          return true;
        }

        while (mSendStreamDecoded->getTotalReadBuffersAvailable() > 0) {
          StreamHeaderPtr header;
          SecureByteBlockPtr buffer = mSendStreamDecoded->read(&header);

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

          ZS_LOG_DEBUG(log("sending data on wire") + ", buffer size=" + string(output->SizeInBytes()) + ", decrypted size=" + string(buffer->SizeInBytes()) + ", encrypted size=" + string(encrypted->SizeInBytes()) + ", key=" + IHelper::convertToHex(*(keyInfo.mSendKey)) + ", iv=" + hexIV + ", integrity=" + IHelper::convertToHex(*calculatedIntegrity))
          mSendStreamEncoded->write(output, header);
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
        case SessionState_Pending:                      return "Pending";
        case SessionState_WaitingForNeededInformation:  return "Waiting for needed information";
        case SessionState_Connected:                    return "Connected";
        case SessionState_Shutdown:                     return "Shutdown";
      }
      return "UNDEFINED";
    }

    //-----------------------------------------------------------------------
    String IMessageLayerSecurityChannel::toDebugString(IMessageLayerSecurityChannelPtr channel, bool includeCommaPrefix)
    {
      return internal::MessageLayerSecurityChannel::toDebugString(channel, includeCommaPrefix);
    }

    //-----------------------------------------------------------------------
    IMessageLayerSecurityChannelPtr IMessageLayerSecurityChannel::create(
                                                                         IMessageLayerSecurityChannelDelegatePtr delegate,
                                                                         ITransportStreamPtr receiveStreamEncoded,
                                                                         ITransportStreamPtr receiveStreamDecoded,
                                                                         ITransportStreamPtr sendStreamDecoded,
                                                                         ITransportStreamPtr sendStreamEncoded,
                                                                         const char *contextID
                                                                         )
    {
      return internal::IMessageLayerSecurityChannelFactory::singleton().create(delegate, receiveStreamEncoded, receiveStreamDecoded, sendStreamDecoded, sendStreamEncoded, contextID);
    }
  }
}
