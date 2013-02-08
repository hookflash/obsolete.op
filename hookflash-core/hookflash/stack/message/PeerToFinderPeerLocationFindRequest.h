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

#include <hookflash/stack/message/hookflashTypes.h>
#include <hookflash/stack/message/MessageRequest.h>
#include <hookflash/services/IICESocket.h>
#include <hookflash/stack/message/MessageFactoryStack.h>

#include <list>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace internal { class PeerToFinderPeerLocationFindRequest; class PeerToFinderPeerLocationFindReply; }

      class PeerToFinderPeerLocationFindRequest : public MessageRequest
      {
      public:
        typedef zsLib::String String;
        typedef zsLib::XML::DocumentPtr DocumentPtr;

        friend class internal::PeerToFinderPeerLocationFindRequest;
        friend class internal::PeerToFinderPeerLocationFindReply;

        typedef Message::Methods Methods;
        typedef std::list<zsLib::String> ExcludedLocationList;

        enum AttributeTypes
        {
          AttributeType_Location,
          AttributeType_LocalContactID,
          AttributeType_RemoteContactID,
          AttributeType_Routes,
          AttributeType_ExcludedLocations
        };

      public:
        static PeerToFinderPeerLocationFindRequestPtr convert(MessagePtr message);

        static PeerToFinderPeerLocationFindRequestPtr create();

        virtual DocumentPtr encode(IPeerFilesPtr peerFile = IPeerFilesPtr());

        virtual Methods method() const                                  {return (Message::Methods)MessageFactoryStack::Method_PeerToFinder_PeerLocationFind;}

        virtual IMessageFactoryPtr factory() const                      {return MessageFactoryStack::singleton();}

        bool hasAttribute(AttributeTypes type) const;

        const String &localContactID() const                            {return mLocalContactID;}
        void localContactID(const String &val)                          {mLocalContactID = val;}

        const String &remoteContactID() const                           {return mRemoteContactID;}
        void remoteContactID(const String &val)                         {mRemoteContactID = val;}

        const ExcludedLocationList &excludeLocations() const            {return mExcludedLocations;}
        void excludeLocations(const ExcludedLocationList &excludeList)  {mExcludedLocations = excludeList;}

        const RouteList &routes() const                                 {return mRoutes;}
        void routes(const RouteList &routes)                            {mRoutes = routes;}

        const Location &location() const                                {return mLocation;}
        void location(const Location &location)                         {mLocation = location;}

      protected:
        PeerToFinderPeerLocationFindRequest();

        String mLocalContactID;
        String mRemoteContactID;
        ExcludedLocationList mExcludedLocations;
        RouteList mRoutes;
        Location mLocation;
      };
    }
  }
}
