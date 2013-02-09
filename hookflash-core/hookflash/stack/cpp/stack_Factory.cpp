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

#include <hookflash/stack/internal/stack_Factory.h>

#include <zsLib/Log.h>

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helper)
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Factory
      #pragma mark

      //-----------------------------------------------------------------------
      void Factory::override(FactoryPtr override)
      {
        singleton()->mOverride = override;
      }

      //-----------------------------------------------------------------------
      FactoryPtr &Factory::singleton()
      {
        static FactoryPtr global = Factory::create();
        if (global->mOverride) return global->mOverride;
        return global;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Factory => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      FactoryPtr Factory::create()
      {
        return FactoryPtr(new Factory);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IAccountFactory &IAccountFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      AccountPtr IAccountFactory::create(
                                         IAccountDelegatePtr delegate,
                                         IServicePeerContactSessionPtr peerContactSession
                                         )
      {
        return Account::create(delegate, peerContactSession);
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountFinderFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IAccountFinderFactory &IAccountFinderFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      AccountFinderPtr IAccountFinderFactory::create(
                                                     IAccountFinderDelegatePtr delegate,
                                                     AccountPtr outer
                                                     )
      {
        return AccountFinder::create(delegate, outer);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountPeerLocationFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IAccountPeerLocationFactory &IAccountPeerLocationFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      AccountPeerLocationPtr IAccountPeerLocationFactory::create(
                                                                 IAccountPeerLocationDelegatePtr delegate,
                                                                 AccountPtr outer,
                                                                 const LocationInfo &locationInfo
                                                                 )
      {
        return AccountPeerLocation::create(delegate, outer, locationInfo);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IBootstrappedNetworkFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IBootstrappedNetworkFactory &IBootstrappedNetworkFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      BootstrappedNetworkPtr IBootstrappedNetworkFactory::prepare(
                                                                  const char *domain,
                                                                  IBootstrappedNetworkDelegatePtr delegate
                                                                  )
      {
        return BootstrappedNetwork::prepare(domain, delegate);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IBootstrappedNetworkFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IBootstrappedNetworkManagerFactory &IBootstrappedNetworkManagerFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      BootstrappedNetworkManagerPtr IBootstrappedNetworkManagerFactory::createBootstrappedNetworkManager()
      {
        return BootstrappedNetworkManager::create();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageIncomingFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IMessageIncomingFactory &IMessageIncomingFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      MessageIncomingPtr IMessageIncomingFactory::create(
                                                         AccountPtr account,
                                                         LocationPtr location,
                                                         message::MessagePtr message
                                                         )
      {
        return MessageIncoming::create(account, location, message);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageMonitorFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IMessageMonitorFactory &IMessageMonitorFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      MessageMonitorPtr IMessageMonitorFactory::monitor(
                                                        IMessageMonitorDelegatePtr delegate,
                                                        message::MessagePtr requestMessage,
                                                        Duration timeout
                                                        )
      {
        return MessageMonitor::monitor(delegate, requestMessage, timeout);
      }

      //-----------------------------------------------------------------------
      MessageMonitorPtr IMessageMonitorFactory::monitorAndSendToLocation(
                                                                         IMessageMonitorDelegatePtr delegate,
                                                                         ILocationPtr peerLocation,
                                                                         message::MessagePtr message,
                                                                         Duration timeout
                                                                         )
      {
        return MessageMonitor::monitorAndSendToLocation(delegate, peerLocation, message, timeout);
      }

      //-----------------------------------------------------------------------
      MessageMonitorPtr IMessageMonitorFactory::monitorAndSendToService(
                                                                        IMessageMonitorDelegatePtr delegate,
                                                                        IBootstrappedNetworkPtr bootstrappedNetwork,
                                                                        const char *serviceType,
                                                                        const char *serviceMethodName,
                                                                        message::MessagePtr message,
                                                                        Duration timeout
                                                                        )
      {
        return MessageMonitor::monitorAndSendToService(delegate, bootstrappedNetwork, serviceType, serviceMethodName, message, timeout);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageMonitorManagerFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IMessageMonitorManagerFactory &IMessageMonitorManagerFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      MessageMonitorManagerPtr IMessageMonitorManagerFactory::createMessageMonitorManager()
      {
        return MessageMonitorManager::create();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ILocationFactory
      #pragma mark

      //-----------------------------------------------------------------------
      ILocationFactory &ILocationFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      LocationPtr ILocationFactory::getForLocal(IAccountPtr account)
      {
        return Location::getForLocal(account);
      }

      //-----------------------------------------------------------------------
      LocationPtr ILocationFactory::getForFinder(IAccountPtr account)
      {
        return Location::getForFinder(account);
      }

      //-----------------------------------------------------------------------
      LocationPtr ILocationFactory::getForPeer(
                                               IPeerPtr peer,
                                               const char *locationID
                                               )
      {
        return Location::getForPeer(peer, locationID);
      }

      //-----------------------------------------------------------------------
      LocationPtr ILocationFactory::create(
                                           IMessageSourcePtr messageSource,
                                           const char *peerURI,
                                           const char *locationID
                                           )
      {
        return Location::create(messageSource, peerURI, locationID);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPeerFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IPeerFactory &IPeerFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      PeerPtr IPeerFactory::create(
                                   IAccountPtr account,
                                   IPeerFilePublicPtr peerFilePublic
                                   )
      {
        return Peer::create(account, peerFilePublic);
      }

      //-----------------------------------------------------------------------
      PeerPtr IPeerFactory::getFromSignature(
                                             IAccountPtr account,
                                             ElementPtr signedElement
                                             )
      {
        return Peer::getFromSignature(account, signedElement);
      }

      //-----------------------------------------------------------------------
      PeerPtr IPeerFactory::create(
                                   AccountPtr account,
                                   const char *peerURI
                                   )
      {
        return Peer::create(account, peerURI);
      }

    }
  }
}
