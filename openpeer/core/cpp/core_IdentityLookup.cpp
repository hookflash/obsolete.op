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

#include <openpeer/core/internal/core_IdentityLookup.h>
#include <openpeer/core/internal/core_Account.h>
#include <openpeer/core/internal/core_Helper.h>
#include <openpeer/core/internal/core_Stack.h>
#include <openpeer/core/internal/core_Contact.h>

#include <openpeer/stack/IServiceIdentity.h>
#include <openpeer/stack/IPeerFilePublic.h>

#include <openpeer/stack/message/IMessageHelper.h>
#include <openpeer/stack/message/identity-lookup/IdentityLookupCheckRequest.h>
#include <openpeer/stack/message/identity-lookup/IdentityLookupRequest.h>

#include <zsLib/Stringize.h>
#include <zsLib/helpers.h>
#include <zsLib/Log.h>

#define OPENPEER_CORE_IDENTITY_LOOK_REQUEST_TIMEOUT_SECONDS (60)

namespace openpeer { namespace core { ZS_DECLARE_SUBSYSTEM(openpeer_core) } }


namespace openpeer
{
  namespace core
  {
    namespace internal
    {
      using zsLib::Stringize;

      typedef stack::message::IdentityInfoList StackIdentityInfoList;
      typedef stack::message::IdentityInfo StackIdentityInfo;
      using stack::message::IMessageHelper;
      using stack::message::identity_lookup::IdentityLookupCheckRequest;
      using stack::message::identity_lookup::IdentityLookupCheckRequestPtr;
      using stack::message::identity_lookup::IdentityLookupRequest;
      using stack::message::identity_lookup::IdentityLookupRequestPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      static char getSafeSplitChar(const String &identifier)
      {
        const char *testChars = ",; :./\\*#!$%&@?~+=-_|^<>[]{}()";

        while (*testChars) {
          if (String::npos == identifier.find(*testChars)) {
            return *testChars;
          }

          ++testChars;
        }

        return 0;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IdentityLookup
      #pragma mark

      //-----------------------------------------------------------------------
      IdentityLookup::IdentityLookup(
                                     IMessageQueuePtr queue,
                                     AccountPtr account,
                                     IIdentityLookupDelegatePtr delegate,
                                     const char *identityServiceDomain
                                     ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mAccount(account),
        mDelegate(IIdentityLookupDelegateProxy::createWeak(IStackForInternal::queueApplication(), delegate)),
        mErrorCode(0),
        mIdentityServiceDomain(identityServiceDomain),
        mAlreadyIssuedForProviderDomain(false)
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void IdentityLookup::init(const IdentityLookupInfoList &identities)
      {
        AutoRecursiveLock lock(getLock());

        for (IdentityLookupInfoList::const_iterator iter = identities.begin(); iter != identities.end(); ++iter) {
          const String &identityURI = (*iter).mIdentityURI;
          const Time &lastUpdated = (*iter).mLastUpdated;

          if (!IServiceIdentity::isValid(identityURI)) {
            ZS_LOG_WARNING(Detail, log("identity not valid") + ", identity=" + identityURI)
            continue;
          }

          String domainOrType;
          String identifier;
          if (!IServiceIdentity::splitURI(identityURI, domainOrType, identifier)) {
            ZS_LOG_WARNING(Detail, log("failed to parse identity") + ", identity=" + identityURI)
            continue;
          }

          if (identifier.isEmpty()) {
            ZS_LOG_WARNING(Detail, log("failed to obtain identifier for identity") + ", identity=" + identityURI)
            continue;
          }

          if (IServiceIdentity::isLegacy(identityURI)) {
            prepareIdentity(mIdentityServiceDomain, domainOrType, identifier, lastUpdated);
          } else {
            prepareIdentity(domainOrType, domainOrType, identifier, lastUpdated);
          }
        }

        // all identities should be prepared so not figure out which character can safely be used to split the string into parts
        for (IdentifierSafeCharDomainLegacyTypeMap::iterator iter = mConcatDomains.begin(); iter != mConcatDomains.end(); )
        {
          IdentifierSafeCharDomainLegacyTypeMap::iterator current = iter;
          ++iter;

          const String &type = (*current).first;
          String &concat = (*current).second;

          char safeChar = getSafeSplitChar(concat);
          if (0 == safeChar) {
            ZS_LOG_WARNING(Detail, log("failed to obain a safe char to split for domain or legacy type") + ", domain or type=" + type)
            mConcatDomains.erase(current);
            mDomainOrLegacyTypeIdentifiers.erase(mDomainOrLegacyTypeIdentifiers.find(type));
            continue;
          }

          concat.clear();

          bool firstTime = true;

          IdentifierMap &identifiers = (*(mDomainOrLegacyTypeIdentifiers.find(type))).second;
          for (IdentifierMap::iterator innerIter = identifiers.begin(); innerIter != identifiers.end(); ++innerIter)
          {
            const String &identifier = (*innerIter).first;
            if (firstTime) {
              concat = identifier;
              firstTime = false;
              continue;
            }

            concat += safeChar + identifier;
          }

          ZS_LOG_DEBUG(log("adding safe char for type") + ", type=" + type + ", safe char=" + (String() + safeChar))

          mSafeCharDomains[type] = String() + safeChar;
        }

        // we now have a list of domains and a list types/identifiers
        step();
      }

      //-----------------------------------------------------------------------
      IdentityLookup::~IdentityLookup()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      IdentityLookupPtr IdentityLookup::convert(IIdentityLookupPtr contact)
      {
        return boost::dynamic_pointer_cast<IdentityLookup>(contact);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IdentityLookup => IIdentityLookup
      #pragma mark

      //-----------------------------------------------------------------------
      String IdentityLookup::toDebugString(IIdentityLookupPtr identity, bool includeCommaPrefix)
      {
        if (!identity) return includeCommaPrefix ? String(", identity lookup=(null)") : String("identity lookup=(null)");
        return IdentityLookup::convert(identity)->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      IdentityLookupPtr IdentityLookup::create(
                                               IAccountPtr account,
                                               IIdentityLookupDelegatePtr delegate,
                                               const IdentityLookupInfoList &identities,
                                               const char *identityServiceDomain
                                               )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!account)
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(!identityServiceDomain)

        ZS_THROW_INVALID_ARGUMENT_IF(!stack::IHelper::isValidDomain(identityServiceDomain))

        IdentityLookupPtr pThis(new IdentityLookup(IStackForInternal::queueCore(), Account::convert(account), delegate, identityServiceDomain));
        pThis->mThisWeak = pThis;
        pThis->init(identities);
        return pThis;
      }

      //-----------------------------------------------------------------------
      bool IdentityLookup::isComplete() const
      {
        AutoRecursiveLock lock(getLock());
        return !mDelegate;
      }

      //-----------------------------------------------------------------------
      bool IdentityLookup::wasSuccessful(
                                         WORD *outErrorCode,
                                         String *outErrorReason
                                         ) const
      {
        AutoRecursiveLock lock(getLock());
        return 0 == mErrorCode;
      }

      //-----------------------------------------------------------------------
      void IdentityLookup::cancel()
      {
        AutoRecursiveLock lock(getLock());

        IdentityLookupPtr pThis = mThisWeak.lock();

        if ((pThis) &&
            (mDelegate)) {
          try {
            ZS_LOG_DEBUG(log("notifying outer of completion") + getDebugValueString())
            mDelegate->onIdentityLookupCompleted(pThis);
          } catch(IIdentityLookupDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Detail, log("delegate gone"))
          }
        }

        mDelegate.reset();

        mBootstrappedNetworks.clear();
        mDomainOrLegacyTypeIdentifiers.clear();
        mConcatDomains.clear();
        mTypeToDomainMap.clear();

        // cancel all the monitors
        for (MonitorMap::iterator iter = mMonitors.begin(); iter != mMonitors.end(); ++iter)
        {
          IMessageMonitorPtr monitor = (*iter).second;
          monitor->cancel();
        }

        mMonitors.clear();
      }

      //-----------------------------------------------------------------------
      IdentityInfoListPtr IdentityLookup::getIdentities() const
      {
        AutoRecursiveLock lock(getLock());
        IdentityInfoListPtr result(new IdentityInfoList);

        for (IdentityInfoList::const_iterator iter = mResults.begin(); iter != mResults.end(); ++iter)
        {
          const IdentityInfo &info = (*iter);
          ZS_LOG_TRACE(log("found result") + ", identity=" + info.mIdentityURI)
          result->push_back(info);
        }

        return result;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IdentityLookup => IBootstrappedNetworkDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void IdentityLookup::onBootstrappedNetworkPreparationCompleted(IBootstrappedNetworkPtr network)
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("bootstrapped network prepared notification") + IBootstrappedNetwork::toDebugString(network))

        BootstrappedNetworkMap::iterator found = mBootstrappedNetworks.find(network->getDomain());
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

          if (mIdentityServiceDomain == network->getDomain()) {
            ZS_LOG_ERROR(Detail, log("cannot access peer contact service's identity lookup service") + ", provider domain=" + mIdentityServiceDomain)
            setError(errorCode, errorResaon);
            step();
            return;
          }

          // see if there is already an attempt to use the bootstrapped network for the peer contact service's domain
          BootstrappedNetworkMap::iterator foundProvider = mBootstrappedNetworks.find(mIdentityServiceDomain);
          if (foundProvider == mBootstrappedNetworks.end()) {
            IBootstrappedNetworkPtr providerNetwork = IBootstrappedNetwork::prepare(mIdentityServiceDomain, mThisWeak.lock());
            if (!providerNetwork) {
              ZS_LOG_ERROR(Detail, log("failed to create bootstrapper for domain") + ", domain=" + mIdentityServiceDomain)
              setError(errorCode, errorResaon);
              step();
              return;
            }

            // bootstrapped network not prepared yet for this domain, attempt to prepare it now
            mBootstrappedNetworks[mIdentityServiceDomain] = providerNetwork;

            // delay handling this failure until later...
            mFailedBootstrappedNetworks[network->getDomain()] = true;

            ZS_LOG_DETAIL(log("waiting to perform lookup on backup service peer contact's identity lookup service") + ", failed domain=" + network->getDomain() + ", backup=" + providerNetwork->getDomain())

            // wait for the bootstrapped network to complete
            return;
          }

          IBootstrappedNetworkPtr providerNetwork = (*foundProvider).second;
          if (!providerNetwork->isPreparationComplete()) {
            mFailedBootstrappedNetworks[network->getDomain()] = true;

            ZS_LOG_DETAIL(log("waiting to perform lookup on backup service peer contact's identity lookup service") + ", failed domain=" + network->getDomain() + ", backup=" + providerNetwork->getDomain())
            return;
          }

          if (!providerNetwork->wasSuccessful(&errorCode, &errorResaon)) {
            ZS_LOG_ERROR(Detail, log("failed to create bootstrapper for domain") + ", domain=" + mIdentityServiceDomain)
            setError(errorCode, errorResaon);
            step();
            return;
          }

          // the domain lookuped was a failure but we can issue a request using the provider's network instead
          mFailedBootstrappedNetworks[network->getDomain()] = true;

          // pretend we are re-issuing a request from the provider domain
          network = providerNetwork;
        }

