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

#include <hookflash/core/internal/core_Factory.h>

#include <zsLib/Log.h>

namespace hookflash { namespace core { ZS_DECLARE_SUBSYSTEM(hookflash_core) } }

namespace hookflash
{
  namespace core
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
      AccountPtr IAccountFactory::login(
                                        IAccountDelegatePtr delegate,
                                        IConversationThreadDelegatePtr conversationThreadDelegate,
                                        ICallDelegatePtr callDelegate,
                                        const char *peerContactServiceDomain,
                                        IIdentityPtr identity
                                        )
      {
        return Account::login(delegate, conversationThreadDelegate, callDelegate, peerContactServiceDomain, identity);
      }

      //-----------------------------------------------------------------------
      AccountPtr IAccountFactory::relogin(
                                          IAccountDelegatePtr delegate,
                                          IConversationThreadDelegatePtr conversationThreadDelegate,
                                          ICallDelegatePtr callDelegate,
                                          ElementPtr peerFilePrivateEl,
                                          const char *peerFilePrivateSecret
                                          )
      {
        return Account::relogin(delegate, conversationThreadDelegate, callDelegate, peerFilePrivateEl, peerFilePrivateSecret);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountFactory
      #pragma mark

      //-----------------------------------------------------------------------
      ICallFactory &ICallFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      CallPtr ICallFactory::placeCall(
                                      IConversationThreadPtr conversationThread,
                                      IContactPtr toContact,
                                      bool includeAudio,
                                      bool includeVideo
                                      )
      {
        return Call::placeCall(conversationThread, toContact, includeAudio, includeVideo);
      }

      //-----------------------------------------------------------------------
      CallPtr ICallFactory::createForIncomingCall(
                                                  ConversationThreadPtr inConversationThread,
                                                  ContactPtr callerContact,
                                                  const DialogPtr &remoteDialog
                                                  )
      {
        return createForIncomingCall(inConversationThread, callerContact, remoteDialog);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICallTransportFactory
      #pragma mark

      //-----------------------------------------------------------------------
      ICallTransportFactory &ICallTransportFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      CallTransportPtr ICallTransportFactory::create(
                                               ICallTransportDelegatePtr delegate,
                                               const char *turnServer,
                                               const char *turnServerUsername,
                                               const char *turnServerPassword,
                                               const char *stunServer
                                               )
      {
        return CallTransport::create(delegate, turnServer, turnServerUsername, turnServerPassword, stunServer);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IContactFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IContactFactory &IContactFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      ContactPtr IContactFactory::createFromPeerURI(
                                                    IAccountPtr account,
                                                    const char *peerURI,
                                                    const char *findSecret,
                                                    const char *inStableID,
                                                    const char *inUserID
                                                    )
      {
        return Contact::createFromPeerURI(account, peerURI, findSecret, inStableID, inUserID);
      }

      //-----------------------------------------------------------------------
      ContactPtr IContactFactory::createFromPeer(
                                                 AccountPtr account,
                                                 IPeerPtr peer,
                                                 const char *userIDIfKnown
                                                 )
      {
        return Contact::createFromPeer(account, peer, userIDIfKnown);
      }

      //-----------------------------------------------------------------------
      ContactPtr IContactFactory::createFromPeerFilePublic(
                                                           AccountPtr account,
                                                           IPeerFilePublicPtr peerFilePublic,
                                                           const char *previousStableUniqueID
                                                           )
      {
        return Contact::createFromPeerFilePublic(account, peerFilePublic, previousStableUniqueID);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IContactPeerFilePublicLookupFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IContactPeerFilePublicLookupFactory &IContactPeerFilePublicLookupFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      ContactPeerFilePublicLookupPtr IContactPeerFilePublicLookupFactory::create(
                                                                                 IContactPeerFilePublicLookupDelegatePtr delegate,
                                                                                 const ContactList &contacts
                                                                                 )
      {
        return ContactPeerFilePublicLookup::create(delegate, contacts);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IConversationThreadFactory &IConversationThreadFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      //-----------------------------------------------------------------------
      ConversationThreadPtr IConversationThreadFactory::createConversationThread(
                                                                                 IAccountPtr account,
                                                                                 ElementPtr profileBundleEl
                                                                                 )
      {
        return ConversationThread::create(account, profileBundleEl);
      }

      //-----------------------------------------------------------------------
      ConversationThreadPtr IConversationThreadFactory::createConversationThread(
                                                                                 AccountPtr account,
                                                                                 ILocationPtr peerLocation,
                                                                                 IPublicationMetaDataPtr metaData,
                                                                                 const SplitMap &split
                                                                                 )
      {
        return ConversationThread::create(account, peerLocation, metaData, split);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadDocumentFetcherFactory
      #pragma mark

      //-----------------------------------------------------------------------
      IConversationThreadDocumentFetcherFactory &IConversationThreadDocumentFetcherFactory::singleton()
      {
        return *(Factory::singleton().get());
      }

      ConversationThreadDocumentFetcherPtr IConversationThreadDocumentFetcherFactory::create(
                                                                                             IConversationThreadDocumentFetcherDelegatePtr delegate,
                                                                                             IPublicationRepositoryPtr repository
                                                                                             )
      {
        return ConversationThreadDocumentFetcher::create(delegate, repository);
      }
    }
  }
}
