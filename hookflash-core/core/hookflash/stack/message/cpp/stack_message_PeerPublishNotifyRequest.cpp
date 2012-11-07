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

#include <hookflash/stack/message/internal/stack_message_PeerPublishNotifyRequest.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>

#include <zsLib/Stringize.h>
#include <zsLib/XML.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      typedef zsLib::ULONG ULONG;
      typedef zsLib::XML::DocumentPtr DocumentPtr;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

      PeerPublishNotifyRequestPtr PeerPublishNotifyRequest::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<PeerPublishNotifyRequest>(message);
      }

      PeerPublishNotifyRequest::PeerPublishNotifyRequest()
      {
      }

      PeerPublishNotifyRequestPtr PeerPublishNotifyRequest::create()
      {
        return internal::PeerPublishNotifyRequest::create();
      }

      DocumentPtr PeerPublishNotifyRequest::encode(IPeerFilesPtr peerFile)
      {
        return internal::PeerPublishNotifyRequest::encode(*this);
      }

      bool PeerPublishNotifyRequest::hasAttribute(AttributeTypes type) const
      {
        switch (type) {
          case AttributeType_PublicationList: return (mPublicationList.size() > 0);
        }
        return false;
      }

      namespace internal
      {
        PeerPublishNotifyRequestPtr PeerPublishNotifyRequest::create(ElementPtr rootEl)
        {
          PeerPublishNotifyRequestPtr ret(new message::PeerPublishNotifyRequest);

          if (rootEl)
          {
            ret->mID = IMessageHelper::getAttributeID(rootEl);

            try {
              ElementPtr documentsEl = rootEl->findFirstChildElementChecked("documents");

              documentsEl->orphan();

              do {
                ElementPtr documentEl = documentsEl->findFirstChildElement("document");
                if (!documentEl)
                  break;

                documentEl->orphan();

                rootEl->adoptAsLastChild(documentEl);

                IPublicationPtr publication;
                IPublicationMetaDataPtr publicationMetaData;
                MessageHelper::fillFrom(ret, rootEl, publication, publicationMetaData);

                documentEl->orphan();

                ret->mPublicationList.push_back(publicationMetaData);
              } while (true);
            } catch (CheckFailed &) {
            }
          }

          return ret;
        }

        DocumentPtr PeerPublishNotifyRequest::encode(message::PeerPublishNotifyRequest &msg)
        {
          ULONG maxDataSize = HOOKFLASH_STACK_MESSAGE_PEER_PUBLISH_NOTIFY_MAX_DOCUMENT_PUBLICATION_SIZE_IN_BYTES;

          DocumentPtr current;
          for (message::PeerPublishNotifyRequest::PublicationList::iterator iter = msg.mPublicationList.begin(); iter != msg.mPublicationList.end(); ++iter)
          {
            IPublicationMetaDataPtr metaData = (*iter);

            ULONG filledSize = 0;
            DocumentPtr doc = MessageHelper::createDocument(msg, metaData, &maxDataSize, msg.mPeerCache);
            maxDataSize -= (filledSize < maxDataSize ? filledSize : maxDataSize);
            try {
              ElementPtr rootEl = doc->getFirstChildElementChecked();
              ElementPtr documentEl = rootEl->findFirstChildElementChecked("document");

              documentEl->orphan();

              ElementPtr documentsEl = IMessageHelper::createElement("documents");

              rootEl->adoptAsLastChild(documentsEl);

              if (!current) {
                current = doc;
              }

              rootEl = current->getFirstChildElementChecked();
              documentsEl = rootEl->findFirstChildElementChecked("documents");

              documentsEl->adoptAsLastChild(documentEl);
            } catch (CheckFailed &) {
              return DocumentPtr();
            }
          }

          return current;
        }
      }
    }
  }
}
