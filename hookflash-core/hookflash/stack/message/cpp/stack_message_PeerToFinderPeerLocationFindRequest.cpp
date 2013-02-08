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

#include <hookflash/stack/message/internal/stack_message_PeerToFinderPeerLocationFindRequest.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      typedef zsLib::String String;
      typedef zsLib::XML::DocumentPtr DocumentPtr;
      typedef zsLib::XML::ElementPtr ElementPtr;

      PeerToFinderPeerLocationFindRequestPtr PeerToFinderPeerLocationFindRequest::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<PeerToFinderPeerLocationFindRequest>(message);
      }

      PeerToFinderPeerLocationFindRequest::PeerToFinderPeerLocationFindRequest()
      {
      }

      bool PeerToFinderPeerLocationFindRequest::hasAttribute(AttributeTypes type) const
      {
        switch (type)
        {
          case AttributeType_Location:
            return !mLocation.mID.isEmpty() || !mLocation.mHost.isEmpty() || !mLocation.mOS.isEmpty() || !mLocation.mSystem.isEmpty() || !mLocation.mUserAgent.isEmpty();
          case AttributeType_LocalContactID:
            return (!mLocalContactID.isEmpty());
          case AttributeType_RemoteContactID:
            return (!mRemoteContactID.isEmpty());
          case AttributeType_Routes:
            return (mRoutes.size() != 0);
          case AttributeType_ExcludedLocations:
            return (mExcludedLocations.size() != 0);
          default:
            break;
        }
        return false;
      }

      DocumentPtr PeerToFinderPeerLocationFindRequest::encode(IPeerFilesPtr peerFile)
      {
        return internal::PeerToFinderPeerLocationFindRequest::encode(*this);
      }

      PeerToFinderPeerLocationFindRequestPtr PeerToFinderPeerLocationFindRequest::create()
      {
        return internal::PeerToFinderPeerLocationFindRequest::create();
      }

      namespace internal
      {
        PeerToFinderPeerLocationFindRequestPtr PeerToFinderPeerLocationFindRequest::create(ElementPtr root)
        {
          PeerToFinderPeerLocationFindRequestPtr ret(new message::PeerToFinderPeerLocationFindRequest);

          if (root)
          {
            ret->mID = IMessageHelper::getAttributeID(root);

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

            ElementPtr exclude = root->findFirstChildElement("exclude");
            if (exclude)
            {
              ElementPtr locations = root->findFirstChildElement("locations");
              if (locations)
              {
                message::PeerToFinderPeerLocationFindRequest::ExcludedLocationList exclLst;
                ElementPtr loc = locations->findFirstChildElement("location");
                while (loc)
                {
                  String id = IMessageHelper::getAttributeID(loc);
                  exclLst.push_back(id);

                  loc = loc->getNextSiblingElement();
                }

                if (exclLst.size() > 0)
                  ret->mExcludedLocations = exclLst;
              }
            }

            ElementPtr location = root->findFirstChildElement("location");
            if (location)
            {
              ret->mLocation = MessageHelper::createLocation(location);
            }

          }

          return ret;
        }

        DocumentPtr PeerToFinderPeerLocationFindRequest::encode(message::PeerToFinderPeerLocationFindRequest &msg)
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(msg);
          ElementPtr root = ret->getFirstChildElement();
          ElementPtr loc, candidates, locations, exclude, routes;

          if (msg.hasAttribute(message::PeerToFinderPeerLocationFindRequest::AttributeType_RemoteContactID))
          {
            root->adoptAsLastChild(IMessageHelper::createElementWithID("contact", msg.mRemoteContactID));
          }

          if (msg.hasAttribute(message::PeerToFinderPeerLocationFindRequest::AttributeType_ExcludedLocations))
          {
            exclude = IMessageHelper::createElement("exclude");
            locations = IMessageHelper::createElement("locations");
            exclude->adoptAsLastChild(locations);
            root->adoptAsLastChild(exclude);

            message::PeerToFinderPeerLocationFindRequest::ExcludedLocationList::const_iterator it;
            for(it = msg.mExcludedLocations.begin(); it != msg.mExcludedLocations.end(); ++it)
            {
              String location(*it);
              locations->adoptAsLastChild(IMessageHelper::createElementWithID("location", location));
            }
          }

          if (msg.hasAttribute(message::PeerToFinderPeerLocationFindRequest::AttributeType_Location))
          {
            loc = MessageHelper::createElement(msg.mLocation);
            root->adoptAsLastChild(loc);

            if (msg.hasAttribute(message::PeerToFinderPeerLocationFindRequest::AttributeType_LocalContactID))
            {
              loc->adoptAsLastChild(IMessageHelper::createElementWithID("contact", msg.mLocalContactID));
            }
          }

          if (msg.hasAttribute(message::PeerToFinderPeerLocationFindRequest::AttributeType_Routes))
          {
            routes = IMessageHelper::createElement("routes");
            root->adoptAsLastChild(routes);

            RouteList::const_iterator it;
            for(it = msg.mRoutes.begin(); it != msg.mRoutes.end(); ++it)
            {
              String route(*it);
              routes->adoptAsLastChild(IMessageHelper::createElementWithID("route", route));
            }
          }

          return ret;
        }

      }
    }
  }
}
