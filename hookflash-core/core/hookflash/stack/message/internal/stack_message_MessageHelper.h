/*
 
 Copyright (c) 2012, SMB Phone Inc.
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

#include <zsLib/zsTypes.h>
#include <zsLib/String.h>
#include <zsLib/XML.h>

#include <hookflash/stack/message/hookflashTypes.h>
#include <hookflash/stack/message/Message.h>
#include <hookflash/stack/message/IMessageHelper.h>
#include <hookflash/stack/IPublicationMetaData.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace internal
      {
        class MessageHelper
        {
        public:
          typedef zsLib::UINT UINT;
          typedef zsLib::WORD WORD;
          typedef zsLib::ULONG ULONG;
          typedef zsLib::Time Time;
          typedef zsLib::String String;
          typedef zsLib::XML::ElementPtr ElementPtr;
          typedef zsLib::XML::DocumentPtr DocumentPtr;
          typedef zsLib::XML::TextPtr TextPtr;
          typedef zsLib::XML::NodePtr NodePtr;
          typedef stack::IPublicationMetaData::PublishToRelationshipsMap PublishToRelationshipsMap;
          typedef message::Candidate Candidate;
          typedef message::Location Location;
          typedef message::MessagePtr MessagePtr;
          typedef Message::Methods Methods;
          typedef Message::MessageTypes MessageTypes;

          static ElementPtr createElement(const Candidate &candidate);
          static ElementPtr createElement(const Location &candidate);

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
                               MessagePtr msg,
                               ElementPtr root,
                               IPublicationPtr &outPublication,
                               IPublicationMetaDataPtr &outPublicationMetaData
                               );

          static int stringToInt(const String &s);
          static UINT stringToUint(const String &s);

          static WORD getErrorCode(ElementPtr root);
          static String getErrorReason(ElementPtr root);

          static Location     createLocation(ElementPtr elem);
          static Candidate    createCandidate(ElementPtr elem);
          static Service      createService(ElementPtr elem);
          static Finder       createFinder(ElementPtr elem);
        };
      }
    }
  }
}
