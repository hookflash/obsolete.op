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

#include <hookflash/services/internal/types.h>
#include <hookflash/services/ISTUNDiscovery.h>
#include <hookflash/services/IDNS.h>
#include <hookflash/services/ISTUNRequester.h>
#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/Timer.h>

namespace hookflash
{
  namespace services
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark STUNDiscovery
      #pragma mark

      class STUNDiscovery : public MessageQueueAssociator,
                            public ISTUNDiscovery,
                            public IDNSDelegate,
                            public ISTUNRequesterDelegate
      {
      public:
        friend interaction ISTUNDiscoveryFactory;

        typedef std::list<IPAddress> IPAddressList;

      protected:
        STUNDiscovery(
                      IMessageQueuePtr queue,
                      ISTUNDiscoveryDelegatePtr delegate
                      );

        void init(
                  IDNS::SRVResultPtr service,
                  const char *srvName
                  );

      public:
        ~STUNDiscovery();

      protected:

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark STUNDiscovery => ISTUNDiscovery
        #pragma mark

        static STUNDiscoveryPtr create(
                                       IMessageQueuePtr queue,
                                       ISTUNDiscoveryDelegatePtr delegate,
                                       IDNS::SRVResultPtr service
                                       );

        static STUNDiscoveryPtr create(
                                       IMessageQueuePtr queue,
                                       ISTUNDiscoveryDelegatePtr delegate,
                                       const char *srvName
                                       );
        
        virtual PUID getID() const {return mID;}

        virtual bool isComplete() const;

        virtual void cancel();

        virtual IPAddress getMappedAddress() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark STUNDiscovery => IDNSDelegate
        #pragma mark

        virtual void onLookupCompleted(IDNSQueryPtr query);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark STUNDiscovery => ISTUNRequesterDelegate
        #pragma mark

        virtual void onSTUNRequesterSendPacket(
                                               ISTUNRequesterPtr requester,
                                               IPAddress destination,
                                               boost::shared_array<BYTE> packet,
                                               ULONG packetLengthInBytes
                                               );

        virtual bool handleSTUNRequesterResponse(
                                                 ISTUNRequesterPtr requester,
                                                 IPAddress fromIPAddress,
                                                 STUNPacketPtr response
                                                 );

        virtual void onSTUNRequesterTimedOut(ISTUNRequesterPtr requester);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark STUNDiscovery => (internal)
        #pragma mark

        String log(const char *message) const;

        void step();
        bool hasContactedServerBefore(const IPAddress &server);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark STUNDiscovery => (data)
        #pragma mark

        mutable RecursiveLock mLock;
        STUNDiscoveryWeakPtr mThisWeak;

        PUID mID;

        IDNSQueryPtr mSRVQuery;
        IDNS::SRVResultPtr mSRVResult;

        ISTUNDiscoveryDelegatePtr mDelegate;
        ISTUNRequesterPtr mSTUNRequester;

        IPAddress mServer;
        IPAddress mMapppedAddress;

        IPAddressList mPreviouslyContactedServers;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ISTUNDiscoveryFactory
      #pragma mark

      interaction ISTUNDiscoveryFactory
      {
        static ISTUNDiscoveryFactory &singleton();

        virtual STUNDiscoveryPtr create(
                                        IMessageQueuePtr queue,
                                        ISTUNDiscoveryDelegatePtr delegate,
                                        IDNS::SRVResultPtr service
                                        );

        virtual STUNDiscoveryPtr create(
                                        IMessageQueuePtr queue,
                                        ISTUNDiscoveryDelegatePtr delegate,
                                        const char *srvName
                                        );

      };
      
    }
  }
}
