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

#include <hookflash/stack/internal/stack_Peer.h>
#include <hookflash/stack/internal/stack_Account.h>
#include <hookflash/stack/internal/stack_Helper.h>

#include <hookflash/stack/IPeerFilePublic.h>

#include <zsLib/Log.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>
#include <zsLib/XML.h>

#include <zsLib/RegEx.h>

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPeerForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      PeerPtr IPeerForAccount::create(
                                       AccountPtr account,
                                       IPeerFilePublicPtr peerFilePublic
                                       )
      {
        return IPeerFactory::singleton().create(account, peerFilePublic);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPeerForLocation
      #pragma mark

      //-----------------------------------------------------------------------
      PeerPtr IPeerForLocation::create(
                                       AccountPtr account,
                                       const char *peerURI
                                       )
      {
        return IPeerFactory::singleton().create(account, peerURI);
      }

      //-----------------------------------------------------------------------
      bool IPeerForLocation::isValid(const char *peerURI)
      {
        return Peer::isValid(peerURI);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPeerForMessages
      #pragma mark

      //-----------------------------------------------------------------------
      PeerPtr IPeerForMessages::create(
                                       AccountPtr account,
                                       IPeerFilePublicPtr peerFilePublic
                                       )
      {
        return IPeerFactory::singleton().create(account, peerFilePublic);
      }

      //-----------------------------------------------------------------------
      PeerPtr IPeerForMessages::getFromSignature(
                                                 AccountPtr account,
                                                 ElementPtr signedElement
                                                 )
      {
        return IPeerFactory::singleton().getFromSignature(account, signedElement);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Peer
      #pragma mark

      //-----------------------------------------------------------------------
      Peer::Peer(
                 AccountPtr account,
                 IPeerFilePublicPtr peerFilePublic,
                 const String &peerURI
                 ) :
        mID(zsLib::createPUID()),
        mAccount(account),
        mPeerFilePublic(peerFilePublic),
        mPeerURI(peerURI)
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void Peer::init()
      {
      }

      //-----------------------------------------------------------------------
      Peer::~Peer()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))

        AccountPtr account = mAccount.lock();
        if (account) {
          account->forPeer().notifyDestroyed(*this);
        }
      }

      //-----------------------------------------------------------------------
      PeerPtr Peer::convert(IPeerPtr peer)
      {
        return boost::dynamic_pointer_cast<Peer>(peer);
      }


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Peer => IPeer
      #pragma mark

      //-----------------------------------------------------------------------
      bool Peer::isValid(const char *peerURI)
      {
        if (!peerURI) {
          ZS_LOG_WARNING(Detail, "Peer [] peer URI is not valid as it is NULL, uri=(null)")
          return false;
        }

        zsLib::RegEx e("^peer:\\/\\/([a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?\\.)+[a-zA-Z]{2,6}\\/([a-f0-9][a-f0-9])+$");
        if (!e.hasMatch(peerURI)) {
          ZS_LOG_WARNING(Detail, String("Peer [] peer URI is not valid, uri=") + peerURI);
          return false;
        }
        return true;
      }

      //-----------------------------------------------------------------------
      bool Peer::splitURI(
                          const char *inPeerURI,
                          String &outDomain,
                          String &outContactID
                          )
      {
        String peerURI(inPeerURI ? String(inPeerURI) : String());

        peerURI.trim();
        peerURI.toLower();

        if (!isValid(peerURI)) {
          ZS_LOG_WARNING(Detail, "Peer [] peer URI was not valid to be able to split, peer URI=" + peerURI)
          return false;
        }

        size_t startPos = strlen("peer://");
        size_t slashPos = peerURI.find('/', startPos);

        ZS_THROW_BAD_STATE_IF(slashPos == String::npos)

        outDomain = peerURI.substr(startPos, slashPos - startPos);
        outContactID = peerURI.substr(slashPos + 1);
        return true;
      }

      //-----------------------------------------------------------------------
      String Peer::joinURI(
                           const char *inDomain,
                           const char *inContactID
                           )
      {
        String domain(inDomain ? inDomain : "");
        String contactID(inContactID ? inContactID : "");

        domain.trim();
        contactID.trim();

        domain.toLower();
        contactID.toLower();

        String result = "peer://" + domain + "/" + contactID;
        if (!IPeer::isValid(result)) {
          ZS_LOG_WARNING(Detail, "Peer [] invalid peer URI createtd after join, peer URI=" + result)
          return String();
        }
        return result;
      }

      //-----------------------------------------------------------------------
      String Peer::toDebugString(IPeerPtr peer, bool includeCommaPrefix)
      {
        if (!peer) return String(includeCommaPrefix ? ", peer=(null)" : "peer=(null)");
        return Peer::convert(peer)->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      PeerPtr Peer::create(
                           IAccountPtr account,
                           IPeerFilePublicPtr peerFilePublic
                           )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!account)
        ZS_THROW_INVALID_ARGUMENT_IF(!peerFilePublic)

        PeerPtr pThis(new Peer(Account::convert(account), peerFilePublic, String()));
        pThis->mThisWeak = pThis;
        pThis->init();

        AutoRecursiveLock lock(pThis->getLock());

        // check if it already exists in the account
        PeerPtr useThis = pThis->mAccount.lock()->forPeer().findExistingOrUse(pThis);

        if (!(useThis->mPeerFilePublic)) {
          useThis->mPeerFilePublic = peerFilePublic;
        }

        if (pThis != useThis) {
          // do not inform account of destruction since it was not used
          ZS_LOG_DEBUG(pThis->log("discarding object since one exists already"))
          pThis->mAccount.reset();
        }

        return useThis;
      }

      //-----------------------------------------------------------------------
      PeerPtr Peer::getFromSignature(
                                     IAccountPtr account,
                                     ElementPtr inSignedElement
                                     )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!account)
        ZS_THROW_INVALID_ARGUMENT_IF(!inSignedElement)

        ElementPtr signature;
        ElementPtr signedElement = IHelper::getSignatureInfo(inSignedElement, &signature);

        if (!signedElement) {
          ZS_LOG_WARNING(Detail, "Peer [] could not find signature information")
          return PeerPtr();
        }

        ElementPtr keyEl = signature->findFirstChildElement("key");
        if (!keyEl) {
          ZS_LOG_WARNING(Detail, "Peer [] could not find key in signature")
          return PeerPtr();
        }

        ElementPtr uriEl = keyEl->findFirstChildElement("uri");
        if (!uriEl) {
          ZS_LOG_WARNING(Detail, "Peer [] could not find key in signature")
          return PeerPtr();
        }

        String peerURI = uriEl->getTextDecoded();
        if (!isValid(peerURI)) {
          ZS_LOG_WARNING(Detail, "Peer [] URI in key is not valid peer URI, uri=" + peerURI)
          return PeerPtr();
        }

        PeerPtr peer = create(Account::convert(account), peerURI);
        if (!peer) {
          ZS_LOG_ERROR(Detail, "Peer [] could not create peer for given peer, uri=" + peerURI)
          return PeerPtr();
        }
        return peer;
      }

      //-----------------------------------------------------------------------
      bool Peer::verifySignature(ElementPtr inSignedElement) const
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!inSignedElement)

        ElementPtr signature;
        ElementPtr signedElement = IHelper::getSignatureInfo(inSignedElement, &signature);

        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("cannot validate signature aas account object is gone"))
          return false;
        }

        PeerPtr peer = getFromSignature(mAccount.lock(), inSignedElement);
        if (!peer) {
          ZS_LOG_ERROR(Detail, log("could not create peer for given signature"))
          return false;
        }

        String peerURI = peer->getPeerURI();
        if (peerURI != mPeerURI) {
          ZS_LOG_WARNING(Detail, log("peer URI in signature does not match peer") + ", signature: " + IPeer::toDebugString(peer, false) + ", self: " + getDebugValueString(false))
          return false;
        }

        IPeerFilePublicPtr peerFilePublic = peer->getPeerFilePublic();
        if (!peerFilePublic) {
          ZS_LOG_WARNING(Detail, log("public peer file is not known yet for peer") + IPeer::toDebugString(peer))
          return false;
        }

        if (!peerFilePublic->verifySignature(signedElement)) {
          ZS_LOG_WARNING(Detail, peer->log("signature failed to validate") + IPeer::toDebugString(peer))
          return false;
        }

        ZS_LOG_DEBUG(log("signature passed for peer") + IPeer::toDebugString(peer))
        return true;
      }

      //-----------------------------------------------------------------------
      String Peer::getPeerURI() const
      {
        AutoRecursiveLock lock(getLock());
        if (mPeerFilePublic) {
          return mPeerFilePublic->getPeerURI();
        }
        return mPeerURI;
      }

      //-----------------------------------------------------------------------
      IPeerFilePublicPtr Peer::getPeerFilePublic() const
      {
        AutoRecursiveLock lock(getLock());
        return mPeerFilePublic;
      }

      //-----------------------------------------------------------------------
      IPeer::PeerFindStates Peer::getFindState() const
      {
        AutoRecursiveLock lock(getLock());
        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("get fine state account gone") + getDebugValueString())
          return PeerFindState_Completed;
        }
        return account->forPeer().getPeerState(getPeerURI());
      }

      //-----------------------------------------------------------------------
      LocationListPtr Peer::getLocationsForPeer(bool includeOnlyConnectedLocations) const
      {
        AutoRecursiveLock lock(getLock());
        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("locations are not available as account is gone") + getDebugValueString())
          LocationListPtr locations(new LocationList);
          return locations;
        }
        return account->forPeer().getPeerLocations(getPeerURI(), includeOnlyConnectedLocations);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Peer => IPeerForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      void Peer::setPeerFilePublic(IPeerFilePublicPtr peerFilePublic)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!peerFilePublic)

        AutoRecursiveLock lock(getLock());
        mPeerFilePublic = peerFilePublic;
        mPeerURI.clear();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Peer => IPeerForLocation
      #pragma mark

      //-----------------------------------------------------------------------
      PeerPtr Peer::create(
                           AccountPtr account,
                           const char *peerURI
                           )
      {
        if (!account) return PeerPtr();
        if (!peerURI) return PeerPtr();

        if (!isValid(peerURI)) {
          ZS_LOG_DEBUG(String("Peer [] cannot create peer as URI is not valid, peer URI=") + peerURI)
          return PeerPtr();
        }

        PeerPtr pThis(new Peer(Account::convert(account), IPeerFilePublicPtr(), peerURI));
        pThis->mThisWeak = pThis;
        pThis->init();

        // check if it already exists in the account
        PeerPtr useThis = pThis->mAccount.lock()->forPeer().findExistingOrUse(pThis);
        if (useThis != pThis) {
          // do not inform account of destruction since it is not used
          ZS_LOG_DEBUG(pThis->log("discarding object since one exists already"))
          pThis->mAccount.reset();
        }
        return useThis;
      }

      //-----------------------------------------------------------------------
      AccountPtr Peer::getAccount() const
      {
        return mAccount.lock();
      }

      //-----------------------------------------------------------------------
      String Peer::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("peer id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("peer uri", getPeerURI(), firstTime) +
               Helper::getDebugValue("peer file public", mPeerFilePublic ? String("true") : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Peer => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &Peer::getLock() const
      {
        AccountPtr account = mAccount.lock();
        if (!account) return mBogusLock;
        return account->forPeer().getLock();
      }

      //-----------------------------------------------------------------------
      String Peer::log(const char *message) const
      {
        return String("Peer [" + Stringize<typeof(mID)>(mID).string() + "] " + message);
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IPeer
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IPeer::toString(PeerFindStates state)
    {
      switch (state)
      {
        case PeerFindState_Idle:      return "Idle";
        case PeerFindState_Finding:   return "Finding";
        case PeerFindState_Completed: return "Complete";
      }
    }

    //-------------------------------------------------------------------------
    bool IPeer::isValid(const char *peerURI)
    {
      return internal::Peer::isValid(peerURI);
    }

    //-------------------------------------------------------------------------
    bool IPeer::splitURI(
                         const char *peerURI,
                         String &outDomain,
                         String &outContactID
                         )
    {
      return internal::Peer::splitURI(peerURI, outDomain, outContactID);
    }

    //-------------------------------------------------------------------------
    String IPeer::joinURI(
                          const char *domain,
                          const char *contactID
                          )
    {
      return internal::Peer::joinURI(domain, contactID);
    }

    //-------------------------------------------------------------------------
    String IPeer::toDebugString(IPeerPtr peer, bool includeCommaPrefix)
    {
      return internal::Peer::toDebugString(peer, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    IPeerPtr IPeer::create(
                           IAccountPtr account,
                           IPeerFilePublicPtr peerFilePublic
                           )
    {
      return internal::IPeerFactory::singleton().create(account, peerFilePublic);
    }

    //-------------------------------------------------------------------------
    IPeerPtr IPeer::create(
                           IAccountPtr account,
                           const char *peerURI
                           )
    {
      return internal::IPeerFactory::singleton().create(internal::Account::convert(account), peerURI);
    }

    //-------------------------------------------------------------------------
    IPeerPtr IPeer::getFromSignature(
                                     IAccountPtr account,
                                     ElementPtr signedElement
                                     )
    {
      return internal::IPeerFactory::singleton().getFromSignature(account, signedElement);
    }
  }
}
