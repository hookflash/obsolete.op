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

#include <hookflash/stack/message/MessageNotify.h>
#include <hookflash/stack/message/identity/MessageFactoryIdentity.h>

#include <utility>
#include <list>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace identity
      {
        class IdentityLoginNotify : public MessageNotify
        {
        public:
          enum AttributeTypes
          {
            AttributeType_IdentityBase,
            AttributeType_IdentityURI,
            AttributeType_IdentityReloginAccessKey,
            AttributeType_ClientToken,
            AttributeType_ServerToken,
            AttributeType_BrowserVisibility,
            AttributeType_PostLoginRedirectURL,
            AttributeType_ClientLoginSecret,
          };

          enum BrowserVisibilities
          {
            BrowserVisibility_NA,

            BrowserVisibility_Hidden,
            BrowserVisibility_Visible,
            BrowserVisibility_VisibleOnDemand,
          };

          static const char *toString(BrowserVisibilities visibility);

        public:
          static IdentityLoginNotifyPtr convert(MessagePtr message);

          static IdentityLoginNotifyPtr create();

          virtual DocumentPtr encode();

          virtual Methods method() const                    {return (Message::Methods)MessageFactoryIdentity::Method_IdentityLoginNotify;}

          virtual IMessageFactoryPtr factory() const        {return MessageFactoryIdentity::singleton();}

          bool hasAttribute(AttributeTypes type) const;

          // IdentityInfo members expecting to be set
          //
          // mBase (either this or mURI or mReloginAccessKey)
          // mURI (either this or mBase or mReloginAccessKey)
          //
          // mReloginAccessKey (either this or mBase or mURI)

          const IdentityInfo &identityInfo() const          {return mIdentityInfo;}
          void identityInfo(const IdentityInfo &val)        {mIdentityInfo = val;}

          const String &clientToken() const                 {return mClientToken;}
          void clientToken(const String &val)               {mClientToken = val;}

          const String &serverToken() const                 {return mServerToken;}
          void serverToken(const String &val)               {mServerToken = val;}

          BrowserVisibilities browserVisibility() const     {return mVisibility;}
          void browserVisibility(BrowserVisibilities val)   {mVisibility = val;}

          const String &postLoginRedirectURL() const        {return mPostLoginRedirectURL;}
          void postLoginRedirectURL(const String &val)      {mPostLoginRedirectURL = val;}

          const String &clientLoginSecret() const           {return mClientLoginSecret;}
          void clientLoginSecret(const String &val)         {mClientLoginSecret = val;}

        protected:
          IdentityLoginNotify();

          IdentityInfo mIdentityInfo;

          String mClientToken;
          String mServerToken;

          BrowserVisibilities mVisibility;

          String mPostLoginRedirectURL;
          String mClientLoginSecret;
        };
      }
    }
  }
}