        typedef IdentityLookupCheckRequest::Provider Provider;
        typedef IdentityLookupCheckRequest::ProviderList ProviderList;

        ProviderList providers;

        // find all identities using this domain
        for (DomainOrLegacyTypeToDomainMap::iterator iter = mTypeToDomainMap.begin(); iter != mTypeToDomainMap.end(); ++iter)
        {
          const String &domainOrType = (*iter).first;
          String &domain = (*iter).second;

          bool lookupThisDomain = (network->getDomain() == domain);

          if (mIdentityServiceDomain == network->getDomain()) {
            if (mAlreadyIssuedForProviderDomain)
              lookupThisDomain = false; // don't issue unless this is for a failed domain

            if (mFailedBootstrappedNetworks.find(domain) != mFailedBootstrappedNetworks.end()) {
              ZS_LOG_DETAIL(log("performing lookup on failed domain") + ", failed domain=" + domain + ", lookup now done on domain=" + mIdentityServiceDomain)
              lookupThisDomain = true;
            }
          }

          if (lookupThisDomain) {
            ZS_LOG_DEBUG(log("will perform lookup on type") + ", type/domain=" + domainOrType)

            // this type uses this domain
            IdentifierSafeCharDomainLegacyTypeMap::iterator foundConcat = mConcatDomains.find(domainOrType);
            IdentifierSafeCharDomainLegacyTypeMap::iterator foundSafeChar = mSafeCharDomains.find(domainOrType);

            ZS_THROW_BAD_STATE_IF(foundConcat == mConcatDomains.end())
            ZS_THROW_BAD_STATE_IF(foundSafeChar == mSafeCharDomains.end())

            const String &identifiers = (*foundConcat).second;
            const String &splitChar = (*foundSafeChar).second;

            if (identifiers.isEmpty()) {
              ZS_LOG_WARNING(Detail, log("no identifiers found for this domain/type") + ", domain=" + network->getDomain() + ", type/domain=" + domainOrType)
              continue;
            }

            Provider provider;
            provider.mBase = IServiceIdentity::joinURI(domainOrType, "");
            provider.mSeparator = splitChar;
            provider.mIdentities = identifiers;

            ZS_LOG_DEBUG(log("adding provider to list") + ", base=" + provider.mBase + ", seperator=" + provider.mSeparator + ", identities" + provider.mIdentities)
            providers.push_back(provider);
          }
        }

