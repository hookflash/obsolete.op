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

#pragma once

#include <hookflash/stack/IBootstrappedNetwork.h>
#include <hookflash/stack/IServiceCertificates.h>
#include <hookflash/stack/IServiceIdentity.h>
#include <hookflash/stack/IServiceLockbox.h>
#include <hookflash/stack/IServiceNamespaceGrant.h>
#include <hookflash/stack/IServiceSalt.h>
#include <hookflash/stack/internal/types.h>
#include <hookflash/stack/IMessageSource.h>
#include <hookflash/stack/message/types.h>

#include <hookflash/services/IDNS.h>
#include <hookflash/services/IHTTP.h>

#include <zsLib/MessageQueueAssociator.h>

// set to 1 to force over HTTP instead of HTTPS
#define HOOKFLASH_STACK_BOOTSTRAPPER_SERVICE_FORCE_OVER_INSECURE_HTTP 0

#define HOOKFLASH_STACK_BOOSTRAPPER_SERVICES_GET_URL_METHOD_NAME "openpeer-services-get"

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IBootstrappedNetworkForAccount
      #pragma mark

      interaction IBootstrappedNetworkForAccount
      {
        IBootstrappedNetworkForAccount &forAccount() {return *this;}
        const IBootstrappedNetworkForAccount &forAccount() const {return *this;}

        virtual String getDomain() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IBootstrappedNetworkForAccountFinder
      #pragma mark

      interaction IBootstrappedNetworkForAccountFinder
      {
        IBootstrappedNetworkForAccountFinder &forAccountFinder() {return *this;}
        const IBootstrappedNetworkForAccountFinder &forAccountFinder() const {return *this;}
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IBootstrappedNetworkForServices
      #pragma mark

      interaction IBootstrappedNetworkForServices
      {
        IBootstrappedNetworkForServices &forServices() {return *this;}
        const IBootstrappedNetworkForServices &forServices() const {return *this;}

        static BootstrappedNetworkPtr prepare(
                                              const char *domain,
                                              IBootstrappedNetworkDelegatePtr delegate = IBootstrappedNetworkDelegatePtr()
                                              );

        virtual String getDomain() const = 0;

        virtual bool isPreparationComplete() const = 0;

        virtual bool wasSuccessful(
                                   WORD *outErrorCode = NULL,
                                   String *outErrorReason = NULL
                                   ) const = 0;

        virtual bool sendServiceMessage(
                                        const char *serviceType,
                                        const char *serviceMethodName,
                                        message::MessagePtr message
                                        ) = 0;

        virtual String getServiceURI(
                                     const char *serviceType,
                                     const char *serviceMethodName
                                     ) const = 0;

        virtual bool isValidSignature(ElementPtr signedElement) const = 0;

        virtual bool isValidSignature(
                                      const String &id,
                                      const String &domain,
                                      const String &service,
                                      SecureByteBlockPtr buffer,
                                      SecureByteBlockPtr bufferSigned
                                      ) const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IBootstrappedNetworkForBootstrappedNetworkManager
      #pragma mark

      interaction IBootstrappedNetworkForBootstrappedNetworkManager
      {
        IBootstrappedNetworkForBootstrappedNetworkManager &forBootstrappedNetworkManager() {return *this;}
        const IBootstrappedNetworkForBootstrappedNetworkManager &forBootstrappedNetworkManager() const {return *this;}

        virtual PUID getID() const = 0;

        virtual String getDomain() const = 0;

        virtual bool isPreparationComplete() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IBootstrappedNetworkAsyncDelegate
      #pragma mark

      interaction IBootstrappedNetworkAsyncDelegate
      {
        virtual void onStep() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetwork
      #pragma mark

      class BootstrappedNetwork : public Noop,
                                  public zsLib::MessageQueueAssociator,
                                  public IBootstrappedNetwork,
                                  public IServiceCertificates,
                                  public IServiceIdentity,
                                  public IServiceLockbox,
                                  public IServiceNamespaceGrant,
                                  public IServiceSalt,
                                  public IBootstrappedNetworkForAccount,
                                  public IBootstrappedNetworkForAccountFinder,
                                  public IBootstrappedNetworkForServices,
                                  public IBootstrappedNetworkForBootstrappedNetworkManager,
                                  public IBootstrappedNetworkAsyncDelegate,
                                  public IDNSDelegate,
                                  public IHTTPQueryDelegate,
                                  public IMessageSource
      {
      public:
        friend interaction IBootstrappedNetworkFactory;
        friend interaction IBootstrappedNetwork;
        friend interaction IServiceCertificates;
        friend interaction IServiceIdentity;
        friend interaction IServiceLockbox;
        friend interaction IServiceNamespaceGrant;
        friend interaction IServiceSalt;

        typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
        typedef message::ServiceMap ServiceMap;
        typedef message::Service Service;
        typedef message::MessagePtr MessagePtr;
        typedef message::CertificateMap CertificateMap;
        typedef message::ServiceTypeMap ServiceTypeMap;

        enum ErrorCodes
        {
          ErrorCode_BadRequest =          IHTTP::HTTPStatusCode_BadRequest,
          ErrorCode_NotFound =            IHTTP::HTTPStatusCode_NotFound,
          ErrorCode_InternalServerError = IHTTP::HTTPStatusCode_InternalServerError,
          ErrorCode_ServiceUnavailable =  IHTTP::HTTPStatusCode_ServiceUnavailable,
          ErrorCode_UserCancelled =       IHTTP::HTTPStatusCode_ClientClosedRequest,
        };

        const char *toString(ErrorCodes errorCode);

      protected:
        BootstrappedNetwork(
                            IMessageQueuePtr queue,
                            const char *domain
                            );
        
        BootstrappedNetwork(Noop) : Noop(true), MessageQueueAssociator(IMessageQueuePtr()) {};
        
        void init();

      public:
        ~BootstrappedNetwork();

        static BootstrappedNetworkPtr convert(IBootstrappedNetworkPtr network);
        static BootstrappedNetworkPtr convert(IServiceCertificatesPtr network);
        static BootstrappedNetworkPtr convert(IServiceIdentityPtr network);
        static BootstrappedNetworkPtr convert(IServiceLockboxPtr network);
        static BootstrappedNetworkPtr convert(IServiceNamespaceGrantPtr network);
        static BootstrappedNetworkPtr convert(IServiceSaltPtr network);

        typedef std::map<IHTTPQueryPtr, message::MessagePtr> PendingRequestMap;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IBootstrappedNetwork
        #pragma mark

        static String toDebugString(IBootstrappedNetworkPtr network, bool includeCommaPrefix = true);

        static BootstrappedNetworkPtr prepare(
                                              const char *domain,
                                              IBootstrappedNetworkDelegatePtr delegate
                                              );

        virtual PUID getID() const {return mID;}

        virtual String getDomain() const;

        virtual bool isPreparationComplete() const;
        virtual bool wasSuccessful(
                                   WORD *outErrorCode = NULL,
                                   String *outErrorReason = NULL
                                   ) const;

        // (duplicate) virtual void cancel();

        // use IMessageMonitor to monitor the result (if result is important)
        virtual bool sendServiceMessage(
                                        const char *serviceType,
                                        const char *serviceMethodName,
                                        message::MessagePtr message
                                        );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IServiceCertificates
        #pragma mark

        static IServiceCertificatesPtr createServiceCertificatesFrom(IBootstrappedNetworkPtr preparedBootstrappedNetwork);

        // (duplicate) virtual PUID getID() const;

        virtual IBootstrappedNetworkPtr getBootstrappedNetwork() const;

        virtual bool isValidSignature(ElementPtr signedElement) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IServiceIdentity
        #pragma mark

        static IServiceIdentityPtr createServiceIdentityFrom(IBootstrappedNetworkPtr bootstrappedNetwork);

        // (duplicate) virtual PUID getID() const;

        // (duplicate) virtual IBootstrappedNetworkPtr getBootstrappedNetwork() const;

        // (duplicate) virtual String getDomain() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IServiceLockbox
        #pragma mark

        static IServiceLockboxPtr createServiceLockboxFrom(IBootstrappedNetworkPtr bootstrappedNetwork);

        // (duplicate) virtual PUID getID() const;

        // (duplicate) virtual IBootstrappedNetworkPtr getBootstrappedNetwork() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IServiceNamespaceGrant
        #pragma mark

        static IServiceNamespaceGrantPtr createServiceNamespaceGrantFrom(IBootstrappedNetworkPtr bootstrappedNetwork);

        // (duplicate) virtual PUID getID() const;

        // (duplicate) virtual IBootstrappedNetworkPtr getBootstrappedNetwork() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IServiceSalt
        #pragma mark

        static IServiceSaltPtr createServiceSaltFrom(IBootstrappedNetworkPtr bootstrappedNetwork);

        // (duplicate) virtual PUID getID() const;

        // (duplicate) virtual IBootstrappedNetworkPtr getBootstrappedNetwork() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IBootstrappedNetworkForAccount
        #pragma mark

        // (duplicate) virtual String getDomain() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IBootstrappedNetworkForServices
        #pragma mark

        // (duplicate) virtual String getDomain() const;

        // (duplicate) static BootstrappedNetworkPtr prepare(
        //                                                   const char *domain,
        //                                                   IBootstrappedNetworkDelegatePtr delegate
        //                                                   );

        // (duplicate) virtual bool isPreparationComplete() const;

        // (duplicate) virtual bool wasSuccessful(
        //                                        WORD *outErrorCode = NULL,
        //                                        String *outErrorReason = NULL
        //                                        ) const;

        // (duplicate) virtual bool sendServiceMessage(
        //                                             const char *serviceType,
        //                                             const char *serviceMethodName,
        //                                             message::MessagePtr message
        //                                             );

        // (duplicate) virtual bool isValidSignature(ElementPtr signedElement) const;

        virtual String getServiceURI(
                                     const char *serviceType,
                                     const char *serviceMethodName
                                     ) const;

        virtual bool isValidSignature(
                                      const String &id,
                                      const String &domain,
                                      const String &service,
                                      SecureByteBlockPtr buffer,
                                      SecureByteBlockPtr bufferSigned
                                      ) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IBootstrappedNetworkForBootstrappedNetworkManager
        #pragma mark

        // (duplicate) virtual PUID getID() const;

        // (duplicate) virtual String getDomain() const;

        // (duplicate) virtual bool isPreparationComplete() const;
        // (duplicate) virtual bool wasSuccessful(
        //                                        WORD *outErrorCode = NULL,
        //                                        String *outErrorReason = NULL
        //                                        ) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IBootstrappedNetworkAsyncDelegate
        #pragma mark

        virtual void onStep();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IDNSDelegate
        #pragma mark

        virtual void onLookupCompleted(IDNSQueryPtr query);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => IHTTPQueryDelegate
        #pragma mark

        virtual void onHTTPReadDataAvailable(IHTTPQueryPtr query);
        virtual void onHTTPCompleted(IHTTPQueryPtr query);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => (internal)
        #pragma mark

        RecursiveLock &getLock() const;

        String log(const char *message) const;
        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        void go();
        void reuse();

        void step();
        void cancel();
        void setFailure(
                        WORD errorCode,
                        const char *reason = NULL
                        );

        const Service::Method *findServiceMethod(
                                                 const char *serviceType,
                                                 const char *method
                                                 ) const;

        MessagePtr getMessageFromQuery(
                                       IHTTPQueryPtr query,
                                       DocumentPtr *outDocument = NULL
                                       );

        IHTTPQueryPtr post(
                           const char *url,
                           MessagePtr message
                           );
        bool handledError(
                          const char *requestType,
                          MessagePtr message
                          );

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mBogusLock;
        BootstrappedNetworkWeakPtr mThisWeak;
        String mDomain;

        BootstrappedNetworkManagerWeakPtr mManager;

        bool mCompleted;

        WORD mErrorCode;
        String mErrorReason;

        IDNSQueryPtr mSRVLookup;
        IDNS::SRVResultPtr mSRVResult;
        String mServicesGetDNSName;

        IHTTPQueryPtr mServicesGetQuery;
        IHTTPQueryPtr mCertificatesGetQuery;

        ULONG mRedirectionAttempts;

        ServiceTypeMap mServiceTypeMap;
        CertificateMap mCertificates;

        PendingRequestMap mPendingRequests;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IBootstrappedNetworkFactory
      #pragma mark

      interaction IBootstrappedNetworkFactory
      {
        static IBootstrappedNetworkFactory &singleton();

        virtual BootstrappedNetworkPtr prepare(
                                               const char *domain,
                                               IBootstrappedNetworkDelegatePtr delegate
                                               );
      };

    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IBootstrappedNetworkAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()
