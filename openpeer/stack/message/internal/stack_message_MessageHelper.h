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

#pragma once

#include <openpeer/stack/message/internal/types.h>
#include <openpeer/stack/message/Message.h>
#include <openpeer/stack/message/IMessageHelper.h>
#include <openpeer/stack/IPublicationMetaData.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace internal
      {
        class MessageHelper : public IMessageHelper
        {
        public:
          typedef IPublicationMetaData::PublishToRelationshipsMap PublishToRelationshipsMap;
          typedef Message::Methods Methods;
          typedef Message::MessageTypes MessageTypes;

          static ElementPtr createElement(
                                          const Candidate &candidate,
                                          const SecureByteBlock *encryptionKey = NULL
                                          );
          static ElementPtr createElement(
                                          const LocationInfo &candidate,
                                          const SecureByteBlock *encryptionKey = NULL
                                          );
          static ElementPtr createElement(
                                          const IdentityInfo &identity,
                                          bool forcePriorityWeightOutput = false
                                          );
          static ElementPtr createElement(const LockboxInfo &info);
          static ElementPtr createElement(const AgentInfo &info);
          static ElementPtr createElement(const NamespaceGrantChallengeInfo &info);
          static ElementPtr createElement(const NamespaceInfo &info);
          static ElementPtr createElement(const RolodexInfo &info);

          static ElementPtr createElement(
                                          const PublishToRelationshipsMap &relationships,
                                          const char *elementName
                                          );

          static DocumentPtr createDocument(
                                            Message &msg,
                                            IPublicationMetaDataPtr publicationMetaData,
                                            ULONG *notifyPeerPublishMaxDocumentSizeInBytes = NULL,
                                            IPublicationRepositoryPeerCachePtr peerCache = IPublicationRepositoryPeerCachePtr()
                                            );

          static void fillFrom(
                               IMessageSourcePtr messageSource,
                               MessagePtr msg,
                               ElementPtr root,
                               IPublicationPtr &outPublication,
                               IPublicationMetaDataPtr &outPublicationMetaData
                               );

          static int stringToInt(const String &s);
          static UINT stringToUint(const String &s);

          static WORD getErrorCode(ElementPtr root);
          static String getErrorReason(ElementPtr root);

          static LocationInfo createLocation(
                                             ElementPtr elem,
                                             IMessageSourcePtr messageSource,
                                             const SecureByteBlock *encryptionKey = NULL
                                             );
          static Candidate    createCandidate(
                                              ElementPtr elem,
                                              const SecureByteBlock *encryptionKey = NULL
                                              );
          static Finder                       createFinder(ElementPtr elem);
          static Service                      createService(ElementPtr serviceEl);
          static IdentityInfo                 createIdentity(ElementPtr elem);
          static LockboxInfo                  createLockbox(ElementPtr elem);
          static AgentInfo                    createAgent(ElementPtr elem);
          static NamespaceGrantChallengeInfo  createNamespaceGrantChallenge(ElementPtr elem);
          static RolodexInfo                  createRolodex(ElementPtr elem);
        };
      }
    }
  }
}
