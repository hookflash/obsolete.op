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

#include <openpeer/stack/types.h>
#include <openpeer/stack/message/types.h>
#include <openpeer/stack/message/MessageResult.h>
#include <openpeer/services/IHTTP.h>


namespace openpeer
{
  namespace stack
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IMessageMonitor
    #pragma mark

    interaction IMessageMonitor
    {
      static String toDebugString(IMessageMonitorPtr monitor, bool includeCommaPrefix = true);

      static IMessageMonitorPtr monitor(
                                        IMessageMonitorDelegatePtr delegate,
                                        message::MessagePtr requestMessage,
                                        Duration timeout
                                        );

      static IMessageMonitorPtr monitorAndSendToLocation(
                                                         IMessageMonitorDelegatePtr delegate,
                                                         ILocationPtr peerLocation,
                                                         message::MessagePtr message,
                                                         Duration timeout
                                                         );

      static IMessageMonitorPtr monitorAndSendToService(
                                                        IMessageMonitorDelegatePtr delegate,
                                                        IBootstrappedNetworkPtr bootstrappedNetwork,
                                                        const char *serviceType,
                                                        const char *serviceMethodName,
                                                        message::MessagePtr message,
                                                        Duration timeout
                                                        );

      static bool handleMessageReceived(message::MessagePtr message);

      virtual PUID getID() const = 0;

      virtual bool isComplete() const = 0;
      virtual bool wasHandled() const = 0;

      virtual void cancel() = 0;

      virtual String getMonitoredMessageID() = 0;
      virtual message::MessagePtr getMonitoredMessage() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IMessageMonitorDelegate
    #pragma mark

    interaction IMessageMonitorDelegate
    {
      virtual bool handleMessageMonitorMessageReceived(
                                                       IMessageMonitorPtr monitor,
                                                       message::MessagePtr message
                                                       ) = 0;

      virtual void onMessageMonitorTimedOut(IMessageMonitorPtr monitor) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IMessageMonitorResultDelegate
    #pragma mark

    template <typename typeMessage>
    interaction IMessageMonitorResultDelegate : public IMessageMonitorDelegate
    {
      typedef boost::shared_ptr<IMessageMonitorResultDelegate> Ptr;
      typedef boost::weak_ptr<IMessageMonitorResultDelegate> WeakPtr;

      typedef typeMessage MessageResultType;
      typedef boost::shared_ptr<MessageResultType> MessageResultTypePtr;
      typedef boost::weak_ptr<MessageResultType> MessageResultTypeWeakPtr;

      static Ptr convert(Ptr convertFrom) {return convertFrom;}

      IMessageMonitorResultDelegate() {}

      virtual bool handleMessageMonitorResultReceived(
                                                      IMessageMonitorPtr monitor,
                                                      MessageResultTypePtr result
                                                      ) {return false;}

      virtual bool handleMessageMonitorErrorResultReceived(
                                                           IMessageMonitorPtr monitor,
                                                           MessageResultTypePtr ignore, // will always be NULL
                                                           message::MessageResultPtr result
                                                           ) {return false;}

      virtual bool handleMessageMonitorUnknownResult(
                                                     IMessageMonitorPtr monitor,
                                                     MessageResultTypePtr ignore, // will always be NULL
                                                     message::MessagePtr message
                                                     ) {return false;}

      virtual void onMessageMonitorTimedOut(
                                           IMessageMonitorPtr monitor,
                                           MessageResultTypePtr response  // will always be NULL
                                           ) {}

    protected:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageMonitorResultDelegate => IMessageMonitorDelegate
      #pragma mark

      virtual bool handleMessageMonitorMessageReceived(
                                                       IMessageMonitorPtr monitor,
                                                       message::MessagePtr message
                                                       )
      {
        MessageResultTypePtr response = boost::dynamic_pointer_cast<MessageResultType>(message);
        if (response) {
          return handleMessageMonitorResultReceived(monitor, response);
        }
        message::MessageResultPtr result = boost::dynamic_pointer_cast<message::MessageResult>(message);
        if (result) {
          if (result->hasError()) {
            return handleMessageMonitorErrorResultReceived(monitor, MessageResultTypePtr(), result);
          }
        }
        return handleMessageMonitorUnknownResult(monitor, MessageResultTypePtr(), message);
      }

      virtual void onMessageMonitorTimedOut(IMessageMonitorPtr monitor)
      {
        if (!monitor->wasHandled()) {
          message::MessageResultPtr timeoutResult = message::MessageResult::create(monitor->getMonitoredMessage(), IHTTP::HTTPStatusCode_RequestTimeout, IHTTP::toString(IHTTP::HTTPStatusCode_RequestTimeout));
          handleMessageMonitorErrorResultReceived(monitor, MessageResultTypePtr(), timeoutResult);
        }
        onMessageMonitorTimedOut(monitor, MessageResultTypePtr());
      }

    protected:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageMonitorResultDelegate => (data)
      #pragma mark
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::stack::IMessageMonitorDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::stack::IMessageMonitorPtr, IMessageMonitorPtr)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::stack::message::MessagePtr, MessagePtr)
ZS_DECLARE_PROXY_METHOD_SYNC_RETURN_2(handleMessageMonitorMessageReceived, bool, IMessageMonitorPtr, MessagePtr)
ZS_DECLARE_PROXY_METHOD_1(onMessageMonitorTimedOut, IMessageMonitorPtr)
ZS_DECLARE_PROXY_END()
