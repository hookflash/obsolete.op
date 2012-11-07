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

#include <hookflash/stack/message/internal/stack_message_PeerToFinderSessionCreateRequest.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/IPeerFiles.h>
#include <hookflash/stack/IPeerFilePublic.h>
#include <hookflash/stack/IPeerFilePrivate.h>
#include <hookflash/services/IHelper.h>

#include <zsLib/Stringize.h>
#include <zsLib/zsHelpers.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      using zsLib::Stringize;

      typedef zsLib::Time Time;
      typedef zsLib::XML::DocumentPtr DocumentPtr;
      typedef zsLib::XML::Text Text;
      typedef zsLib::XML::Element Element;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef zsLib::XML::Exceptions Exceptions;

      PeerToFinderSessionCreateRequestPtr PeerToFinderSessionCreateRequest::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<PeerToFinderSessionCreateRequest>(message);
      }

      PeerToFinderSessionCreateRequest::PeerToFinderSessionCreateRequest() :
        mOneTimeKey(services::IHelper::randomString(32))
      {
      }

      bool PeerToFinderSessionCreateRequest::hasAttribute(AttributeTypes type) const
      {
        switch (type)
        {
          case AttributeType_FinderID:
            return (!mFinderID.isEmpty());
          case AttributeType_ContactID:
            return (!mContactID.isEmpty());
          case AttributeType_Expires:
            return (Time() != mExpires);
          case AttributeType_PeerFile:
            return (mPeerFile);
          case AttributeType_OneTimeKey:
            return (!mOneTimeKey.isEmpty());
          case AttributeType_Location:
            return (mLocation.hasData());
          default:
            break;
        }
        return false;
      }

      zsLib::XML::DocumentPtr PeerToFinderSessionCreateRequest::encode(IPeerFilesPtr peerFile)
      {
        return internal::PeerToFinderSessionCreateRequest::encode(*this, peerFile);
      }

      PeerToFinderSessionCreateRequestPtr PeerToFinderSessionCreateRequest::create()
      {
        return internal::PeerToFinderSessionCreateRequest::create();
      }


      namespace internal
      {
        PeerToFinderSessionCreateRequestPtr PeerToFinderSessionCreateRequest::create()
        {
          PeerToFinderSessionCreateRequestPtr ret(new message::PeerToFinderSessionCreateRequest);
          return ret;
        }

        DocumentPtr PeerToFinderSessionCreateRequest::encode(
                                                             message::PeerToFinderSessionCreateRequest &msg,
                                                             IPeerFilesPtr peerFile
                                                             )
        {
          zsLib::XML::DocumentPtr ret = IMessageHelper::createDocumentWithRoot(msg);
          zsLib::XML::ElementPtr root = ret->getFirstChildElement();
          zsLib::XML::ElementPtr elTb, elT, el;
          zsLib::XML::TextPtr elTxt;

          elT = Element::create("token");

          elTb = IMessageHelper::createElement("tokenBundle");
          elTb->adoptAsFirstChild(elT);

          root->adoptAsFirstChild(elTb);

          if(msg.hasAttribute(message::PeerToFinderSessionCreateRequest::AttributeType_FinderID))
          {
            el = Element::create();
            el->setValue("finder");
            IMessageHelper::setAttributeID(el, msg.mFinderID);

            elT->adoptAsLastChild(el);
          }

          if(msg.hasAttribute(message::PeerToFinderSessionCreateRequest::AttributeType_ContactID))
          {
            el = Element::create();
            el->setValue("contact");
            IMessageHelper::setAttributeID(el, msg.mContactID);

            elT->adoptAsLastChild(el);
          }

          if(msg.hasAttribute(message::PeerToFinderSessionCreateRequest::AttributeType_OneTimeKey))
          {
            el = Element::create();
            el->setValue("oneTimeKey");
            IMessageHelper::setAttributeID(el, msg.mOneTimeKey);

            elT->adoptAsLastChild(el);
          }

          if(msg.hasAttribute(message::PeerToFinderSessionCreateRequest::AttributeType_Expires))
          {
            elTxt = Text::create();
            elTxt->setValue(Stringize<time_t>(zsLib::toEpoch(msg.mExpires)));

            el = Element::create();
            el->setValue("expires");
            el->adoptAsFirstChild(elTxt);

            elT->adoptAsLastChild(el);
          }

          if(peerFile)
          {
            IPeerFilePublicPtr pubPeer = peerFile->getPublic();
            if (pubPeer)
            {
              ElementPtr pubPeerEl = pubPeer->saveToXML();
              if (pubPeerEl)
                elT->adoptAsLastChild(pubPeerEl);
            }

            IPeerFilePrivatePtr privPeer = peerFile->getPrivate();
            if (privPeer)
            {
              privPeer->signElement(elT);
            }
          }

          if(msg.hasAttribute( message::PeerToFinderSessionCreateRequest::AttributeType_Location))
          {
            root->adoptAsLastChild(MessageHelper::createElement(msg.mLocation));
          }

          return ret;
        }

      }
    }
  }
}
