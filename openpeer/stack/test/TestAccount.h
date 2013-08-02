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

#if 0

#include <openpeer/stack/internal/stack_ServiceLockboxSession.h>
#include <openpeer/stack/internal/stack_Factory.h>
#include <openpeer/services/internal/services_Factory.h>
#include <openpeer/services/internal/services_HTTP.h>
#include <openpeer/services/internal/services_RUDPICESocket.h>

#include <zsLib/Timer.h>

using namespace openpeer::stack::internal;
using namespace openpeer::stack::message;
using namespace openpeer::services::internal;
using namespace openpeer::services;

#define USE_FAKE_BOOTSTRAPPED_NETWORK 1
#define USE_FAKE_IDENTITY_SESSION 1
//#define USE_FAKE_PEER_CONTACT_SESSION 1
//#define USE_FAKE_ACCOUNT 1

namespace openpeer
{
  namespace stack
  {
    namespace test
    {
      
      class TestServiceLockboxSessionForAccount;
      typedef boost::shared_ptr<TestServiceLockboxSessionForAccount> TestServiceLockboxSessionForAccountPtr;
      typedef boost::weak_ptr<TestServiceLockboxSessionForAccount> TestServiceLockboxSessionForAccountWeakPtr;
      
      class TestBootstrappedNetworkForAccount;
      typedef boost::shared_ptr<TestBootstrappedNetworkForAccount> TestBootstrappedNetworkForAccountPtr;
      typedef boost::weak_ptr<TestBootstrappedNetworkForAccount> TestBootstrappedNetworkForAccountWeakPtr;
      
      class TestServiceIdentitySessionForAccount;
      typedef boost::shared_ptr<TestServiceIdentitySessionForAccount> TestServiceIdentitySessionForAccountPtr;
      typedef boost::weak_ptr<TestServiceIdentitySessionForAccount> TestServiceIdentitySessionForAccountWeakPtr;
      
      class TestAccount;
      typedef boost::shared_ptr<TestAccount> TestAccountPtr;
      typedef boost::weak_ptr<TestAccount> TestAccountWeakPtr;
      
      class TestAccountFinderForAccount;
      typedef boost::shared_ptr<TestAccountFinderForAccount> TestAccountFinderForAccountPtr;
      typedef boost::weak_ptr<TestAccountFinderForAccount> TestAccountFinderForAccountWeakPtr;
      
      class TestRUDPICESocketForAccount;
      typedef boost::shared_ptr<TestRUDPICESocketForAccount> TestRUDPICESocketForAccountPtr;
      typedef boost::weak_ptr<TestRUDPICESocketForAccount> TestRUDPICESocketForAccountWeakPtr;
      
      class TestFactoryForAccount;
      typedef boost::shared_ptr<TestFactoryForAccount> TestFactoryForAccountPtr;
      typedef boost::weak_ptr<TestFactoryForAccount> TestFactoryForAccountWeakPtr;
      
      class TestServicesFactoryForAccount;
      typedef boost::shared_ptr<TestServicesFactoryForAccount> TestServicesFactoryForAccountPtr;
      typedef boost::weak_ptr<TestServicesFactoryForAccount> TestServicesFactoryForAccountWeakPtr;
      
      class TestCallbackForAccount;
      typedef boost::shared_ptr<TestCallbackForAccount> TestCallbackForAccountPtr;
      typedef boost::weak_ptr<TestCallbackForAccount> TestCallbackForAccountWeakPtr;
      
      class TestHTTPQueryForAccount;
      typedef boost::shared_ptr<TestHTTPQueryForAccount> TestHTTPQueryForAccountPtr;
      typedef boost::weak_ptr<TestHTTPQueryForAccount> TestHTTPQueryForAccountWeakPtr;
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
#pragma mark
#pragma mark TestHTTPQueryForAccount
#pragma mark
      class TestHTTPQueryForAccount :public openpeer::services::internal::HTTP::HTTPQuery
      {
      protected:
        TestHTTPQueryForAccount(
                      services::internal::HTTPPtr outer,
                      services::IHTTPQueryDelegatePtr delegate,
                      bool isPost,
                      const char *userAgent,
                      const char *url,
                      const BYTE *postData,
                      ULONG postDataLengthInBytes,
                      const char *postDataMimeType,
                      Duration timeout
                      ) : HTTPQuery(outer, delegate, isPost, userAgent, url, postData, postDataLengthInBytes, postDataMimeType, timeout){}
      public:
        void writeBody(zsLib::String messageBody);
        static TestHTTPQueryForAccountPtr create(
                                       services::internal::HTTPPtr outer,
                                       services::IHTTPQueryDelegatePtr delegate,
                                       bool isPost,
                                       const char *userAgent,
                                       const char *url,
                                       const BYTE *postData,
                                       ULONG postDataLengthInBytes,
                                       const char *postDataMimeType,
                                       Duration timeout
                                       );
        
      };
#pragma mark
#pragma mark TestCallbackForAccount
#pragma mark
      