        if (network->getDomain() == mIdentityServiceDomain)
          mAlreadyIssuedForProviderDomain = true;

        mFailedBootstrappedNetworks.clear();

        if (providers.size() > 0) {
          // let's issue a request to discover these identities
          IdentityLookupCheckRequestPtr request = IdentityLookupCheckRequest::create();
          request->domain(network->getDomain());
          request->providers(providers);

          IMessageMonitorPtr monitor = IMessageMonitor::monitorAndSendToService(IMessageMonitorResultDelegate<IdentityLookupCheckResult>::convert(mThisWeak.lock()), network, "identity-lookup", "identity-lookup-check", request, Seconds(OPENPEER_CORE_IDENTITY_LOOK_REQUEST_TIMEOUT_SECONDS));

          if (!monitor) {
            ZS_LOG_ERROR(Detail, log("failed to create monitor for request"))
            setError(IHTTP::HTTPStatusCode_InternalServerError, "Failed to create monitor for identity lookup request");
            step();
            return;
          }

          ZS_LOG_DEBUG(log("monitoring identity lookup request") + IMessageMonitor::toDebugString(monitor))
          mMonitors[monitor->getID()] = monitor;
        }

        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IdentityLookup => IMessageMonitorResultDelegate<IdentityLookupCheckResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool IdentityLookup::handleMessageMonitorResultReceived(
                                                              IMessageMonitorPtr monitor,
                                                              IdentityLookupCheckResultPtr result
                                                              )
      {
        AutoRecursiveLock lock(getLock());

        MonitorMap::iterator found = mMonitors.find(monitor->getID());
        if (found == mMonitors.end()) {
          ZS_LOG_WARNING(Detail, log("notified about obsolete monitor") + IMessageMonitor::toDebugString(monitor))
          return false;
        }

        mMonitors.erase(found);

        IdentityLookupCheckRequestPtr originalRequest = IdentityLookupCheckRequest::convert(monitor->getMonitoredMessage());
        ZS_THROW_BAD_STATE_IF(!originalRequest)

        String originalDomain = originalRequest->domain();
        ZS_THROW_BAD_STATE_IF(!stack::IHelper::isValidDomain(originalDomain))

        IBootstrappedNetworkPtr network = IBootstrappedNetwork::prepare(originalDomain);
        ZS_THROW_BAD_STATE_IF(!originalRequest)

        const StackIdentityInfoList &resultInfos = result->identities();

        typedef IdentityLookupCheckRequest::Provider Provider;
        typedef IdentityLookupCheckRequest::ProviderList ProviderList;

        const ProviderList &originalRequestProviders = originalRequest->providers();

        ProviderList providers;

        for (StackIdentityInfoList::const_iterator iter = resultInfos.begin(); iter != resultInfos.end(); ++iter)
        {
          const StackIdentityInfo &resultInfo = (*iter);

          String domainOrType;
          String identifier;
          if (!IServiceIdentity::splitURI(resultInfo.mURI, domainOrType, identifier)) {
            ZS_LOG_ERROR(Detail, log("failed to split an identity") + ", identity url=" + resultInfo.mURI)
            continue;
          }

          IdentifierDomainOrLegacyTypeMap::iterator foundType = mDomainOrLegacyTypeIdentifiers.find(domainOrType);
          if (foundType == mDomainOrLegacyTypeIdentifiers.end()) {
            ZS_LOG_ERROR(Detail, log("failed to find previous known last update for identity") + ", identity url=" + resultInfo.mURI)
            continue;
          }

          IdentifierMap &identifiers = (*foundType).second;
          IdentifierMap::iterator foundIdentifier = identifiers.find(identifier);
          if (foundIdentifier == identifiers.end()) {
            ZS_LOG_ERROR(Detail, log("failed to find previous identifier in identitifers map") + ", identity url=" + resultInfo.mURI)
            continue;
          }

          Time lastKnownUpdate = (*foundIdentifier).second;

          if (lastKnownUpdate == resultInfo.mUpdated) {
            ZS_LOG_TRACE(log("identity information has not changed since last time") + ", identity url=" + resultInfo.mURI + ", last updated=" + IMessageHelper::timeToString(resultInfo.mUpdated))

            // nothing about this identity has changed since last time
            IdentityInfo info;

            info.mIdentityURI = resultInfo.mURI;
            info.mIdentityProvider = resultInfo.mProvider;
            info.mLastUpdated = resultInfo.mUpdated;

            mResults.push_back(info);
            continue;
          }

          String base = IServiceIdentity::joinURI(domainOrType, "");

          bool found = false;

          // scope: see if a provider for the type/domain has already beena added before, if so append the identifier to the list
          {
            for (ProviderList::iterator provIter = providers.begin(); provIter != providers.end(); ++provIter) {
              Provider &provider = (*provIter);
              if (base == provider.mBase) {
                // correct base to use, add identifier
                provider.mIdentities += provider.mSeparator + identifier;
                found = true;
                break;
              }
            }
          }

          if (!found) {
            // this provider has not been seen before - find a template to base it upon from the last issued request
            for (ProviderList::const_iterator provIter = originalRequestProviders.begin(); provIter != originalRequestProviders.end(); ++provIter) {
              const Provider &provider = (*provIter);
              if (base == provider.mBase) {
                // found correct template for a provider to use
                Provider newProvider(provider);
                newProvider.mIdentities = identifier; // only need to lookup this one identifier (some more may be found later)
                providers.push_back(newProvider);
                found = true;
                break;
              }
            }
          }

          if (!found) {
            ZS_LOG_ERROR(Detail, log("failed to find a previous provider base that can satisfy this identifier's type/domain") + ", identity url=" + resultInfo.mURI)
            continue;
          }

          ZS_LOG_TRACE(log("will perform new detailed lookup for identity") + ", identity" + resultInfo.mURI)
        }

        if (providers.size() > 0) {
          // let's issue a request to discover these identities
          IdentityLookupRequestPtr request = IdentityLookupRequest::create();
          request->domain(originalDomain);
          request->providers(providers);

          IMessageMonitorPtr newMonitor = IMessageMonitor::monitorAndSendToService(IMessageMonitorResultDelegate<IdentityLookupResult>::convert(mThisWeak.lock()), network, "identity-lookup", "identity-lookup", request, Seconds(OPENPEER_CORE_IDENTITY_LOOK_REQUEST_TIMEOUT_SECONDS));
          mMonitors[newMonitor->getID()] = newMonitor;
        }

        step();
        return true;
      }

