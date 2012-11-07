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

#include <hookflash/stack/message/internal/stack_message_PeerToFinderPeerLocationFindReply.h>
#include <hookflash/stack/message/internal/stack_message_PeerToFinderPeerLocationFindRequest.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>

#include <zsLib/zsHelpers.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      typedef zsLib::String String;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef zsLib::XML::DocumentPtr DocumentPtr;

      PeerToFinderPeerLocationFindReplyPtr PeerToFinderPeerLocationFindReply::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<PeerToFinderPeerLocationFindReply>(message);
      }

      PeerToFinderPeerLocationFindReply::PeerToFinderPeerLocationFindReply()
      {
      }

      bool PeerToFinderPeerLocationFindReply::hasAttribute(PeerToFinderPeerLocationFindReply::AttributeTypes type) const
      {
        switch (type)
        {
          case AttributeType_Location:
            return !mLocation.mID.isEmpty() || !mLocation.mHost.isEmpty() || !mLocation.mOS.isEmpty() || !mLocation.mSystem.isEmpty() || !mLocation.mUserAgent.isEmpty();
          case AttributeType_Routes:
            return mRoutes.size() > 0;
          default:
            break;
        }
        return MessageReply::hasAttribute((MessageReply::AttributeTypes)type);
      }

      DocumentPtr PeerToFinderPeerLocationFindReply::encode(IPeerFilesPtr peerFile)
      {
        return internal::PeerToFinderPeerLocationFindReply::encode(*this);
      }

      PeerToFinderPeerLocationFindReplyPtr PeerToFinderPeerLocationFindReply::create(PeerToFinderPeerLocationFindRequestPtr request)
      {
        return internal::PeerToFinderPeerLocationFindReply::create(request);
      }


      namespace internal
      {
        PeerToFinderPeerLocationFindReplyPtr PeerToFinderPeerLocationFindReply::create(ElementPtr root)
        {
          PeerToFinderPeerLocationFindReplyPtr ret(new message::PeerToFinderPeerLocationFindReply);

          ret->mID = IMessageHelper::getAttributeID(root);

          ret->mTime = IMessageHelper::getAttributeEpoch(root);

          ElementPtr locationEl = root->findFirstChildElement("location");

          if (locationEl)
            ret->mLocation = MessageHelper::createLocation(locationEl);

          ElementPtr routes = root->findFirstChildElement("routes");
          if (routes)
          {
            RouteList routeLst;
            ElementPtr route = routes->findFirstChildElement("route");
            while (route)
            {
              String id = IMessageHelper::getAttributeID(route);
              routeLst.push_back(id);

              route = route->getNextSiblingElement();
            }

            if (routeLst.size() > 0)
              ret->mRoutes = routeLst;
          }

          return ret;
        }


        PeerToFinderPeerLocationFindReplyPtr PeerToFinderPeerLocationFindReply::create(PeerToFinderPeerLocationFindRequestPtr request)
        {
          PeerToFinderPeerLocationFindReplyPtr ret(new message::PeerToFinderPeerLocationFindReply);

          ret->mID = request->messageID();

          if (request->hasAttribute(message::PeerToFinderPeerLocationFindRequest::AttributeType_Routes))
          {
            ret->mRoutes = request->mRoutes;
          }

          return ret;
        }


        DocumentPtr PeerToFinderPeerLocationFindReply::encode(message::PeerToFinderPeerLocationFindReply &msg)
        {
          msg.mTime = zsLib::now();

          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(msg);
          ElementPtr root = ret->getFirstChildElement();
          ElementPtr routes;
          ElementPtr locationEl;
          ElementPtr candidates;

          if (msg.hasAttribute(message::PeerToFinderPeerLocationFindReply::AttributeTypes(MessageReply::AttributeType_Time)))
          {
            IMessageHelper::setAttributeEpoch(root, msg.mTime);
          }

          if (msg.hasAttribute(message::PeerToFinderPeerLocationFindReply::AttributeType_Location))
          {
            locationEl = MessageHelper::createElement(msg.location());
            root->adoptAsLastChild(locationEl);
          }

          if (msg.hasAttribute(message::PeerToFinderPeerLocationFindReply::AttributeType_Routes))
          {
            routes = IMessageHelper::createElement("routes");
            root->adoptAsLastChild(routes);

            RouteList::const_iterator it;
            for(it = msg.mRoutes.begin(); it != msg.mRoutes.end(); ++it)
            {
              String routeId(*it);
              routes->adoptAsLastChild(IMessageHelper::createElementWithID("route", routeId));
            }
          }

          return ret;
        }
      }
    }
  }
}
