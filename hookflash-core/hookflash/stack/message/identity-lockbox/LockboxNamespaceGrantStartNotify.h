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
#include <hookflash/stack/message/identity-lockbox/MessageFactoryIdentityLockbox.h>

#include <utility>
#include <list>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace identity_lockbox
      {
        class LockboxNamespaceGrantStartNotify : public MessageNotify
        {
        public:
          enum AttributeTypes
          {
            AttributeType_AgentInfo,
            AttributeType_LockboxInfo,
            AttributeType_GrantID,
            AttributeType_NamespaceInfos,
            AttributeType_BrowserVisibility,
            AttributeType_BrowserPopup,
            AttributeType_OuterFrameURL,
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
          static LockboxNamespaceGrantStartNotifyPtr convert(MessagePtr message);

          static LockboxNamespaceGrantStartNotifyPtr create();

          virtual DocumentPtr encode();

          virtual Methods method() const                    {return (Message::Methods)MessageFactoryIdentityLockbox::Method_LockboxNamespaceGrantStart;}

          virtual IMessageFactoryPtr factory() const        {return MessageFactoryIdentityLockbox::singleton();}

          bool hasAttribute(AttributeTypes type) const;

          const AgentInfo &agentInfo() const                {return mAgentInfo;}
          void agentInfo(const AgentInfo &val)              {mAgentInfo = val;}

          const LockboxInfo &lockboxInfo() const            {return mLockboxInfo;}
          void lockboxInfo(const LockboxInfo &val)          {mLockboxInfo = val;}

          const String &grantID() const                     {return mGrantID;}
          void grantID(const String &val)                   {mGrantID = val;}

          const NamespaceInfoMap &namespaceURLs() const     {return mNamespaceInfos;}
          void namespaceURLs(const NamespaceInfoMap &val)   {mNamespaceInfos = val;}

          BrowserVisibilities browserVisibility() const     {return mVisibility;}
          void browserVisibility(BrowserVisibilities val)   {mVisibility = val;}

          bool popup() const                                {return (mPopup > 0);}
          void popup(bool val)                              {mPopup = (val ? 1 : 0);}

          const String &outerFrameURL() const               {return mOuterFrameURL;}
          void outerFrameURL(const String &val)             {mOuterFrameURL = val;}

        protected:
          LockboxNamespaceGrantStartNotify();

          AgentInfo mAgentInfo;
          LockboxInfo mLockboxInfo;

          String mGrantID;
          NamespaceInfoMap mNamespaceInfos;

          BrowserVisibilities mVisibility;
          int mPopup;
          String mOuterFrameURL;
        };
      }
    }
  }
}
