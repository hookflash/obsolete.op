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

#include <openpeer/stack/message/peer-finder/ChannelMapNotify.h>
#include <openpeer/stack/message/internal/stack_message_MessageHelper.h>

#include <openpeer/services/IHelper.h>

#include <zsLib/XML.h>
#include <zsLib/helpers.h>
#include <zsLib/Numeric.h>

namespace openpeer { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(openpeer_stack_message) } } }

namespace openpeer
{
  namespace stack
  {
    namespace message
    {
      namespace peer_finder
      {
        using zsLib::Numeric;

        typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

        using services::IHelper;
        using message::internal::MessageHelper;

        //---------------------------------------------------------------------
        ChannelMapNotifyPtr ChannelMapNotify::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<ChannelMapNotify>(message);
        }

        //---------------------------------------------------------------------
        ChannelMapNotify::ChannelMapNotify()
        {
        }

        //---------------------------------------------------------------------
        ChannelMapNotifyPtr ChannelMapNotify::create(
                                                     ElementPtr root,
                                                     IMessageSourcePtr messageSource
                                                     )
        {
          ChannelMapNotifyPtr ret(new ChannelMapNotify);
          IMessageHelper::fill(*ret, root, messageSource);

          try {

            try {
              String channelNumber = IMessageHelper::getElementTextAndDecode(root->findFirstChildElementChecked("channel"));
              ret->mChannelNumber = Numeric<ChannelNumber>(channelNumber);
            } catch(Numeric<ChannelNumber>::ValueOutOfRange &) {
              ZS_LOG_WARNING(Detail, "ChannelMapNotify [] missing channel number")
              return ChannelMapNotifyPtr();
            }

            ret->mNonce = IMessageHelper::getElementTextAndDecode(root->findFirstChildElementChecked("nonce"));

            ElementPtr relayEl = root->findFirstChildElementChecked("relay");

            ret->mLocalContext = IMessageHelper::getElementTextAndDecode(relayEl->findFirstChildElementChecked("localContext"));
            ret->mRemoteContext = IMessageHelper::getElementTextAndDecode(relayEl->findFirstChildElementChecked("remoteContext"));
            ret->mRelayAccessToken = IMessageHelper::getElementTextAndDecode(relayEl->findFirstChildElementChecked("accessToken"));
            ret->mRelayAccessSecretProof = IMessageHelper::getElementTextAndDecode(relayEl->findFirstChildElementChecked("accessSecretProof"));
            ret->mRelayAccessSecretProofExpires = IHelper::stringToTime(IMessageHelper::getElementTextAndDecode(relayEl->findFirstChildElementChecked("accessSecretProofExpires")));

          } catch(CheckFailed &) {
            ZS_LOG_WARNING(Detail, "ChannelMapNotify [] expected element is missing")
            return ChannelMapNotifyPtr();
          }

          return ret;
        }

        //---------------------------------------------------------------------
        bool ChannelMapNotify::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_ChannelNumber:             return (0 != mChannelNumber);
            case AttributeType_Nonce:                     return mNonce.hasData();
            case AttributeType_LocalContext:              return mLocalContext.hasData();
            case AttributeType_RemoteContext:             return mRemoteContext.hasData();
            case AttributeType_AccessToken:               return mRelayAccessToken.hasData();
            case AttributeType_AccessSecretProof:         return mRelayAccessSecretProof.hasData();
            case AttributeType_AccessSecretProofExpires:  return Time() != mRelayAccessSecretProofExpires;
          }
          return false;
        }


      }
    }
  }
}
