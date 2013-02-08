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

#include <hookflash/stack/message/internal/stack_message_PeerToPeerPeerIdentifyRequest.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/IPeerFilePublic.h>
#include <hookflash/stack/IPeerFilePrivate.h>
#include <hookflash/stack/IPeerFiles.h>

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
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef zsLib::XML::Exceptions Exceptions;

      PeerToPeerPeerIdentifyRequestPtr PeerToPeerPeerIdentifyRequest::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<PeerToPeerPeerIdentifyRequest>(message);
      }

      PeerToPeerPeerIdentifyRequest::PeerToPeerPeerIdentifyRequest():
        mOneTimeKey(services::IHelper::randomString(32)),
        mProofID(services::IHelper::randomString(32))
      {
      }

      bool PeerToPeerPeerIdentifyRequest::hasAttribute(AttributeTypes type) const
      {
        switch (type)
        {
          case AttributeType_ProofID:
            return (!mProofID.isEmpty());
          case AttributeType_LocationID:
            return (!mLocationID.isEmpty());
          case AttributeType_FindSecret:
            return (!mFindSecret.isEmpty());
          case AttributeType_Expires:
            return (Time() != mExpires);
          case AttributeType_UserAgent:
            return (!mUserAgent.isEmpty());
          case AttributeType_PeerFile:
            return (mPeerFilePublic != 0);
          case AttributeType_OneTimeKey:
            return (!mOneTimeKey.isEmpty());
          default:
            break;
        }
        return false;
      }

      DocumentPtr PeerToPeerPeerIdentifyRequest::encode(IPeerFilesPtr peerFile)
      {
        return internal::PeerToPeerPeerIdentifyRequest::encode(*this, peerFile);
      }

      PeerToPeerPeerIdentifyRequestPtr PeerToPeerPeerIdentifyRequest::create()
      {
        return internal::PeerToPeerPeerIdentifyRequest::create();
      }


      namespace internal
      {
        PeerToPeerPeerIdentifyRequestPtr PeerToPeerPeerIdentifyRequest::create(ElementPtr root)
        {
          PeerToPeerPeerIdentifyRequestPtr ret(new message::PeerToPeerPeerIdentifyRequest);

          if (root)
          {
            try
            {
              ElementPtr el;
              ElementPtr proofEl = root->findFirstChildElementChecked("proofBundle")->findFirstChildElementChecked("proof");

              ret->mID = IMessageHelper::getAttributeID(root);

              ret->mProofID = IMessageHelper::getAttributeID(proofEl);
              ret->mLocationID = IMessageHelper::getAttributeID(proofEl->findFirstChildElement("location"));
              ret->mFindSecret = IMessageHelper::getElementText(proofEl->findFirstChildElement("findSecret"));
              ret->mOneTimeKey = IMessageHelper::getAttributeID(proofEl->findFirstChildElement("oneTimeKey"));
              ret->mExpires = IMessageHelper::stringToTime(IMessageHelper::getElementText(proofEl->findFirstChildElement("expires")));
              ret->mUserAgent = IMessageHelper::getElementTextAndEntityDecode(proofEl->findFirstChildElement("userAgent"));
              ret->mPeerFilePublic = IPeerFilePublic::loadFromXML(proofEl->findFirstChildElement("peer"));
            }
            catch (Exceptions::CheckFailed &e) {}
          }

          return ret;
        }


        DocumentPtr PeerToPeerPeerIdentifyRequest::encode(
                                                          message::PeerToPeerPeerIdentifyRequest &msg,
                                                          IPeerFilesPtr peerFile
                                                          )
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(msg);
          ElementPtr root = ret->getFirstChildElement();
          ElementPtr elPb, elP, el;


          elP = IMessageHelper::createElement("proof");
          if(msg.hasAttribute(message::PeerToPeerPeerIdentifyRequest::AttributeType_ProofID))
          {
            IMessageHelper::setAttributeID(elP, msg.mProofID);
          }

          elPb = IMessageHelper::createElement("proofBundle");
          elPb->adoptAsFirstChild(elP);

          root->adoptAsFirstChild(elPb);

          if(msg.hasAttribute(message::PeerToPeerPeerIdentifyRequest::AttributeType_LocationID))
          {
            elP->adoptAsLastChild(IMessageHelper::createElementWithID("location", msg.mLocationID));
          }

          if(msg.hasAttribute(message::PeerToPeerPeerIdentifyRequest::AttributeType_FindSecret))
          {
            elP->adoptAsLastChild(IMessageHelper::createElementWithText("findSecret", msg.mFindSecret));
          }

          if(msg.hasAttribute(message::PeerToPeerPeerIdentifyRequest::AttributeType_OneTimeKey))
          {
            elP->adoptAsLastChild(IMessageHelper::createElementWithID("oneTimeKey", msg.mOneTimeKey));
          }

          if(msg.hasAttribute(message::PeerToPeerPeerIdentifyRequest::AttributeType_Expires))
          {
            elP->adoptAsLastChild(IMessageHelper::createElementWithText("expires", Stringize<time_t>(zsLib::toEpoch(msg.mExpires))));
          }

          if(msg.hasAttribute(message::PeerToPeerPeerIdentifyRequest::AttributeType_UserAgent))
          {
            elP->adoptAsLastChild(IMessageHelper::createElementWithTextAndEntityEncode("userAgent", msg.mUserAgent));
          }

          if(peerFile)
          {
            IPeerFilePublicPtr pubPeer = peerFile->getPublic();
            if (pubPeer)
            {
              ElementPtr pubPeerEl = pubPeer->saveToXML();
              if (pubPeerEl)
                elP->adoptAsLastChild(pubPeerEl);
            }

            IPeerFilePrivatePtr privPeer = peerFile->getPrivate();
            if (privPeer)
            {
              privPeer->signElement(elP);
            }
          }

          return ret;
        }

      }
    }
  }
}
