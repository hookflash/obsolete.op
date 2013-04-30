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

#include <hookflash/core/internal/core_ContactPeerFilePublicLookup.h>
#include <hookflash/core/internal/core_Account.h>
#include <hookflash/core/internal/core_Helper.h>
#include <hookflash/core/internal/core_Stack.h>
#include <hookflash/core/internal/core_Contact.h>

#include <hookflash/stack/IServiceIdentity.h>
#include <hookflash/stack/IPeerFilePublic.h>

#include <zsLib/Stringize.h>
#include <zsLib/helpers.h>
#include <zsLib/Log.h>

#define HOOKFLASH_CORE_PUBLIC_PEER_FILES_GET_REQUEST_TIMEOUT_SECONDS (60)

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
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ContactPeerFilePublicLookup
      #pragma mark

      //-----------------------------------------------------------------------
      ContactPeerFilePublicLookup::ContactPeerFilePublicLookup(
                                                               IMessageQueuePtr queue,
                                                               IContactPeerFilePublicLookupDelegatePtr delegate,
                                                               const ContactList &contacts
                                                               ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(IContactPeerFilePublicLookupDelegateProxy::createWeak(IStackForInternal::queueApplication(), delegate)),
        mErrorCode(0),
        mResults(contacts)
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void ContactPeerFilePublicLookup::init()
      {
        for (ContactList::iterator iter = mResults.begin(); iter != mResults.end(); ++iter) {
          ContactPtr contact = Contact::convert(*iter);

          IPeerPtr peer = contact->forContactPeerFilePublicLookup().getPeer();

          if (peer->getPeerFilePublic()) {
            ZS_LOG_DEBUG(log("already have a public peer file thus no need to lookup again") + IPeer::toDebugString(peer))
            continue;
          }

          PeerInfo info;
          info.mPeerURI = peer->getPeerURI();
          info.mFindSecret = contact->forContactPeerFilePublicLookup().getFindSecret();

          if (info.mFindSecret.isEmpty()) {
            ZS_LOG_WARNING(Detail, log("missing find secret so server will likely reject request") + ", peer=" + info.mPeerURI)
          }

          if (mContacts.end() != mContacts.find(info.mPeerURI)) {
            ZS_LOG_WARNING(Detail, log("already performing lookup on this contact") + ", peer=" + info.mPeerURI)
            continue;
          }

          mContacts[info.mPeerURI] = contact;

          String domain;
          String contactID;
          if (!IPeer::splitURI(info.mPeerURI, domain, contactID)) {
            ZS_LOG_ERROR(Detail, log("failed to split domain from contact ID for peer URI") + ", peer=" + info.mPeerURI)
            continue;
          }

          // scope: prepare list of URIs to lookup per domain
          {
            DomainPeerInfoMap::iterator found = mDomainPeerInfos.find(domain);
            if (found == mDomainPeerInfos.end()) {
              PeerInfoList empty;
              mDomainPeerInfos[domain] = empty;
              found = mDomainPeerInfos.find(domain);
            }

            PeerInfoList &infoList = (*found).second;
            infoList.push_back(info);

            ZS_LOG_DEBUG(log("added peer info") + getDebugValueString())
          }

          // scope: prepare bootstrapper for the domain
          {
            BootstrappedNetworkMap::iterator found = mBootstrappedNetworks.find(domain);
            if (found == mBootstrappedNetworks.end()) {
              ZS_LOG_DEBUG(log("domain not found, adding new bootstrapper") + ", domain=" + domain)
              IBootstrappedNetworkPtr network = IBootstrappedNetwork::prepare(domain, mThisWeak.lock());
              if (!network) {
                ZS_LOG_WARNING(Detail, log("failed to create bootstrapper for domain") + ", domain=" + domain)
                return;
              }

              // bootstrapper was created for this domain
              mBootstrappedNetworks[domain] = network;
            }
          }
        }

        // we now have a list of domains and a list types/identifiers
        step();
      }

      //-----------------------------------------------------------------------
      ContactPeerFilePublicLookup::~ContactPeerFilePublicLookup()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      ContactPeerFilePublicLookupPtr ContactPeerFilePublicLookup::convert(IContactPeerFilePublicLookupPtr contact)
      {
        return boost::dynamic_pointer_cast<ContactPeerFilePublicLookup>(contact);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ContactPeerFilePublicLookup => IContactPeerFilePublicLookup
      #pragma mark

      //-----------------------------------------------------------------------
      String ContactPeerFilePublicLookup::toDebugString(IContactPeerFilePublicLookupPtr identity, bool includeCommaPrefix)
      {
        if (!identity) return includeCommaPrefix ? String(", identity lookup=(null)") : String("identity lookup=(null)");
        return ContactPeerFilePublicLookup::convert(identity)->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      ContactPeerFilePublicLookupPtr ContactPeerFilePublicLookup::create(
                                                                         IContactPeerFilePublicLookupDelegatePtr delegate,
                                                                         const ContactList &contacts
                                                                         )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)

        ContactPeerFilePublicLookupPtr pThis(new ContactPeerFilePublicLookup(IStackForInternal::queueCore(), delegate, contacts));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      bool ContactPeerFilePublicLookup::isComplete() const
      {
        AutoRecursiveLock lock(getLock());
        return !mDelegate;
      }

      //-----------------------------------------------------------------------
      bool ContactPeerFilePublicLookup::wasSuccessful(
                                                      WORD *outErrorCode,
                                                      String *outErrorReason
                                                      ) const
      {
        AutoRecursiveLock lock(getLock());
        return 0 == mErrorCode;
      }

      //-----------------------------------------------------------------------
      void ContactPeerFilePublicLookup::cancel()
      {
        AutoRecursiveLock lock(getLock());

        ContactPeerFilePublicLookupPtr pThis = mThisWeak.lock();

        if ((pThis) &&
            (mDelegate)) {
          try {
            ZS_LOG_DEBUG(log("notifying outer of completion") + getDebugValueString())
            mDelegate->onContactPeerFilePublicLookupCompleted(pThis);
          } catch(IContactPeerFilePublicLookupDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Detail, log("delegate gone"))
          }
        }

        mDelegate.reset();

        mBootstrappedNetworks.clear();
        mDomainPeerInfos.clear();
        mContacts.clear();

        // cancel all the monitors
        for (MonitorMap::iterator iter = mMonitors.begin(); iter != mMonitors.end(); ++iter)
        {
          IMessageMonitorPtr monitor = (*iter).second;
          monitor->cancel();
        }

        mMonitors.clear();
      }

      //-----------------------------------------------------------------------
      ContactListPtr ContactPeerFilePublicLookup::getContacts() const
      {
        AutoRecursiveLock lock(getLock());
        ContactListPtr result(new ContactList);
        (*result) = mResults;
        return result;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ContactPeerFilePublicLookup => IBootstrappedNetworkDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ContactPeerFilePublicLookup::onBootstrappedNetworkPreparationCompleted(IBootstrappedNetworkPtr network)
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("bootstrapped network prepared notification") + IBootstrappedNetwork::toDebugString(network))

        String networkDomain = network->getDomain();

        BootstrappedNetworkMap::iterator found = mBootstrappedNetworks.find(networkDomain);
        if (found == mBootstrappedNetworks.end()) {
          ZS_LOG_WARNING(Detail, log("notified about obsolete bootstrapped network") + IBootstrappedNetwork::toDebugString(network))
          return;
        }

        mBootstrappedNetworks.erase(found);
        
        WORD errorCode = 0;
        String errorResaon;
        bool success = network->wasSuccessful(&errorCode, &errorResaon);
        if (!success) {
          ZS_LOG_ERROR(Detail, log("bootstrapped network failed") + IBootstrappedNetwork::toDebugString(network))
          setError(errorCode, errorResaon);
          step();
          return;
        }


        DomainPeerInfoMap::iterator foundDomain = mDomainPeerInfos.find(networkDomain);
        if (foundDomain == mDomainPeerInfos.end()) {
          ZS_LOG_ERROR(Detail, log("failed to find domain for bootstrapper") + IBootstrappedNetwork::toDebugString(network))
          step();
          return;
        }

        PeerInfoList &infos = (*foundDomain).second;


        if (infos.size() > 0) {
          // let's issue a request to discover these identities
          PublicPeerFilesGetRequestPtr request = PublicPeerFilesGetRequest::create();
          request->domain(networkDomain);
          request->peers(infos);

          IMessageMonitorPtr monitor = IMessageMonitor::monitorAndSendToService(IMessageMonitorResultDelegate<PublicPeerFilesGetResult>::convert(mThisWeak.lock()), network, "peer-contact", "public-peer-files-get", request, Seconds(HOOKFLASH_CORE_PUBLIC_PEER_FILES_GET_REQUEST_TIMEOUT_SECONDS));

          if (!monitor) {
            ZS_LOG_ERROR(Detail, log("failed to create monitor for request"))
            setError(IHTTP::HTTPStatusCode_InternalServerError, "Failed to create monitor for public peer file get request");
            step();
            return;
          }

          ZS_LOG_DEBUG(log("monitoring public peer file get request") + IMessageMonitor::toDebugString(monitor))
          mMonitors[monitor->getID()] = monitor;
        }

        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ContactPeerFilePublicLookup => IMessageMonitorResultDelegate<ContactPeerFilePublicLookupResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool ContactPeerFilePublicLookup::handleMessageMonitorResultReceived(
                                                                           IMessageMonitorPtr monitor,
                                                                           PublicPeerFilesGetResultPtr result
                                                                           )
      {
        AutoRecursiveLock lock(getLock());

        MonitorMap::iterator found = mMonitors.find(monitor->getID());
        if (found == mMonitors.end()) {
          ZS_LOG_WARNING(Detail, log("notified about obsolete monitor") + IMessageMonitor::toDebugString(monitor))
          return false;
        }

        mMonitors.erase(found);

        typedef PublicPeerFilesGetResult::PeerFilePublicList PeerFilePublicList;

        const PeerFilePublicList &peerFiles = result->peerFiles();

        for (PeerFilePublicList::const_iterator iter = peerFiles.begin(); iter != peerFiles.end(); ++iter)
        {
          IPeerFilePublicPtr peerFile = (*iter);
          ZS_THROW_BAD_STATE_IF(!peerFile)

          String peerURI = peerFile->getPeerURI();

          ContactMap::iterator found = mContacts.find(peerURI);
          if (found == mContacts.end()) {
            ZS_LOG_WARNING(Detail, log("peer file returned but peer lookup was not requested") + IPeerFilePublic::toDebugString(peerFile))
            continue;
          }

          ContactPtr contact = (*found).second;
          ZS_THROW_BAD_STATE_IF(!contact)

          contact->forContactPeerFilePublicLookup().setPeerFilePublic(peerFile);
        }

        step();
        return true;
      }

      //-----------------------------------------------------------------------
      bool ContactPeerFilePublicLookup::handleMessageMonitorErrorResultReceived(
                                                                                IMessageMonitorPtr monitor,
                                                                                PublicPeerFilesGetResultPtr ignore, // will always be NULL
                                                                                MessageResultPtr result
                                                                                )
      {
        AutoRecursiveLock lock(getLock());

        MonitorMap::iterator found = mMonitors.find(monitor->getID());
        if (found == mMonitors.end()) {
          ZS_LOG_WARNING(Detail, log("notified about failure for obsolete monitor") + IMessageMonitor::toDebugString(monitor))
          return false;
        }

        mMonitors.erase(found);

        setError(result->errorCode(), result->errorReason());
        step();
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ContactPeerFilePublicLookup => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String ContactPeerFilePublicLookup::log(const char *message) const
      {
        return String("ContactPeerFilePublicLookup [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String ContactPeerFilePublicLookup::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("contact peer file public lookup", Stringize<typeof(mID)>(mID).string(), firstTime) +
              Helper::getDebugValue("delegate", mDelegate ? String("true") : String(), firstTime) +
              Helper::getDebugValue("error code", 0 != mErrorCode ? Stringize<typeof(mErrorCode)>(mErrorCode).string() : String(), firstTime) +
              Helper::getDebugValue("error reason", mErrorReason, firstTime) +
              Helper::getDebugValue("bootstrapped networks", mBootstrappedNetworks.size() > 0 ? Stringize<size_t>(mBootstrappedNetworks.size()).string() : String(), firstTime) +
              Helper::getDebugValue("monitors", mMonitors.size() > 0 ? Stringize<size_t>(mMonitors.size()).string() : String(), firstTime) +
        Helper::getDebugValue("domain peer infos", mDomainPeerInfos.size() > 0 ? Stringize<size_t>(mDomainPeerInfos.size()).string() : String(), firstTime) +
        Helper::getDebugValue("contacts (map)", mContacts.size() > 0 ? Stringize<size_t>(mContacts.size()).string() : String(), firstTime) +
        Helper::getDebugValue("contacts (result)", mResults.size() > 0 ? Stringize<size_t>(mResults.size()).string() : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      RecursiveLock &ContactPeerFilePublicLookup::getLock() const
      {
        return mLock;
      }

      //-----------------------------------------------------------------------
      void ContactPeerFilePublicLookup::step()
      {
        ZS_LOG_DEBUG(log("step"))

        if ((mMonitors.size() < 1) &&
            (mBootstrappedNetworks.size() < 1)) {
          ZS_LOG_DEBUG(log("contact peer file public lookup is finished"))
          cancel();
          return;
        }

        ZS_LOG_DEBUG(log("contact peer file public lookup is still pending"))
      }

      //-----------------------------------------------------------------------
      void ContactPeerFilePublicLookup::setError(WORD errorCode, const char *inErrorReason)
      {
        if (0 == errorCode) {
          ZS_LOG_DEBUG(log("no error set"))
          return;
        }

        String reason(inErrorReason ? String(inErrorReason) : String());
        if (reason.isEmpty()) {
          reason = IHTTP::toString(IHTTP::toStatusCode(errorCode));
        }

        if (0 != mErrorCode) {
          ZS_LOG_ERROR(Detail, log("error already set (thus ignoring new error)") + ", error code=" + Stringize<typeof(errorCode)>(errorCode).string() + ", reason=" + reason + getDebugValueString())
          return;
        }

        mErrorCode = errorCode;
        mErrorReason = reason;

        ZS_LOG_ERROR(Detail, log("error set") + getDebugValueString())
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
    #pragma mark IContactPeerFilePublicLookup
    #pragma mark

    //-------------------------------------------------------------------------
    String IContactPeerFilePublicLookup::toDebugString(IContactPeerFilePublicLookupPtr identity, bool includeCommaPrefix)
    {
      return internal::ContactPeerFilePublicLookup::toDebugString(identity);
    }

    //-------------------------------------------------------------------------
    IContactPeerFilePublicLookupPtr IContactPeerFilePublicLookup::create(
                                                                         IContactPeerFilePublicLookupDelegatePtr delegate,
                                                                         const ContactList &contacts
                                                                         )
    {
      return internal::IContactPeerFilePublicLookupFactory::singleton().create(delegate, contacts);
    }
    
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
  }
}
