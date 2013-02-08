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

#include <hookflash/stack/IMessageIncoming.h>
#include <hookflash/stack/internal/types.h>

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageIncomingForAccount
      #pragma mark

      interaction IMessageIncomingForAccount
      {
        IMessageIncomingForAccount &forAccount() {return *this;}
        const IMessageIncomingForAccount &forAccount() const {return *this;}

        static MessageIncomingPtr create(
                                         AccountPtr account,
                                         LocationPtr location,
                                         message::MessagePtr message
                                         );

        virtual LocationPtr getLocation(bool internal = true) const = 0;
        virtual message::MessagePtr getMessage() const = 0;

        virtual String getDebugValueString(bool includeCommaPrefix = true) const = 0;
      };


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageIncoming
      #pragma mark

      class MessageIncoming : public IMessageIncoming,
                              public IMessageIncomingForAccount
      {
      public:
        friend interaction IMessageIncoming;
        friend interaction IMessageIncomingForAccount;

      protected:
        MessageIncoming(
                        AccountPtr account,
                        LocationPtr location,
                        message::MessagePtr message
                        );

        void init();

      public:
        ~MessageIncoming();

        static MessageIncomingPtr convert(IMessageIncomingPtr peer);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageIncoming => IMessageIncoming
        #pragma mark

        static String toDebugString(IMessageIncomingPtr messageIncoming, bool includeCommaPrefix = true);

        virtual PUID getID() const {return mID;}

        virtual ILocationPtr getLocation() const;
        virtual message::MessagePtr getMessage() const;

        virtual bool sendResponse(message::MessagePtr message);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageIncoming => IMessageIncomingForAccount
        #pragma mark

        // (duplicate) virtual ILocationPtr getLocation() const;
        // (duplicate) virtual message::MessagePtr getMessage() const;

        static MessageIncomingPtr create(
                                         AccountPtr account,
                                         LocationPtr location,
                                         message::MessagePtr message
                                         );

        virtual LocationPtr getLocation(bool) const;

        // (duplicate) virtual message::MessagePtr getMessage() const;

        // (duplicate) virtual String getDebugValueString(bool includeCommaPrefix = true) const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageIncoming => (internal)
        #pragma mark

        RecursiveLock &getLock() const;
        String log(const char *message) const;

        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MessageIncoming => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mBogusLock;
        MessageIncomingWeakPtr mThisWeak;

        AccountWeakPtr mAccount;
        LocationPtr mLocation;
        message::MessagePtr mMessage;

        bool mResponseSent;
      };
    }
  }
}
