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

#include <hookflash/stack/internal/stack_BootstrappedNetworkManager.h>
#include <hookflash/stack/internal/stack_BootstrappedNetwork.h>
#include <hookflash/stack/internal/stack_Stack.h>

#include <zsLib/helpers.h>

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
      BootstrappedNetworkManager::BootstrappedNetworkManager() :
        mID(zsLib::createPUID())
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
        if(isNoop()) return;
        
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
        static BootstrappedNetworkManagerPtr global = IBootstrappedNetworkManagerFactory::singleton().createBootstrappedNetworkManager();
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
