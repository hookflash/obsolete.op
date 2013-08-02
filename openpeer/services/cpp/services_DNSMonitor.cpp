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

#include <openpeer/services/internal/services_DNSMonitor.h>
#include <openpeer/services/internal/services_Helper.h>
#include <zsLib/Exception.h>
#include <zsLib/Socket.h>
#include <zsLib/helpers.h>

namespace openpeer { namespace services { ZS_DECLARE_SUBSYSTEM(openpeer_services) } }


namespace openpeer
{
  namespace services
  {
    namespace internal
    {
      DNSMonitor::DNSMonitor(IMessageQueuePtr queue) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mCtx(NULL)
      {
      }

      void DNSMonitor::init()
      {
      }

      DNSMonitor::~DNSMonitor()
      {
        for (ResultMap::iterator iter = mOutstandingQueries.begin(); iter != mOutstandingQueries.end(); ++iter)
        {
          (*iter).second->cancel();
        }
        mOutstandingQueries.clear();

        cleanIfNoneOutstanding();
      }

      DNSMonitorPtr DNSMonitor::create(IMessageQueuePtr queue)
      {
        DNSMonitorPtr pThis(new DNSMonitor(queue));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      DNSMonitorPtr DNSMonitor::singleton()
      {
        AutoRecursiveLock lock(Helper::getGlobalLock());
        static DNSMonitorPtr monitor = DNSMonitor::create(Helper::getServiceQueue());
        return monitor;
      }

      void DNSMonitor::createDNSContext()
      {
        if (NULL != mCtx)
          return;

        // this has to be done first because on windows it is possible that dns_open will fail because socket initialize routine on windows won't be called otherwise
        mSocket = Socket::create();

        dns_reset(&dns_defctx);

        mCtx = dns_new(NULL);
        ZS_THROW_BAD_STATE_IF(NULL == mCtx)

        dns_init(mCtx, 0);  // do open ourselves...

        bool triedZero = false;

        int result = 0;
        for (int tries = 0; tries < 20; ++tries) {
          result = dns_open(mCtx);
          if (result < 0) {
            // try a different random port instead of a hard coded fixed port - application should find one hopefully within 20 tries
            if (!triedZero) {
              dns_set_opt(mCtx, DNS_OPT_PORT, 0);
              triedZero = true;
            } else
              dns_set_opt(mCtx, DNS_OPT_PORT, rand()%(65534-5000) + 5000);
          }
          else
            break;
        }
        if (result < 0) {
          dns_free(mCtx);
          mCtx = NULL;
        }

        mSocket->adopt((SOCKET)result);
        mSocket->setDelegate(mThisWeak.lock());

        mTimer = Timer::create(mThisWeak.lock(), Seconds(1), true);
      }

      void DNSMonitor::cleanIfNoneOutstanding()
      {
        if (mOutstandingQueries.size() > 0) return;   // still outstanding queries

        if (NULL == mCtx)
          return;

        dns_close(mCtx);
        dns_free(mCtx);
        mCtx = NULL;

        mSocket->orphan();
        mSocket.reset();

        mTimer->cancel();
        mTimer.reset();
      }

      void DNSMonitor::cancel(struct dns_query *query)
      {
        AutoRecursiveLock lock(mLock);

        if (!done(query)) // only cancel the DNS if it is a known DNS
          return;

        if (!mCtx)
          return;

        dns_cancel(mCtx, query);
      }

      bool DNSMonitor::done(struct dns_query *query)
      {
        AutoRecursiveLock lock(mLock);

        ResultMap::iterator iter = mOutstandingQueries.find((PTRNUMBER)query);
        if (iter != mOutstandingQueries.end()) {
          mOutstandingQueries.erase(iter);
          return true;
        }
        return false;
      }

      struct dns_query *DNSMonitor::submitAQuery(const char *name, int flags, IResultPtr result)
      {
        AutoRecursiveLock lock(mLock);
        createDNSContext();
        if (!mCtx) {
          result->cancel(); // this result is now bogus
          return NULL;
        }

        struct dns_query *query = dns_submit_a4(mCtx, name, flags, DNSMonitor::dns_query_a4, result.get());
        if (NULL != query) {
          mOutstandingQueries[((PTRNUMBER)query)] = result;
          result->setQuery(query);  // need to retain the query for later lookup
        } else {
          result->cancel();         // this result is now bogus since the query object could not be created
        }
        cleanIfNoneOutstanding();
        return query;
      }

      struct dns_query *DNSMonitor::submitAAAAQuery(const char *name, int flags, IResultPtr result)
      {
        AutoRecursiveLock lock(mLock);
        createDNSContext();
        if (!mCtx) {
          result->cancel(); // this result is now bogus
          return NULL;
        }

        struct dns_query *query = dns_submit_a6(mCtx, name, flags, DNSMonitor::dns_query_a6, result.get());
        if (NULL != query) {
          mOutstandingQueries[((PTRNUMBER)query)] = result;
          result->setQuery(query);  // need to retain the query for later lookup
        } else {
          result->cancel();         // this result is now bogus since the query object could not be created
        }
        cleanIfNoneOutstanding();
        return query;
      }

      struct dns_query *DNSMonitor::submitSRVQuery(const char *name, const char *service, const char *protocol, int flags, IResultPtr result)
      {
        AutoRecursiveLock lock(mLock);
        createDNSContext();
        if (!mCtx) {
          result->cancel(); // this result is now bogus
          return NULL;
        }

        struct dns_query *query = dns_submit_srv(mCtx, name, service, protocol, flags, DNSMonitor::dns_query_srv, result.get());
        if (NULL != query) {
          mOutstandingQueries[((PTRNUMBER)query)] = result;
          result->setQuery(query);  // need to retain the query for later lookup
        } else {
          result->cancel();         // this result is now bogus since the query object could not be created
        }
        cleanIfNoneOutstanding();
        return query;
      }

      void DNSMonitor::dns_query_a4(struct dns_ctx *ctx, struct dns_rr_a4 *record, void *data)
      {
        IResult *result = ((IResult *)data);

        DNSMonitorPtr monitor = result->getMonitor();
        if (!monitor) {
          // monitor was destroyed therefor object was already cancelled
          result->cancel();
          return;
        }

        // notify of the result
        result->onAResult(record);

        monitor->done(result->getQuery());  // monitor can now forget about this query
        result->done();                     // this object no longer has a hard reference to itself
      }

      void DNSMonitor::dns_query_a6(struct dns_ctx *ctx, struct dns_rr_a6 *record, void *data)
      {
        IResult *result = ((IResult *)data);

        DNSMonitorPtr monitor = result->getMonitor();
        if (!monitor) {
          // monitor was destroyed therefor object was already cancelled
          result->cancel();
          return;
        }

        // notify of the result
        result->onAAAAResult(record);

        monitor->done(result->getQuery()); // monitor can now forget about this query
        result->done();                     // this object no longer has a hard reference to itself
      }

      void DNSMonitor::dns_query_srv(struct dns_ctx *ctx, struct dns_rr_srv *record, void *data)
      {
        IResult *result = ((IResult *)data);

        DNSMonitorPtr monitor = result->getMonitor();
        if (!monitor) {
          // monitor was destroyed therefor object was already cancelled
          result->cancel();
          return;
        }

        // notify of the result
        result->onSRVResult(record);

        monitor->done(result->getQuery());  // monitor can now forget about this query
        result->done();                     // this object no longer has a hard reference to itself
      }

      void DNSMonitor::onReadReady(ISocketPtr socket)
      {
        AutoRecursiveLock lock(mLock);
        if (!mCtx)
          return;

        if (socket != mSocket) return;

        dns_ioevent(mCtx, 0);
        dns_timeouts(mCtx, -1, 0);
        mSocket->onReadReadyReset();

        cleanIfNoneOutstanding();
      }

      void DNSMonitor::onWriteReady(ISocketPtr socket)
      {
        // we can ignore the write ready, it only writes during a timeout event or during creation
      }

      void DNSMonitor::onException(ISocketPtr socket)
      {
        AutoRecursiveLock lock(mLock);
        if (NULL == mCtx)
          return;

        if (socket != mSocket) {
          ZS_LOG_WARNING(Detail, log("notified of exception on obsolete socket"))
          return;
        }

        for (ResultMap::iterator iter = mOutstandingQueries.begin(); iter != mOutstandingQueries.end(); ++iter)
        {
          (*iter).second->cancel();
        }
        mOutstandingQueries.clear();

        cleanIfNoneOutstanding();
      }

      void DNSMonitor::onTimer(TimerPtr timer)
      {
        AutoRecursiveLock lock(mLock);
        if (NULL == mCtx)
          return;

        dns_timeouts(mCtx, -1, 0);
        cleanIfNoneOutstanding();
      }

      String DNSMonitor::log(const char *message) const
      {
        return String("DNSMonitor [") + string(mID) + "] " + message;
      }
    }
  }
}
