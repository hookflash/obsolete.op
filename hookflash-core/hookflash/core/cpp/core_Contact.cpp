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

#include <hookflash/core/internal/core_Contact.h>
#include <hookflash/core/internal/core_Account.h>
#include <hookflash/core/internal/core_Helper.h>

#include <hookflash/stack/IPeerFilePublic.h>
#include <hookflash/stack/IHelper.h>

#include <zsLib/Stringize.h>
#include <zsLib/helpers.h>


namespace hookflash { namespace core { ZS_DECLARE_SUBSYSTEM(hookflash_core) } }

namespace hookflash
{
  namespace core
  {
    namespace internal
    {
      using zsLib::Stringize;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IContactForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      ContactPtr IContactForAccount::createFromPeer(
                                                    AccountPtr account,
                                                    IPeerPtr peer,
                                                    const char *userIDIfKnown
                                                    )
      {
        return IContactFactory::singleton().createFromPeer(account, peer, userIDIfKnown);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IContactForConversationThread
      #pragma mark

      //-----------------------------------------------------------------------
      ContactPtr IContactForConversationThread::createFromPeerFilePublic(
                                                                         AccountPtr account,
                                                                         IPeerFilePublicPtr peerFilePublic
                                                                         )
      {
        return IContactFactory::singleton().createFromPeerFilePublic(account, peerFilePublic);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IContactForCall
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IContactForIdentityLookup
      #pragma mark

      //-----------------------------------------------------------------------
      ContactPtr IContactForIdentityLookup::createFromPeerURI(
                                                              AccountPtr account,
                                                              const char *peerURI,
                                                              const char *findSecret,
                                                              const char *userID
                                                              )
      {
        return IContactFactory::singleton().createFromPeerURI(account, peerURI, findSecret, NULL, userID);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Contact
      #pragma mark

      //-----------------------------------------------------------------------
      Contact::Contact() :
        mID(zsLib::createPUID())
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void Contact::init()
      {
        ZS_LOG_DEBUG(log("init") + getDebugValueString())
      }

      //-----------------------------------------------------------------------
      Contact::~Contact()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
      }

      //-----------------------------------------------------------------------
      ContactPtr Contact::convert(IContactPtr contact)
      {
        return boost::dynamic_pointer_cast<Contact>(contact);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Contact => IContact
      #pragma mark

      //-----------------------------------------------------------------------
      String Contact::toDebugString(IContactPtr contact, bool includeCommaPrefix)
      {
        if (!contact) return includeCommaPrefix ? String(", contact=(null)") : String("contact=(null)");
        return Contact::convert(contact)->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      ContactPtr Contact::createFromPeerFilePublic(
                                                   IAccountPtr inAccount,
                                                   ElementPtr peerFilePublicEl,
                                                   const char *previousStableUniqueID
                                                   )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!inAccount)
        ZS_THROW_INVALID_ARGUMENT_IF(!peerFilePublicEl)

        AccountPtr account = Account::convert(inAccount);

        IPeerFilePublicPtr peerFilePublic = IPeerFilePublic::loadFromElement(peerFilePublicEl);
        if (!peerFilePublic) {
          ZS_LOG_ERROR(Detail, "failed to load peer file public from element")
          return ContactPtr();
        }

        return IContactFactory::singleton().createFromPeerFilePublic(account, peerFilePublic, previousStableUniqueID);
      }

      //-----------------------------------------------------------------------
      ContactPtr Contact::createFromPeerURI(
                                            IAccountPtr inAccount,
                                            const char *inPeerURI,
                                            const char *inFindSecret,
                                            const char *inStableID,
                                            const char *inUserID
                                            )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!inAccount)

        AccountPtr account = Account::convert(inAccount);
        stack::IAccountPtr stackAcount = account->forContact().getStackAccount();

        String peerURI(inPeerURI ? String(inPeerURI) : String());
        String findSecret(inFindSecret ? String(inFindSecret) : String());
        String stableID(inStableID ? String(inStableID) : String());
        String userID(inUserID ? String(inUserID) : String());

        if (!stackAcount) {
          ZS_LOG_ERROR(Detail, "stack account is not ready")
          return ContactPtr();
        }

        if (!IPeer::isValid(peerURI)) {
          ZS_LOG_ERROR(Detail, "peer URI specified is not valid, peer URI=" + peerURI)
          return ContactPtr();
        }

        IPeerPtr peer = IPeer::create(stackAcount, peerURI);

        AutoRecursiveLock lock(account->forContact().getLock());

        ContactPtr existingPeer = account->forContact().findContact(peerURI);
        if (existingPeer) {
          if (existingPeer->mFindSecret.isEmpty()) {
            existingPeer->mFindSecret = findSecret;
          }
          if (existingPeer->mCalculatedUniqueID.isEmpty()) {
            existingPeer->mCalculatedUniqueID = stableID;
          }
          if (existingPeer->mUserID.isEmpty()) {
            existingPeer->mUserID = userID;
          }
          return existingPeer;
        }

        if (!peer->getPeerFilePublic()) {
          if (findSecret.isEmpty()) {
            ZS_LOG_WARNING(Detail, "find secret was not set (may not be capable of downloading peer file public as a result), peer URI=" + peerURI)
          }
        }

        ContactPtr pThis(new Contact);
        pThis->mThisWeak = pThis;
        pThis->mAccount = account;
        pThis->mPeer = peer;
        pThis->mUserID = userID;
        pThis->mCalculatedUniqueID = stableID;
        pThis->mFindSecret = findSecret;
        pThis->init();
        account->forContact().notifyAboutContact(pThis);
        return pThis;
      }

