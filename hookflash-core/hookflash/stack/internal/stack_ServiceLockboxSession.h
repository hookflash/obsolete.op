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
#include <hookflash/stack/IServiceLockbox.h>
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
      #pragma mark IServiceLockboxSessionForAccount
      #pragma mark

      interaction IServiceLockboxSessionForAccount
      {
        IServiceLockboxSessionForAccount &forAccount() {return *this;}
        const IServiceLockboxSessionForAccount &forAccount() const {return *this;}

        virtual IServiceLockboxSession::SessionStates getState(
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
      #pragma mark IServiceLockboxSessionForServiceIdentity
      #pragma mark

      interaction IServiceLockboxSessionForServiceIdentity
      {
        IServiceLockboxSessionForServiceIdentity &forServiceIdentity() {return *this;}
        const IServiceLockboxSessionForServiceIdentity &forServiceIdentity() const {return *this;}

        virtual IServiceLockboxSession::SessionStates getState(
                                                                   WORD *lastErrorCode = NULL,
                                                                   String *lastErrorReason = NULL
                                                                   ) const = 0;

        virtual IPeerFilesPtr getPeerFiles() const = 0;

        virtual void notifyStateChanged() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IServiceLockboxSessionAsync
      #pragma mark

      interaction IServiceLockboxSessionAsync
      {
        virtual void onStep() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceLockboxSession
      #pragma mark

      class ServiceLockboxSession : public Noop,
                                        public zsLib::MessageQueueAssociator,
                                        public IServiceLockboxSession,
                                        public IMessageSource,
                                        public IServiceLockboxSessionForAccount,
                                        public IServiceLockboxSessionForServiceIdentity,
                                        public IServiceLockboxSessionAsync,
                                        public IBootstrappedNetworkDelegate,
                                        public IServiceSaltFetchSignedSaltQueryDelegate,
                                        public IMessageMonitorResultDelegate<PeerContactLoginResult>,
                                        public IMessageMonitorResultDelegate<PrivatePeerFileGetResult>,
                                        public IMessageMonitorResultDelegate<PrivatePeerFileSetResult>,
                                        public IMessageMonitorResultDelegate<PeerContactIdentityAssociateResult>,
                                        public IMessageMonitorResultDelegate<PeerContactServicesGetResult>
      {
      public:
        friend interaction IServiceLockboxSessionFactory;
        friend interaction IServiceLockboxSession;

        typedef PUID ServiceIdentitySessionID;
        typedef std::map<ServiceIdentitySessionID, ServiceIdentitySessionPtr> ServiceIdentitySessionMap;

      protected:
        ServiceLockboxSession(
                              IMessageQueuePtr queue,
                              BootstrappedNetworkPtr network,
                              IServiceLockboxSessionDelegatePtr delegate
                              );
        
        ServiceLockboxSession(Noop) : Noop(true), MessageQueueAssociator(IMessageQueuePtr()) {};

        void init();

      public:
        ~ServiceLockboxSession();

        static ServiceLockboxSessionPtr convert(IServiceLockboxSessionPtr query);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceLockboxSession => IServiceLockboxSession
        #pragma mark

        static String toDebugString(IServiceLockboxSessionPtr session, bool includeCommaPrefix = true);

        static ServiceLockboxSessionPtr login(
                                              IServiceLockboxSessionDelegatePtr delegate,
                                              IServiceLockboxPtr serviceLockbox,
                                              IServiceIdentitySessionPtr identitySession
                                              );

        static ServiceLockboxSessionPtr relogin(
                                                IServiceLockboxSessionDelegatePtr delegate,
                                                IServiceLockboxPtr serviceLockbox,
                                                const char *lockboxAccountID,
                                                const char *identityHalfLockboxKey,
                                                const char *lockboxHalfLockboxKey
                                                );

        virtual PUID getID() const {return mID;}

        virtual IServiceLockboxPtr getService() const;

        virtual SessionStates getState(
                                       WORD *lastErrorCode,
                                       String *lastErrorReason
                                       ) const;

        virtual IPeerFilesPtr getPeerFiles() const;

        virtual String getLockboxAccountID() const;
        virtual void getLockboxKey(
                                   SecureByteBlockPtr &outIdentityHalf,
                                   SecureByteBlockPtr &outLockboxHalf
                                   );
        

        virtual ServiceIdentitySessionListPtr getAssociatedIdentities() const;
        virtual void associateIdentities(
                                         const ServiceIdentitySessionList &identitiesToAssociate,
                                         const ServiceIdentitySessionList &identitiesToRemove
                                         );

        virtual String getInnerBrowserWindowFrameURL() const;

        virtual void notifyBrowserWindowVisible();
        virtual void notifyBrowserWindowClosed();

        virtual DocumentPtr getNextMessageForInnerBrowerWindowFrame();
        virtual void handleMessageFromInnerBrowserWindowFrame(DocumentPtr unparsedMessage);

        virtual void cancel();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceLockboxSession => IMessageSource
        #pragma mark

        // (duplicate) virtual PUID getID() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceLockboxSession => IServiceLockboxSessionForAccount
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
        #pragma mark ServiceLockboxSession => IServiceLockboxSessionForServiceIdentity
        #pragma mark

        // (duplicate) virtual SessionStates getState(
        //                                            WORD *lastErrorCode,
        //                                            String *lastErrorReason
        //                                            ) const;

        // (duplicate) virtual IPeerFilesPtr getPeerFiles() const;

        virtual void notifyStateChanged();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceLockboxSession => IServiceLockboxSessionAsync
        #pragma mark

        virtual void onStep();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceLockboxSession => IBootstrappedNetworkDelegate
        #pragma mark

        virtual void onBootstrappedNetworkPreparationCompleted(IBootstrappedNetworkPtr bootstrappedNetwork);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceLockboxSession => IServiceSaltFetchSignedSaltQueryDelegate
        #pragma mark

        virtual void onServiceSaltFetchSignedSaltCompleted(IServiceSaltFetchSignedSaltQueryPtr query);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceLockboxSession => IMessageMonitorResultDelegate<PeerContactLoginResult>
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
        #pragma mark ServiceLockboxSession => IMessageMonitorResultDelegate<PrivatePeerFileGetResult>
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
        #pragma mark ServiceLockboxSession => IMessageMonitorResultDelegate<PrivatePeerFileSetResult>
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
        #pragma mark ServiceLockboxSession => IMessageMonitorResultDelegate<PeerContactServicesGetResult>
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
        #pragma mark ServiceLockboxSession => IMessageMonitorResultDelegate<PeerContactIdentityAssociateResult>
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
        #pragma mark ServiceLockboxSession => (internal)
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
        #pragma mark ServiceLockboxSession => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;
        ServiceLockboxSessionWeakPtr mThisWeak;

        IServiceLockboxSessionDelegatePtr mDelegate;
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
      #pragma mark IServiceLockboxSessionFactory
      #pragma mark

      interaction IServiceLockboxSessionFactory
      {
        static IServiceLockboxSessionFactory &singleton();

        virtual ServiceLockboxSessionPtr login(
                                               IServiceLockboxSessionDelegatePtr delegate,
                                               IServiceLockboxPtr ServiceLockbox,
                                               IServiceIdentitySessionPtr identitySession
                                               );
        
        virtual ServiceLockboxSessionPtr relogin(
                                                 IServiceLockboxSessionDelegatePtr delegate,
                                                 IServiceLockboxPtr serviceLockbox,
                                                 const char *lockboxAccountID,
                                                 const char *identityHalfLockboxKey,
                                                 const char *lockboxHalfLockboxKey
                                                 );
      };
      
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IServiceLockboxSessionAsync)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()
