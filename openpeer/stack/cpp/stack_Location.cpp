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

#include <openpeer/stack/internal/stack_Location.h>
#include <openpeer/stack/internal/stack_Peer.h>
#include <openpeer/stack/internal/stack_Account.h>
#include <openpeer/stack/internal/stack_Peer.h>
#include <openpeer/stack/internal/stack_Helper.h>
#include <zsLib/Log.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>

namespace openpeer { namespace stack { ZS_DECLARE_SUBSYSTEM(openpeer_stack) } }

namespace openpeer
{
  namespace stack
  {
    namespace internal
    {
      typedef zsLib::String String;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ILocationForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      LocationPtr ILocationForAccount::getForLocal(AccountPtr account)
      {
        return ILocationFactory::singleton().getForLocal(account);
      }

      //-----------------------------------------------------------------------
      LocationPtr ILocationForAccount::getForFinder(AccountPtr account)
      {
        return ILocationFactory::singleton().getForFinder(account);
      }

      //-----------------------------------------------------------------------
      LocationPtr ILocationForAccount::getForPeer(
                                                  PeerPtr peer,
                                                  const char *locationID
                                                  )
      {
        return ILocationFactory::singleton().getForPeer(peer, locationID);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ILocationForMessages
      #pragma mark

      //-----------------------------------------------------------------------
      LocationPtr ILocationForMessages::convert(IMessageSourcePtr messageSource)
      {
        return Location::convert(messageSource);
      }

      //-----------------------------------------------------------------------
      LocationPtr ILocationForMessages::getForLocal(AccountPtr account)
      {
        return ILocationFactory::singleton().getForLocal(account);
      }

      //-----------------------------------------------------------------------
      LocationPtr ILocationForMessages::create(
                                               IMessageSourcePtr messageSource,
                                               const char *peerURI,
                                               const char *locationID
                                               )
      {
        return ILocationFactory::singleton().create(messageSource, peerURI, locationID);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ILocationForPublication
      #pragma mark

      //-----------------------------------------------------------------------
      int ILocationForPublication::locationCompare(
                                                   const ILocationPtr &left,
                                                   const ILocationPtr &right,
                                                   const char * &outReason
                                                   )
      {
        if (left) {
          if (right) {
            if (left->getLocationType() < right->getLocationType()) {
              outReason = "location type L < location type R";
              return -1;
            }
            if (left->getLocationType() > right->getLocationType()) {
              outReason = "location type L > location type R";
              return 1;
            }

            switch (left->getLocationType())
            {
              case ILocation::LocationType_Finder:  break;  // match
              case ILocation::LocationType_Local:
              case ILocation::LocationType_Peer:
              {
                IPeerPtr leftPeer = left->getPeer();
                IPeerPtr rightPeer = right->getPeer();
                if (leftPeer) {
                  if (rightPeer) {
                    // both set, now compare
                    String leftPeerURI = leftPeer->getPeerURI();
                    String rightPeerURI = rightPeer->getPeerURI();
                    if (leftPeerURI < rightPeerURI) {
                      outReason = "location L peer URI < R peer URI";
                      return -1;
                    }
                    if (leftPeerURI > rightPeerURI) {
                      outReason = "location L peer URI > R peer URI";
                      return 1;
                    }
                    String leftLocationID = left->getLocationID();
                    String rightLocationID = right->getLocationID();
                    if (leftLocationID < rightLocationID) {
                      outReason = "L location ID < R location ID";
                      return -1;
                    }
                    if (leftLocationID > rightLocationID) {
                      outReason = "L location ID > R location ID";
                      return 1;
                    }
                    break;
                  }
                  // left > right
                  outReason = "location peer L set but R not set";
                  return 1;
                }
                if (rightPeer) {
                  // left < right
                  outReason = "location peer R set but L not set";
                  return -1;
                }
                break;
              }
            }
            return 0;
          }

          // left > right
          outReason = "location L set but R not set";
          return 1;
        }
        if (right) {
          // left < right
          outReason = "location R set but L not set";
          return -1;
        }
        return 0;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ILocationForPublicationRepository
      #pragma mark

      //-----------------------------------------------------------------------
      int ILocationForPublicationRepository::locationCompare(
                                                             const ILocationPtr &left,
                                                             const ILocationPtr &right,
                                                             const char * &outReason
                                                             )
      {
        return ILocationForPublication::locationCompare(left, right, outReason);
      }

      //-----------------------------------------------------------------------
      LocationPtr ILocationForPublicationRepository::getForLocal(AccountPtr account)
      {
        return ILocationFactory::singleton().getForLocal(account);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Location
      #pragma mark

      //-----------------------------------------------------------------------
      Location::Location(
                         AccountPtr account,
                         LocationTypes type,
                         PeerPtr peer,
                         const char *locationID
                         ) :
        mID(zsLib::createPUID()),
        mAccount(account),
        mType(type),
        mPeer(peer),
        mLocationID(locationID ? String(locationID) : String())
      {
        ZS_LOG_DEBUG(log("created") + getDebugValueString())
      }

      //-----------------------------------------------------------------------
      void Location::init()
      {
      }

      //-----------------------------------------------------------------------
      Location::~Location()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))

        AccountPtr account = mAccount.lock();

        if (account) {
          account->forLocation().notifyDestroyed(*this);
        }
      }

      //-----------------------------------------------------------------------
      LocationPtr Location::convert(ILocationPtr location)
      {
        return boost::dynamic_pointer_cast<Location>(location);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Location => ILocation
      #pragma mark

      //-----------------------------------------------------------------------
      String Location::toDebugString(ILocationPtr location, bool includeCommaPrefix)
      {
        if (!location) return String(includeCommaPrefix ? ", location=(null)" : "location=(null)");
        return Location::convert(location)->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      LocationPtr Location::getForLocal(IAccountPtr inAccount)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!inAccount)
        AccountPtr account = Account::convert(inAccount);

        String locationID = account->forLocation().getLocationID();
        PeerPtr peerLocal = account->forLocation().getPeerForLocal();

        LocationPtr existing = account->forLocation().getLocationForLocal();
        if (existing) {
          ZS_LOG_DEBUG(existing->log("using existing local location"))
          return existing;
        }

        LocationPtr pThis(new Location(account, LocationType_Local, peerLocal, locationID));
        pThis->mThisWeak = pThis;
        pThis->init();

        return pThis;
      }

      //-----------------------------------------------------------------------
      LocationPtr Location::getForFinder(IAccountPtr inAccount)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!inAccount)
        AccountPtr account = Account::convert(inAccount);

