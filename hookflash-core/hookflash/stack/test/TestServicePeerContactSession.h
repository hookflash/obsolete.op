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

#include <hookflash/stack/internal/stack_ServicePeerContactSession.h>
#include <hookflash/stack/internal/stack_Factory.h>
#include <hookflash/services/internal/services_HTTP.h>

#include <zsLib/Timer.h>

using namespace hookflash::stack::internal;
using namespace hookflash::stack::message;

#define USE_FAKE_BOOTSTRAPPED_NETWORK 1
#define USE_FAKE_IDENTITY_SESSION 1
//#define USE_FAKE_PEER_CONTACT_SESSION 1

namespace hookflash
{
  namespace stack
  {
    namespace test
    {
      class TestServicePeerContactSession;
      typedef boost::shared_ptr<TestServicePeerContactSession> TestServicePeerContactSessionPtr;
      typedef boost::weak_ptr<TestServicePeerContactSession> TestServicePeerContactSessionWeakPtr;
      
      class TestBootstrappedNetworkForPeerContact;
      typedef boost::shared_ptr<TestBootstrappedNetworkForPeerContact> TestBootstrappedNetworkForPeerContactPtr;
      typedef boost::weak_ptr<TestBootstrappedNetworkForPeerContact> TestBootstrappedNetworkForPeerContactWeakPtr;
      
      class TestServiceIdentitySessionForPeerContact;
      typedef boost::shared_ptr<TestServiceIdentitySessionForPeerContact> TestServiceIdentitySessionForPeerContactPtr;
      typedef boost::weak_ptr<TestServiceIdentitySessionForPeerContact> TestServiceIdentitySessionForPeerContactWeakPtr;
      
      class TestFactoryForPeerContact;
      typedef boost::shared_ptr<TestFactoryForPeerContact> TestFactoryForPeerContactPtr;
      typedef boost::weak_ptr<TestFactoryForPeerContact> TestFactoryForPeerContactWeakPtr;
      
      class TestCallbackForPeerContact;
      typedef boost::shared_ptr<TestCallbackForPeerContact> TestCallbackForPeerContactPtr;
      typedef boost::weak_ptr<TestCallbackForPeerContact> TestCallbackForPeerContactWeakPtr;
      
      class TestHTTPQueryForPeerContact;
      typedef boost::shared_ptr<TestHTTPQueryForPeerContact> TestHTTPQueryForPeerContactPtr;
      typedef boost::weak_ptr<TestHTTPQueryForPeerContact> TestHTTPQueryForPeerContactWeakPtr;
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
#pragma mark
#pragma mark TestHTTPQueryForPeerContact
#pragma mark
      class TestHTTPQueryForPeerContact :public hookflash::services::internal::HTTP::HTTPQuery
      {
      protected:
        TestHTTPQueryForPeerContact(
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
        static TestHTTPQueryForPeerContactPtr create(
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
#pragma mark TestCallbackForPeerContact
#pragma mark
      
      class TestCallbackForPeerContact : public MessageQueueAssociator,
                           public IBootstrappedNetworkDelegate,
                           public IServiceIdentitySessionDelegate,
                           public IServicePeerContactSessionDelegate
      {
      public:
        enum LoginScenarios{
          LoginScenario_None,
          LoginScenario_Login,
          LoginScenario_Relogin
        };
      private:
        TestCallbackForPeerContact(IMessageQueuePtr queue);
        
        void init();
        
      public:
        ~TestCallbackForPeerContact();
        
        static TestCallbackForPeerContactPtr create(zsLib::IMessageQueuePtr queue);
        
      protected:
        //IBootstrappedNetworkDelegate methods
        virtual void onBootstrappedNetworkPreparationCompleted(IBootstrappedNetworkPtr bootstrappedNetwork);
        
        //IServiceIdentitySessionDelegate methods
        virtual void onServiceIdentitySessionStateChanged(
                                                          IServiceIdentitySessionPtr session,
                                                          hookflash::stack::IServiceIdentitySession::SessionStates state
                                                          );
        
        virtual void onServiceIdentitySessionPendingMessageForInnerBrowserWindowFrame(IServiceIdentitySessionPtr session);
        
        virtual void onServicePeerContactSessionStateChanged(
                                                             IServicePeerContactSessionPtr session,
                                                             hookflash::stack::IServicePeerContactSession::SessionStates state
                                                             );
        virtual void onServicePeerContactSessionAssociatedIdentitiesChanged(IServicePeerContactSessionPtr session);
        
      public:
        mutable RecursiveLock mLock;
        TestCallbackForPeerContactWeakPtr mThisWeak;
        
        ULONG mCount;
        
        IBootstrappedNetworkPtr mNetwork;
        IServicePeerContactSessionPtr mPeerContactSession;
        IServiceIdentitySessionPtr mIdentitySession;
        bool mNetworkDone;
        
        ElementPtr mPeerFilesElement;
        zsLib::String mPeerFilePassword;
        IPeerFilesPtr mPeerFilesPtr;
        LoginScenarios mLoginScenario;
      };
      
#pragma mark
#pragma mark TestBootstrappedNetworkForPeerContact
#pragma mark
      class TestBootstrappedNetworkForPeerContact : public MessageQueueAssociator,
                                      public internal::BootstrappedNetwork
//                                      public IBootstrappedNetworkAsyncDelegate
      {
      public:
        friend interaction TestFactoryForPeerContact;
      protected:
        TestBootstrappedNetworkForPeerContact(IMessageQueuePtr queue) : zsLib::MessageQueueAssociator(queue), BootstrappedNetwork(zsLib::Noop()) {}
      public:
        ~TestBootstrappedNetworkForPeerContact();
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
//        virtual void onStep();
      protected:
        //TestBootstrappedNetworkForPeerContactWeakPtr mThisWeak;
      };
      
#pragma mark
#pragma mark TestServiceIdentitySessionForPeerContact
#pragma mark
      class TestServiceIdentitySessionForPeerContact : public internal::ServiceIdentitySession
      {
      public:
        friend interaction TestFactoryForPeerContact;
      protected:
        TestServiceIdentitySessionForPeerContact() : ServiceIdentitySession(zsLib::Noop()) {}
        
      public:
        ~TestServiceIdentitySessionForPeerContact();
        //---------------------------------------------------------------------
        virtual bool isLoginComplete() const;
        
        virtual void associate(ServicePeerContactSessionPtr peerContact);
      };
      
#pragma mark
#pragma mark TestServicePeerContactSession
#pragma mark
      
      class TestServicePeerContactSession : public internal::ServicePeerContactSession
      {
      public:
        friend interaction TestFactoryForPeerContact;
        
      protected:
        TestServicePeerContactSession() : ServicePeerContactSession(zsLib::Noop()) {}
        
      public:
        ~TestServicePeerContactSession();
        //---------------------------------------------------------------------
        
      };
#pragma mark
#pragma mark TestFactoryForPeerContact
#pragma mark
      
      class TestFactoryForPeerContact : public internal::Factory
      {
      public:
        TestFactoryForPeerContact() {}
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
        virtual ServicePeerContactSessionPtr login(
                                                   IServicePeerContactSessionDelegatePtr delegate,
                                                   IServicePeerContactPtr servicePeerContact,
                                                   IServiceIdentitySessionPtr identitySession
                                                   );
        
        virtual ServicePeerContactSessionPtr relogin(
                                                     IServicePeerContactSessionDelegatePtr delegate,
                                                     IPeerFilesPtr existingPeerFiles
                                                     );
#endif //USE_FAKE_PEER_CONTACT_SESSION
      };
      }
    }
  }
