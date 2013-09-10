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

#include <openpeer/stack/message/MessageNotify.h>
#include <openpeer/stack/message/peer-finder/MessageFactoryPeerFinder.h>

namespace openpeer
{
  namespace stack
  {
    namespace message
    {
      namespace peer_finder
      {
        class ChannelMapNotify : public MessageNotify
        {
        public:
          enum AttributeTypes
          {
            AttributeType_ChannelNumber,
            AttributeType_Nonce,
            AttributeType_LocalContext,
            AttributeType_RemoteContext,
            AttributeType_AccessToken,
            AttributeType_AccessSecretProof,
            AttributeType_AccessSecretProofExpires,
          };

          typedef DWORD ChannelNumber;

        public:
          static ChannelMapNotifyPtr convert(MessagePtr message);

          static ChannelMapNotifyPtr create(
                                            ElementPtr root,
                                            IMessageSourcePtr messageSource
                                            );

          virtual Methods method() const                                {return (Message::Methods)MessageFactoryPeerFinder::Method_ChannelMap;}
          virtual IMessageFactoryPtr factory() const                    {return MessageFactoryPeerFinder::singleton();}

          bool hasAttribute(AttributeTypes type) const;

          const ChannelNumber &channelNumber() const                    {return mChannelNumber;}
          void channelNumber(const ChannelNumber &value)                {mChannelNumber = value;}

          const String &nonce() const                                   {return mNonce;}
          void nonce(const String &value)                               {mNonce = value;}

          const String &localContext() const                            {return mLocalContext;}
          void localContext(const String &value)                        {mLocalContext = value;}

          const String &remoteContext() const                           {return mRemoteContext;}
          void remoteContext(const String &value)                       {mRemoteContext = value;}

          const String &relayAccessToken() const                        {return mRelayAccessToken;}
          void relayAccessToken(const String &value)                    {mRelayAccessToken = value;}

          const String &relayAccessSecretProof() const                  {return mRelayAccessSecretProof;}
          void relayAccessSecretProof(const String &value)              {mRelayAccessSecretProof = value;}

          const Time &relayAccessSecretProofExpires() const             {return mRelayAccessSecretProofExpires;}
          void relayAccessSecretProofExpires(const Time &value)         {mRelayAccessSecretProofExpires = value;}

        protected:
          ChannelMapNotify();

          ChannelNumber mChannelNumber;

          String mNonce;
          String mLocalContext;
          String mRemoteContext;
          String mRelayAccessToken;
          String mRelayAccessSecretProof;
          Time mRelayAccessSecretProofExpires;
        };
      }
    }
  }
}
