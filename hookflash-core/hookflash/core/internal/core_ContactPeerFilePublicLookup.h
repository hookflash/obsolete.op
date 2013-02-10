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

#include <hookflash/core/internal/types.h>
#include <hookflash/core/IContactPeerFilePublicLookup.h>

#include <hookflash/stack/IBootstrappedNetwork.h>
#include <hookflash/stack/IMessageMonitor.h>
#include <hookflash/stack/message/peer-contact/PublicPeerFilesGetRequest.h>
#include <hookflash/stack/message/peer-contact/PublicPeerFilesGetResult.h>

#include <zsLib/MessageQueueAssociator.h>

namespace hookflash
{
  namespace core
  {
    namespace internal
    {
      using stack::IMessageMonitorPtr;
      using stack::IMessageMonitorResultDelegate;
      using stack::message::MessageResultPtr;
      using stack::message::peer_contact::PublicPeerFilesGetRequest;
      using stack::message::peer_contact::PublicPeerFilesGetRequestPtr;
      using stack::message::peer_contact::PublicPeerFilesGetResult;
      using stack::message::peer_contact::PublicPeerFilesGetResultPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IContactPeerFilePublicLookupForAccount
      #pragma mark

      interaction IContactPeerFilePublicLookupForAccount
      {
        IContactPeerFilePublicLookupForAccount &forAccount() {return *this;}
        const IContactPeerFilePublicLookupForAccount &forAccount() const {return *this;}

      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ContactPeerFilePublicLookup
      #pragma mark

      class ContactPeerFilePublicLookup : public MessageQueueAssociator,
                                          public IContactPeerFilePublicLookup,
                                          public IContactPeerFilePublicLookupForAccount,
                                          public IBootstrappedNetworkDelegate,
                                          public IMessageMonitorResultDelegate<PublicPeerFilesGetResult>
      {
      public:
        friend interaction IContactPeerFilePublicLookupFactory;
        friend interaction IContactPeerFilePublicLookup;

        typedef String Domain;
        typedef std::map<Domain, IBootstrappedNetworkPtr> BootstrappedNetworkMap;

        typedef PublicPeerFilesGetRequest::PeerInfo PeerInfo;
        typedef PublicPeerFilesGetRequest::PeerInfoList PeerInfoList;

        typedef std::map<Domain, PeerInfoList> DomainPeerInfoMap;

        typedef String PeerURI;
        typedef std::map<PeerURI, ContactPtr> ContactMap;

        typedef PUID MonitorID;
        typedef std::map<MonitorID, IMessageMonitorPtr> MonitorMap;

      protected:
        ContactPeerFilePublicLookup(
                                    IMessageQueuePtr queue,
                                    IContactPeerFilePublicLookupDelegatePtr delegate,
                                    const ContactList &contacts
                                    );

        void init();

      public:
        ~ContactPeerFilePublicLookup();

        static ContactPeerFilePublicLookupPtr convert(IContactPeerFilePublicLookupPtr lookup);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ContactPeerFilePublicLookup => IContactPeerFilePublicLookup
        #pragma mark

        static String toDebugString(IContactPeerFilePublicLookupPtr lookup, bool includeCommaPrefix = true);

        static ContactPeerFilePublicLookupPtr create(
                                                     IContactPeerFilePublicLookupDelegatePtr delegate,
                                                     const ContactList &contacts
                                                     );

        virtual PUID getID() const {return mID;}

        virtual bool isComplete() const;
        virtual bool wasSuccessful(
                                   WORD *outErrorCode,
                                   String *outErrorReason
                                   ) const;

        virtual void cancel();

        virtual ContactListPtr getContacts() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ContactPeerFilePublicLookup => IContactPeerFilePublicLookupForAccount
        #pragma mark

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ContactPeerFilePublicLookup => IBootstrappedNetworkDelegate
        #pragma mark

        virtual void onBootstrappedNetworkPreparationCompleted(IBootstrappedNetworkPtr bootstrappedNetwork);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ContactPeerFilePublicLookup => IMessageMonitorResultDelegate<PublicPeerFilesGetResult>
        #pragma mark

        virtual bool handleMessageMonitorResultReceived(
                                                        IMessageMonitorPtr monitor,
                                                        PublicPeerFilesGetResultPtr result
                                                        );

        virtual bool handleMessageMonitorErrorResultReceived(
                                                             IMessageMonitorPtr monitor,
                                                             PublicPeerFilesGetResultPtr ignore, // will always be NULL
                                                             MessageResultPtr result
                                                             );

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ContactPeerFilePublicLookup => (internal)
        #pragma mark

        String log(const char *message) const;
        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        RecursiveLock &getLock() const;

        void step();

        void setError(WORD errorCode, const char *errorReason);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ContactPeerFilePublicLookup => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;
        ContactPeerFilePublicLookupWeakPtr mThisWeak;

        IContactPeerFilePublicLookupDelegatePtr mDelegate;

        WORD mErrorCode;
        String mErrorReason;

        BootstrappedNetworkMap mBootstrappedNetworks;
        MonitorMap mMonitors;

        DomainPeerInfoMap mDomainPeerInfos;

        ContactMap mContacts;
        ContactList mResults;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IContactPeerFilePublicLookupFactory
      #pragma mark

      interaction IContactPeerFilePublicLookupFactory
      {
        static IContactPeerFilePublicLookupFactory &singleton();

        virtual ContactPeerFilePublicLookupPtr create(
                                                      IContactPeerFilePublicLookupDelegatePtr delegate,
                                                      const ContactList &contacts
                                                      );
      };
      
    }
  }
}
