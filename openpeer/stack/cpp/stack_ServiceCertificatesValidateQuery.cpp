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

#include <hookflash/stack/internal/stack_ServiceCertificatesValidateQuery.h>
#include <hookflash/stack/internal/stack_PeerFilePrivate.h>
#include <hookflash/stack/internal/stack_BootstrappedNetwork.h>
#include <hookflash/stack/internal/stack_Helper.h>
#include <hookflash/stack/internal/stack_Stack.h>

#include <zsLib/Log.h>
#include <zsLib/XML.h>
#include <zsLib/helpers.h>

#include <zsLib/Stringize.h>

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

      using zsLib::Stringize;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceCertificatesValidateQuery
      #pragma mark

      //-----------------------------------------------------------------------
      ServiceCertificatesValidateQuery::ServiceCertificatesValidateQuery(
                                                                         IMessageQueuePtr queue,
                                                                         IServiceCertificatesValidateQueryDelegatePtr delegate,
                                                                         ElementPtr signedElement
                                                                         ) :
        zsLib::MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(IServiceCertificatesValidateQueryDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate))
      {
        ZS_LOG_DEBUG(log("created"))

        ElementPtr signatureEl;
        String id;
        String domain;
        String service;

        signedElement = IHelper::getSignatureInfo(signedElement, &signatureEl, NULL, &id, &domain, &service);

        if (!signedElement) {
          ZS_LOG_WARNING(Detail, log("signature validation failed because no signed element found"))
          return;
        }

        if (!IHelper::isValidDomain(domain)) {
          ZS_LOG_WARNING(Detail, log("signature domain is not valid") + ", domain=" + domain)
          return;
        }

        // found the signature reference, now check if the peer URIs match - they must...
        try {
          String algorithm = signatureEl->findFirstChildElementChecked("algorithm")->getTextDecoded();
          if (algorithm != HOOKFLASH_STACK_PEER_FILE_SIGNATURE_ALGORITHM) {
            ZS_LOG_WARNING(Detail, log("signature validation algorithm is not understood, algorithm=") + algorithm)
            return;
          }

          String signatureDigestAsString = signatureEl->findFirstChildElementChecked("digestValue")->getTextDecoded();

          GeneratorPtr generator = Generator::createJSONGenerator();
          boost::shared_array<char> signedElAsJSON = generator->write(signedElement);

          SecureByteBlockPtr actualDigest = IHelper::hash((const char *)(signedElAsJSON.get()), IHelper::HashAlgorthm_SHA1);

          if (0 == IHelper::compare(*actualDigest, *IHelper::convertFromBase64(signatureDigestAsString))) {
            ZS_LOG_WARNING(Detail, log("digest values did not match, signature digest=") + signatureDigestAsString + ", actual digest=" + IHelper::convertToBase64(*actualDigest))
            return;
          }

          mCertificateID = id;
          mDomain = domain;
          mService = service;
          mDigest = actualDigest;
          mDigestSigned = IHelper::convertFromBase64(signatureEl->findFirstChildElementChecked("digestSigned")->getTextDecoded());
        } catch(CheckFailed &) {
          ZS_LOG_WARNING(Detail, log("signature failed to validate due to missing signature element") + ", signature id=" + id + ", signature domain=" + domain + ", signature service=" + service)
        }
      }

      //-----------------------------------------------------------------------
      ServiceCertificatesValidateQuery::~ServiceCertificatesValidateQuery()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      void ServiceCertificatesValidateQuery::init()
      {
        if (!mDigestSigned) {
          cancel();
          return;
        }

        mBootstrappedNetwork = IBootstrappedNetworkForServices::prepare(mDomain, mThisWeak.lock());
      }

      //-----------------------------------------------------------------------
      ServiceCertificatesValidateQueryPtr ServiceCertificatesValidateQuery::convert(IServiceCertificatesValidateQueryPtr query)
      {
        return boost::dynamic_pointer_cast<ServiceCertificatesValidateQuery>(query);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceCertificatesValidateQuery => IServiceCertificatesValidateQuery
      #pragma mark

      //-----------------------------------------------------------------------
      String ServiceCertificatesValidateQuery::toDebugString(IServiceCertificatesValidateQueryPtr query, bool includeCommaPrefix)
      {
        if (!query) return includeCommaPrefix ? String(", certificate validate query=(null)") : String("certificate validate query=(null)");
        return ServiceCertificatesValidateQuery::convert(query)->getDebugStringValue(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      ServiceCertificatesValidateQueryPtr ServiceCertificatesValidateQuery::queryIfValidSignature(
                                                                                                IServiceCertificatesValidateQueryDelegatePtr delegate,
                                                                                                ElementPtr signedElement
                                                                                                )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ServiceCertificatesValidateQueryPtr pThis(new ServiceCertificatesValidateQuery(IStackForInternal::queueStack(), delegate, signedElement));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      IServiceCertificatesPtr ServiceCertificatesValidateQuery::getService() const
      {
        AutoRecursiveLock lock(getLock());
        return mBootstrappedNetwork;
      }

      //-----------------------------------------------------------------------
      String ServiceCertificatesValidateQuery::getSignatureDomain() const
      {
        AutoRecursiveLock lock(getLock());
        return mDomain;
      }

      //-----------------------------------------------------------------------
      String ServiceCertificatesValidateQuery::getSignatureService() const
      {
        AutoRecursiveLock lock(getLock());
        return mService;
      }

      //-----------------------------------------------------------------------
      bool ServiceCertificatesValidateQuery::isComplete() const
      {
        AutoRecursiveLock lock(getLock());
        if (!mBootstrappedNetwork) return true;

        return mBootstrappedNetwork->forServices().isPreparationComplete();
      }

      //-----------------------------------------------------------------------
      bool ServiceCertificatesValidateQuery::isValidSignature(
                                                              WORD *outErrorCode,
                                                              String *outErrorReason
                                                              ) const
      {
        AutoRecursiveLock lock(getLock());
        if (!mBootstrappedNetwork) return false;

        if (!mBootstrappedNetwork->forServices().isPreparationComplete()) {
          ZS_LOG_WARNING(Detail, log("certificate is not valid because of bootstrapper isn't ready"))
          return false;
        }

        if (!mBootstrappedNetwork->forServices().wasSuccessful(outErrorCode, outErrorReason)) {
          ZS_LOG_WARNING(Detail, log("certificate is not valid because of bootstrapper failure"))
          return false;
        }

        return mBootstrappedNetwork->forServices().isValidSignature(mCertificateID, mDomain, mService, mDigest, mDigestSigned);
      }

      //-----------------------------------------------------------------------
      void ServiceCertificatesValidateQuery::cancel()
      {
        ZS_LOG_DEBUG(log("cancelling certificate validation"))

        AutoRecursiveLock lock(getLock());
        if (!mDelegate) {
          ZS_LOG_DEBUG(log("certificate query already cancelled"))
          return;
        }

        ServiceCertificatesValidateQueryPtr pThis = mThisWeak.lock();
        if (pThis) {
          try {
            mDelegate->onServiceCertificatesValidateQueryCompleted(pThis);
          } catch(IServiceCertificatesValidateQueryDelegateProxy::Exceptions::DelegateGone &) {
          }
        }

        mDelegate.reset();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceCertificatesValidateQuery => IBootstrappedNetworkDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ServiceCertificatesValidateQuery::onBootstrappedNetworkPreparationCompleted(IBootstrappedNetworkPtr bootstrappedNetwork)
      {
        ZS_LOG_DEBUG(log("bootstrapper reported complete"))

        AutoRecursiveLock lock(getLock());
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceCertificatesValidateQuery => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &ServiceCertificatesValidateQuery::getLock() const
      {
        return mLock;
      }

      //-----------------------------------------------------------------------
      String ServiceCertificatesValidateQuery::log(const char *message) const
      {
        return String("ServiceCertificatesValidateQuery [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String ServiceCertificatesValidateQuery::getDebugStringValue(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("certificate ID", mCertificateID, firstTime) +
               Helper::getDebugValue("domain", mDomain, firstTime) +
               Helper::getDebugValue("domain", mService, firstTime) +
               Helper::getDebugValue("delegate", mDelegate ? String("true") : String(), firstTime) +
               Helper::getDebugValue("digest", mDigest ? IHelper::convertToBase64(*mDigest) : String(), firstTime) +
               Helper::getDebugValue("digest signed", mDigestSigned ? IHelper::convertToBase64(*mDigestSigned) : String(), firstTime) +
               IBootstrappedNetwork::toDebugString(mBootstrappedNetwork);
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
    #pragma mark IServiceCertificatesValidateQuery
    #pragma mark

    //-------------------------------------------------------------------------
    String IServiceCertificatesValidateQuery::toDebugString(IServiceCertificatesValidateQueryPtr query, bool includeCommaPrefix)
    {
      return internal::ServiceCertificatesValidateQuery::toDebugString(query, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    IServiceCertificatesValidateQueryPtr IServiceCertificatesValidateQuery::queryIfValidSignature(
                                                                                                IServiceCertificatesValidateQueryDelegatePtr delegate,
                                                                                                ElementPtr signedElement
                                                                                                )
    {
      return internal::IServiceCertificatesValidateQueryFactory::singleton().queryIfValidSignature(delegate, signedElement);
    }
  }
}
