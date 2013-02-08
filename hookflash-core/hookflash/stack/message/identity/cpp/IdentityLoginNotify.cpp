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

#include <hookflash/stack/message/identity/IdentityLoginNotify.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/IHelper.h>

#include <zsLib/XML.h>
#include <zsLib/helpers.h>

namespace hookflash { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(hookflash_stack_message) } } }

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace identity
      {
        using zsLib::Seconds;
        using internal::MessageHelper;

        //---------------------------------------------------------------------
        const char *IdentityLoginNotify::toString(BrowserVisibilities visibility)
        {
          switch (visibility) {
            case BrowserVisibility_NA:              return "";

            case BrowserVisibility_Hidden:          return "hidden";
            case BrowserVisibility_Visible:         return "visbile";
            case BrowserVisibility_VisibleOnDemand: return "visible-on-demand";
          }
          return "";
        }

        //---------------------------------------------------------------------
        IdentityLoginNotifyPtr IdentityLoginNotify::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<IdentityLoginNotify>(message);
        }

        //---------------------------------------------------------------------
        IdentityLoginNotify::IdentityLoginNotify() :
          mVisibility(BrowserVisibility_NA)
        {
        }

        //---------------------------------------------------------------------
        IdentityLoginNotifyPtr IdentityLoginNotify::create()
        {
          IdentityLoginNotifyPtr ret(new IdentityLoginNotify);
          return ret;
        }

        //---------------------------------------------------------------------
        bool IdentityLoginNotify::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_IdentityBase:              return !mIdentityInfo.mBase.isEmpty();
            case AttributeType_IdentityURI:               return !mIdentityInfo.mURI.isEmpty();
            case AttributeType_IdentityReloginAccessKey:  return !mIdentityInfo.mReloginAccessKeyEncrypted.isEmpty();
            case AttributeType_ClientToken:               return !mClientToken.isEmpty();
            case AttributeType_ServerToken:               return !mServerToken.isEmpty();
            case AttributeType_BrowserVisibility:         return (BrowserVisibility_NA != mVisibility);
            case AttributeType_PostLoginRedirectURL:      return !mPostLoginRedirectURL.isEmpty();
            case AttributeType_ClientLoginSecret:         return !mClientLoginSecret.isEmpty();
            default:                                      break;
          }
          return false;
        }

        //---------------------------------------------------------------------
        DocumentPtr IdentityLoginNotify::encode()
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(*this);
          ElementPtr root = ret->getFirstChildElement();

          IdentityInfo info;

          info.mBase = mIdentityInfo.mBase;
          info.mURI = mIdentityInfo.mURI;
          info.mReloginAccessKey = mIdentityInfo.mReloginAccessKey;

          if (info.hasData()) {
            root->adoptAsLastChild(MessageHelper::createElement(mIdentityInfo));
          }

          if (hasAttribute(AttributeType_ClientToken)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("clientToken", mClientToken));
          }
          if (hasAttribute(AttributeType_ServerToken)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("serverToken", mServerToken));
          }

          if (hasAttribute(AttributeType_BrowserVisibility)) {
            ElementPtr browserEl = Element::create("browser");
            browserEl->adoptAsLastChild(IMessageHelper::createElementWithText("visbility", toString(mVisibility)));
          }

          if (hasAttribute(AttributeType_PostLoginRedirectURL)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("postLoginRedirectURL", mPostLoginRedirectURL));
          }
          if (hasAttribute(AttributeType_ClientLoginSecret)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("clientLoginSecret", mClientLoginSecret));
          }

          return ret;
        }
      }
    }
  }
}
