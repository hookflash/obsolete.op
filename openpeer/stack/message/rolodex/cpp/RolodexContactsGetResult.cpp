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

#include <openpeer/stack/message/rolodex/RolodexContactsGetResult.h>
#include <openpeer/stack/message/internal/stack_message_MessageHelper.h>

#include <zsLib/XML.h>
#include <zsLib/helpers.h>
#include <zsLib/Log.h>

namespace openpeer { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(openpeer_stack_message) } } }

namespace openpeer
{
  namespace stack
  {
    namespace message
    {
      namespace rolodex
      {
        using internal::MessageHelper;

        //---------------------------------------------------------------------
        RolodexContactsGetResultPtr RolodexContactsGetResult::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<RolodexContactsGetResult>(message);
        }

        //---------------------------------------------------------------------
        RolodexContactsGetResult::RolodexContactsGetResult()
        {
        }

        //---------------------------------------------------------------------
        RolodexContactsGetResultPtr RolodexContactsGetResult::create(
                                                                     ElementPtr rootEl,
                                                                     IMessageSourcePtr messageSource
                                                                     )
        {
          RolodexContactsGetResultPtr ret(new RolodexContactsGetResult);
          IMessageHelper::fill(*ret, rootEl, messageSource);

          ElementPtr identitiesEl = rootEl->findFirstChildElement("identities");
          if (identitiesEl) {
            ElementPtr identityEl = identitiesEl->findFirstChildElement("identity");
            while (identityEl) {
              IdentityInfo info = MessageHelper::createIdentity(identityEl);
              if (info.hasData()) {

#define OPENPEER_STACK_MESSAGE_ROLODEX_IMPROPER_FACEBOOK_URL "identity://facebook/"
#define OPENPEER_STACK_MESSAGE_ROLODEX_ROPER_FACEBOOK_URL "identity://facebook.com/"

#define HORRIBLE_HACK_TO_PREVENT_INVALID_FORMED_FACEBOOK_URIS 1
                if (info.mURI.hasData()) {
                  if (0 == info.mURI.compare(0, strlen(OPENPEER_STACK_MESSAGE_ROLODEX_IMPROPER_FACEBOOK_URL), OPENPEER_STACK_MESSAGE_ROLODEX_IMPROPER_FACEBOOK_URL)) {
                    ZS_LOG_WARNING(Detail, String("RolodexContactsGetResult [") + ret->mID + "] Fixed imporer facebook identity URI. Please contact rolodex admin to repair rolodex service. URI=" + info.mURI)
                    info.mURI = OPENPEER_STACK_MESSAGE_ROLODEX_ROPER_FACEBOOK_URL + info.mURI.substr(strlen(OPENPEER_STACK_MESSAGE_ROLODEX_IMPROPER_FACEBOOK_URL));
                  }
                }
#define HORRIBLE_HACK_TO_PREVENT_INVALID_FORMED_FACEBOOK_URIS 2

                ret->mIdentities.push_back(info);
              }
              identityEl = identityEl->findNextSiblingElement("identity");
            }
          }

          ret->mRolodexInfo = MessageHelper::createRolodex(rootEl->findFirstChildElement("rolodex"));

          return ret;
        }

        //---------------------------------------------------------------------
        bool RolodexContactsGetResult::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_Identities:            return (mIdentities.size() > 0);
            case AttributeType_RolodexInfo:           return (mRolodexInfo.hasData());
            default:                                  break;
          }
          return MessageResult::hasAttribute((MessageResult::AttributeTypes)type);
        }
      }
    }
  }
}
