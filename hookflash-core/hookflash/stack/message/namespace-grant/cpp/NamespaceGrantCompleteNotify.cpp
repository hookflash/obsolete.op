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

#include <hookflash/stack/message/namespace-grant/NamespaceGrantCompleteNotify.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>

#include <zsLib/XML.h>
#include <zsLib/helpers.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace namespace_grant
      {
        using internal::MessageHelper;

        //---------------------------------------------------------------------
        NamespaceGrantCompleteNotifyPtr NamespaceGrantCompleteNotify::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<NamespaceGrantCompleteNotify>(message);
        }

        //---------------------------------------------------------------------
        NamespaceGrantCompleteNotify::NamespaceGrantCompleteNotify()
        {
        }

        //---------------------------------------------------------------------
        NamespaceGrantCompleteNotifyPtr NamespaceGrantCompleteNotify::create(
                                                                             ElementPtr rootEl,
                                                                             IMessageSourcePtr messageSource
                                                                             )
        {
          NamespaceGrantCompleteNotifyPtr ret(new NamespaceGrantCompleteNotify);
          IMessageHelper::fill(*ret, rootEl, messageSource);

          ret->mGrantInfo = MessageHelper::createGrant(rootEl->findFirstChildElement("grant"));
          ElementPtr grantEl = rootEl->findFirstChildElement("grant");

          ElementPtr namespacesEl = rootEl->findFirstChildElement("namespaces");
          if (namespacesEl) {
            ElementPtr namespaceEl = namespacesEl->findFirstChildElement("namespace");
            while (namespaceEl) {
              NamespaceInfo info;
              info.mURL = IMessageHelper::getAttributeID(namespaceEl);
              info.mLastUpdated = IMessageHelper::stringToTime(IMessageHelper::getAttribute(namespaceEl, "updated"));
              if (info.mURL.hasData()) {
                ret->mNamespaceInfos[info.mURL] = info;
              }
              namespaceEl = namespaceEl->findNextSiblingElement("namespace");
            }
          }

          return ret;
        }

        //---------------------------------------------------------------------
        bool NamespaceGrantCompleteNotify::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_GrantInfo:             return mGrantInfo.hasData();
            case AttributeType_NamespaceInfos:        return (mNamespaceInfos.size() > 0);
            default:                                  break;
          }
          return false;
        }

      }
    }
  }
}
