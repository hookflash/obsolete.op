//
//  stack_BootstrappedNetworkManager.cpp
//  hfstack
//
//  Created by Robin Raymond on 1/17/13.
//
//

#include <hookflash/stack/internal/stack_BootstrappedNetworkManager.h>
#include <hookflash/stack/internal/stack_BootstrappedNetwork.h>
#include <hookflash/stack/internal/stack_Stack.h>

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      typedef zsLib::String String;
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IBootstrappedNetworkManagerForBootstrappedNetwork
      #pragma mark

      //-----------------------------------------------------------------------
      BootstrappedNetworkManagerPtr IBootstrappedNetworkManagerForBootstrappedNetwork::singleton()
      {
        return BootstrappedNetworkManager::singleton();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetworkManager
      #pragma mark

      //-----------------------------------------------------------------------
      BootstrappedNetworkManager::BootstrappedNetworkManager()
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetworkManager::init()
      {
      }

      //-----------------------------------------------------------------------
      BootstrappedNetworkManager::~BootstrappedNetworkManager()
      {
        ZS_LOG_DEBUG(log("destroyed"))
      }

      //-----------------------------------------------------------------------
      BootstrappedNetworkManagerPtr BootstrappedNetworkManager::create()
      {
        BootstrappedNetworkManagerPtr pThis(new BootstrappedNetworkManager);
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetworkManager => IBootstrappedNetworkManagerForBootstrappedNetwork
      #pragma mark

      //-----------------------------------------------------------------------
      BootstrappedNetworkManagerPtr BootstrappedNetworkManager::singleton()
      {
        static BootstrappedNetworkManagerPtr global = BootstrappedNetworkManager::create();
        return global;
      }

      //-----------------------------------------------------------------------
      BootstrappedNetworkPtr BootstrappedNetworkManager::findExistingOrUse(BootstrappedNetworkPtr network)
      {
        AutoRecursiveLock lock(getLock());

        String domain = network->forBootstrappedNetworkManager().getDomain();

        BootstrappedNetworkMap::iterator found = mBootstrappedNetworks.find(domain);
        if (found != mBootstrappedNetworks.end()) {
          ZS_LOG_DEBUG(log("using existing bootstrapped network object") + ", domain=" + domain)
          BootstrappedNetworkPtr &result = (*found).second;
          return result;
        }

        ZS_LOG_DEBUG(log("using new bootstrapped network obejct") + ", domain=" + domain)

        mBootstrappedNetworks[domain] = network;
        return network;
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetworkManager::registerDelegate(
                                                        BootstrappedNetworkPtr network,
                                                        IBootstrappedNetworkDelegatePtr inDelegate
                                                        )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!network)
        ZS_THROW_INVALID_ARGUMENT_IF(!inDelegate)

        AutoRecursiveLock lock(getLock());

        IBootstrappedNetworkDelegatePtr delegate = IBootstrappedNetworkDelegateProxy::createWeak(IStackForInternal::queueDelegate(), inDelegate);

        if (network->forBootstrappedNetworkManager().isPreparationComplete()) {
          ZS_LOG_DEBUG(log("bootstrapper has already completed"))

          try {
            delegate->onBootstrappedNetworkPreparationCompleted(network);
          } catch(IBootstrappedNetworkDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_DEBUG(log("delegate gone"))
          }
          return;
        }

        mPendingDelegates.push_back(PendingDelegatePair(network, delegate));
      }

      //-----------------------------------------------------------------------
      void BootstrappedNetworkManager::notifyComplete(BootstrappedNetworkPtr network)
      {
        AutoRecursiveLock lock(getLock());
        for (PendingDelegateList::iterator iter = mPendingDelegates.begin(); iter != mPendingDelegates.end(); )
        {
          PendingDelegateList::iterator current = iter;
          ++iter;

          BootstrappedNetworkPtr &pendingNetwork = (*current).first;

          if (network->forBootstrappedNetworkManager().getID() == pendingNetwork->forBootstrappedNetworkManager().getID()) {
            // this pending network is now complete...
            IBootstrappedNetworkDelegatePtr delegate = (*current).second;

            try {
              delegate->onBootstrappedNetworkPreparationCompleted(network);
            } catch(IBootstrappedNetworkDelegateProxy::Exceptions::DelegateGone &) {
              ZS_LOG_DEBUG(log("delegate gone"))
            }

            mPendingDelegates.erase(current);
          }
        }
      }

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark BootstrappedNetworkManager => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String BootstrappedNetworkManager::log(const char *message) const
      {
        return String("BootstrappedNetworkManager [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }
    }
  }
}
