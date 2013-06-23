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

#include <hookflash/stack/internal/stack_MessageIncoming.h>
#include <hookflash/stack/internal/stack_Account.h>
#include <hookflash/stack/internal/stack_Location.h>
#include <hookflash/stack/internal/stack_Helper.h>
#include <hookflash/stack/message/Message.h>

#include <zsLib/Log.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageIncomingForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      MessageIncomingPtr IMessageIncomingForAccount::create(
                                                            AccountPtr account,
                                                            LocationPtr location,
                                                            message::MessagePtr message
                                                            )
      {
        return IMessageIncomingFactory::singleton().create(account, location, message);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageIncoming
      #pragma mark

      //-----------------------------------------------------------------------
      MessageIncoming::MessageIncoming(
                                       AccountPtr account,
                                       LocationPtr location,
                                       message::MessagePtr message
                                       ) :
        mID(zsLib::createPUID()),
        mAccount(account),
        mLocation(location),
        mMessage(message),
        mResponseSent(false)
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void MessageIncoming::init()
      {
      }

      //-----------------------------------------------------------------------
      MessageIncoming::~MessageIncoming()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))

        if (mResponseSent) {
          ZS_LOG_DEBUG(log("response already sent") + getDebugValueString())
          return;
        }

        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("automatic failure response cannot be sent as account gone") + getDebugValueString())
          return;
        }

        account->forMessageIncoming().notifyMessageIncomingResponseNotSent(*this);
      }

      //-----------------------------------------------------------------------
      MessageIncomingPtr MessageIncoming::convert(IMessageIncomingPtr messageIncoming)
      {
        return boost::dynamic_pointer_cast<MessageIncoming>(messageIncoming);
      }


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageIncoming => IMessageIncoming
      #pragma mark

      //-----------------------------------------------------------------------
      String MessageIncoming::toDebugString(IMessageIncomingPtr messageIncoming, bool includeCommaPrefix)
      {
        if (!messageIncoming) return String(includeCommaPrefix ? ", message incoming=(null)" : "message incoming=(null)");

        MessageIncomingPtr pThis = MessageIncoming::convert(messageIncoming);
        return pThis->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      ILocationPtr MessageIncoming::getLocation() const
      {
        return mLocation;
      }

      //-----------------------------------------------------------------------
      message::MessagePtr MessageIncoming::getMessage() const
      {
        return mMessage;
      }

      //-----------------------------------------------------------------------
      bool MessageIncoming::sendResponse(message::MessagePtr message)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!message)

        AutoRecursiveLock lock(getLock());

        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("failed to send incoming message response as account is gone") + getDebugValueString())
          return false;
        }
        bool sent = account->forMessageIncoming().send(mLocation, message);
        mResponseSent = mResponseSent ||  sent;
        return sent;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageIncoming => IMessageIncomingForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      MessageIncomingPtr MessageIncoming::create(
                                                 AccountPtr account,
                                                 LocationPtr location,
                                                 message::MessagePtr message
                                                 )
      {
        MessageIncomingPtr pThis(new MessageIncoming(account, location, message));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      LocationPtr MessageIncoming::getLocation(bool) const
      {
        return mLocation;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageIncoming => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &MessageIncoming::getLock() const
      {
        AccountPtr account = mAccount.lock();
        if (!account) return mBogusLock;
        return account->forPeer().getLock();
      }

      //-----------------------------------------------------------------------
      String MessageIncoming::log(const char *message) const
      {
        return String("MessageIncoming [" + Stringize<typeof(mID)>(mID).string() + "] " + message);
      }

      //-----------------------------------------------------------------------
      String MessageIncoming::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("message incoming id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               ILocation::toDebugString(mLocation) +
               Message::toDebugString(mMessage);
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    String IMessageIncoming::toDebugString(IMessageIncomingPtr messageIncoming, bool includeCommaPrefix)
    {
      return internal::MessageIncoming::toDebugString(messageIncoming, includeCommaPrefix);
    }

  }
}
