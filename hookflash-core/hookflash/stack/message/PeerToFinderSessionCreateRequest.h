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

#include <hookflash/stack/message/hookflashTypes.h>
#include <hookflash/stack/message/MessageRequest.h>
#include <hookflash/stack/message/MessageFactoryStack.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace internal { class PeerToFinderSessionCreateRequest;}

      class PeerToFinderSessionCreateRequest : public MessageRequest
      {
      public:
        typedef zsLib::Time Time;

        friend class internal::PeerToFinderSessionCreateRequest;

        enum AttributeTypes
        {
          AttributeType_FinderID,
          AttributeType_ContactID,
          AttributeType_OneTimeKey,
          AttributeType_Expires,
          AttributeType_PeerFile,
          AttributeType_Location,
        };

      public:
        static PeerToFinderSessionCreateRequestPtr convert(MessagePtr message);

        static PeerToFinderSessionCreateRequestPtr create();

        virtual zsLib::XML::DocumentPtr encode(IPeerFilesPtr peerFile = IPeerFilesPtr());

        virtual Methods method() const              {return (Message::Methods)MessageFactoryStack::Method_PeerToFinder_SessionCreate;}

        virtual IMessageFactoryPtr factory() const  {return MessageFactoryStack::singleton();}

        const String &finderID() const              {return mFinderID;}
        void finderID(const String &val)            {mFinderID = val;}

        const String &contactID() const             {return mContactID;}
        void contactID(const String &val)           {mContactID = val;}

        Time expires() const                        {return mExpires;}
        void expires(Time val)                      {mExpires = val;}

        const Location &location()                  {return mLocation;}
        void location(const Location &val)          {mLocation = val;}

        const String &oneTimeKey() const            {return mOneTimeKey;}
        void oneTimeKey(const String &val)          {mOneTimeKey = val;}

        bool hasAttribute(AttributeTypes type) const;

      protected:
        PeerToFinderSessionCreateRequest();

        String mFinderID;
        String mContactID;
        Time mExpires;
        Location mLocation;

        IPeerFilesPtr mPeerFile;
        String mOneTimeKey;
      };
    }
  }
}
