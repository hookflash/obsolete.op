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

#include <udns/udns.h>
#include <zsLib/ISocket.h>
#include <zsLib/Proxy.h>
#include <zsLib/Timer.h>

namespace openpeer
{
  namespace services
  {
    namespace internal
    {
      class DNSQuery;
      class DNSAQuery;
      class DNSAAAAQuery;
      class DNSSRVQuery;

      class DNSMonitor;
      typedef boost::shared_ptr<DNSMonitor> DNSMonitorPtr;
      typedef boost::weak_ptr<DNSMonitor> DNSMonitorWeakPtr;

      class DNSMonitor : public MessageQueueAssociator,
                         public ISocketDelegate,
                         public ITimerDelegate
      {
        friend class DNSQuery;
        friend class DNSAQuery;
        friend class DNSAAAAQuery;
        friend class DNSSRVQuery;

        interaction IResult
        {
          virtual void setQuery(struct dns_query *query) = 0;
          virtual struct dns_query *getQuery() = 0;

          virtual DNSMonitorPtr getMonitor() = 0;

          virtual void cancel() = 0;
          virtual void done() = 0;

          virtual void onAResult(struct dns_rr_a4 *record) = 0;
          virtual void onAAAAResult(struct dns_rr_a6 *record) = 0;
          virtual void onSRVResult(struct dns_rr_srv *record) = 0;
        };

        typedef boost::shared_ptr<IResult> IResultPtr;
        typedef boost::weak_ptr<IResult> IResultWeakPtr;

      protected:
        DNSMonitor(IMessageQueuePtr queue);
        void init();
        static DNSMonitorPtr create(IMessageQueuePtr queue);

      public:
        ~DNSMonitor();

        static DNSMonitorPtr singleton();

      protected:
        void createDNSContext();
        void cleanIfNoneOutstanding();
        void cancel(struct dns_query *query);
        bool done(struct dns_query *query);
        struct dns_query *submitAQuery(const char *name, int flags, IResultPtr result);
        struct dns_query *submitAAAAQuery(const char *name, int flags, IResultPtr result);
        struct dns_query *submitSRVQuery(const char *name, const char *service, const char *protocol, int flags, IResultPtr result);

        // UDNS callback routines
        static void dns_query_a4(struct dns_ctx *ctx, struct dns_rr_a4 *result, void *data);
        static void dns_query_a6(struct dns_ctx *ctx, struct dns_rr_a6 *result, void *data);
        static void dns_query_srv(struct dns_ctx *ctx, struct dns_rr_srv *result, void *data);

        // ISocketDelegate
        virtual void onReadReady(ISocketPtr socket);
        virtual void onWriteReady(ISocketPtr socket);
        virtual void onException(ISocketPtr socket);

        // ITimerDelegate
        virtual void onTimer(TimerPtr timer);

        // other
        String log(const char *message) const;

      private:
        PUID mID;

        RecursiveLock mLock;
        DNSMonitorWeakPtr mThisWeak;
        SocketPtr mSocket;
        TimerPtr mTimer;

        dns_ctx *mCtx;

        typedef std::map<PTRNUMBER, IResultPtr> ResultMap;
        ResultMap mOutstandingQueries;
      };
    }
  }
}
