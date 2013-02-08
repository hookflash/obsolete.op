//
//  PeerContactLoginResult.h
//  hfstack
//
//  Created by Robin Raymond on 1/23/13.
//
//

#ifndef __hfstack__PeerContactLoginResult__
#define __hfstack__PeerContactLoginResult__

#include <iostream>

#endif /* defined(__hfstack__PeerContactLoginResult__) */
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
#include <hookflash/stack/message/peer-contact/MessageFactoryPeerContact.h>

#include <list>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace peer_contact
      {
        class PeerContactLoginResult : public MessageResult
        {
        public:
          enum AttributeTypes
          {
            AttributeType_ContactUserID = AttributeType_Last + 1,
            AttributeType_ContactAccessToken,
            AttributeType_ContactAccessSecret,
            AttributeType_ContactAccessExpires,
            AttributeType_PeerFilesRegenerate,
          };

        public:
          static PeerContactLoginResultPtr convert(MessagePtr message);

          static PeerContactLoginResultPtr create(
                                                  ElementPtr root,
                                                  IMessageSourcePtr messageSource
                                                  );

          virtual Methods method() const                  {return (Message::Methods)MessageFactoryPeerContact::Method_PeerContactLogin;}

          virtual IMessageFactoryPtr factory() const      {return MessageFactoryPeerContact::singleton();}

          bool hasAttribute(AttributeTypes type) const;

          const String &contactUserID() const             {return mContactUserID;}
          void contactUserID(const String &val)           {mContactUserID = val;}

          const String &contactAccessToken() const        {return mContactAccessToken;}
          void contactAccessToken(const String &val)      {mContactAccessToken = val;}

          const String &contactAccessSecret() const       {return mContactAccessSecret;}
          void contactAccessSecret(const String &val)     {mContactAccessSecret = val;}

          Time contactAccessExpires() const               {return mContactAccessExpires;}
          void contactAccessExpires(Time val)             {mContactAccessExpires = val;}

          bool peerFilesRegenerate() const                {return (mPeerFilesRegenerate > 0);}
          void peerFilesRegenerate(bool val)              {mPeerFilesRegenerate = (val ? 1 : 0);}

        protected:
          PeerContactLoginResult();

          String mContactUserID;
          String mContactAccessToken;
          String mContactAccessSecret;
          Time mContactAccessExpires;
          int mPeerFilesRegenerate;
        };
      }
    }
  }
}
