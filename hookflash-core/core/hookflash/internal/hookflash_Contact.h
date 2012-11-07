/*
 
 Copyright (c) 2012, SMB Phone Inc.
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

#include <hookflash/IContact.h>
#include <hookflash/internal/hookflashTypes.h>
#include <hookflash/stack/hookflashTypes.h>
#include <hookflash/stack/message/hookflashTypes.h>

namespace hookflash
{
  namespace internal
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IContactForAccount
    #pragma mark

    interaction IContactForAccount
    {
      typedef zsLib::String String;
      typedef stack::IPeerFilePublicPtr IPeerFilePublicPtr;

      static IContactForAccountPtr convert(IContactPtr contact);
      virtual IContactPtr convertIContact() const = 0;

      static IContactForAccountPtr createFromPeerFilePublic(
                                                            IAccountForContactPtr account,
                                                            IPeerFilePublicPtr publicPeerFile
                                                            );

      virtual String getContactID() const = 0;

      virtual IPeerFilePublicPtr getPeerFilePublic() const = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IContactForConversationThread
    #pragma mark

    interaction IContactForConversationThread
    {
      typedef zsLib::String String;
      typedef stack::IPeerFilePublicPtr IPeerFilePublicPtr;

      static IContactForConversationThreadPtr convert(IContactPtr contact);
      static IContactForConversationThreadPtr convert(IContactForAccountPtr contact);

      virtual IContactPtr convertIContact() const = 0;

      static IContactForConversationThreadPtr createFromPeerFilePublic(
                                                                       IAccountForConversationThreadPtr account,
                                                                       IPeerFilePublicPtr publicPeerFile
                                                                       );

      virtual String getContactID() const = 0;

      virtual bool isSelf() = 0;

      virtual IPeerFilePublicPtr getPeerFilePublic() const = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IContactForCall
    #pragma mark

    interaction IContactForCall
    {
      typedef zsLib::String String;

      static IContactForCallPtr convert(IContactPtr contact);
      static IContactForCallPtr convert(IContactForConversationThreadPtr contact);

      virtual IContactPtr convertIContact() const = 0;

      virtual String getContactID() const = 0;

      virtual bool isSelf() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Contact
    #pragma mark

    class Contact : public IContact,
                    public IContactForAccount,
                    public IContactForConversationThread,
                    public IContactForCall
    {
    public:
      typedef zsLib::UINT UINT;
      typedef zsLib::String String;
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef stack::IPeerFilesPtr IPeerFilesPtr;
      typedef stack::IPeerFilePublicPtr IPeerFilePublicPtr;
      typedef stack::IPeerContactProfilePtr IPeerContactProfilePtr;

    protected:
      Contact();

    public:
      ~Contact();

      static ContactPtr convert(IContactPtr contact);

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Contact => IContact
      #pragma mark

      virtual hookflash::IAccountPtr getAccount() const;

      virtual String getContactID() const;

      static ContactPtr createSelfContact(
                                          IAccountPtr account,
                                          IPeerFilesPtr peerFiles
                                          );

      static ContactPtr createDownloadable(
                                           IAccountPtr account,
                                           const char *contactID,
                                           const char *hintGroupID,
                                           const char *contactProfileSecret
                                           );

      static IContactPtr createFromPeerFilePublic(
                                                  IAccountPtr account,
                                                  const char *publicPeerFile
                                                  );

      virtual bool isSelf();
      virtual ContactTypes getContactType();

      virtual bool isEditable();
      virtual bool isPublicXMLEditable();

      virtual String getPublicXML();
      virtual String getPrivateXML();

      virtual bool updateProfile(
                                 const char *publicXML,
                                 const char *privateXML
                                 );

      virtual ULONG getProfileVersion();

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Contact => IContactForAccount
      #pragma mark

      virtual IContactPtr convertIContact() const {return mThisWeak.lock();}

      static ContactPtr createFromPeerFilePublic(
                                                 IAccountForContactPtr account,
                                                 IPeerFilePublicPtr peerFilePublic
                                                 );

      // (duplicate) virtual String getContactID() const;

      virtual IPeerFilePublicPtr getPeerFilePublic() const;

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Contact => IContactForConversationThread
      #pragma mark

      // (duplicate) virtual String getContactID() const;

      // (duplicate) virtual IContactPtr convertIContact() const;

      static ContactPtr createFromPeerFilePublic(
                                                 IAccountForConversationThreadPtr account,
                                                 IPeerFilePublicPtr publicPeerFile
                                                 );

      // (duaplicate) virtual String getContactID() const;
      // (duplicate) virtual bool isSelf();

      // (duplicate) virtual IPeerFilePublicPtr getPeerFilePublic() const;

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Contact => IContactForCall
      #pragma mark

      // (duplicate) virtual IContactPtr convertIContact() const;
      // (duaplicate) virtual String getContactID() const;

      // (duplicate) virtual bool isSelf();

    private:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Contact => (internal)
      #pragma mark

      bool areDetailsReady();
      void updateFrom(IPeerContactProfilePtr &contactProfile);
      bool requiresUpdate();
      void notifyServerUpdated();

      void fixContactProfile();

    protected:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Contact => (data)
      #pragma mark

      mutable RecursiveLock mLock;
      ContactWeakPtr mThisWeak;

      IAccountForContactWeakPtr mAccount;
      bool mExternal;

      String mContactID;
      IPeerFilePublicPtr mPeerFilePublic;

      String mContactProfileSecret;
      IPeerContactProfilePtr mContactProfile;

      // lookup information
      String mHintGroupID;

      UINT mServerVersion;
    };
  }
}
