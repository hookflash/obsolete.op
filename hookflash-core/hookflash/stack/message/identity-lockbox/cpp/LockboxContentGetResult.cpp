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

#include <hookflash/stack/message/identity-lockbox/LockboxContentGetResult.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>

#include <zsLib/XML.h>
#include <zsLib/helpers.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace identity_lockbox
      {
        using internal::MessageHelper;

        //---------------------------------------------------------------------
        LockboxContentGetResultPtr LockboxContentGetResult::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<LockboxContentGetResult>(message);
        }

        //---------------------------------------------------------------------
        LockboxContentGetResult::LockboxContentGetResult()
        {
        }

        //---------------------------------------------------------------------
        LockboxContentGetResultPtr LockboxContentGetResult::create(
                                                                   ElementPtr root,
                                                                   IMessageSourcePtr messageSource
                                                                   )
        {
          LockboxContentGetResultPtr ret(new LockboxContentGetResult);
          IMessageHelper::fill(*ret, root, messageSource);

          ElementPtr grantEl = root->findFirstChildElement("grant");
          if (grantEl) {
            ret->mGrantID = IMessageHelper::getAttributeID(grantEl);

            ElementPtr namespacesEl = grantEl->findFirstChildElement("namespaces");
            if (namespacesEl) {
              ElementPtr namespaceEl = namespacesEl->findFirstChildElement("namespace");
              while (namespaceEl) {
                String namespaceURL = IMessageHelper::getAttributeID(namespaceEl);
                if (namespaceURL.hasData()) {
                  NameValueMap values;

                  String updated = IMessageHelper::getAttribute(namespaceEl, "updated");
                  if (updated.hasData()) {
                    values["$updated"] = updated;
                  }

                  ElementPtr childEl = namespaceEl->getFirstChildElement();
                  while (childEl) {
                    String name = childEl->getValue();
                    String value = IMessageHelper::getElementTextAndDecode(childEl);
                    childEl = childEl->getNextSiblingElement();

                    if ((name.hasData()) &&
                        (value.hasData())) {
                      values[name] = value;
                    }
                  }

                  if (values.size() > 0) {
                    ret->mNamespaceURLNameValues[namespaceURL] = values;
                  }
                }
                namespaceEl = namespaceEl->findNextSiblingElement("namespace");
              }
            }
          }

          return ret;
        }

        //---------------------------------------------------------------------
        bool LockboxContentGetResult::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_GrantID:                 return mGrantID.hasData();
            case AttributeType_NamespaceURLNameValues:  return (mNamespaceURLNameValues.size() > 0);
            default:                                    break;
          }
          return MessageResult::hasAttribute((MessageResult::AttributeTypes)type);
        }
      }
    }
  }
}