        LocationPtr existing = account->forLocation().getLocationForFinder();
        if (existing) {
          ZS_LOG_DEBUG(existing->log("using existing finder location"))
          return existing;
        }

        LocationPtr pThis(new Location(account, LocationType_Finder, PeerPtr(), NULL));
        pThis->mThisWeak = pThis;
        pThis->init();

        return pThis;
      }

      //-----------------------------------------------------------------------
      LocationPtr Location::Location::getForPeer(
                                                 IPeerPtr inPeer,
                                                 const char *locationID
                                                 )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!inPeer)
        ZS_THROW_INVALID_ARGUMENT_IF(!locationID)

        PeerPtr peer = Peer::convert(inPeer);
        AccountPtr account = peer->forLocation().getAccount();

        LocationPtr pThis(new Location(account, LocationType_Peer, peer, locationID));
        pThis->mThisWeak = pThis;
        pThis->init();

        LocationPtr useThis = account->forLocation().findExistingOrUse(pThis);
        if (useThis != pThis) {
          // do not inform the account of destruction since it was not used
          ZS_LOG_DEBUG(pThis->log("discarding object since one exists already"))
          pThis->mAccount.reset();
        }
        return useThis;
      }

      //-----------------------------------------------------------------------
      LocationPtr Location::convert(IMessageSourcePtr messageSource)
      {
        return boost::dynamic_pointer_cast<Location>(messageSource);
      }

      //-----------------------------------------------------------------------
      ILocation::LocationTypes Location::getLocationType() const
      {
        return mType;
      }

      //-----------------------------------------------------------------------
      String Location::getPeerURI() const
      {
        if (!mPeer) return String();

        return mPeer->forLocation().getPeerURI();
      }

      //-----------------------------------------------------------------------
      String Location::getLocationID() const
      {
        return mLocationID;
      }

      //-----------------------------------------------------------------------
      LocationInfoPtr Location::getLocationInfo() const
      {
        AccountPtr account = mAccount.lock();
        if (!account) return LocationInfoPtr();
        return account->forLocation().getLocationInfo(mThisWeak.lock());
      }

      //-----------------------------------------------------------------------
      IPeerPtr Location::getPeer() const
      {
        return mPeer;
      }

      //-----------------------------------------------------------------------
      bool Location::isConnected() const
      {
        return LocationConnectionState_Connected == getConnectionState();
      }

      //-----------------------------------------------------------------------
      ILocation::LocationConnectionStates Location::getConnectionState() const
      {
        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("location is disconnected as account is gone") + getDebugValueString())
          return LocationConnectionState_Disconnected;
        }
        return account->forLocation().getConnectionState(mThisWeak.lock());
      }

      //-----------------------------------------------------------------------
      bool Location::sendMessage(message::MessagePtr message) const
      {
        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("send message failed as account is gone") + getDebugValueString())
          return false;
        }
        return account->forLocation().send(mThisWeak.lock(), message);
      }

      //-----------------------------------------------------------------------
      void Location::hintNowAvailable()
      {
        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("send message failed as account is gone") + getDebugValueString())
          return;
        }
        return account->forLocation().hintNowAvailable(mThisWeak.lock());
      }

      //---------------------------------------------------------------------
      #pragma mark
      #pragma mark Location => ILocationForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Location => ILocationForMessages
      #pragma mark

      //-----------------------------------------------------------------------
      LocationPtr Location::create(
                                   IMessageSourcePtr messageSource,
                                   const char *inPeerURI,
                                   const char *inLocationID
                                   )
      {
        LocationPtr sourceLocation = convert(messageSource);
        if (!sourceLocation) return LocationPtr();

        AccountPtr account = sourceLocation->mAccount.lock();
        if (!account) return LocationPtr();

        String peerURI(inPeerURI ? inPeerURI : "");
        String locationID(inLocationID ? inLocationID : "");

        peerURI.toLower();

        if ((peerURI.isEmpty()) &&
            (locationID.isEmpty())) {
          return getForFinder(account);
        }

        if ((peerURI.isEmpty()) ||
            (locationID.isEmpty())) {
          ZS_LOG_DEBUG(String("Location [] cannot create location as missing peer URI or location ID, peer URI=") + peerURI + ", location ID=" + locationID)
          return LocationPtr();
        }

        LocationPtr selfLocation = getForLocal(account);
        PeerPtr selfPeer = Peer::convert(selfLocation->getPeer());
        String selfPeerURI = selfPeer->forLocation().getPeerURI();

        if ((selfLocation->getLocationID() == locationID) &&
            (selfPeerURI == peerURI)) {
          return selfLocation;
        }

        PeerPtr peer = IPeerForLocation::create(account, peerURI);
        if (!peer) {
          ZS_LOG_DEBUG("Location [] cannot create location as peer failed to create")
          return LocationPtr();
        }

        return getForPeer(peer, locationID);
      }

      //-----------------------------------------------------------------------
      PeerPtr Location::getPeer(bool) const
      {
        return mPeer;
      }

      //-----------------------------------------------------------------------
      AccountPtr Location::getAccount() const
      {
        return mAccount.lock();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Location => ILocationForPeerSubscription
      #pragma mark

      //-----------------------------------------------------------------------
      String Location::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("location id", string(mID), firstTime) +
               Helper::getDebugValue("type", toString(mType), firstTime) +
               IPeer::toDebugString(mPeer) +
               Helper::getDebugValue("location id(s)", mLocationID, firstTime);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Location => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String Location::log(const char *message) const
      {
        return String("Location [" + string(mID) + "] " + message);
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ILocation
    #pragma mark

    //-------------------------------------------------------------------------
    const char *ILocation::toString(LocationTypes type)
    {
      switch (type)
      {
        case LocationType_Local:  return "Local";
        case LocationType_Finder: return "Finder";
        case LocationType_Peer:   return "Peer";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *ILocation::toString(LocationConnectionStates state)
    {
      switch (state)
      {
        case LocationConnectionState_Pending:       return "Pending";
        case LocationConnectionState_Connected:     return "Connected";
        case LocationConnectionState_Disconnecting: return "Disconnecting";
        case LocationConnectionState_Disconnected:  return "Disconnected";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    String ILocation::toDebugString(ILocationPtr location, bool includeCommaPrefix)
    {
      return internal::Location::toDebugString(location, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    ILocationPtr ILocation::getForLocal(IAccountPtr account)
    {
      return internal::ILocationFactory::singleton().getForLocal(account);
    }

    //-------------------------------------------------------------------------
    ILocationPtr ILocation::getForFinder(IAccountPtr account)
    {
      return internal::ILocationFactory::singleton().getForFinder(account);
    }

    //-------------------------------------------------------------------------
    ILocationPtr ILocation::getForPeer(
                                       IPeerPtr peer,
                                       const char *locationID
                                       )
    {
      return internal::ILocationFactory::singleton().getForPeer(peer, locationID);
    }

    //-------------------------------------------------------------------------
    ILocationPtr ILocation::convert(IMessageSourcePtr messageSource)
    {
      return internal::Location::convert(messageSource);
    }
  }
}