      class TestCallbackForAccount : public MessageQueueAssociator,
      public IBootstrappedNetworkDelegate,
      public IServiceIdentitySessionDelegate,
      public IServiceLockboxSessionDelegate,
      public IAccountDelegate
      {
      public:
        enum LoginScenarios{
          LoginScenario_None,
          LoginScenario_Login,
          LoginScenario_Relogin
        };
      private:
        TestCallbackForAccount(IMessageQueuePtr queue);
        
        void init();
        
      public:
        ~TestCallbackForAccount();
        
        static TestCallbackForAccountPtr create(zsLib::IMessageQueuePtr queue);
        
      protected:
        //IBootstrappedNetworkDelegate methods
        virtual void onBootstrappedNetworkPreparationCompleted(IBootstrappedNetworkPtr bootstrappedNetwork);
        
        //IServiceIdentitySessionDelegate methods
        virtual void onServiceIdentitySessionStateChanged(
                                                          IServiceIdentitySessionPtr session,
                                                          openpeer::stack::IServiceIdentitySession::SessionStates state
                                                          );
        
        virtual void onServiceIdentitySessionPendingMessageForInnerBrowserWindowFrame(IServiceIdentitySessionPtr session);
        
        //IServiceLockboxSessionDelegate methods
        virtual void onServiceLockboxSessionStateChanged(
                                                             IServiceLockboxSessionPtr session,
                                                             openpeer::stack::IServiceLockboxSession::SessionStates state
                                                             );
        virtual void onServiceLockboxSessionAssociatedIdentitiesChanged(IServiceLockboxSessionPtr session);
        
        //IAccountDelegate methods
        virtual void onAccountStateChanged(
                                           IAccountPtr account,
                                           openpeer::stack::internal::Account::AccountStates state
                                           );
        
      public:
        mutable RecursiveLock mLock;
        TestCallbackForAccountWeakPtr mThisWeak;
        
        ULONG mCount;
        
        IBootstrappedNetworkPtr mNetwork;
        IServiceLockboxSessionPtr mPeerContactSession;
        IServiceIdentitySessionPtr mIdentitySession;
        IAccountPtr mAccount;
        bool mNetworkDone;
        
        ElementPtr mPeerFilesElement;
        zsLib::String mPeerFilePassword;
        IPeerFilesPtr mPeerFilesPtr;
        LoginScenarios mLoginScenario;
      };
      
#pragma mark
#pragma mark TestBootstrappedNetworkForAccount
#pragma mark
      class TestBootstrappedNetworkForAccount : public MessageQueueAssociator,
      public internal::BootstrappedNetwork
      //                                      publicIWakeDelegate
      {
      public:
        friend interaction TestFactoryForAccount;
      protected:
        TestBootstrappedNetworkForAccount(IMessageQueuePtr queue) : zsLib::MessageQueueAssociator(queue), BootstrappedNetwork(zsLib::Noop()) {}
      public:
        ~TestBootstrappedNetworkForAccount();
        //---------------------------------------------------------------------
        //virtual void onHTTPCompleted(IHTTPQueryPtr query);
        void initialize(IBootstrappedNetworkDelegatePtr delegate);
        
        virtual bool isPreparationComplete() const;
        virtual bool wasSuccessful(
                                   WORD *outErrorCode = NULL,
                                   String *outErrorReason = NULL
                                   ) const;
        virtual bool sendServiceMessage(
                                        const char *serviceType,
                                        const char *serviceMethodName,
                                        message::MessagePtr message
                                        );
        virtual bool isValidSignature(ElementPtr signedElement) const;
        //        virtual IHTTPQueryPtr post(
        //                                  const char *url,
        //                                  MessagePtr message
        //                                   );
        //        virtual void onWake();
      protected:
        //TestBootstrappedNetworkForAccountWeakPtr mThisWeak;
      };
      
#pragma mark
#pragma mark TestServiceIdentitySessionForAccount
#pragma mark
      class TestServiceIdentitySessionForAccount : public internal::ServiceIdentitySession
      {
      public:
        friend interaction TestFactoryForAccount;
      protected:
        TestServiceIdentitySessionForAccount() : ServiceIdentitySession(zsLib::Noop()) {}
        
      public:
        ~TestServiceIdentitySessionForAccount();
        //---------------------------------------------------------------------
        virtual bool isLoginComplete() const;
        
        virtual void associate(ServiceLockboxSessionPtr peerContact);
      };
      
#pragma mark
#pragma mark TestServiceLockboxSessionForAccount
#pragma mark
      
      class TestServiceLockboxSessionForAccount : public internal::ServiceLockboxSession
      {
      public:
        friend interaction TestFactoryForAccount;
        
      protected:
        TestServiceLockboxSessionForAccount() : ServiceLockboxSession(zsLib::Noop()) {}
        
      public:
        ~TestServiceLockboxSessionForAccount();
        //---------------------------------------------------------------------
        
      };
      
#pragma mark
#pragma mark TestAccount
#pragma mark
      
      class TestAccount : public internal::Account
      {
      public:
        friend interaction TestFactoryForAccount;
        
      protected:
        TestAccount() : Account(zsLib::Noop()) {}
        
