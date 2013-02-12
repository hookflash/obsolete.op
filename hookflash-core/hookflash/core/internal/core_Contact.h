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

#include <hookflash/core/IContact.h>
#include <hookflash/core/internal/types.h>
//#include <hookflash/stack/types.h>
//#include <hookflash/stack/message/types.h>

namespace hookflash
{
  namespace core
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IContactForAccount
      #pragma mark

      interaction IContactForAccount
      {
        IContactForAccount &forAccount() {return *this;}
        const IContactForAccount &forAccount() const {return *this;}

        static ContactPtr createFromPeer(
                                         AccountPtr account,
                                         IPeerPtr peer,
                                         const char *userIDIfKnown = NULL
                                         );

        virtual String getPeerURI() const = 0;
        virtual IPeerPtr getPeer() const = 0;

        virtual IPeerFilePublicPtr getPeerFilePublic() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IContactForContactPeerFilePublicLookup
      #pragma mark

      interaction IContactForContactPeerFilePublicLookup
      {
        IContactForContactPeerFilePublicLookup &forContactPeerFilePublicLookup() {return *this;}
        const IContactForContactPeerFilePublicLookup &forContactPeerFilePublicLookup() const {return *this;}

        virtual String getFindSecret() const = 0;

        virtual IPeerPtr getPeer() const = 0;

        virtual void setPeerFilePublic(IPeerFilePublicPtr peerFile) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IContactForConversationThread
      #pragma mark

      interaction IContactForConversationThread
      {
        IContactForConversationThread &forConversationThread() {return *this;}
        const IContactForConversationThread &forConversationThread() const {return *this;}

        static ContactPtr createFromPeerFilePublic(
                                                   AccountPtr account,
                                                   IPeerFilePublicPtr peerFilePublic
                                                   );

        virtual String getPeerURI() const = 0;
        virtual IPeerPtr getPeer() const = 0;

        virtual bool isSelf() const = 0;

        virtual IPeerFilePublicPtr getPeerFilePublic() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IContactForCall
      #pragma mark

      interaction IContactForCall
      {
        IContactForCall &forCall() {return *this;}
        const IContactForCall &forCall() const {return *this;}

        virtual bool isSelf() const = 0;

        virtual String getPeerURI() const = 0;
        virtual IPeerPtr getPeer() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IContactForIdentityLookup
      #pragma mark

      interaction IContactForIdentityLookup
      {
        IContactForIdentityLookup &forIdentityLookup() {return *this;}
        const IContactForIdentityLookup &forIdentityLookup() const {return *this;}

        static ContactPtr createFromPeerURI(
                                            AccountPtr account,
                                            const char *peerURI,
                                            const char *findSecret,
                                            const char *userID
                                            );
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Contact
      #pragma mark

      class Contact : public Noop,
                      public IContact,
                      public IContactForAccount,
                      public IContactForContactPeerFilePublicLookup,
                      public IContactForConversationThread,
                      public IContactForCall
      {
      public:
        friend interaction IContactFactory;
        friend interaction IContact;

      protected:
        Contact();
        
        Contact(Noop) : Noop(true) {};

        void init();

      public:
        ~Contact();

        static ContactPtr convert(IContactPtr contact);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Contact => IContact
        #pragma mark

        static String toDebugString(IContactPtr contact, bool includeCommaPrefix = true);

        static ContactPtr createFromPeerFilePublic(
                                                   IAccountPtr account,
                                                   ElementPtr peerFilePublicEl,
                                                   const char *previousStableUniqueID = NULL // (if known)
                                                   );

        static ContactPtr createFromPeerURI(
                                            IAccountPtr account,
                                            const char *peerURI,
                                            const char *findSecret,
                                            const char *inStableID,
                                            const char *inUserID
                                            );

        static IContactPtr getForSelf(IAccountPtr account);

        virtual PUID getID() const {return  mID;}

        virtual bool isSelf() const;

        virtual String getPeerURI() const;
        virtual String getFindSecret() const;
        virtual String getStableUniqueID() const;

        virtual bool hasPeerFilePublic() const;
        virtual ElementPtr savePeerFilePublic() const;

        virtual IAccountPtr getAssociatedAccount() const;

        virtual void hintAboutLocation(const char *contactsLocationID);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Contact => IContactForAccount
        #pragma mark

        static ContactPtr createFromPeer(
                                         AccountPtr account,
                                         IPeerPtr peer,
                                         const char *userIDIfKnown = NULL
                                         );

        // (duplicate) virtual String getPeerURI() const;
        virtual IPeerPtr getPeer() const;

        virtual IPeerFilePublicPtr getPeerFilePublic() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Contact => IContactForContactPeerFilePublicLookup
        #pragma mark

        // (duplicate) virtual String getFindSecret() const;

        // (duplicate) virtual IPeerPtr getPeer() const;

        virtual void setPeerFilePublic(IPeerFilePublicPtr peerFile);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Contact => IContactForConversationThread
        #pragma mark

        static ContactPtr createFromPeerFilePublic(
                                                   AccountPtr account,
                                                   IPeerFilePublicPtr publicPeerFile,
                                                   const char *previousStableUniqueID = NULL // (if known)
                                                   );

        // (duplicate) virtual String getPeerURI() const;
        // (duplicate) virtual IPeerPtr getPeer() const;

        // (duplicate) virtual bool isSelf() const;

        // (duplicate) virtual IPeerFilePublicPtr getPeerFilePublic() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Contact => IContactForCall
        #pragma mark

        // (duplicate) virtual bool isSelf() const;

        // (duplicate) virtual String getPeerURI() const;
        // (duplicate) virtual IPeerPtr getPeer() const;

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Contact => (internal)
        #pragma mark

        String log(const char *message) const;
        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        RecursiveLock &getLock() const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Contact => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mBogusLock;
        ContactWeakPtr mThisWeak;

        AccountWeakPtr mAccount;

        IPeerPtr mPeer;
        String mUserID;
        mutable String mCalculatedUniqueID;

        String mFindSecret;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IContactFactory
      #pragma mark

      interaction IContactFactory
      {
        static IContactFactory &singleton();

        virtual ContactPtr createFromPeerURI(
                                             IAccountPtr account,
                                             const char *peerURI,
                                             const char *findSecret,
                                             const char *inStableID,
                                             const char *inUserID
                                             );

        virtual ContactPtr createFromPeer(
                                          AccountPtr account,
                                          IPeerPtr peer,
                                          const char *userIDIfKnown = NULL
                                          );

        virtual ContactPtr createFromPeerFilePublic(
                                                    AccountPtr account,
                                                    IPeerFilePublicPtr publicPeerFile,
                                                    const char *previousStableUniqueID = NULL // (if known)
                                                    );
      };

    }
  }
}
