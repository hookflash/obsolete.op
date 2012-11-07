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
#include <hookflash/stack/message/MessageResult.h>

namespace hookflash
{
  namespace provisioning
  {
    namespace message
    {
      namespace internal { class ProfileGetResult;}

      class ProfileGetResult : public stack::message::MessageResult
      {
      public:
        typedef stack::message::MessagePtr MessagePtr;
        typedef stack::message::IMessageFactoryPtr IMessageFactoryPtr;
        typedef provisioning::IAccount::IdentityInfoList IdentityInfoList;

        friend class internal::ProfileGetResult;

        enum AttributeTypes
        {
          AttributeType_LastProfileUpdateTimestamp = AttributeType_Last + 1,

          AttributeType_ContactID,
          AttributeType_PrivatePeerFile,
          AttributeType_PublicPeerFile,

          AttributeType_Name,

          AttributeType_Profiles,
        };

      public:
        static ProfileGetResultPtr convert(MessagePtr message);

        virtual Methods method() const                {return (Message::Methods)MessageFactoryProvisioning::Method_ProfilePut;}

        virtual IMessageFactoryPtr factory() const    {return MessageFactoryProvisioning::singleton();}

        bool hasAttribute(AttributeTypes type) const;

        Time lastProfileUpdateTimestamp() const       {return mLastProfileUpdateTimestamp;}
        void lastProfileUpdateTimestamp(Time val)     {mLastProfileUpdateTimestamp = val;}

        const String &contactID() const               {return mContactID;}
        void contactID(const String &val)             {mContactID = val;}

        const String &privatePeerFile() const         {return mPrivatePeerFile;}
        void privatePeerFile(const String &val)       {mPrivatePeerFile = val;}

        const String &publicPeerFile() const          {return mPublicPeerFile;}
        void publicPeerFile(const String &val)        {mPublicPeerFile = val;}

        const String &name() const                    {return mName;}
        void name(const String &val)                  {mName = val;}

        const IdentityInfoList &profiles() const      {return mProfiles;}
        void profiles(const IdentityInfoList &val)    {mProfiles = val;}

      protected:
        ProfileGetResult();

        Time mLastProfileUpdateTimestamp;

        String mContactID;
        String mPrivatePeerFile;
        String mPublicPeerFile;

        String mName;

        IdentityInfoList mProfiles;
      };
    }
  }
}
