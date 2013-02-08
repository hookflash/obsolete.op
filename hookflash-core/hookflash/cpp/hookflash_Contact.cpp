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

#include <hookflash/internal/hookflash_Contact.h>
#include <hookflash/internal/hookflash_Account.h>
#include <hookflash/stack/IPeerFilePublic.h>
#include <hookflash/stack/IPeerContactProfile.h>
#include <hookflash/stack/IPeerFiles.h>
#include <hookflash/stack/IHelper.h>
#include <zsLib/Log.h>
#include <zsLib/XML.h>

#include <cryptopp/sha.h>

namespace hookflash { ZS_DECLARE_SUBSYSTEM(hookflash) }


namespace hookflash
{
  namespace internal
  {
    typedef zsLib::ULONG ULONG;
    typedef zsLib::CSTR CSTR;
    typedef zsLib::String String;
    typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
    typedef zsLib::XML::Element Element;
    typedef zsLib::XML::ElementPtr ElementPtr;
    typedef zsLib::XML::Document Document;
    typedef zsLib::XML::DocumentPtr DocumentPtr;
    typedef stack::IPeerFilePublic IPeerFilePublic;
    typedef stack::IPeerFilePublicPtr IPeerFilePublicPtr;

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IContactForAccount
    #pragma mark

    //-------------------------------------------------------------------------
    IContactForAccountPtr IContactForAccount::convert(IContactPtr contact)
    {
      return boost::dynamic_pointer_cast<Contact>(contact);
    }

