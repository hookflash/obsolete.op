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
#include <openpeer/stack/message/identity/MessageFactoryIdentity.h>

#include <utility>
#include <list>

namespace openpeer
{
  namespace stack
  {
    namespace message
    {
      namespace identity
      {
        class IdentityLookupUpdateRequest : public MessageRequest
        {
        public:
          enum AttributeTypes
          {
            AttributeType_IdentityInfo,
          };

        public:
          static IdentityLookupUpdateRequestPtr convert(MessagePtr message);

          static IdentityLookupUpdateRequestPtr create();

          virtual DocumentPtr encode();

          virtual Methods method() const              {return (Message::Methods)MessageFactoryIdentity::Method_IdentityLookupUpdate;}

          virtual IMessageFactoryPtr factory() const  {return MessageFactoryIdentity::singleton();}

          bool hasAttribute(AttributeTypes type) const;

          // LockboxInfo
          //
          // mID
          // mAccessToken
          // mAccessSeecret
          
          const LockboxInfo &lockboxInfo() const      {return mLockboxInfo;}
          void lockboxInfo(const LockboxInfo &val)    {mLockboxInfo = val;}

          // IdentityInfo members need to be set:
          //
          // mURI
          // mProvider
          //
          // mAccessToken
          // mAccessSecret
          // mAccessSecretExpires

          // mStableID
          // mPeerFilePublic

          // mPriority
          // mWeight

          const IdentityInfo &identityInfo() const    {return mIdentityInfo;}
          void identityInfo(const IdentityInfo &val)  {mIdentityInfo = val;}

          IPeerFilesPtr peerFiles() const             {return mPeerFiles;}
          void peerFiles(IPeerFilesPtr peerFiles)     {mPeerFiles = peerFiles;}

        protected:
          IdentityLookupUpdateRequest();

          LockboxInfo mLockboxInfo;

          IdentityInfo mIdentityInfo;
          IPeerFilesPtr mPeerFiles;
        };
      }
    }
  }
}
