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

#include <openpeer/stack/internal/stack_BootstrappedNetwork.h>
#include <openpeer/stack/internal/stack_BootstrappedNetworkManager.h>
#include <openpeer/stack/internal/stack_Helper.h>
#include <openpeer/stack/internal/stack_PeerFilePrivate.h>
#include <openpeer/stack/internal/stack_Stack.h>
#include <openpeer/stack/IHelper.h>
#include <openpeer/stack/IPeer.h>
#include <openpeer/stack/IMessageMonitor.h>
#include <openpeer/stack/IRSAPublicKey.h>

#include <openpeer/stack/message/bootstrapper/ServicesGetRequest.h>
#include <openpeer/stack/message/bootstrapper/ServicesGetResult.h>
#include <openpeer/stack/message/certificates/CertificatesGetRequest.h>
#include <openpeer/stack/message/certificates/CertificatesGetResult.h>

#include <zsLib/Log.h>
#include <zsLib/helpers.h>
#include <zsLib/XML.h>

#define HOOKFLASH_STACK_BOOTSTRAPPED_NETWORK_MAX_REDIRECTION_ATTEMPTS (5)

#define HOOKFLASH_STACK_BOOTSTRAPPED_NETWORK_DEFAULT_MIME_TYPE "text/json"

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      using namespace stack::message;
      using namespace stack::message::bootstrapper;
      using namespace stack::message::certificates;

      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IBootstrappedNetworkForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IBootstrappedNetworkForAccountFinder
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IBootstrappedNetworkForServices
      #pragma mark

      //-----------------------------------------------------------------------
      BootstrappedNetworkPtr IBootstrappedNetworkForServices::prepare(
                                                                      const char *domain,
                                                                      IBootstrappedNetworkDelegatePtr delegate
                                                                      )
      {
        return IBootstrappedNetworkFactory::singleton().prepare(domain, delegate);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork
      #pragma mark

      //-----------------------------------------------------------------------
      const char *BootstrappedNetwork::toString(ErrorCodes errorCode)
      {
        return IHTTP::toString(IHTTP::toStatusCode(errorCode));
      }

      //-----------------------------------------------------------------------
      BootstrappedNetwork::BootstrappedNetwork(
                                               IMessageQueuePtr queue,
                                               const char *domain
                                               ) :
        zsLib::MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mManager(IBootstrappedNetworkManagerForBootstrappedNetwork::singleton()),
        mDomain(domain ? String(domain) : String()),
        mCompleted(false),
        mErrorCode(0),
        mRedirectionAttempts(0)
      {
        ZS_LOG_DEBUG(log("created") + getDebugValueString())
        mDomain.toLower();
      }

      //-----------------------------------------------------------------------
      BootstrappedNetwork::~BootstrappedNetwork()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed") + getDebugValueString())

        cancel();
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::init()
      {
      }

      //-----------------------------------------------------------------------
      BootstrappedNetworkPtr BootstrappedNetwork::convert(IBootstrappedNetworkPtr network)
      {
        return boost::dynamic_pointer_cast<BootstrappedNetwork>(network);
      }

      //-----------------------------------------------------------------------
      BootstrappedNetworkPtr BootstrappedNetwork::convert(IServiceCertificatesPtr network)
      {
        return boost::dynamic_pointer_cast<BootstrappedNetwork>(network);
      }

      //-----------------------------------------------------------------------
      BootstrappedNetworkPtr BootstrappedNetwork::convert(IServiceIdentityPtr network)
      {
        return boost::dynamic_pointer_cast<BootstrappedNetwork>(network);
      }

      //-----------------------------------------------------------------------
      BootstrappedNetworkPtr BootstrappedNetwork::convert(IServiceLockboxPtr network)
      {
        return boost::dynamic_pointer_cast<BootstrappedNetwork>(network);
      }

      //-----------------------------------------------------------------------
      BootstrappedNetworkPtr BootstrappedNetwork::convert(IServiceSaltPtr network)
      {
        return boost::dynamic_pointer_cast<BootstrappedNetwork>(network);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => IBootstrappedNetwork
      #pragma mark

      //-----------------------------------------------------------------------
      String BootstrappedNetwork::toDebugString(IBootstrappedNetworkPtr network, bool includeCommaPrefix)
      {
        if (!network) return includeCommaPrefix ? String(", network=(null)") : String("network=(null)");
        return BootstrappedNetwork::convert(network)->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      BootstrappedNetworkPtr BootstrappedNetwork::prepare(
                                                          const char *inDomain,
                                                          IBootstrappedNetworkDelegatePtr delegate
                                                          )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!inDomain)

        String domain(inDomain ? String(inDomain) : String());
        ZS_THROW_INVALID_ARGUMENT_IF(domain.isEmpty())

        domain.toLower();

        BootstrappedNetworkManagerPtr manager = IBootstrappedNetworkManagerForBootstrappedNetwork::singleton();
        ZS_THROW_BAD_STATE_IF(!manager)

        BootstrappedNetworkPtr pThis(new BootstrappedNetwork(IStackForInternal::queueStack(), domain));
        pThis->mThisWeak = pThis;
        pThis->init();

        AutoRecursiveLock lock(pThis->getLock());

        BootstrappedNetworkPtr useThis = manager->forBootstrappedNetwork().findExistingOrUse(pThis);

        if (pThis->getID() != useThis->getID()) {
          ZS_LOG_DEBUG(useThis->log("reusing existing object") + useThis->getDebugValueString())
          useThis->reuse();
          pThis->dontUse();
        }

        if (delegate) {
          manager->forBootstrappedNetwork().registerDelegate(useThis, delegate);
        }

        if (pThis->getID() == useThis->getID()) {
          ZS_LOG_DEBUG(useThis->log("preparing new object") + pThis->getDebugValueString())
          pThis->go();
        }
        return useThis;
      }

      //-----------------------------------------------------------------------
      String BootstrappedNetwork::getDomain() const
      {
        return mDomain;
      }

      //-----------------------------------------------------------------------
      bool BootstrappedNetwork::isPreparationComplete() const
      {
        AutoRecursiveLock lock(getLock());
        return mCompleted;
      }

      //-----------------------------------------------------------------------
      bool BootstrappedNetwork::wasSuccessful(
                                              WORD *outErrorCode,
                                              String *outErrorReason
                                              ) const
      {
        AutoRecursiveLock lock(getLock());

        if (outErrorCode) *outErrorCode = mErrorCode;
        if (outErrorReason) *outErrorReason = mErrorReason;

        return (0 == mErrorCode);
      }

      //-----------------------------------------------------------------------
      bool BootstrappedNetwork::sendServiceMessage(
                                                   const char *serviceType,
                                                   const char *serviceMethodName,
                                                   message::MessagePtr message
                                                   )
      {
        ZS_LOG_DEBUG(log("sending message to service") + ", type=" + serviceType + ", method=" + serviceMethodName + Message::toDebugString(message))
        AutoRecursiveLock lock(getLock());
        if (!mCompleted) {
          ZS_LOG_WARNING(Detail, log("bootstrapper isn't complete and thus cannot send message"))
          return false;
        }

        if (!message) {
          ZS_LOG_WARNING(Detail, log("bootstrapped was asked to send a null message"))
          return false;
        }

        const Service::Method *service = findServiceMethod(serviceType, serviceMethodName);
        if (!service) {
          ZS_LOG_WARNING(Detail, log("failed to find service to send to") + ", type=" + serviceType + ", method=" + serviceMethodName)
          if ((message->isRequest()) ||
              (message->isNotify())) {
            MessageResultPtr result = MessageResult::create(message, ErrorCode_NotFound, toString(ErrorCode_NotFound));
            if (!result) {
              ZS_LOG_WARNING(Detail, log("failed to create result for message"))
              return false;
            }
            IMessageMonitor::handleMessageReceived(result);
          }
          return false;
        }

        if (service->mURI.isEmpty()) {
          ZS_LOG_WARNING(Detail, log("failed to find service URI to send to") + ", type=" + serviceType + ", method=" + serviceMethodName)
          return false;
        }

        IHTTPQueryPtr query = post(service->mURI, message);
        if (!query) {
          ZS_LOG_WARNING(Detail, log("failed to create query for message"))
          if ((message->isRequest()) ||
              (message->isNotify())) {
            MessageResultPtr result = MessageResult::create(message, ErrorCode_InternalServerError, toString(ErrorCode_InternalServerError));
            if (!result) {
              ZS_LOG_WARNING(Detail, log("failed to create result for message"))
              return false;
            }
            IMessageMonitor::handleMessageReceived(result);
          }
          return false;
        }

        mPendingRequests[query] = message;
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => IServiceCertificates
      #pragma mark

      //-------------------------------------------------------------------------
      IBootstrappedNetworkPtr BootstrappedNetwork::getBootstrappedNetwork() const
      {
        return mThisWeak.lock();
      }
      
      //-----------------------------------------------------------------------
      IServiceCertificatesPtr BootstrappedNetwork::createServiceCertificatesFrom(IBootstrappedNetworkPtr preparedBootstrappedNetwork)
      {
        return BootstrappedNetwork::convert(preparedBootstrappedNetwork);
      }

      //-----------------------------------------------------------------------
      bool BootstrappedNetwork::isValidSignature(ElementPtr signedElement) const
      {
        ElementPtr signatureEl;
        String id;
        String domain;
        String service;
        signedElement = IHelper::getSignatureInfo(signedElement, &signatureEl, NULL, &id, &domain, &service);

        if (!signedElement) {
          ZS_LOG_WARNING(Detail, log("signature validation failed because no signed element found"))
          return false;
        }

        if (domain != mDomain) {
          ZS_LOG_WARNING(Detail, log("signature is not valid as domains do not match") + ", signature domain=" + domain + ", domain=" + mDomain)
          return false;
        }

        CertificateMap::const_iterator found = mCertificates.find(id);
        if (found == mCertificates.end()) {
          ZS_LOG_WARNING(Detail, log("no signature with the id specified found") + ", signature id=" + id)
          return false;
        }

        const Certificate &certificate = (*found).second;
        if (certificate.mService != service) {
          ZS_LOG_WARNING(Detail, log("signature is not valid as services do not match") + ", signature service=" + service + certificate.getDebugValueString())
          return false;
        }

        if (!certificate.mPublicKey) {
          ZS_LOG_WARNING(Detail, log("certificate missing public key") + certificate.getDebugValueString())
          return false;
        }

        // found the signature reference, now check if the peer URIs match - they must...
        try {
          String algorithm = signatureEl->findFirstChildElementChecked("algorithm")->getTextDecoded();
          if (algorithm != HOOKFLASH_STACK_PEER_FILE_SIGNATURE_ALGORITHM) {
            ZS_LOG_WARNING(Detail, log("signature validation algorithm is not understood, algorithm=") + algorithm)
            return false;
          }

          String signatureDigestAsString = signatureEl->findFirstChildElementChecked("digestValue")->getTextDecoded();

          GeneratorPtr generator = Generator::createJSONGenerator();
          boost::shared_array<char> signedElAsJSON = generator->write(signedElement);

          SecureByteBlockPtr actualDigest = IHelper::hash((const char *)(signedElAsJSON.get()), IHelper::HashAlgorthm_SHA1);

          if (0 == IHelper::compare(*actualDigest, *IHelper::convertFromBase64(signatureDigestAsString))) {
            ZS_LOG_WARNING(Detail, log("digest values did not match, signature digest=") + signatureDigestAsString + ", actual digest=" + IHelper::convertToBase64(*actualDigest))
            return false;
          }

          SecureByteBlockPtr signatureDigestSigned = IHelper::convertFromBase64(signatureEl->findFirstChildElementChecked("digestSigned")->getTextDecoded());

          if (!certificate.mPublicKey->verify(*actualDigest, *signatureDigestSigned)) {
            ZS_LOG_WARNING(Detail, log("signature failed to validate") + certificate.getDebugValueString())
            return false;
          }
        } catch(CheckFailed &) {
          ZS_LOG_WARNING(Detail, log("signature failed to validate due to missing signature element") + certificate.getDebugValueString())
          return false;
        }
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => IServiceIdentity
      #pragma mark

      //-----------------------------------------------------------------------
      IServiceIdentityPtr BootstrappedNetwork::createServiceIdentityFrom(IBootstrappedNetworkPtr bootstrappedNetwork)
      {
        return BootstrappedNetwork::convert(bootstrappedNetwork);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => IServiceLockbox
      #pragma mark

      //-----------------------------------------------------------------------
      IServiceLockboxPtr BootstrappedNetwork::createServiceLockboxFrom(IBootstrappedNetworkPtr bootstrappedNetwork)
      {
        return BootstrappedNetwork::convert(bootstrappedNetwork);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => IServiceSalt
      #pragma mark

      //-----------------------------------------------------------------------
      IServiceSaltPtr BootstrappedNetwork::createServiceSaltFrom(IBootstrappedNetworkPtr bootstrappedNetwork)
      {
        return BootstrappedNetwork::convert(bootstrappedNetwork);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => IBootstrappedNetworkForCertificateServiceValidationQuery
      #pragma mark

      //-----------------------------------------------------------------------
      String BootstrappedNetwork::getServiceURI(
                                                const char *serviceType,
                                                const char *serviceMethodName
                                                ) const
      {
        AutoRecursiveLock lock(getLock());
        if (!mCompleted) return String();

        const Service::Method *service = findServiceMethod(serviceType, serviceMethodName);
        if (!service) return String();

        return service->mURI;
      }

      //-----------------------------------------------------------------------
      bool BootstrappedNetwork::isValidSignature(
                                                 const String &id,
                                                 const String &domain,
                                                 const String &service,
                                                 SecureByteBlockPtr buffer,
                                                 SecureByteBlockPtr bufferSigned
                                                 ) const
      {
        AutoRecursiveLock lock(getLock());

        if (domain != mDomain) {
          ZS_LOG_WARNING(Detail, log("signature is not valid as domains do not match") + ", signature domain=" + domain + ", domain=" + mDomain)
          return false;
        }

        CertificateMap::const_iterator found = mCertificates.find(id);
        if (found == mCertificates.end()) {
          ZS_LOG_WARNING(Detail, log("no signature with the id specified found") + ", signature id=" + id)
          return false;
        }

        const Certificate &certificate = (*found).second;
        if (certificate.mService != service) {
          ZS_LOG_WARNING(Detail, log("signature is not valid as services do not match") + ", signature service=" + service + certificate.getDebugValueString())
          return false;
        }

        if (!certificate.mPublicKey) {
          ZS_LOG_WARNING(Detail, log("certificate missing public key") + certificate.getDebugValueString())
          return false;
        }

        if (!certificate.mPublicKey->verify(*buffer, *bufferSigned)) {
          ZS_LOG_WARNING(Detail, log("signature failed to validate") + certificate.getDebugValueString())
          return false;
        }

        ZS_LOG_DEBUG(log("signature validated") + certificate.getDebugValueString())
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => IBootstrappedNetworkAsyncDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::onStep()
      {
        ZS_LOG_DEBUG(log("on step"))
        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => IDNSDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::onLookupCompleted(IDNSQueryPtr query)
      {
        AutoRecursiveLock lock(getLock());

        step();
      }

      //---------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => IHTTPQueryDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::onHTTPReadDataAvailable(IHTTPQueryPtr query)
      {
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::onHTTPCompleted(IHTTPQueryPtr query)
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("on http complete") + ", query ID=" + Stringize<PUID>(query->getID()).string())

        // do step asynchronously
        IBootstrappedNetworkAsyncDelegateProxy::create(mThisWeak.lock())->onStep();

        PendingRequestMap::iterator found = mPendingRequests.find(query);
        if (found == mPendingRequests.end()) {
          ZS_LOG_DEBUG(log("could not find as pending request (probably okay)"))
          return;
        }

        MessagePtr originalMessage = (*found).second;

        mPendingRequests.erase(found);

        ZS_LOG_DEBUG(log("found pending request"))

        MessagePtr resultMessage = getMessageFromQuery(query);

        if (resultMessage) {
          if (IMessageMonitor::handleMessageReceived(resultMessage)) {
            ZS_LOG_DEBUG(log("http result was handled by message monitor"))
            return;
          }
        }

        ZS_LOG_WARNING(Detail, log("failed to create message from pending query completion"))

        MessageResultPtr result = MessageResult::create(originalMessage, IHTTP::HTTPStatusCode_BadRequest);
        if (!result) {
          ZS_LOG_WARNING(Detail, log("failed to create result for message"))
          return;
        }
        IMessageMonitor::handleMessageReceived(result);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &BootstrappedNetwork::getLock() const
      {
        BootstrappedNetworkManagerPtr manager = mManager.lock();
        if (!manager) return mBogusLock;
        return manager->forBootstrappedNetwork().getLock();
      }

      //-----------------------------------------------------------------------
      String BootstrappedNetwork::log(const char *message) const
      {
        return String("BootstrappedNetwork [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String BootstrappedNetwork::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("bootstrapped network id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("domain", mDomain, firstTime) +
               Helper::getDebugValue("complete", mCompleted ? String("true") : String(), firstTime) +
               Helper::getDebugValue("error code", 0 != mErrorCode ? Stringize<typeof(mErrorCode)>(mErrorCode).string() : String(), firstTime) +
               Helper::getDebugValue("error reason", mErrorReason, firstTime) +
               Helper::getDebugValue("service get dns name", mServicesGetDNSName, firstTime) +
               Helper::getDebugValue("redirection attempts", 0 != mRedirectionAttempts ? Stringize<typeof(mRedirectionAttempts)>(mRedirectionAttempts).string() : String(), firstTime) +
               Helper::getDebugValue("service types", mServiceTypeMap.size() > 0 ? Stringize<size_t>(mServiceTypeMap.size()).string() : String(), firstTime) +
               Helper::getDebugValue("certificates", mCertificates.size() > 0 ? Stringize<size_t>(mCertificates.size()).string() : String(), firstTime) +
               Helper::getDebugValue("pending", mPendingRequests.size() > 0 ? Stringize<size_t>(mPendingRequests.size()).string() : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::go()
      {
        step();
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::reuse()
      {
        ZS_LOG_DEBUG(log("reuse called"))

        AutoRecursiveLock lock(getLock());

        if (!isPreparationComplete()) {
          ZS_LOG_DEBUG(log("preparation is still pending"))
          return;
        }

        if (wasSuccessful()) {
          ZS_LOG_DEBUG(log("preparation is complete and was successful"))
          return;
        }

        ZS_LOG_DEBUG(log("reuse called"))

        mCompleted = false;

        mErrorCode = 0;
        mErrorReason.clear();

        mSRVLookup.reset();
        mSRVResult.reset();

        mServicesGetDNSName.clear();

        if (mServicesGetQuery) {
          mServicesGetQuery->cancel();
          mServicesGetQuery.reset();
        }

        if (mCertificatesGetQuery) {
          mCertificatesGetQuery->cancel();
          mCertificatesGetQuery.reset();
        }

        mRedirectionAttempts = 0;

        mServiceTypeMap.clear();
        mServiceTypeMap.clear();

        mCertificates.clear();

        go();
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::dontUse()
      {
        ZS_LOG_DEBUG(log("not using this object (will self destruct)"))
        mCompleted = true;
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::step()
      {
        if (mCompleted) {
          ZS_LOG_DEBUG(log("step - already completed"))
          return;
        }

        ZS_LOG_DEBUG(log("step") + getDebugValueString())

        if (!mSRVLookup) {
          ZS_LOG_DEBUG(log("step - creating DNS lookup"))
          mSRVLookup = IDNS::lookupSRV(mThisWeak.lock(), mDomain, "_bootstrapper", "_tls");
        }

        if (!mSRVLookup->isComplete()) {
          ZS_LOG_DEBUG(log("step - waiting for SRV lookup to complete"))
          return;
        }

        if (!mSRVResult) {
          mSRVResult = mSRVLookup->getSRV();
          if (!mSRVLookup) {
            ZS_LOG_WARNING(Detail, log("SRV lookup for domain failed"))
            setFailure(ErrorCode_ServiceUnavailable);
            cancel();
            return;
          }
        }

        if (mServicesGetDNSName.isEmpty()) {
          while (true) {
            if (!mSRVResult) {
              ZS_LOG_WARNING(Detail, log("SRV lookup failed to find any appropriate DNS records") + getDebugValueString())
              setFailure(ErrorCode_ServiceUnavailable);
              cancel();
              return;
            }
            if (mSRVResult->mRecords.size() < 1) {
              ZS_LOG_WARNING(Detail, log("SRV lookup failed to find any appropriate DNS records") + getDebugValueString())
              setFailure(ErrorCode_ServiceUnavailable);
              cancel();
              return;
            }

            String name;

            // extract out the next record's name
            {
              IDNS::SRVResult::SRVRecord &record = mSRVResult->mRecords.front();
              name = record.mName;
              mSRVResult->mRecords.pop_front();
            }

            if (String::npos == name.find('.')) {
              // this is a domain prefix
              name += "." + mDomain;
            }

            if (!IHelper::isValidDomain(name)) {
              ZS_LOG_WARNING(Detail, log("SRV result not a valid domain name"))
              continue;
            }

            if (name.length() < mDomain.length()) {
              ZS_LOG_WARNING(Detail, log("SRV name returned was too short thus is not legal") + ", result=" + name + getDebugValueString())
              continue;
            }

            // check if the name is legal
            if (0 != name.compareNoCase(mDomain)) {
              // must one level up sub-domain

              String domainAdditive = "." + mDomain;

              if (name.length() < domainAdditive.length()) {
                ZS_LOG_WARNING(Detail, log("SRV domain name does not match legal domain") + ", result=" + name + getDebugValueString())
                continue;
              }

              String postfix = name.substr(name.length() - domainAdditive.length());
              String prefix = name.substr(0, name.length() - domainAdditive.length());
              if (postfix != domainAdditive) {
                ZS_LOG_WARNING(Detail, log("SRV domain postfix name does not match legal domain") + ", result=" + name + getDebugValueString())
                continue;
              }

              if (String::npos != prefix.find(".")) {
                ZS_LOG_WARNING(Detail, log("SRV domain prefix contains too many sub-domain levels to be legal") + ", result=" + name + getDebugValueString())
                continue;
              }
            }

            ZS_LOG_DEBUG(log("legal boostrapper service name found") + ", result=" + name + getDebugValueString())
            mServicesGetDNSName = name;
            break;
          }
        }

#if (0 != HOOKFLASH_STACK_BOOTSTRAPPER_SERVICE_FORCE_OVER_INSECURE_HTTP)
#define HOOKFLASH_STACK_BOOTSTRAPPER_WARNING_FORCING_OVER_INSECURE_HTTP 1
#define HOOKFLASH_STACK_BOOTSTRAPPER_WARNING_FORCING_OVER_INSECURE_HTTP 2
#endif //(0 != HOOKFLASH_STACK_BOOTSTRAPPER_SERVICE_FORCE_OVER_INSECURE_HTTP)

        // now we have the DNS service name...
        if (!mServicesGetQuery) {
          bool forceOverHTTP = (0 == HOOKFLASH_STACK_BOOTSTRAPPER_SERVICE_FORCE_OVER_INSECURE_HTTP ? false : true);
          String serviceURL = (forceOverHTTP ? "http://" : "https://") + mServicesGetDNSName + "/.well-known/" + HOOKFLASH_STACK_BOOSTRAPPER_SERVICES_GET_URL_METHOD_NAME;
          ZS_LOG_DEBUG(log("step - performing services get request") + ", services-get URL=" + serviceURL)

          ServicesGetRequestPtr request = ServicesGetRequest::create();
          request->domain(mDomain);
          mServicesGetQuery = post(serviceURL, request);
        }

        if (!mServicesGetQuery->isComplete()) {
          ZS_LOG_DEBUG(log("waiting for services get query to complete"))
          return;
        }

        if (mServiceTypeMap.size() < 1) {
          DocumentPtr doc;
          MessagePtr message = getMessageFromQuery(mServicesGetQuery, &doc);
          if (!message) {
            setFailure(mServicesGetQuery->getStatusCode());
            cancel();
            return;
          }

          MessageResultPtr messageResult = MessageResult::convert(message);
          if (messageResult) {
            if (messageResult->hasError()) {
              IHTTP::isRedirection(IHTTP::toStatusCode(messageResult->errorCode()));
              String redirectionURL;
              try {
                redirectionURL = doc->getFirstChildElementChecked()->findFirstChildElementChecked("error")->findFirstChildElementChecked("location")->getTextDecoded();
              } catch (CheckFailed &) {
                ZS_LOG_WARNING(Detail, log("redirection specified but no redirection location found"))
              }

              if (!redirectionURL.isEmpty()) {
                ++mRedirectionAttempts;

                if (mRedirectionAttempts < HOOKFLASH_STACK_BOOTSTRAPPED_NETWORK_MAX_REDIRECTION_ATTEMPTS) {
                  mServicesGetQuery->cancel();
                  mServicesGetQuery.reset();

                  ZS_LOG_DEBUG(log("redirecting services get to a new URL") + ", url=" + redirectionURL)

                  ServicesGetRequestPtr request = ServicesGetRequest::create();
                  request->domain(mDomain);
                  mServicesGetQuery = post(redirectionURL, request);
                  return;
                }
                ZS_LOG_WARNING(Detail, log("too many redirection attempts") + getDebugValueString())
              }
            }
          }

          if (handledError("services get", message)) return;

          ServicesGetResultPtr result = ServicesGetResult::convert(message);
          if (!result) {
            ZS_LOG_WARNING(Detail, log("services get failed to return proper result"))
            setFailure(ErrorCode_BadRequest, "Failed to obtain services");
            cancel();
            return;
          }

          mServiceTypeMap = result->servicesByType();
          if (mServiceTypeMap.size() < 1) {
            ZS_LOG_WARNING(Detail, log("services get failed to return any services"))
            setFailure(ErrorCode_NotFound, "Failed to obtain services");
            cancel();
            return;
          }
        }

        if (!mCertificatesGetQuery) {
          const Service::Method *method = findServiceMethod("certificates", "certificates-get");
          if (!method) {
            ZS_LOG_WARNING(Detail, log("failed to obtain certificate service information"))
            setFailure(ErrorCode_ServiceUnavailable, "Certificate service is not available");
            return;
          }

          if (method->mURI.isEmpty()) {
            ZS_LOG_WARNING(Detail, log("failed to obtain certificate service URI information"))
            setFailure(ErrorCode_ServiceUnavailable, "Certificate service URI is not available");
            return;
          }

          CertificatesGetRequestPtr request = CertificatesGetRequest::create();
          request->domain(mDomain);

          mCertificatesGetQuery = post(method->mURI, request);
        }

        if (!mCertificatesGetQuery->isComplete()) {
          ZS_LOG_DEBUG(log("waiting for certificates get query to complete"))
          return;
        }

        if (mCertificates.size() < 1) {
          MessagePtr message = getMessageFromQuery(mCertificatesGetQuery);
          if (!message) {
            setFailure(mCertificatesGetQuery->getStatusCode());
            cancel();
            return;
          }

          if (handledError("certificates get", message)) return;

          CertificatesGetResultPtr result = CertificatesGetResult::convert(message);
          if (!result) {
            ZS_LOG_WARNING(Detail, log("certifictes get failed to return proper result"))
            setFailure(ErrorCode_BadRequest);
            cancel();
            return;
          }

          mCertificates = result->certificates();
          if (mCertificates.size() < 1) {
            ZS_LOG_WARNING(Detail, log("certificates get failed to return any certificates"))
            setFailure(ErrorCode_NotFound);
            cancel();
            return;
          }
        }

        mCompleted = true;

        // these are no longer needed...

        mSRVLookup->cancel();
        mSRVLookup.reset();

        mSRVResult.reset();

        mServicesGetDNSName.clear();

        mServicesGetQuery->cancel();
        mServicesGetQuery.reset();

        mCertificatesGetQuery->cancel();
        mCertificatesGetQuery.reset();

        mRedirectionAttempts = 0;

        BootstrappedNetworkManagerPtr manager = mManager.lock();
        if (manager) {
          manager->forBootstrappedNetwork().notifyComplete(mThisWeak.lock());
        }
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::cancel()
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("cancel called"))

        BootstrappedNetworkPtr pThis = mThisWeak.lock();

        if (!mCompleted) {
          setFailure(ErrorCode_UserCancelled);
        }
        mCompleted = true;

        if (mSRVLookup) {
          mSRVLookup->cancel();
          mSRVLookup.reset();
        }

        mSRVResult.reset();

        if (mServicesGetQuery) {
          mServicesGetQuery->cancel();
          mServicesGetQuery.reset();
        }

        if (mCertificatesGetQuery) {
          mCertificatesGetQuery->cancel();
          mCertificatesGetQuery.reset();
        }

        mRedirectionAttempts = 0;

        mServiceTypeMap.clear();

        mCertificates.clear();

        if (pThis) {
          BootstrappedNetworkManagerPtr manager = mManager.lock();
          if (manager) {
            manager->forBootstrappedNetwork().notifyComplete(pThis);
          }
        }
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetwork::setFailure(
                                           WORD errorCode,
                                           const char *reason
                                           )
      {
        if (0 == errorCode) {
          errorCode = ErrorCode_InternalServerError;
        }
        if (!reason) {
          reason = toString((ErrorCodes)errorCode);
        }

        if (0 != mErrorCode) {
          ZS_LOG_WARNING(Detail, log("failure reason already set") + ", requesting error=" + Stringize<typeof(errorCode)>(errorCode).string() + ", requesting reason=" + reason + getDebugValueString())
          return;
        }

        mCompleted = true;

        mErrorCode = errorCode;
        mErrorReason = reason;

        ZS_LOG_WARNING(Detail, log("failure set") + getDebugValueString())
      }

      //-----------------------------------------------------------------------
      const Service::Method *BootstrappedNetwork::findServiceMethod(
                                                                    const char *inServiceType,
                                                                    const char *inMethod
                                                                    ) const
      {
        String serviceType(inServiceType ? String(inServiceType) : String());
        String method(inMethod ? String(inMethod) : String());

        if ((serviceType.isEmpty()) ||
            (method.isEmpty())) {
          ZS_LOG_WARNING(Detail, log("missing service type or method") + ", type=" + serviceType + ", method=" + method)
          return NULL;
        }

        ServiceTypeMap::const_iterator found = mServiceTypeMap.find(serviceType);
        if (found == mServiceTypeMap.end()) {
          ZS_LOG_WARNING(Debug, log("service type not found") + ", type=" + serviceType + ", method=" + method)
          return NULL;
        }

        const Service *service = &(*found).second;

        Service::MethodMap::const_iterator foundMethod = service->mMethods.find(method);
        if (foundMethod == service->mMethods.end()) {
          ZS_LOG_WARNING(Debug, log("service method not found") + ", type=" + serviceType + ", method=" + method)
          return NULL;
        }

        ZS_LOG_TRACE(log("service method found") + ", type=" + serviceType + ", method=" + method)
        return &((*foundMethod).second);
      }

      //-----------------------------------------------------------------------
      MessagePtr BootstrappedNetwork::getMessageFromQuery(
                                                          IHTTPQueryPtr query,
                                                          DocumentPtr *outDocument
                                                          )
      {
        ULONG size = query->getReadDataAvailableInBytes();
        SecureByteBlock buffer;
        buffer.CleanNew(size+sizeof(char));
        query->readData(buffer, size);
        ZS_LOG_TRACE(log("------------ http data read start --------------"))
        ZS_LOG_TRACE(log("HTTP DATA") + ", size=" + Stringize<typeof(size)>(size).string() + ", data=\n" + ((const char *)((const BYTE *)buffer)) + "\n")
        ZS_LOG_TRACE(log("------------- http data read end ---------------"))
        DocumentPtr doc = Document::createFromAutoDetect((const char *)((const BYTE *)buffer));
        if (outDocument) {
          (*outDocument) = doc;
        }
        return Message::create(doc, mThisWeak.lock());
      }

      //-----------------------------------------------------------------------
      IHTTPQueryPtr BootstrappedNetwork::post(
                                              const char *url,
                                              MessagePtr message
                                              )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!url)

        DocumentPtr doc = message->encode();
        ULONG size = 0;
        boost::shared_array<char> buffer = doc->writeAsJSON(&size);

        ZS_LOG_TRACE(log("posting message") + ", message=" + buffer.get())

        return IHTTP::post(mThisWeak.lock(), IStackForInternal::userAgent(), url, (const BYTE *)buffer.get(), size, HOOKFLASH_STACK_BOOTSTRAPPED_NETWORK_DEFAULT_MIME_TYPE);
      }

      //-----------------------------------------------------------------------
      bool BootstrappedNetwork::handledError(
                                             const char *requestType,
                                             MessagePtr message
                                             )
      {
        MessageResultPtr result = MessageResult::convert(message);
        if (result->hasError()) {
          ZS_LOG_WARNING(Detail, log("result has an error") + ", type=" + requestType)
          setFailure(result->errorCode(), result->errorReason());
          cancel();
          return true;
        }
        return false;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IBootstrappedNetwork
    #pragma mark

    //-------------------------------------------------------------------------
    String IBootstrappedNetwork::toDebugString(IBootstrappedNetworkPtr network, bool includeCommaPrefix)
    {
      return internal::BootstrappedNetwork::toDebugString(network);
    }

    //-------------------------------------------------------------------------
    IBootstrappedNetworkPtr IBootstrappedNetwork::prepare(
                                                          const char *domain,
                                                          IBootstrappedNetworkDelegatePtr delegate
                                                          )
    {
      return internal::IBootstrappedNetworkFactory::singleton().prepare(domain, delegate);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceCertificates
    #pragma mark

    IServiceCertificatesPtr IServiceCertificates::createServiceCertificatesFrom(IBootstrappedNetworkPtr preparedBootstrappedNetwork)
    {
      return internal::BootstrappedNetwork::createServiceCertificatesFrom(preparedBootstrappedNetwork);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceIdentity
    #pragma mark

    IServiceIdentityPtr IServiceIdentity::createServiceIdentityFrom(IBootstrappedNetworkPtr bootstrappedNetwork)
    {
      return internal::BootstrappedNetwork::createServiceIdentityFrom(bootstrappedNetwork);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceLockbox
    #pragma mark

    IServiceLockboxPtr IServiceLockbox::createServiceLockboxFrom(IBootstrappedNetworkPtr bootstrappedNetwork)
    {
      return internal::BootstrappedNetwork::createServiceLockboxFrom(bootstrappedNetwork);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceSalt
    #pragma mark

    IServiceSaltPtr IServiceSalt::createServiceSaltFrom(IBootstrappedNetworkPtr bootstrappedNetwork)
    {
      return internal::BootstrappedNetwork::createServiceSaltFrom(bootstrappedNetwork);
    }
  }
}
