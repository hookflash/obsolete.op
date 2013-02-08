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
        class PrivatePeerFileGetResult : public MessageResult
        {
        public:
          enum AttributeTypes
          {
            AttributeType_PrivatePeer = AttributeType_Last + 1,
          };

        public:
          static PrivatePeerFileGetResultPtr convert(MessagePtr message);

          static PrivatePeerFileGetResultPtr create(
                                                    ElementPtr root,
                                                    IMessageSourcePtr messageSource
                                                    );

          virtual Methods method() const                  {return (Message::Methods)MessageFactoryPeerContact::Method_PrivatePeerFileGet;}

          virtual IMessageFactoryPtr factory() const      {return MessageFactoryPeerContact::singleton();}

          bool hasAttribute(AttributeTypes type) const;

          const ElementPtr &privatePeer() const           {return mPrivatePeerEl;}
          void privatePeer(ElementPtr val);

        protected:
          PrivatePeerFileGetResult();

          ElementPtr mPrivatePeerEl;
        };
      }
    }
  }
}
