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

#include <openpeer/stack/message/peer-finder/SessionCreateResult.h>

#include <openpeer/stack/message/internal/stack_message_MessageHelper.h>

#include <openpeer/stack/internal/stack_Location.h>
#include <openpeer/stack/internal/stack_Peer.h>
#include <openpeer/stack/internal/stack_Account.h>

#include <openpeer/stack/IPeerFiles.h>
#include <openpeer/stack/IPeerFilePublic.h>
#include <openpeer/stack/IPeerFilePrivate.h>

#include <openpeer/services/IHelper.h>

#include <zsLib/XML.h>
#include <zsLib/helpers.h>

namespace openpeer { namespace stack { namespace message { ZS_DECLARE_SUBSYSTEM(openpeer_stack_message) } } }

namespace openpeer
{
  namespace stack
  {
    namespace message
    {
      using services::IHelper;
      using namespace stack::internal;
      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

      namespace peer_finder
      {
        //---------------------------------------------------------------------
        SessionCreateResultPtr SessionCreateResult::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<SessionCreateResult>(message);
        }

        //---------------------------------------------------------------------
        SessionCreateResult::SessionCreateResult()
        {
        }

        //---------------------------------------------------------------------
        SessionCreateResultPtr SessionCreateResult::create(
                                                           ElementPtr rootEl,
                                                           IMessageSourcePtr messageSource
                                                           )
        {
          SessionCreateResultPtr ret(new SessionCreateResult);
          IMessageHelper::fill(*ret, rootEl, messageSource);

          ElementPtr relayEl;

          try {
            ElementPtr serverProofBundleEl = rootEl->findFirstChildElementChecked("serverProofBundle");
            ElementPtr serverProofEl = serverProofBundleEl->findFirstChildElementChecked("serverProof");

            relayEl = serverProofEl->findFirstChildElementChecked("relay");
          } catch(CheckFailed &) {
            ZS_LOG_WARNING(Detail, "SessionCreateResult[] session create relay token missing")
            return SessionCreateResultPtr();
          }

          LocationPtr messageLocation = ILocationForMessages::convert(messageSource);
          if (!messageLocation) {
            ZS_LOG_ERROR(Detail, "SessionCreateResult [] message source was not a known location")
            return SessionCreateResultPtr();
          }

          AccountPtr account = messageLocation->forMessages().getAccount();
          if (!account) {
            ZS_LOG_ERROR(Detail, "SessionCreateResult [] account object is gone")
            return SessionCreateResultPtr();
          }

          IPeerFilesPtr peerFiles = account->forMessages().getPeerFiles();
          if (!peerFiles) {
            ZS_LOG_ERROR(Detail, "SessionCreateResult [] peer files not found in account")
            return SessionCreateResultPtr();
          }

          IPeerFilePrivatePtr peerFilePrivate = peerFiles->getPeerFilePrivate();
          if (!peerFilePrivate) {
            ZS_LOG_ERROR(Detail, "SessionCreateResult [] peer file private was null")
            return SessionCreateResultPtr();
          }

          IPeerFilePublicPtr peerFilePublic = peerFiles->getPeerFilePublic();
          if (!peerFilePublic) {
            ZS_LOG_ERROR(Detail, "SessionCreateResult [] peer file public was null")
            return SessionCreateResultPtr();
          }

#define WARNING_NEED_TO_VERIFY_SERVER_SIGNATURE 1
#define WARNING_NEED_TO_VERIFY_SERVER_SIGNATURE 2

          if (relayEl) {
            ret->mRelayAccessToken = IMessageHelper::getElementTextAndDecode(relayEl->findFirstChildElement("accessToken"));
            String accessSecretEncrypted = IMessageHelper::getElementTextAndDecode(relayEl->findFirstChildElement("accessSecretEncrypted"));
            if (accessSecretEncrypted.hasData()) {
              ret->mRelayAccessSecret = IHelper::convertToString(*peerFilePrivate->decrypt(*IHelper::convertFromBase64(accessSecretEncrypted)));
            } else {
              ret->mRelayAccessSecret = IMessageHelper::getElementTextAndDecode(relayEl->findFirstChildElement("accessSecret"));
            }
          }

          ret->mServerAgent = IMessageHelper::getElementTextAndDecode(rootEl->findFirstChildElement("server"));
          ret->mExpires = IHelper::stringToTime(IMessageHelper::getElementText(rootEl->findFirstChildElement("expires")));

          return ret;
        }

        //---------------------------------------------------------------------
        bool SessionCreateResult::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_RelayAccessToken:    return mRelayAccessToken.hasData();
            case AttributeType_RelayAccessSecret:   return mRelayAccessSecret.hasData();
            case AttributeType_ServerAgent:         return mServerAgent.hasData();
            case AttributeType_Expires:             return (Time() != mExpires);
            default:                                break;
          }
          return MessageResult::hasAttribute((MessageResult::AttributeTypes)type);
        }
      }
    }
  }
}
