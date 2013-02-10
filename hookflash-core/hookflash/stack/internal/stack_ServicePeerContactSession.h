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

#include <hookflash/stack/internal/types.h>
#include <hookflash/stack/IBootstrappedNetwork.h>
#include <hookflash/stack/IMessageMonitor.h>
#include <hookflash/stack/IMessageSource.h>
#include <hookflash/stack/IServicePeerContact.h>
#include <hookflash/stack/message/peer-contact/PeerContactLoginResult.h>
#include <hookflash/stack/message/peer-contact/PrivatePeerFileGetResult.h>
#include <hookflash/stack/message/peer-contact/PrivatePeerFileSetResult.h>
#include <hookflash/stack/message/peer-contact/PeerContactIdentityAssociateResult.h>
#include <hookflash/stack/message/peer-contact/PeerContactServicesGetResult.h>
#include <hookflash/stack/IServiceSalt.h>

#include <zsLib/MessageQueueAssociator.h>

#include <list>

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using message::peer_contact::PeerContactLoginResult;
      using message::peer_contact::PeerContactLoginResultPtr;
      using message::peer_contact::PrivatePeerFileGetResult;
      using message::peer_contact::PrivatePeerFileGetResultPtr;
      using message::peer_contact::PrivatePeerFileSetResult;
      using message::peer_contact::PrivatePeerFileSetResultPtr;
      using message::peer_contact::PeerContactIdentityAssociateResult;
      using message::peer_contact::PeerContactIdentityAssociateResultPtr;
      using message::peer_contact::PeerContactServicesGetResult;
      using message::peer_contact::PeerContactServicesGetResultPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IServicePeerContactSessionForAccount
      #pragma mark

      interaction IServicePeerContactSessionForAccount
      {
        IServicePeerContactSessionForAccount &forAccount() {return *this;}
        const IServicePeerContactSessionForAccount &forAccount() const {return *this;}

        virtual IServicePeerContactSession::SessionStates getState(
                                                                   WORD *lastErrorCode = NULL,
                                                                   String *lastErrorReason = NULL
                                                                   ) const = 0;

        virtual IPeerFilesPtr getPeerFiles() const = 0;

        virtual void attach(AccountPtr account) = 0;

        virtual Service::MethodPtr findServiceMethod(
                                                     const char *serviceType,
                                                     const char *method
                                                     ) const = 0;

        virtual BootstrappedNetworkPtr getBootstrappedNetwork() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IServicePeerContactSessionForServiceIdentity
      #pragma mark

      interaction IServicePeerContactSessionForServiceIdentity
      {
        IServicePeerContactSessionForServiceIdentity &forServiceIdentity() {return *this;}
        const IServicePeerContactSessionForServiceIdentity &forServiceIdentity() const {return *this;}

        virtual IServicePeerContactSession::SessionStates getState(
                                                                   WORD *lastErrorCode = NULL,
                                                                   String *lastErrorReason = NULL
                                                                   ) const = 0;

        virtual IPeerFilesPtr getPeerFiles() const = 0;
        virtual String getContactUserID() const = 0;

        virtual void notifyStateChanged() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IServicePeerContactSessionAsync
      #pragma mark

      interaction IServicePeerContactSessionAsync
      {
        virtual void onStep() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServicePeerContactSession
      #pragma mark

      class ServicePeerContactSession : public zsLib::MessageQueueAssociator,
                                        public IServicePeerContactSession,
                                        public IMessageSource,
                                        public IServicePeerContactSessionForAccount,
                                        public IServicePeerContactSessionForServiceIdentity,
                                        public IServicePeerContactSessionAsync,
                                        public IBootstrappedNetworkDelegate,
                                        public IServiceSaltFetchSignedSaltQueryDelegate,
                                        public IMessageMonitorResultDelegate<PeerContactLoginResult>,
                                        public IMessageMonitorResultDelegate<PrivatePeerFileGetResult>,
                                        public IMessageMonitorResultDelegate<PrivatePeerFileSetResult>,
                                        public IMessageMonitorResultDelegate<PeerContactIdentityAssociateResult>,
                                        public IMessageMonitorResultDelegate<PeerContactServicesGetResult>
      {
      public:
        friend interaction IServicePeerContactSessionFactory;
        friend interaction IServicePeerContactSession;

        typedef PUID ServiceIdentitySessionID;
        typedef std::map<ServiceIdentitySessionID, ServiceIdentitySessionPtr> ServiceIdentitySessionMap;

      protected:
        ServicePeerContactSession(
                                  IMessageQueuePtr queue,
                                  BootstrappedNetworkPtr network,
                                  IServicePeerContactSessionDelegatePtr delegate
                                  );

        void init();

      public:
        ~ServicePeerContactSession();

        static ServicePeerContactSessionPtr convert(IServicePeerContactSessionPtr query);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServicePeerContactSession => IServicePeerContactSession
        #pragma mark

        static String toDebugString(IServicePeerContactSessionPtr session, bool includeCommaPrefix = true);

        static ServicePeerContactSessionPtr login(
                                                  IServicePeerContactSessionDelegatePtr delegate,
                                                  IServicePeerContactPtr servicePeerContact,
                                                  IServiceIdentitySessionPtr identitySession
                                                  );

        static ServicePeerContactSessionPtr relogin(
                                                    IServicePeerContactSessionDelegatePtr delegate,
                                                    IPeerFilesPtr existingPeerFiles
                                                    );

        virtual PUID getID() const {return mID;}

        virtual IServicePeerContactPtr getService() const;

        virtual SessionStates getState(
                                       WORD *lastErrorCode,
                                       String *lastErrorReason
                                       ) const;

        virtual IPeerFilesPtr getPeerFiles() const;
        virtual String getContactUserID() const;

        virtual ServiceIdentitySessionListPtr getAssociatedIdentities() const;
        virtual void associateIdentities(
                                         const ServiceIdentitySessionList &identitiesToAssociate,
                                         const ServiceIdentitySessionList &identitiesToRemove
                                         );

        virtual void cancel();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServicePeerContactSession => IMessageSource
        #pragma mark

        // (duplicate) virtual PUID getID() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServicePeerContactSession => IServicePeerContactSessionForAccount
        #pragma mark

        // (duplicate) virtual SessionStates getState(
        //                                            WORD *lastErrorCode,
        //                                            String *lastErrorReason
        //                                            ) const;

        // (duplicate) virtual IPeerFilesPtr getPeerFiles() const;

        virtual void attach(AccountPtr account);

        virtual Service::MethodPtr findServiceMethod(
                                                     const char *serviceType,
                                                     const char *method
                                                     ) const;

        virtual BootstrappedNetworkPtr getBootstrappedNetwork() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServicePeerContactSession => IServicePeerContactSessionForServiceIdentity
        #pragma mark

        // (duplicate) virtual SessionStates getState(
        //                                            WORD *lastErrorCode,
        //                                            String *lastErrorReason
        //                                            ) const;

        // (duplicate) virtual IPeerFilesPtr getPeerFiles() const;
        // (duplicate) virtual String getContactUserID() const;

        virtual void notifyStateChanged();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServicePeerContactSession => IServicePeerContactSessionAsync
        #pragma mark

        virtual void onStep();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServicePeerContactSession => IBootstrappedNetworkDelegate
        #pragma mark

        virtual void onBootstrappedNetworkPreparationCompleted(IBootstrappedNetworkPtr bootstrappedNetwork);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServicePeerContactSession => IServiceSaltFetchSignedSaltQueryDelegate
        #pragma mark

        virtual void onServiceSaltFetchSignedSaltCompleted(IServiceSaltFetchSignedSaltQueryPtr query);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServicePeerContactSession => IMessageMonitorResultDelegate<PeerContactLoginResult>
        #pragma mark

        virtual bool handleMessageMonitorResultReceived(
                                                        IMessageMonitorPtr monitor,
                                                        PeerContactLoginResultPtr result
                                                        );

        virtual bool handleMessageMonitorErrorResultReceived(
                                                             IMessageMonitorPtr monitor,
                                                             PeerContactLoginResultPtr ignore, // will always be NULL
                                                             message::MessageResultPtr result
                                                             );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServicePeerContactSession => IMessageMonitorResultDelegate<PrivatePeerFileGetResult>
        #pragma mark

        virtual bool handleMessageMonitorResultReceived(
                                                        IMessageMonitorPtr monitor,
                                                        PrivatePeerFileGetResultPtr result
                                                        );

        virtual bool handleMessageMonitorErrorResultReceived(
                                                             IMessageMonitorPtr monitor,
                                                             PrivatePeerFileGetResultPtr ignore, // will always be NULL
                                                             message::MessageResultPtr result
                                                             );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServicePeerContactSession => IMessageMonitorResultDelegate<PrivatePeerFileSetResult>
        #pragma mark

        virtual bool handleMessageMonitorResultReceived(
                                                        IMessageMonitorPtr monitor,
                                                        PrivatePeerFileSetResultPtr result
                                                        );

        virtual bool handleMessageMonitorErrorResultReceived(
                                                             IMessageMonitorPtr monitor,
                                                             PrivatePeerFileSetResultPtr ignore, // will always be NULL
                                                             message::MessageResultPtr result
                                                             );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServicePeerContactSession => IMessageMonitorResultDelegate<PeerContactServicesGetResult>
        #pragma mark

        virtual bool handleMessageMonitorResultReceived(
                                                        IMessageMonitorPtr monitor,
                                                        PeerContactServicesGetResultPtr result
                                                        );

        virtual bool handleMessageMonitorErrorResultReceived(
                                                             IMessageMonitorPtr monitor,
                                                             PeerContactServicesGetResultPtr ignore, // will always be NULL
                                                             message::MessageResultPtr result
                                                             );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServicePeerContactSession => IMessageMonitorResultDelegate<PeerContactIdentityAssociateResult>
        #pragma mark

        virtual bool handleMessageMonitorResultReceived(
                                                        IMessageMonitorPtr monitor,
                                                        PeerContactIdentityAssociateResultPtr result
                                                        );

        virtual bool handleMessageMonitorErrorResultReceived(
                                                             IMessageMonitorPtr monitor,
                                                             PeerContactIdentityAssociateResultPtr ignore, // will always be NULL
                                                             message::MessageResultPtr result
                                                             );

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServicePeerContactSession => (internal)
        #pragma mark

        RecursiveLock &getLock() const;

        String log(const char *message) const;
        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        bool isShutdown() const {return SessionState_Shutdown == mCurrentState;}

        void step();
        bool stepLogin();
        bool stepPeerFiles();
        bool stepServices();
        bool stepAssociate();
        void postStep();

        void clearShutdown(ServiceIdentitySessionMap &identities) const;
        void handleRemoveDisposition(
                                     const IdentityInfo &info,
                                     ServiceIdentitySessionMap &sessions
                                     ) const;

        void setState(SessionStates state);
        void setError(WORD errorCode, const char *reason = NULL);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServicePeerContactSession => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;
        ServicePeerContactSessionWeakPtr mThisWeak;

        IServicePeerContactSessionDelegatePtr mDelegate;
        AccountWeakPtr mAccount;

        BootstrappedNetworkPtr mBootstrappedNetwork;

        IMessageMonitorPtr mLoginMonitor;
        IMessageMonitorPtr mPeerFilesGetMonitor;
        IMessageMonitorPtr mPeerFilesSetMonitor;
        IMessageMonitorPtr mServicesMonitor;
        IMessageMonitorPtr mAssociateMonitor;

        SessionStates mCurrentState;

        WORD mLastError;
        String mLastErrorReason;

        IPeerFilesPtr mPeerFiles;

        ServiceIdentitySessionPtr mLoginIdentity;

        String mContactUserID;
        String mContactAccessToken;
        String mContactAccessSecret;
        Time mContactAccessExpires;

        bool mRegeneratePeerFiles;

        IServiceSaltFetchSignedSaltQueryPtr mSaltQuery;
        ServiceTypeMap mServicesByType;

        ServiceIdentitySessionMap mAssociatedIdentities;
        SecureByteBlockPtr mLastNotificationHash;

        ServiceIdentitySessionMap mPendingUpdateIdentities;
        ServiceIdentitySessionMap mPendingRemoveIdentities;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IServicePeerContactSessionFactory
      #pragma mark

      interaction IServicePeerContactSessionFactory
      {
        static IServicePeerContactSessionFactory &singleton();

        virtual ServicePeerContactSessionPtr login(
                                                   IServicePeerContactSessionDelegatePtr delegate,
                                                   IServicePeerContactPtr servicePeerContact,
                                                   IServiceIdentitySessionPtr identitySession
                                                   );

        virtual ServicePeerContactSessionPtr relogin(
                                                     IServicePeerContactSessionDelegatePtr delegate,
                                                     IPeerFilesPtr existingPeerFiles
                                                     );
      };
      
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IServicePeerContactSessionAsync)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()
