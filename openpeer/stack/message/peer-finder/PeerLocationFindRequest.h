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

#include <openpeer/stack/message/MessageRequest.h>
#include <openpeer/stack/message/peer-finder/MessageFactoryPeerFinder.h>

#include <list>

namespace openpeer
{
  namespace stack
  {
    namespace message
    {
      namespace peer_finder
      {
        class PeerLocationFindRequest : public MessageRequest
        {
        public:
          friend class PeerLocationFindNotify;

          typedef Message::Methods Methods;
          typedef std::list<zsLib::String> ExcludedLocationList;

          enum AttributeTypes
          {
            AttributeType_RequestfindProofBundleDigestValue,  // filled when decoding from incoming request or after calling encode
            AttributeType_FindPeer,
            AttributeType_PeerSecret,                         // filled when decoding from incoming request and must be set when creating a request
            AttributeType_ExcludedLocations,
            AttributeType_LocationInfo,
            AttributeType_Routes,                             // filled when decoding from incoming request
            AttributeType_PeerFiles,
          };

        public:
          static PeerLocationFindRequestPtr convert(MessagePtr message);

          static PeerLocationFindRequestPtr create();
          static PeerLocationFindRequestPtr create(
                                                   ElementPtr root,
                                                   IMessageSourcePtr messageSource
                                                   );

          virtual DocumentPtr encode();

          virtual Methods method() const                                  {return (Message::Methods)MessageFactoryPeerFinder::Method_PeerLocationFind;}

          virtual IMessageFactoryPtr factory() const                      {return MessageFactoryPeerFinder::singleton();}

          bool hasAttribute(AttributeTypes type) const;

          const String &requestFindProofBundleDigestValue() const         {return mRequestfindProofBundleDigestValue;}
          void requestFindProofBundleDigestValue(const String &secret)    {mRequestfindProofBundleDigestValue = secret;}

          const IPeerPtr &findPeer() const                                {return mFindPeer;}
          void findPeer(const IPeerPtr &peer)                             {mFindPeer = peer;}

          const SecureByteBlockPtr &peerSecret() const                    {return mPeerSecret;}
          void peerSecret(const SecureByteBlockPtr &secret)               {mPeerSecret = secret;}

          const ExcludedLocationList &excludeLocations() const            {return mExcludedLocations;}
          void excludeLocations(const ExcludedLocationList &excludeList)  {mExcludedLocations = excludeList;}

          const RouteList &routes() const                                 {return mRoutes;}
          void routes(const RouteList &routes)                            {mRoutes = routes;}

          const LocationInfo &locationInfo() const                        {return mLocationInfo;}
          void locationInfo(const LocationInfo &location)                 {mLocationInfo = location;}

          IPeerFilesPtr peerFiles() const                                 {return mPeerFiles;}
          void peerFiles(IPeerFilesPtr peerFiles)                         {mPeerFiles = peerFiles;}

        protected:
          PeerLocationFindRequest();

          String mRequestfindProofBundleDigestValue;

          IPeerPtr mFindPeer;

          SecureByteBlockPtr mPeerSecret;

          ExcludedLocationList mExcludedLocations;

          RouteList mRoutes;
          LocationInfo mLocationInfo;

          IPeerFilesPtr mPeerFiles;
        };
      }
    }
  }
}
