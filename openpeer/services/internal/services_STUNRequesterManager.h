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

#include <openpeer/services/internal/types.h>
#include <openpeer/services/ISTUNRequesterManager.h>

#include <map>
#include <utility>

namespace openpeer
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
      #pragma mark ISTUNRequesterManagerForSTUNRequester
      #pragma mark

      interaction ISTUNRequesterManagerForSTUNRequester
      {
        ISTUNRequesterManagerForSTUNRequester &forRequester() {return *this;}
        const ISTUNRequesterManagerForSTUNRequester &forRequester() const {return *this;}

        static STUNRequesterManagerPtr singleton();

        virtual void monitorStart(
                                  STUNRequesterPtr requester,
                                  STUNPacketPtr stunRequest
                                  ) = 0;
        virtual void monitorStop(STUNRequester *requester) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark STUNRequesterManager
      #pragma mark

      class STUNRequesterManager : public Noop,
                                   public ISTUNRequesterManager,
                                   public ISTUNRequesterManagerForSTUNRequester
      {
      public:
        friend interaction ISTUNRequesterManagerFactory;
        friend interaction ISTUNRequesterManager;
        friend interaction ISTUNRequesterManagerForSTUNRequester;

        typedef std::pair<QWORD, QWORD> QWORDPair;

      protected:
        STUNRequesterManager();
        
        STUNRequesterManager(Noop) : Noop(true) {};

      public:
        ~STUNRequesterManager();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark STUNRequesterManager => ISTUNRequesterManagerFactory
        #pragma mark

        static STUNRequesterManagerPtr create();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark STUNRequesterManager => ISTUNRequesterManager
        #pragma mark

        static STUNRequesterManagerPtr singleton();

        virtual ISTUNRequesterPtr handleSTUNPacket(
                                                   IPAddress fromIPAddress,
                                                   STUNPacketPtr stun
                                                   );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark STUNRequesterManager => ISTUNRequesterManagerForSTUNRequester
        #pragma mark

        // (duplicate) static STUNRequesterManagerPtr singleton();

        virtual void monitorStart(
                                  STUNRequesterPtr requester,
                                  STUNPacketPtr stunRequest
                                  );
        virtual void monitorStop(STUNRequester *requester);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark STUNRequesterManager => (internal)
        #pragma mark

        String log(const char *message) const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark STUNRequesterManager => (data)
        #pragma mark

        RecursiveLock mLock;
        PUID mID;
        STUNRequesterManagerWeakPtr mThisWeak;

        typedef std::pair<STUNRequesterWeakPtr, STUNRequester *> STUNRequesterPair;
        typedef std::map<QWORDPair, STUNRequesterPair> STUNRequesterMap;
        STUNRequesterMap mRequesters;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ISTUNRequesterManagerFactory
      #pragma mark

      interaction ISTUNRequesterManagerFactory
      {
        static ISTUNRequesterManagerFactory &singleton();

        virtual STUNRequesterManagerPtr createSTUNRequesterManager();
      };

    }
  }
}
