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

#pragma once

#include <hookflash/provisioning/message/MessageFactoryProvisioning.h>
#include <hookflash/stack/message/MessageRequest.h>

#include <hookflash/provisioning/IAccount.h>

namespace hookflash
{
  namespace provisioning
  {
    namespace message
    {
      namespace internal { class MultiPartyAPNSPushRequest;}

      class MultiPartyAPNSPushRequest : public stack::message::MessageRequest
      {
      public:
        typedef stack::IPeerFilesPtr IPeerFilesPtr;
        typedef stack::message::MessagePtr MessagePtr;
        typedef stack::message::IMessageFactoryPtr IMessageFactoryPtr;
        typedef provisioning::IAccount::UserIDList UserIDList;

        friend class internal::MultiPartyAPNSPushRequest;

        enum AttributeTypes
        {
          AttributeType_UserID,
          AttributeType_AccessKey,
          AttributeType_AccessSecret,

          AttributeType_LocationID,

          AttributeType_MessageType,
          AttributeType_MessageData,

          AttributeType_UserIDs,
        };

      public:
        static MultiPartyAPNSPushRequestPtr convert(MessagePtr message);

        static MultiPartyAPNSPushRequestPtr create();

        virtual DocumentPtr encode(IPeerFilesPtr peerFile = IPeerFilesPtr());

        virtual Methods method() const                {return (Message::Methods)MessageFactoryProvisioning::Method_MultiPartyAPNSPush;}

        virtual IMessageFactoryPtr factory() const    {return MessageFactoryProvisioning::singleton();}

        bool hasAttribute(AttributeTypes type) const;

        const String &userID() const                  {return mUserID;}
        void userID(const String &val)                {mUserID = val;}

        const String &accessKey() const               {return mAccessKey;}
        void accessKey(const String &val)             {mAccessKey = val;}

        const String &accessSecret() const            {return mAccessSecret;}
        void accessSecret(const String &val)          {mAccessSecret = val;}

        const String &locationID() const              {return mLocationID;}
        void locationID(const String &val)            {mLocationID = val;}

        const String &msgType() const                 {return mMessageType;}
        void msgType(const String &val)               {mMessageType = val;}

        const String &msgData() const                 {return mMessageData;}
        void msgData(const String &val)               {mMessageData = val;}

        const UserIDList &userIDs() const             {return mUserIDs;}
        void userIDs(const UserIDList &val)           {mUserIDs = val;}

      protected:
        MultiPartyAPNSPushRequest();

        String mUserID;
        String mAccessKey;
        String mAccessSecret;

        String mLocationID;

        String mMessageType;
        String mMessageData;

        UserIDList mUserIDs;
      };
    }
  }
}