    //-------------------------------------------------------------------------
    IContactForAccountPtr IContactForAccount::createFromPeerFilePublic(
                                                                       IAccountForContactPtr account,
                                                                       IPeerFilePublicPtr peerFilePublic
                                                                       )
    {
      return Contact::createFromPeerFilePublic(account, peerFilePublic);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IContactForConversationThread
    #pragma mark

    //-------------------------------------------------------------------------
    IContactForConversationThreadPtr IContactForConversationThread::convert(IContactPtr contact)
    {
      return boost::dynamic_pointer_cast<Contact>(contact);
    }

    //-------------------------------------------------------------------------
    IContactForConversationThreadPtr IContactForConversationThread::convert(IContactForAccountPtr contact)
    {
      return boost::dynamic_pointer_cast<Contact>(contact);
    }

    //-------------------------------------------------------------------------
    IContactForConversationThreadPtr IContactForConversationThread::createFromPeerFilePublic(
                                                                                             IAccountForConversationThreadPtr account,
                                                                                             IPeerFilePublicPtr peerFilePublic
                                                                                             )
    {
      return Contact::createFromPeerFilePublic(account, peerFilePublic);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IContactForCall
    #pragma mark

    //-------------------------------------------------------------------------
    IContactForCallPtr IContactForCall::convert(IContactPtr contact)
    {
      return boost::dynamic_pointer_cast<Contact>(contact);
    }

    //-------------------------------------------------------------------------
    IContactForCallPtr IContactForCall::convert(IContactForConversationThreadPtr contact)
    {
      return boost::dynamic_pointer_cast<Contact>(contact);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Contact
    #pragma mark

    //-------------------------------------------------------------------------
    Contact::Contact() :
      mExternal(false),
      mServerVersion(0)
    {
    }

    //-------------------------------------------------------------------------
    Contact::~Contact()
    {
    }

    //-------------------------------------------------------------------------
    ContactPtr Contact::convert(IContactPtr contact)
    {
      return boost::dynamic_pointer_cast<Contact>(contact);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Contact => IContact
    #pragma mark

    //-------------------------------------------------------------------------
    hookflash::IAccountPtr Contact::getAccount() const
    {
      AutoRecursiveLock lock(mLock);
      IAccountForContactPtr account = mAccount.lock();
      if (!account) return IAccountPtr();
      return account->convertIAccount();
    }

    //-----------------------------------------------------------------------
    String Contact::getContactID() const
    {
      AutoRecursiveLock lock(mLock);
      if (!mContactID.isEmpty()) return mContactID;
      if (mPeerFilePublic) return mPeerFilePublic->getContactID();
      if (mContactProfile) return mContactProfile->getContactID();
      return String();
    }

    //-----------------------------------------------------------------------
    ContactPtr Contact::createSelfContact(
                                          IAccountPtr account,
                                          IPeerFilesPtr peerFiles
                                          )
    {
      ContactPtr pThis(new Contact);
      pThis->mThisWeak = pThis;
      pThis->mAccount = boost::dynamic_pointer_cast<Account>(account);
      pThis->mPeerFilePublic = peerFiles->getPublic();
      pThis->fixContactProfile();
      return pThis;
    }

    ContactPtr Contact::createDownloadable(
                                           IAccountPtr account,
                                           const char *contactID,
                                           const char *hintGroupID,
                                           const char *contactProfileSecret
                                           )
    {
      ContactPtr pThis(new Contact);
      pThis->mThisWeak = pThis;
      pThis->mAccount = boost::dynamic_pointer_cast<Account>(account);
      pThis->mContactID = contactID;
      pThis->mHintGroupID = hintGroupID;
      pThis->mContactProfileSecret = contactProfileSecret;
      return pThis;
    }

    IContactPtr Contact::createFromPeerFilePublic(
                                                  IAccountPtr account,
                                                  const char *publicPeerFile
                                                  )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!account)

      if (!publicPeerFile) return IContactPtr();

      DocumentPtr document = Document::create();
      document->parse(publicPeerFile);
      ElementPtr root = document->getFirstChildElement();
      if (!root) return IContactPtr();

      ContactPtr pThis(new Contact);
      pThis->mThisWeak = pThis;
      pThis->mAccount = boost::dynamic_pointer_cast<Account>(account);
      pThis->mPeerFilePublic = IPeerFilePublic::loadFromXML(root);
      if (!pThis->mPeerFilePublic) return IContactPtr();
      pThis->mContactID = pThis->mPeerFilePublic->getContactID();

      account->notifyAboutContact(pThis);
      return pThis;
    }

    //-----------------------------------------------------------------------
    bool Contact::isSelf()
    {
      AutoRecursiveLock lock(mLock);
      if (!mPeerFilePublic) return false;
      return (mPeerFilePublic->getPeerFiles());
    }

    //-----------------------------------------------------------------------
    Contact::ContactTypes Contact::getContactType()
    {
      AutoRecursiveLock lock(mLock);
      fixContactProfile();
      if (isSelf()) return ContactType_OpenPeer;
      if (!mContactProfile) return (mExternal ? ContactType_External : ContactType_OpenPeer);
      return (mContactProfile->isExternal() ? ContactType_External : ContactType_OpenPeer);
    }

    //-----------------------------------------------------------------------
    bool Contact::isEditable()
    {
      AutoRecursiveLock lock(mLock);
      if (isSelf()) return true;
      if (ContactType_External == getContactType()) return true;
      return false;
    }

    //-----------------------------------------------------------------------
    bool Contact::isPublicXMLEditable()
    {
      AutoRecursiveLock lock(mLock);
      return isSelf();
    }

    //-----------------------------------------------------------------------
    String Contact::getPublicXML()
    {
      AutoRecursiveLock lock(mLock);
      fixContactProfile();
      if (!mContactProfile) return String();

      ElementPtr profileElement = mContactProfile->getPublicProfile();
      if (!profileElement) return String();

      DocumentPtr document = Document::create();
      document->adoptAsLastChild(profileElement);
      boost::shared_array<char> output;
      output = document->write();
      return (CSTR)(output.get());
    }

    //-----------------------------------------------------------------------
    String Contact::getPrivateXML()
    {
      AutoRecursiveLock lock(mLock);
      fixContactProfile();
      if (!mContactProfile) return String();

      ElementPtr profileElement = mContactProfile->getPrivateProfile();
      if (!profileElement) return String();

      DocumentPtr document = Document::create();
      document->adoptAsLastChild(profileElement);
      boost::shared_array<char> output;
      output = document->write();
      return (CSTR)(output.get());
    }

    //-----------------------------------------------------------------------
    bool Contact::updateProfile(
                                const char *inPublicXML,
                                const char *inPrivateXML
                                )
    {
      AutoRecursiveLock lock(mLock);

      fixContactProfile();

      IAccountForContactPtr account = mAccount.lock();
      if (!account) return false;

      if (!isEditable()) return false;
      if (!mContactProfile) return false;

      String publicXML = inPublicXML;
      String privateXML = inPrivateXML;
      if (!publicXML.isEmpty()) {
        if (!isPublicXMLEditable()) return false;

        DocumentPtr document = Document::create();
        document->parse(publicXML);
        ElementPtr profileElement = document->findFirstChildElement("profile");
        if (!profileElement) return false;

        if (!mContactProfile->setPublicProfile(profileElement)) return false;
      }

      if (!privateXML.isEmpty()) {
        DocumentPtr document = Document::create();
        document->parse(privateXML);
        ElementPtr profileElement = document->findFirstChildElement("profile");
        if (!profileElement) return false;

        if (!mContactProfile->setPrivateProfile(profileElement)) return false;
      }

      return true;
    }

    //-----------------------------------------------------------------------
    ULONG Contact::getProfileVersion()
    {
      AutoRecursiveLock lock(mLock);
      fixContactProfile();
      if (!mContactProfile) return 0;

      return static_cast<ULONG>(mContactProfile->getVersionNumber());
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Contact => IContactForAccount
    #pragma mark

    //-------------------------------------------------------------------------
    ContactPtr Contact::createFromPeerFilePublic(
                                             IAccountForContactPtr account,
                                             IPeerFilePublicPtr peerFilePublic
                                             )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!account)
      ZS_THROW_INVALID_ARGUMENT_IF(!peerFilePublic)

      ContactPtr pThis(new Contact);
      pThis->mThisWeak = pThis;
      pThis->mAccount = account;
      pThis->mPeerFilePublic = peerFilePublic;
      pThis->mContactID = pThis->mPeerFilePublic->getContactID();

      account->notifyAboutContact(pThis);
      return pThis;
    }

    //-------------------------------------------------------------------------
    IPeerFilePublicPtr Contact::getPeerFilePublic() const
    {
      AutoRecursiveLock lock(mLock);
      return mPeerFilePublic;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Contact => IContactForConversationThread
    #pragma mark

    //-------------------------------------------------------------------------
    ContactPtr Contact::createFromPeerFilePublic(
                                                 IAccountForConversationThreadPtr account,
                                                 IPeerFilePublicPtr publicPeerFile
                                                 )
    {
      return createFromPeerFilePublic(IAccountForContact::convert(account), publicPeerFile);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Contact => IContactForCall
    #pragma mark

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Contact => (internal)
    #pragma mark

    //-------------------------------------------------------------------------
    bool Contact::areDetailsReady()
    {
      AutoRecursiveLock lock(mLock);
      return mContactProfile;
    }

    //-------------------------------------------------------------------------
    void Contact::updateFrom(IPeerContactProfilePtr &contactProfile)
    {
      AutoRecursiveLock lock(mLock);
      if (!contactProfile) return;

      IAccountForContactPtr account = mAccount.lock();
      if (!account) return;

      if (!mContactProfile) {
        if (mPeerFilePublic) {
          IPeerFilesPtr peerFiles = mPeerFilePublic->getPeerFiles();
          if (peerFiles) {
            peerFiles->loadContactProfileFromExistingContactProfile(account->getPassword(), contactProfile);
            mContactProfile = peerFiles->getContactProfile();
          }
        }

        if (!mContactProfile)
          mContactProfile = contactProfile;
      }

      if (!mContactProfileSecret.isEmpty())
        mContactProfile->usesContactProfileSecret(mContactProfileSecret);

      if (!mPeerFilePublic) {
        mPeerFilePublic = mContactProfile->getPeerFilePublic();
      }
      mServerVersion = mContactProfile->getVersionNumber();
    }

    //-------------------------------------------------------------------------
    bool Contact::requiresUpdate()
    {
      AutoRecursiveLock lock(mLock);
      if (!mContactProfile) return false;
      if (mContactProfile->getVersionNumber() > mServerVersion) return true;
      return false;
    }

    //-------------------------------------------------------------------------
    void Contact::notifyServerUpdated()
    {
      AutoRecursiveLock lock(mLock);
      if (!mContactProfile) return;
      mServerVersion = mContactProfile->getVersionNumber();
    }

    //-------------------------------------------------------------------------
    void Contact::fixContactProfile()
    {
      if (mContactProfile) return;
      if (!mPeerFilePublic) return;

      IPeerFilesPtr peerFiles = mPeerFilePublic->getPeerFiles();
      if (!peerFiles) return;

      mContactProfile = peerFiles->getContactProfile();
      if (mContactProfile)
        mServerVersion = mContactProfile->getVersionNumber();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IContact
  #pragma mark

  //---------------------------------------------------------------------------
  hookflash::IContactPtr IContact::createFromPeerFilePublic(
                                                            IAccountPtr account,
                                                            const char *publicPeerFile
                                                            )
  {
    return internal::Contact::createFromPeerFilePublic(account, publicPeerFile);
  }

  //-------------------------------------------------------------------------
  //-------------------------------------------------------------------------
  //-------------------------------------------------------------------------
  //-------------------------------------------------------------------------
}