      //-----------------------------------------------------------------------
      IContactPtr Contact::getForSelf(IAccountPtr inAccount)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!inAccount)

        AccountPtr account = Account::convert(inAccount);
        return account->forContact().getSelfContact();
      }

      //-----------------------------------------------------------------------
      bool Contact::isSelf() const
      {
        ContactPtr pThis = mThisWeak.lock();
        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_ERROR(Detail, log("account object is gone"))
          return false;
        }
        return (account->forContact().getSelfContact() == pThis);
      }

      //-----------------------------------------------------------------------
      String Contact::getPeerURI() const
      {
        return mPeer->getPeerURI();
      }

      //-----------------------------------------------------------------------
      String Contact::getFindSecret() const
      {
        IPeerFilePublicPtr peerFilePublic = mPeer->getPeerFilePublic();
        if (peerFilePublic) {
          return peerFilePublic->getFindSecret();
        }
        AutoRecursiveLock lock(getLock());
        return mFindSecret;
      }

      //-----------------------------------------------------------------------
      String Contact::getStableUniqueID() const
      {
        AutoRecursiveLock lock(getLock());
        if (mCalculatedUniqueID.hasData()) {
          return mCalculatedUniqueID;
        }
        if (mUserID.isEmpty()) return String();

        String peerURI = getPeerURI();

        String domain;
        String contactID;

        IPeer::splitURI(peerURI, domain, contactID);

        mCalculatedUniqueID = stack::IHelper::convertToHex(*stack::IHelper::hash(domain + ":" + mUserID));
        return mCalculatedUniqueID;
      }

      //-----------------------------------------------------------------------
      bool Contact::hasPeerFilePublic() const
      {
        return mPeer->getPeerFilePublic();
      }

      //-----------------------------------------------------------------------
      ElementPtr Contact::savePeerFilePublic() const
      {
        IPeerFilePublicPtr peerFilePublic = mPeer->getPeerFilePublic();
        if (!peerFilePublic) return ElementPtr();

        return peerFilePublic->saveToElement();
      }

      //-----------------------------------------------------------------------
      IAccountPtr Contact::getAssociatedAccount() const
      {
        return mAccount.lock();
      }

      //-----------------------------------------------------------------------
      void Contact::hintAboutLocation(const char *contactsLocationID)
      {
        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_ERROR(Detail, log("account object is gone"))
          return;
        }
        account->forContact().hintAboutContactLocation(mThisWeak.lock(), contactsLocationID);
      }


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Contact => IContactForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      ContactPtr Contact::createFromPeer(
                                         AccountPtr account,
                                         IPeerPtr peer,
                                         const char *userIDIfKnown
                                         )
      {
        stack::IAccountPtr stackAcount = account->forContact().getStackAccount();

        String userID(userIDIfKnown ? String(userIDIfKnown) : String());

        if (!stackAcount) {
          ZS_LOG_ERROR(Detail, "stack account is not ready")
          return ContactPtr();
        }

        AutoRecursiveLock lock(account->forContact().getLock());

        ContactPtr existingPeer = account->forContact().findContact(peer->getPeerURI());
        if (existingPeer) {
          if (userID.hasData()) {
            if (existingPeer->mUserID.isEmpty()) {
              existingPeer->mUserID = userID;
            }
          }
          return existingPeer;
        }

        ContactPtr pThis(new Contact);
        pThis->mThisWeak = pThis;
        pThis->mAccount = account;
        pThis->mPeer = peer;
        pThis->mUserID = userID;
        pThis->init();
        account->forContact().notifyAboutContact(pThis);
        return pThis;
      }

      //-----------------------------------------------------------------------
      IPeerPtr Contact::getPeer() const
      {
        return mPeer;
      }

      //-----------------------------------------------------------------------
      IPeerFilePublicPtr Contact::getPeerFilePublic() const
      {
        return mPeer->getPeerFilePublic();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Contact => IContactForContactPeerFilePublicLookup
      #pragma mark

      //-----------------------------------------------------------------------
      void Contact::setPeerFilePublic(IPeerFilePublicPtr peerFile)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!peerFile)

        AutoRecursiveLock lock(getLock());
        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("account is gone") + getDebugValueString())
          return;
        }

        stack::IAccountPtr stackAccount = account->forContact().getStackAccount();

        IPeerPtr peer = IPeer::create(stackAccount, peerFile);
        ZS_LOG_DEBUG(log("set peer file public complete") + IPeer::toDebugString(peer))
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Contact => IContactForConversationThread
      #pragma mark

      //-----------------------------------------------------------------------
      ContactPtr Contact::createFromPeerFilePublic(
                                                   AccountPtr account,
                                                   IPeerFilePublicPtr peerFilePublic,
                                                   const char *previousStableUniqueID
                                                   )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!peerFilePublic)
        stack::IAccountPtr stackAcount = account->forContact().getStackAccount();

        String stableID(previousStableUniqueID ? String(previousStableUniqueID) : String());

        if (!stackAcount) {
          ZS_LOG_ERROR(Detail, "stack account is not ready")
          return ContactPtr();
        }

        IPeerPtr peer = IPeer::create(stackAcount, peerFilePublic);
        if (!peer) {
          ZS_LOG_ERROR(Detail, "failed to create peer object")
          return ContactPtr();
        }

        AutoRecursiveLock lock(account->forContact().getLock());

        String peerURI = peer->getPeerURI();
        ContactPtr existingPeer = account->forContact().findContact(peerURI);
        if (existingPeer) {
          if (stableID.hasData()) {
            if (existingPeer->mCalculatedUniqueID.isEmpty()) {
              existingPeer->mCalculatedUniqueID = stableID;
            }
          }
          return existingPeer;
        }

        ContactPtr pThis(new Contact);
        pThis->mThisWeak = pThis;
        pThis->mAccount = account;
        pThis->mPeer = peer;
        pThis->mCalculatedUniqueID = stableID;
        pThis->init();
        account->forContact().notifyAboutContact(pThis);
        return pThis;
      }


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Contact => IContactForCall
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Contact => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String Contact::log(const char *message) const
      {
        return String("Contact [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String Contact::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("contact id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               IPeer::toDebugString(mPeer) + (isSelf() ? String(" (self)") : String()) +
               Helper::getDebugValue("user ID", mUserID, firstTime) +
               Helper::getDebugValue("calculated find secret", mCalculatedUniqueID, firstTime) +
               Helper::getDebugValue("find secret", mFindSecret, firstTime);
      }

      //-----------------------------------------------------------------------
      RecursiveLock &Contact::getLock() const
      {
        AccountPtr account = mAccount.lock();
        if (!account) return mBogusLock;
        return account->forContact().getLock();
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
    #pragma mark IContact
    #pragma mark

    //-------------------------------------------------------------------------
    String IContact::toDebugString(IContactPtr contact, bool includeCommaPrefix)
    {
      return internal::Contact::toDebugString(contact, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    IContactPtr IContact::createFromPeerFilePublic(
                                                   IAccountPtr account,
                                                   ElementPtr peerFilePublicEl,
                                                   const char *previousStableUniqueID
                                                   )
    {
      return internal::Contact::createFromPeerFilePublic(account, peerFilePublicEl, previousStableUniqueID);
    }

    //-------------------------------------------------------------------------
    IContactPtr IContact::createFromPeerURI(
                                            IAccountPtr account,
                                            const char *peerURI,
                                            const char *findSecret,
                                            const char *previousStableUniqueID
                                            )
    {
      return internal::IContactFactory::singleton().createFromPeerURI(account, peerURI, findSecret, previousStableUniqueID, NULL);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
  }
}