      //-----------------------------------------------------------------------
      bool IdentityLookup::handleMessageMonitorErrorResultReceived(
                                                                   IMessageMonitorPtr monitor,
                                                                   IdentityLookupCheckResultPtr ignore, // will always be NULL
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
      #pragma mark IdentityLookup => IMessageMonitorResultDelegate<IdentityLookupResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool IdentityLookup::handleMessageMonitorResultReceived(
                                                              IMessageMonitorPtr monitor,
                                                              IdentityLookupResultPtr result
                                                              )
      {
        AutoRecursiveLock lock(getLock());

        MonitorMap::iterator found = mMonitors.find(monitor->getID());
        if (found == mMonitors.end()) {
          ZS_LOG_WARNING(Detail, log("notified about obsolete monitor") + IMessageMonitor::toDebugString(monitor))
          return false;
        }

        mMonitors.erase(found);

        const StackIdentityInfoList &resultInfos = result->identities();

        for (StackIdentityInfoList::const_iterator iter = resultInfos.begin(); iter != resultInfos.end(); ++iter)
        {
          const StackIdentityInfo &resultInfo = (*iter);

          if (!resultInfo.mPeerFilePublic) {
            ZS_LOG_WARNING(Detail, log("peer URI found in result not valid") + resultInfo.getDebugValueString())
            continue;
          }

          IdentityInfo info;

          info.mIdentityURI = resultInfo.mURI;
          info.mIdentityProvider = resultInfo.mProvider;
          info.mStableID = resultInfo.mStableID;

          info.mPeerFilePublic = resultInfo.mPeerFilePublic;
          info.mIdentityProofBundleEl = resultInfo.mIdentityProofBundle;

          info.mPriority = resultInfo.mPriority;
          info.mWeight = resultInfo.mWeight;

          info.mLastUpdated = resultInfo.mUpdated;
          info.mExpires = resultInfo.mExpires;

          info.mName = resultInfo.mName;
          info.mProfileURL = resultInfo.mProfile;
          info.mVProfileURL = resultInfo.mVProfile;

          for (StackIdentityInfo::AvatarList::const_iterator avIter = resultInfo.mAvatars.begin();  avIter != resultInfo.mAvatars.end(); ++avIter)
          {
            const StackIdentityInfo::Avatar &resultAvatar = (*avIter);
            IdentityInfo::Avatar avatar;

            avatar.mName = resultAvatar.mName;
            avatar.mURL = resultAvatar.mURL;
            avatar.mWidth = resultAvatar.mWidth;
            avatar.mHeight = resultAvatar.mHeight;
            info.mAvatars.push_back(avatar);
          }

          mResults.push_back(info);
        }

        step();
        return true;
      }

