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
#include <openpeer/stack/message/peer-to-peer/MessageFactoryPeerToPeer.h>

namespace openpeer
{
  namespace stack
  {
    namespace message
    {
      namespace peer_to_peer
      {
        class PeerIdentifyRequest : public MessageRequest
        {
        public:
          friend class PeerIdentifyResult;

        public:
          enum AttributeTypes
          {
            AttributeType_LocationInfo,
            AttributeType_FindSecret,
            AttributeType_PeerFilePublic,
            AttributeType_PeerFiles,
          };

        public:
          static PeerIdentifyRequestPtr convert(MessagePtr message);

          static PeerIdentifyRequestPtr create();

          static PeerIdentifyRequestPtr create(
                                               ElementPtr root,
                                               IMessageSourcePtr messageSource
                                               );

          virtual DocumentPtr encode();

          virtual Methods method() const                    {return (Message::Methods)MessageFactoryPeerToPeer::Method_PeerIdentify;}

          virtual IMessageFactoryPtr factory() const        {return MessageFactoryPeerToPeer::singleton();}

          bool hasAttribute(AttributeTypes type) const;

          const String &findSecret() const                  {return mFindSecret;}
          void findSecret(const String &val)                {mFindSecret = val;}

          const LocationInfo &locationInfo() const          {return mLocationInfo;}
          void location(const LocationInfo &locationInfo)   {mLocationInfo = locationInfo;}

          IPeerFilePublicPtr peerFilePublic() const         {return mPeerFilePublic;}
          void peerFilesPublic(IPeerFilePublicPtr val)      {mPeerFilePublic = val;}

          IPeerFilesPtr peerFiles() const                   {return mPeerFiles;}
          void peerFiles(IPeerFilesPtr peerFiles)           {mPeerFiles = peerFiles;}

        protected:
          PeerIdentifyRequest();

          LocationInfo mLocationInfo;

          String mFindSecret;

          IPeerFilePublicPtr mPeerFilePublic;

          IPeerFilesPtr mPeerFiles;
        };
      }
    }
  }
}