      public:
        ~TestAccount();
        //---------------------------------------------------------------------
        
      };
      
#pragma mark
#pragma mark TestAccountFinderForAccount
#pragma mark
      
      class TestAccountFinderForAccount : public internal::AccountFinder
      {
      public:
        friend interaction TestFactoryForAccount;
        
      protected:
        TestAccountFinderForAccount() : AccountFinder(zsLib::Noop()) {}
        
      public:
        ~TestAccountFinderForAccount();
        //---------------------------------------------------------------------
        
      };
#pragma mark
#pragma mark TestRUDPICESocketForAccount
#pragma mark
      
      class TestRUDPICESocketForAccount : public services::internal::RUDPICESocket
      {
      public:
        friend interaction TestServicesFactoryForAccount;
        
      protected:
        TestRUDPICESocketForAccount() : RUDPICESocket(zsLib::Noop()) {}
        
      public:
        ~TestRUDPICESocketForAccount();
        //---------------------------------------------------------------------
        
        virtual void shutdown();
        
      };
#pragma mark
#pragma mark TestFactoryForAccount
#pragma mark
      
      class TestFactoryForAccount : public internal::Factory
      {
      public:
        TestFactoryForAccount() {}
#ifdef USE_FAKE_BOOTSTRAPPED_NETWORK
        //bootstrapped network
        virtual BootstrappedNetworkPtr prepare(
                                               const char *domain,
                                               IBootstrappedNetworkDelegatePtr delegate
                                               );
#endif //USE_FAKE_BOOTSTRAPPED_NETWORK
#ifdef USE_FAKE_IDENTITY_SESSION
        //service identity session
        virtual ServiceIdentitySessionPtr loginWithIdentity(
                                                            IServiceIdentitySessionDelegatePtr delegate,
                                                            const char *redirectAfterLoginCompleteURL,
                                                            const char *identityURI,
                                                            IServiceIdentityPtr provider = IServiceIdentityPtr() // required if identity URI does not have domain
                                                            );
        
        virtual ServiceIdentitySessionPtr loginWithIdentityTBD(
                                                               IServiceIdentitySessionDelegatePtr delegate,
                                                               const char *redirectAfterLoginCompleteURL,
                                                               IServiceIdentityPtr provider,
                                                               const char *legacyIdentityBaseURI = NULL
                                                               );
        
        virtual ServiceIdentitySessionPtr loginWithIdentityBundle(
                                                                  IServiceIdentitySessionDelegatePtr delegate,
                                                                  const char *redirectAfterLoginCompleteURL,
                                                                  ElementPtr signedIdentityBundle
                                                                  );
        
        virtual ServiceIdentitySessionPtr relogin(
                                                  IServiceIdentitySessionDelegatePtr delegate,
                                                  const char *redirectAfterLoginCompleteURL,
                                                  IServiceIdentityPtr provider,
                                                  const char *identityReloginAccessKey
                                                  );
#endif //USE_FAKE_IDENTITY_SESSION
#ifdef USE_FAKE_PEER_CONTACT_SESSION
        //service peer contact
        virtual ServiceLockboxSessionPtr login(
                                                   IServiceLockboxSessionDelegatePtr delegate,
                                                   IServiceLockboxPtr ServiceLockbox,
                                                   IServiceIdentitySessionPtr identitySession
                                                   );
        
        virtual ServiceLockboxSessionPtr relogin(
                                                     IServiceLockboxSessionDelegatePtr delegate,
                                                     IPeerFilesPtr existingPeerFiles
                                                     );
#endif //USE_FAKE_PEER_CONTACT_SESSION
#ifdef USE_FAKE_ACCOUNT
        virtual AccountPtr create(
                                  IAccountDelegatePtr delegate,
                                  IServiceLockboxSessionPtr peerContactSession
                                  );
#endif //USE_FAKE_ACCOUNT
        virtual AccountFinderPtr create(
                                        IAccountFinderDelegatePtr delegate,
                                        AccountPtr outer
                                        );
      };
#pragma mark
#pragma mark TestFactoryForAccount
#pragma mark
      
      class TestServicesFactoryForAccount : public openpeer::services::internal::Factory
      {
      public:
        TestServicesFactoryForAccount() {}
      
        ///////// RUDPICE SOCKET
        virtual RUDPICESocketPtr create(
                                        IMessageQueuePtr queue,
                                        IRUDPICESocketDelegatePtr delegate,
                                        const char *turnServer,
                                        const char *turnServerUsername,
                                        const char *turnServerPassword,
                                        const char *stunServer,
                                        WORD port = 0
                                        );
        
        virtual RUDPICESocketPtr create(
                                        IMessageQueuePtr queue,
                                        IRUDPICESocketDelegatePtr delegate,
                                        IDNS::SRVResultPtr srvTURNUDP,
                                        IDNS::SRVResultPtr srvTURNTCP,
                                        const char *turnServerUsername,
                                        const char *turnServerPassword,
                                        IDNS::SRVResultPtr srvSTUN,
                                        WORD port = 0
                                        );
      };
    }
  }
}

#endif //0
