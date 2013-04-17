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

#include <hookflash/stack/message/MessageResult.h>
#include <hookflash/stack/message/identity/MessageFactoryIdentity.h>

#include <list>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace identity
      {
        class IdentityLoginCompleteResult : public MessageResult
        {
        public:
          enum AttributeTypes
          {
            AttributeType_IdentityInfo = AttributeType_Last + 1,
            AttributeType_Custom,
          };

        public:
          static IdentityLoginCompleteResultPtr convert(MessagePtr message);

          static IdentityLoginCompleteResultPtr create(
                                                       ElementPtr root,
                                                       IMessageSourcePtr messageSource
                                                       );

          virtual Methods method() const                  {return (Message::Methods)MessageFactoryIdentity::Method_IdentityLoginComplete;}

          virtual IMessageFactoryPtr factory() const      {return MessageFactoryIdentity::singleton();}

          bool hasAttribute(AttributeTypes type) const;

          // IdentityInfo members expected to be set in result
          //
          // mAccessToken
          // mAccessSecret
          // mAccessSecretExpires
          //
          // mURIEncrypted
          // mHash
          // mProvider
          //
          // mContact                         (if associated)
          // mContactFindSecret               (if associated)
          // mPrivatePeerFileSalt             (if associated)
          // mPrivatePeerFileSecretEncrypted  (if associated)
          //
          // mReloginAccessKeyEncrypted
          // mSecretSalt
          // mSecretEncrypted
          // mSecretDecryptionKeyEncrypted
          //
          // mUpdated
          // mPriority
          // mWeight

          const IdentityInfo &identityInfo() const        {return mIdentityInfo;}
          void identityInfo(const IdentityInfo &val)      {mIdentityInfo = val;}

          const ElementPtr &custom() const                {return mCustom;}
          void custom(const ElementPtr &val);

        protected:
          IdentityLoginCompleteResult();

          IdentityInfo mIdentityInfo;
          ElementPtr mCustom;
        };
      }
    }
  }
}
