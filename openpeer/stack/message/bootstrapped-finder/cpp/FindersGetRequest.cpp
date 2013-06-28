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

#include <openpeer/stack/message/bootstrapped-finder/FindersGetRequest.h>
#include <openpeer/stack/message/internal/stack_message_MessageHelper.h>

#include <zsLib/Stringize.h>
#include <zsLib/XML.h>

namespace openpeer
{
  namespace stack
  {
    namespace message
    {
      using zsLib::Stringize;

      namespace bootstrapped_finder
      {
        //---------------------------------------------------------------------
        FindersGetRequestPtr FindersGetRequest::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<FindersGetRequest>(message);
        }

        //---------------------------------------------------------------------
        FindersGetRequest::FindersGetRequest() :
          mTotalFinders(0)
        {
        }

        //---------------------------------------------------------------------
        FindersGetRequestPtr FindersGetRequest::create()
        {
          FindersGetRequestPtr ret(new FindersGetRequest);
          return ret;
        }

        //---------------------------------------------------------------------
        bool FindersGetRequest::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_TotalServers:  return (mTotalFinders > 0);
            default:                          break;
          }
          return false;
        }

        //---------------------------------------------------------------------
        DocumentPtr FindersGetRequest::encode()
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(*this);
          ElementPtr root = ret->getFirstChildElement();

          if (hasAttribute(AttributeType_TotalServers)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithNumber("servers", Stringize<typeof(mTotalFinders)>(mTotalFinders).string()));
          }
          return ret;
        }

      }
    }
  }
}
