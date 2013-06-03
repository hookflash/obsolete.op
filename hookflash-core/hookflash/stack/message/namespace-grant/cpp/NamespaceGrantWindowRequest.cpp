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

#include <hookflash/stack/message/namespace-grant/NamespaceGrantWindowRequest.h>
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
      namespace namespace_grant
      {
        using internal::MessageHelper;

        //---------------------------------------------------------------------
        NamespaceGrantWindowRequestPtr NamespaceGrantWindowRequest::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<NamespaceGrantWindowRequest>(message);
        }

        //---------------------------------------------------------------------
        NamespaceGrantWindowRequest::NamespaceGrantWindowRequest() :
          mReady(-1),
          mVisible(-1)
        {
        }

        //---------------------------------------------------------------------
        NamespaceGrantWindowRequestPtr NamespaceGrantWindowRequest::create(
                                                                                       ElementPtr root,
                                                                                       IMessageSourcePtr messageSource
                                                                                       )
        {
          NamespaceGrantWindowRequestPtr ret(new NamespaceGrantWindowRequest);
          MessageHelper::fill(*ret, root, messageSource);

          ElementPtr browserEl = root->findFirstChildElement("browser");
          if (browserEl) {
            String ready = IMessageHelper::getElementText(root->findFirstChildElement("ready"));
            if (!ready.isEmpty()) {
              ret->mReady = ("true" == ready ? 1 : 0);
            }
            String visibility = IMessageHelper::getElementText(root->findFirstChildElement("visibility"));
            if (!visibility.isEmpty()) {
              ret->mVisible = ("true" == visibility ? 1 : 0);
            }
          }
          return ret;
        }

        //---------------------------------------------------------------------
        bool NamespaceGrantWindowRequest::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_Ready:         return (mReady >= 0);
            case AttributeType_Visible:       return (mVisible >= 0);
            default:                          break;
          }
          return false;
        }

      }
    }
  }
}
