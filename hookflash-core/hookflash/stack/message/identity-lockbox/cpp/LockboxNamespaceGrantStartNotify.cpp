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

#include <hookflash/stack/message/identity-lockbox/LockboxNamespaceGrantStartNotify.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/internal/stack_Stack.h>
#include <hookflash/stack/IHelper.h>

#include <zsLib/XML.h>
#include <zsLib/helpers.h>

#define HOOKFLASH_STACK_MESSAGE_LOCKBOX_NAMESPACE_GRANT_START_UPDATE_REQUEST_EXPIRES_TIME_IN_SECONDS ((60*60)*24)

namespace hookflash { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(hookflash_stack_message) } } }

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace identity_lockbox
      {
        using zsLib::Seconds;
        using internal::MessageHelper;
        using stack::internal::IStackForInternal;

        //---------------------------------------------------------------------
        const char *LockboxNamespaceGrantStartNotify::toString(BrowserVisibilities visibility)
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
        LockboxNamespaceGrantStartNotifyPtr LockboxNamespaceGrantStartNotify::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<LockboxNamespaceGrantStartNotify>(message);
        }

        //---------------------------------------------------------------------
        LockboxNamespaceGrantStartNotify::LockboxNamespaceGrantStartNotify() :
          mVisibility(BrowserVisibility_NA),
          mPopup(-1)
        {
        }

        //---------------------------------------------------------------------
        LockboxNamespaceGrantStartNotifyPtr LockboxNamespaceGrantStartNotify::create()
        {
          LockboxNamespaceGrantStartNotifyPtr ret(new LockboxNamespaceGrantStartNotify);
          return ret;
        }

        //---------------------------------------------------------------------
        bool LockboxNamespaceGrantStartNotify::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_AgentInfo:         return mAgentInfo.hasData();
            case AttributeType_LockboxInfo:       return mLockboxInfo.hasData();
            case AttributeType_GrantID:           return mGrantID.hasData();
            case AttributeType_NamespaceURLs:     return (mNamespaceURLs.size() > 0);
            case AttributeType_BrowserVisibility: return (BrowserVisibility_NA != mVisibility);
            case AttributeType_BrowserPopup:      return (mPopup >= 0);
            case AttributeType_OuterFrameURL:     return mOuterFrameURL.hasData();

            default:                              break;
          }
          return false;
        }

        //---------------------------------------------------------------------
        DocumentPtr LockboxNamespaceGrantStartNotify::encode()
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(*this);
          ElementPtr root = ret->getFirstChildElement();

          ElementPtr browserEl = Element::create("browser");

          String clientNonce = IHelper::randomString(32);

          LockboxInfo lockboxInfo;

          AgentInfo agentInfo;
          agentInfo.mUserAgent = IStackForInternal::userAgent();
          agentInfo.mName = IStackForInternal::appName();
          agentInfo.mImageURL = IStackForInternal::appImageURL();

          agentInfo.mergeFrom(mAgentInfo, true);

          lockboxInfo.mAccessToken = mLockboxInfo.mAccessToken;
          if (mLockboxInfo.mAccessSecret.hasData()) {
            lockboxInfo.mAccessSecretProofExpires = zsLib::now() + Seconds(HOOKFLASH_STACK_MESSAGE_LOCKBOX_NAMESPACE_GRANT_START_UPDATE_REQUEST_EXPIRES_TIME_IN_SECONDS);
            lockboxInfo.mAccessSecretProof = IHelper::convertToHex(*IHelper::hmac(*IHelper::hmacKey(mLockboxInfo.mAccessSecret), "lockbox-access-validate:" + clientNonce + ":" + IMessageHelper::timeToString(lockboxInfo.mAccessSecretProofExpires) + ":" + lockboxInfo.mAccessToken + ":lockbox-permission-grant"));
          }

          if (mAgentInfo.hasData()) {
            root->adoptAsLastChild(MessageHelper::createElement(mAgentInfo));
          }

          if (lockboxInfo.hasData()) {
            root->adoptAsLastChild(MessageHelper::createElement(lockboxInfo));
          }

          if (hasAttribute(AttributeType_GrantID)) {
            ElementPtr grantEl = IMessageHelper::createElementWithID("grant", mGrantID);

            ElementPtr namespacesEl = IMessageHelper::createElement("namespaces");

            for (NamespaceURLList::iterator iter = mNamespaceURLs.begin(); iter != mNamespaceURLs.end(); ++iter)
            {
              const NamespaceURL &namespaceURL = (*iter);
              namespacesEl->adoptAsLastChild(IMessageHelper::createElementWithID("namespace", namespaceURL));
            }

            if (namespacesEl->hasChildren()) {
              grantEl->adoptAsLastChild(namespacesEl);
            }

            root->adoptAsLastChild(grantEl);
          }

          if (hasAttribute(AttributeType_BrowserVisibility)) {
            browserEl->adoptAsLastChild(IMessageHelper::createElementWithText("visbility", toString(mVisibility)));
          }

          if (hasAttribute(AttributeType_BrowserPopup)) {
            browserEl->adoptAsLastChild(IMessageHelper::createElementWithText("popup", (mPopup ? "allow" : "deny")));
          }

          if (hasAttribute(AttributeType_OuterFrameURL)) {
            browserEl->adoptAsLastChild(IMessageHelper::createElementWithText("outerFrameURL", mOuterFrameURL));
          }

          if (browserEl->hasChildren()) {
            root->adoptAsLastChild(browserEl);
          }

          return ret;
        }
      }
    }
  }
}
