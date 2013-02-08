/*
 
 Copyright (c) 2012, SMB Phone Inc.
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

#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/message/internal/stack_message_PeerDeleteRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerDeleteResult.h>

#include <zsLib/Numeric.h>
#include <zsLib/zsHelpers.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      typedef zsLib::XML::DocumentPtr DocumentPtr;
      typedef zsLib::XML::ElementPtr ElementPtr;

      PeerDeleteResultPtr PeerDeleteResult::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<PeerDeleteResult>(message);
      }

      PeerDeleteResult::PeerDeleteResult()
      {
      }

      PeerDeleteResultPtr PeerDeleteResult::create(PeerDeleteRequestPtr request)
      {
        return internal::PeerDeleteResult::create(request);
      }

      DocumentPtr PeerDeleteResult::encode(IPeerFilesPtr peerFile)
      {
        return internal::PeerDeleteResult::encode(*this);
      }

      bool PeerDeleteResult::hasAttribute(PeerDeleteResult::AttributeTypes type) const
      {
        switch (type)
        {
        }
        return MessageResult::hasAttribute((MessageResult::AttributeTypes)type);
      }

      namespace internal
      {
        PeerDeleteResultPtr PeerDeleteResult::create(ElementPtr root)
        {
          PeerDeleteResultPtr ret(new message::PeerDeleteResult);

          ret->mID = IMessageHelper::getAttributeID(root);
          ret->mTime = IMessageHelper::getAttributeEpoch(root);

          return ret;
        }

        PeerDeleteResultPtr PeerDeleteResult::create(PeerDeleteRequestPtr request)
        {
          PeerDeleteResultPtr ret(new message::PeerDeleteResult);

          ret->mID = request->mID;
          ret->mTime = zsLib::now();

          return ret;
        }

        DocumentPtr PeerDeleteResult::encode(message::PeerDeleteResult &msg)
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(msg);
          return ret;
        }
      }
    }
  }
}