      //-----------------------------------------------------------------------
      bool IdentityLookup::handleMessageMonitorErrorResultReceived(
                                                                   IMessageMonitorPtr monitor,
                                                                   IdentityLookupResultPtr ignore, // will always be NULL
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
      #pragma mark IdentityLookup => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String IdentityLookup::log(const char *message) const
      {
        return String("IdentityLookup [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String IdentityLookup::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("identity lookup", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("delegate", mDelegate ? String("true") : String(), firstTime) +
               Helper::getDebugValue("error code", 0 != mErrorCode ? Stringize<typeof(mErrorCode)>(mErrorCode).string() : String(), firstTime) +
               Helper::getDebugValue("error reason", mErrorReason, firstTime) +
               Helper::getDebugValue("identity service domain", mIdentityServiceDomain, firstTime) +
               Helper::getDebugValue("bootstrapped networks", mBootstrappedNetworks.size() > 0 ? Stringize<size_t>(mBootstrappedNetworks.size()).string() : String(), firstTime) +
               Helper::getDebugValue("monitors", mMonitors.size() > 0 ? Stringize<size_t>(mMonitors.size()).string() : String(), firstTime) +
               Helper::getDebugValue("type identifiers", mDomainOrLegacyTypeIdentifiers.size() > 0 ? Stringize<size_t>(mDomainOrLegacyTypeIdentifiers.size()).string() : String(), firstTime) +
               Helper::getDebugValue("concat domains", mConcatDomains.size() > 0 ? Stringize<size_t>(mConcatDomains.size()).string() : String(), firstTime) +
               Helper::getDebugValue("safe char domains", mSafeCharDomains.size() > 0 ? Stringize<size_t>(mSafeCharDomains.size()).string() : String(), firstTime) +
               Helper::getDebugValue("type to domains", mTypeToDomainMap.size() > 0 ? Stringize<size_t>(mTypeToDomainMap.size()).string() : String(), firstTime) +
               Helper::getDebugValue("results", mResults.size() > 0 ? Stringize<size_t>(mResults.size()).string() : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      RecursiveLock &IdentityLookup::getLock() const
      {
        return mAccount->forIdentityLookup().getLock();
      }

      //-----------------------------------------------------------------------
      void IdentityLookup::prepareIdentity(
                                           const String &domain,
                                           const String &type,
                                           const String &identifier,
                                           const Time &lastUpdated
                                           )
      {
        ZS_LOG_DEBUG(log("preparing domain") + ", domain=" + domain + ", identifier=" + identifier)

        // scope: prepare bootstrapper for domain
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

        // scope prepare identifiers for domain
        {
          IdentifierDomainOrLegacyTypeMap::iterator found = mDomainOrLegacyTypeIdentifiers.find(type);
          if (found == mDomainOrLegacyTypeIdentifiers.end()) {
            IdentifierMap empty;
            mDomainOrLegacyTypeIdentifiers[type] = empty;
            found = mDomainOrLegacyTypeIdentifiers.find(type);

            ZS_LOG_DEBUG(log("adding contact type") + ", type=" + type)

            mConcatDomains[type] = String();
            mTypeToDomainMap[type] = domain;
          }

          IdentifierMap &identifiers = (*found).second;
          identifiers[identifier] = lastUpdated;

          IdentifierSafeCharDomainLegacyTypeMap::iterator foundSafe = mConcatDomains.find(type);
          ZS_THROW_BAD_STATE_IF(foundSafe == mConcatDomains.end())

          // append into one bit string (without any split char yet)
          String &concats = (*foundSafe).second;
          concats += identifier;
        }
      }

      //-----------------------------------------------------------------------
      void IdentityLookup::step()
      {
        ZS_LOG_DEBUG(log("step"))

        if ((mMonitors.size() < 1) &&
            (mBootstrappedNetworks.size() < 1)) {
          ZS_LOG_DEBUG(log("identity lookup is finished"))
          cancel();
          return;
        }

        ZS_LOG_DEBUG(log("identity lookup is still pending"))
      }

      //-----------------------------------------------------------------------
      void IdentityLookup::setError(WORD errorCode, const char *inErrorReason)
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
    #pragma mark IIdentityLookup
    #pragma mark

    //-------------------------------------------------------------------------
    String IIdentityLookup::toDebugString(IIdentityLookupPtr identity, bool includeCommaPrefix)
    {
      return internal::IdentityLookup::toDebugString(identity);
    }

    //-------------------------------------------------------------------------
    IIdentityLookupPtr IIdentityLookup::create(
                                               IAccountPtr account,
                                               IIdentityLookupDelegatePtr delegate,
                                               const IdentityLookupInfoList &identities,
                                               const char *identityServiceDomain
                                               )
    {
      return internal::IIdentityLookupFactory::singleton().create(account, delegate, identities, identityServiceDomain);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIdentityLookup::IdentityLookupInfo
    #pragma mark

    //-------------------------------------------------------------------------
    IIdentityLookup::IdentityLookupInfo::IdentityLookupInfo(const IdentityInfo &identity)
    {
      mIdentityURI = identity.mIdentityURI;
      mLastUpdated = identity.mLastUpdated;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
  }
}
