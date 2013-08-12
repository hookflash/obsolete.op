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


#include <zsLib/MessageQueueThread.h>
#include <zsLib/Exception.h>
#include <openpeer/services/IDNS.h>
#include <openpeer/services/ISTUNDiscovery.h>

#include <openpeer/services/internal/services_Factory.h>

#include <zsLib/Socket.h>

//#include <boost/test/unit_test_suite.hpp>
//#include <boost/test/unit_test.hpp>
//#include <boost/test/test_tools.hpp>

#include "config.h"
#include "boost_replacement.h"

#include <list>

namespace openpeer { namespace services { namespace test { ZS_DECLARE_SUBSYSTEM(openpeer_services_test) } } }

using zsLib::ULONG;
using openpeer::services::IDNS;
using openpeer::services::IDNSPtr;
using openpeer::services::IDNSQuery;
using openpeer::services::IDNSQueryPtr;

namespace openpeer
{
  namespace services
  {
    namespace test
    {
      class TestDNSFactory;
      typedef boost::shared_ptr<TestDNSFactory> TestDNSFactoryPtr;
      typedef boost::weak_ptr<TestDNSFactory> TestDNSFactoryWeakPtr;

      class TestDNSFactory : public services::internal::Factory
      {
      public:
        TestDNSFactory() :
          mACount(0),
          mAAAACount(0),
          mAorAAAACount(0),
          mSRVCount(0)
        {}

        virtual IDNSQueryPtr lookupA(
                                     IDNSDelegatePtr delegate,
                                     const char *name
                                     )
        {
          ++mACount;
          return services::internal::IDNSFactory::lookupA(delegate, name);
        }

        virtual IDNSQueryPtr lookupAAAA(
                                       IDNSDelegatePtr delegate,
                                       const char *name
                                       )
        {
          ++mAAAACount;
          return services::internal::IDNSFactory::lookupAAAA(delegate, name);
        }

        virtual IDNSQueryPtr lookupAorAAAA(
                                          IDNSDelegatePtr delegate,
                                          const char *name
                                          )
        {
          ++mAorAAAACount;
          return services::internal::IDNSFactory::lookupAorAAAA(delegate, name);
        }

        virtual IDNSQueryPtr lookupSRV(
                                       IDNSDelegatePtr delegate,
                                       const char *name,
                                       const char *service,                        // e.g. stun
                                       const char *protocol,                       // e.g. udp
                                       WORD defaultPort,
                                       WORD defaultPriority,
                                       WORD defaultWeight,
                                       SRVLookupTypes lookupType
                                       )
        {
          ++mSRVCount;
          return services::internal::IDNSFactory::lookupSRV(delegate, name, service, protocol, defaultPort, defaultPriority, defaultWeight, lookupType);
        }
        
        ULONG getACount() const {return mACount;}
        ULONG getAAAACount() const {return mAAAACount;}
        ULONG getAorAAAACount() const {return mAorAAAACount;}
        ULONG getSRVCount() const {return mSRVCount;}

      protected:
        ULONG mACount;
        ULONG mAAAACount;
        ULONG mAorAAAACount;
        ULONG mSRVCount;
      };

      class TestDNSCallback;
      typedef boost::shared_ptr<TestDNSCallback> TestDNSCallbackPtr;
      typedef boost::weak_ptr<TestDNSCallback> TestDNSCallbackWeakPtr;

      class TestDNSCallback : public zsLib::MessageQueueAssociator,
                              public IDNSDelegate
      {
      private:
        TestDNSCallback(zsLib::IMessageQueuePtr queue) : zsLib::MessageQueueAssociator(queue), mCount(0)
        {
        }

      public:
        static TestDNSCallbackPtr create(zsLib::IMessageQueuePtr queue)
        {
          return TestDNSCallbackPtr(new TestDNSCallback(queue));
        }

        virtual void onLookupCompleted(IDNSQueryPtr query)
        {
          zsLib::AutoLock lock(mLock);
          ++mCount;
          BOOST_CHECK(((bool)query));

          if (!query->hasResult()) {
            mFailedResults.push_back(query);
            return;
          }

          IDNS::AResultPtr resultA = query->getA();
          IDNS::AAAAResultPtr resultAAAA = query->getAAAA();
          IDNS::SRVResultPtr resultSRV = query->getSRV();
          if (resultA) {
            mAResults.push_back(std::pair<IDNSQueryPtr, IDNS::AResultPtr>(query, resultA));
          }
          if (resultAAAA) {
            mAAAAResults.push_back(std::pair<IDNSQueryPtr, IDNS::AAAAResultPtr>(query, resultAAAA));
          }
          if (resultSRV) {
            mSRVResults.push_back(std::pair<IDNSQueryPtr, IDNS::SRVResultPtr>(query, resultSRV));
          }
        }

        ~TestDNSCallback()
        {
        }

        ULONG getTotalProcessed() const
        {
          zsLib::AutoLock lock(mLock);
          return mCount;
        }

        ULONG getTotalFailed() const
        {
          zsLib::AutoLock lock(mLock);
          return mFailedResults.size();
        }

        ULONG getTotalAProcessed() const
        {
          zsLib::AutoLock lock(mLock);
          return mAResults.size();
        }

        ULONG getTotalAAAAProcessed() const
        {
          zsLib::AutoLock lock(mLock);
          return mAAAAResults.size();
        }

        ULONG getTotalSRVProcessed() const
        {
          zsLib::AutoLock lock(mLock);
          return mSRVResults.size();
        }

        bool isFailed(IDNSQueryPtr query) const
        {
          zsLib::AutoLock lock(mLock);
          for (size_t loop = 0; loop < mFailedResults.size(); ++loop) {
            if (query.get() == mFailedResults[loop].get())
              return true;
          }
          return false;
        }

        IDNS::AResultPtr getA(IDNSQueryPtr query) const
        {
          zsLib::AutoLock lock(mLock);
          for (size_t loop = 0; loop < mAResults.size(); ++loop) {
            if (query.get() == mAResults[loop].first.get())
              return mAResults[loop].second;
          }
          return IDNS::AResultPtr();
        }

        IDNS::AAAAResultPtr getAAAA(IDNSQueryPtr query) const
        {
          zsLib::AutoLock lock(mLock);
          for (size_t loop = 0; loop < mAAAAResults.size(); ++loop) {
            if (query.get() == mAAAAResults[loop].first.get())
              return mAAAAResults[loop].second;
          }
          return IDNS::AAAAResultPtr();
        }

        IDNS::SRVResultPtr getSRV(IDNSQueryPtr query) const
        {
          zsLib::AutoLock lock(mLock);
          for (size_t loop = 0; loop < mSRVResults.size(); ++loop) {
            if (query.get() == mSRVResults[loop].first.get())
              return mSRVResults[loop].second;
          }
          return IDNS::SRVResultPtr();
        }

      private:
        mutable zsLib::Lock mLock;

        ULONG mCount;

        std::vector<IDNSQueryPtr> mFailedResults;

        std::vector< std::pair<IDNSQueryPtr, IDNS::AResultPtr> > mAResults;
        std::vector< std::pair<IDNSQueryPtr, IDNS::AAAAResultPtr> > mAAAAResults;
        std::vector< std::pair<IDNSQueryPtr, IDNS::SRVResultPtr> > mSRVResults;
      };

    }
  }
}

using openpeer::services::test::TestDNSFactory;
using openpeer::services::test::TestDNSFactoryPtr;
using openpeer::services::test::TestDNSCallback;
using openpeer::services::test::TestDNSCallbackPtr;

void doTestDNS()
{
  if (!OPENPEER_SERVICE_TEST_DO_DNS_TEST) return;

  BOOST_INSTALL_LOGGER();

  boost::this_thread::sleep(zsLib::Seconds(1));

  TestDNSFactoryPtr overrideFactory(new TestDNSFactory);

  openpeer::services::internal::Factory::override(overrideFactory);

  zsLib::MessageQueueThreadPtr thread(zsLib::MessageQueueThread::createBasic());

  TestDNSCallbackPtr testObject = TestDNSCallback::create(thread);

  IDNSQueryPtr query = IDNS::lookupA(testObject, "www." OPENPEER_SERVICE_TEST_DNS_ZONE);
  IDNSQueryPtr query2 = IDNS::lookupA(testObject, "sip." OPENPEER_SERVICE_TEST_DNS_ZONE);
  IDNSQueryPtr query3 = IDNS::lookupAAAA(testObject, "unittest." OPENPEER_SERVICE_TEST_DNS_ZONE);
  IDNSQueryPtr query4 = IDNS::lookupAAAA(testObject, "unittest2." OPENPEER_SERVICE_TEST_DNS_ZONE);
  IDNSQueryPtr query5 = IDNS::lookupSRV(testObject, OPENPEER_SERVICE_TEST_DNS_ZONE, "sip", "udp", 5060, IDNS::SRVLookupType_LookupOnly);
  IDNSQueryPtr query6 = IDNS::lookupA(testObject, "bogusbogus." OPENPEER_SERVICE_TEST_DNS_ZONE);
  IDNSQueryPtr query7 = IDNS::lookupAorAAAA(testObject, "sip." OPENPEER_SERVICE_TEST_DNS_ZONE);
  IDNSQueryPtr query8 = IDNS::lookupAorAAAA(testObject, "unittest." OPENPEER_SERVICE_TEST_DNS_ZONE);
  IDNSQueryPtr query9 = IDNS::lookupAAAA(testObject, "bogusbogus." OPENPEER_SERVICE_TEST_DNS_ZONE);
  IDNSQueryPtr query10 = IDNS::lookupSRV(testObject, OPENPEER_SERVICE_TEST_DNS_ZONE, "sip", "udp");
  IDNSQueryPtr query11 = IDNS::lookupSRV(testObject, OPENPEER_SERVICE_TEST_DNS_ZONE, "stun", "udp");

  BOOST_STDOUT() << "WAITING:      Waiting for DNS lookup to resolve (max wait is 60 seconds).\n";

  ULONG expectingTotal = 0;
  expectingTotal += (query ? 1 : 0);
  expectingTotal += (query2 ? 1 : 0);
  expectingTotal += (query3 ? 1 : 0);
  expectingTotal += (query4 ? 1 : 0);
  expectingTotal += (query5 ? 1 : 0);
  expectingTotal += (query6 ? 1 : 0);
  expectingTotal += (query7 ? 1 : 0);
  expectingTotal += (query8 ? 1 : 0);
  expectingTotal += (query9 ? 1 : 0);
  expectingTotal += (query10 ? 1 : 0);
  expectingTotal += (query11 ? 1 : 0);

  ULONG matchingTotal = 0;

  // check to see if all DNS routines have resolved
  {
    ULONG lastResolved = 0;
    ULONG totalWait = 0;
    do
    {
      ULONG totalProcessed = matchingTotal = testObject->getTotalProcessed();
      if (totalProcessed != lastResolved) {
        lastResolved = totalProcessed;
        BOOST_STDOUT() << "WAITING:      [" << totalProcessed << "] Resolved ->  A [" << testObject->getTotalAProcessed() << "]  AAAA [" << testObject->getTotalAAAAProcessed() << "]   SRV [" << testObject->getTotalSRVProcessed() << "]  FAILED[" << testObject->getTotalFailed() << "]\n";
      }
      if (totalProcessed < expectingTotal) {
        ++totalWait;
        boost::this_thread::sleep(zsLib::Seconds(1));
      }
      else
        break;
    } while (totalWait < (60)); // max three minutes
    BOOST_EQUAL(matchingTotal, expectingTotal);
    BOOST_CHECK(totalWait < (60));
  }

  BOOST_STDOUT() << "WAITING:      All DNS queries have finished. Waiting for 'bogus' events to process (10 second wait).\n";
  boost::this_thread::sleep(zsLib::Seconds(10));

  BOOST_EQUAL(matchingTotal, testObject->getTotalProcessed());

  BOOST_EQUAL(8, overrideFactory->getACount())
  BOOST_EQUAL(8, overrideFactory->getAAAACount())
  BOOST_EQUAL(5, overrideFactory->getAorAAAACount())
  BOOST_EQUAL(6, overrideFactory->getSRVCount())

  IDNS::AResultPtr a1 = testObject->getA(query);
  IDNS::AResultPtr a2 = testObject->getA(query2);
  IDNS::AAAAResultPtr aaaa1 = testObject->getAAAA(query3);
  IDNS::AAAAResultPtr aaaa2 = testObject->getAAAA(query4);
  IDNS::SRVResultPtr srv1 = testObject->getSRV(query5);
  IDNS::AResultPtr a3 = testObject->getA(query7);
  IDNS::AAAAResultPtr aaaa3 = testObject->getAAAA(query8);
  IDNS::SRVResultPtr srv2 = testObject->getSRV(query10);
  IDNS::SRVResultPtr srv3 = testObject->getSRV(query11);

  // these should be valid
  BOOST_CHECK(a1)
  BOOST_CHECK(a2)
  BOOST_CHECK(aaaa1)
  BOOST_CHECK(aaaa2)
  BOOST_CHECK(srv1)
  BOOST_CHECK(a3)
  BOOST_CHECK(aaaa3)
  BOOST_CHECK(srv2)
  BOOST_CHECK(srv3)

  // these should be empty
  IDNS::AResultPtr a4 = testObject->getA(query8);
  IDNS::AAAAResultPtr aaaa4 = testObject->getAAAA(query7);
  IDNS::AResultPtr a5 = testObject->getA(query9);
  IDNS::AAAAResultPtr aaaa5 = testObject->getAAAA(query9);
  IDNS::AResultPtr a6 = testObject->getA(query6);
  IDNS::AResultPtr aaaa6 = testObject->getAAAA(query6);

  BOOST_CHECK(!a4)
  BOOST_CHECK(!aaaa4)

  BOOST_CHECK(!a5)
  BOOST_CHECK(OPENPEER_SERVICE_TEST_DNS_PROVIDER_RESOLVES_BOGUS_DNS_AAAA_RECORDS ? ((bool)aaaa5) : (!aaaa5))
  BOOST_CHECK(OPENPEER_SERVICE_TEST_DNS_PROVIDER_RESOLVES_BOGUS_DNS_A_RECORDS ? ((bool)a6) : (!a6))
  BOOST_CHECK(!aaaa6)

  if (a1) {
    // www.domain.com, 1800 TTL and IP = 199.204.138.90
    BOOST_CHECK(a1->mTTL <= 1800);
    BOOST_EQUAL(a1->mIPAddresses.front().string(), "199.204.138.90");
  }

  if (a2) {
    // sip.domain.com, 900, IP = 173.239.150.198
    BOOST_CHECK(a2->mTTL <= 900);
    BOOST_EQUAL(a2->mIPAddresses.front().string(), "173.239.150.198");
  }

  if (aaaa1) {
    // unittest.domain.com, 900, [2001:0:5ef5:79fb:8:fcb:a142:26ed]
    BOOST_CHECK(aaaa1->mTTL <= 900);
    BOOST_EQUAL(aaaa1->mIPAddresses.front().string(), zsLib::IPAddress("2001:0:5ef5:79fb:8:fcb:a142:26ed").string());
  }

  if (aaaa2) {
    // unittest2.domain.com, 900, fe80::2c71:60ff:fe00:1c54
    BOOST_CHECK(aaaa2->mTTL <= 900);
    BOOST_EQUAL(aaaa2->mIPAddresses.front().string(), zsLib::IPAddress("fe80::2c71:60ff:fe00:1c54").string());
  }

  if (srv1) {
    // _sip._udp.domain.com, 900, 10 0 5060 sip.
    BOOST_CHECK(srv1->mTTL <= 900);
    BOOST_EQUAL(srv1->mRecords.front().mPriority, 10);
    BOOST_EQUAL(srv1->mRecords.front().mWeight, 0);
    BOOST_EQUAL(srv1->mRecords.front().mPort, 5060);
    BOOST_EQUAL(srv1->mRecords.front().mName, "sip." OPENPEER_SERVICE_TEST_DNS_ZONE);
    BOOST_EQUAL(srv1->mRecords.front().mAResult->mIPAddresses.front().string(), zsLib::IPAddress("173.239.150.198:5060").string());
    BOOST_CHECK(!(srv1->mRecords.front().mAAAAResult));
  }

  if (a3) {
    // sip.domain.com 900, IP = 173.239.150.198
    BOOST_CHECK(a3->mTTL <= 900);
    BOOST_EQUAL(a3->mIPAddresses.front().string(), "173.239.150.198");
  }

  if (aaaa3) {
    // unittest.domain.com, 900, [2001:0:5ef5:79fb:8:fcb:a142:26ed]
    BOOST_CHECK(aaaa3->mTTL <= 900);
    BOOST_EQUAL(aaaa3->mIPAddresses.front().string(), zsLib::IPAddress("2001:0:5ef5:79fb:8:fcb:a142:26ed").string());
  }

  if (srv2) {
    // _sip._udp.domain.com, 900, 10 0 5060 sip.
    BOOST_CHECK(srv2->mTTL <= 900);
    BOOST_EQUAL(srv2->mRecords.front().mPriority, 10);
    BOOST_EQUAL(srv2->mRecords.front().mWeight, 0);
    BOOST_EQUAL(srv2->mRecords.front().mPort, 5060);
    BOOST_EQUAL(srv2->mRecords.front().mName, "sip." OPENPEER_SERVICE_TEST_DNS_ZONE);
    BOOST_CHECK(srv2->mRecords.front().mAResult->mTTL <= 900);
    BOOST_EQUAL(srv2->mRecords.front().mAResult->mIPAddresses.front().string(), "173.239.150.198:5060");
    BOOST_CHECK(!(srv2->mRecords.front().mAAAAResult));
  }

  const char *first = "stun." OPENPEER_SERVICE_TEST_DNS_ZONE;
  const char *second = "stun." OPENPEER_SERVICE_TEST_DNS_ZONE;

  const char *firstWIP = "216.93.246.14:3478";
  const char *secondWIP = "216.93.246.16:3478";

  if (srv3->mRecords.size() > 0) {
    if (srv3->mRecords.front().mAResult) {
      if (srv3->mRecords.front().mAResult->mIPAddresses.size() > 0) {
        if (srv3->mRecords.front().mAResult->mIPAddresses.front().string() == secondWIP) {
          const char *temp = first;
          const char *tempWIP = firstWIP;
          first = second;
          firstWIP = secondWIP;
          second = temp;
          secondWIP = tempWIP;
        }
      }
    }
  }

  IDNS::SRVResultPtr clone = IDNS::cloneSRV(srv3);  // keep a cloned copy
  IDNS::SRVResultPtr clone2 = IDNS::cloneSRV(srv3);  // keep a second cloned copy
  
  BOOST_CHECK(clone)
  BOOST_CHECK(clone2)

  if (srv3) {
    BOOST_CHECK(srv3->mTTL <= 900)
    BOOST_EQUAL(clone->mTTL, srv3->mTTL)

    BOOST_EQUAL(srv3->mRecords.size(), 1)
    BOOST_EQUAL(clone->mRecords.size(), srv3->mRecords.size())

    if (srv3->mRecords.size() > 0) {
      // _stun._udp.domain.com, 900, 10 0 3478 216.93.246.14 // order is unknown, could be either order
      // _stun._udp.domain.com, 900, 10 0 3478 216.93.246.16
      BOOST_EQUAL(srv3->mRecords.front().mPriority, 10);
      BOOST_EQUAL(srv3->mRecords.front().mWeight, 0);
      BOOST_EQUAL(srv3->mRecords.front().mPort, 3478);
      BOOST_EQUAL(srv3->mRecords.front().mName, first);

      BOOST_CHECK(srv3->mRecords.front().mAResult)

      if (srv3->mRecords.front().mAResult) {
        BOOST_CHECK(srv3->mRecords.front().mAResult->mTTL <= 900);
        BOOST_EQUAL(srv3->mRecords.front().mAResult->mIPAddresses.size(), 2)
        if (srv3->mRecords.front().mAResult->mIPAddresses.size() > 0) {
          BOOST_EQUAL(srv3->mRecords.front().mAResult->mIPAddresses.front().string(), firstWIP);
          srv3->mRecords.front().mAResult->mIPAddresses.pop_front();  // check the next record now
        }
        BOOST_CHECK(!(srv3->mRecords.front().mAAAAResult));

        if (srv3->mRecords.front().mAResult->mIPAddresses.size() > 0) {
          BOOST_EQUAL(srv3->mRecords.front().mAResult->mIPAddresses.front().string(), secondWIP);
        }
      }

      // test cloning of SRV record
      BOOST_EQUAL(clone->mRecords.front().mPriority, 10);
      BOOST_EQUAL(clone->mRecords.front().mWeight, 0);
      BOOST_EQUAL(clone->mRecords.front().mPort, 3478);
      BOOST_EQUAL(clone->mRecords.front().mName, first);

      BOOST_CHECK(clone->mRecords.front().mAResult)

      if (clone->mRecords.front().mAResult) {
        BOOST_CHECK(clone->mRecords.front().mAResult->mTTL <= 900);
        BOOST_EQUAL(clone->mRecords.front().mAResult->mIPAddresses.size(), 2)

        if (clone->mRecords.front().mAResult->mIPAddresses.size() > 0) {
          BOOST_EQUAL(clone->mRecords.front().mAResult->mIPAddresses.front().string(), firstWIP);
          clone->mRecords.front().mAResult->mIPAddresses.pop_front();  // check the next record now
        }
        BOOST_CHECK(!(clone->mRecords.front().mAAAAResult));
        if (clone->mRecords.front().mAResult->mIPAddresses.size() > 0) {
          BOOST_EQUAL(srv3->mRecords.front().mAResult->mIPAddresses.front().string(), secondWIP);
        }
      }
    }
  }

  // test extraction of SRV record
  bool extract = false;
  zsLib::IPAddress extractedIP;
  IDNS::AResultPtr extractedA;
  IDNS::AAAAResultPtr extractedAAAA;

  if (clone2) {
    extract = IDNS::extractNextIP(clone2, extractedIP, &extractedA, &extractedAAAA);
    BOOST_CHECK(extract);

    BOOST_EQUAL(extractedIP.string(), firstWIP);
    BOOST_CHECK(extractedA);
    BOOST_CHECK(!extractedAAAA);
    BOOST_EQUAL(extractedA->mName, first);

    extract = IDNS::extractNextIP(clone2, extractedIP, &extractedA, &extractedAAAA);
    BOOST_CHECK(extract);

    BOOST_EQUAL(extractedIP.string(), secondWIP);
    BOOST_CHECK(extractedA);
    BOOST_CHECK(!extractedAAAA);
    BOOST_EQUAL(extractedA->mName, second);

    extract = IDNS::extractNextIP(clone2, extractedIP, &extractedA, &extractedAAAA);
    BOOST_CHECK(!extract)
    BOOST_CHECK(!extractedA)
    BOOST_CHECK(!extractedAAAA)
  }

  if (!OPENPEER_SERVICE_TEST_DNS_PROVIDER_RESOLVES_BOGUS_DNS_A_RECORDS) {
    if (!testObject->isFailed(query6)) {
      BOOST_CHECK("This next DNS A lookup should have failed to resolve but it did resolve. Verify your provider's DNS is returning no IPs when resolving bogus A lookups; it should be but sometimes Internet providers give \"search\" page results for bogus DNS names")
    }
  }
  BOOST_CHECK(OPENPEER_SERVICE_TEST_DNS_PROVIDER_RESOLVES_BOGUS_DNS_A_RECORDS ? !testObject->isFailed(query6) : testObject->isFailed(query6));

  if (!OPENPEER_SERVICE_TEST_DNS_PROVIDER_RESOLVES_BOGUS_DNS_AAAA_RECORDS) {
    if (!testObject->isFailed(query9)) {
      BOOST_CHECK("This next DNS A or AAAA lookup should have failed to resolve but it did resolve. Verify your provider's DNS is returning no IPs when resolving bogus A or AAAA lookups; it should be but sometimes Internet providers give \"search\" page results for bogus DNS names")
    }
  }
  BOOST_CHECK(OPENPEER_SERVICE_TEST_DNS_PROVIDER_RESOLVES_BOGUS_DNS_AAAA_RECORDS ? !testObject->isFailed(query9) : testObject->isFailed(query9));

  // these tests should not have failed
  BOOST_CHECK(!testObject->isFailed(query));
  BOOST_CHECK(!testObject->isFailed(query2));
  BOOST_CHECK(!testObject->isFailed(query3));
  BOOST_CHECK(!testObject->isFailed(query4));
  BOOST_CHECK(!testObject->isFailed(query5));
  BOOST_CHECK(!testObject->isFailed(query7));
  BOOST_CHECK(!testObject->isFailed(query8));

  query.reset();
  query2.reset();
  query3.reset();
  query4.reset();
  query5.reset();
  query6.reset();
  query7.reset();
  query8.reset();
  query9.reset();
  query10.reset();
  query11.reset();
  testObject.reset();

  // wait for shutdown
  {
    ULONG count = 0;
    do
    {
      count = thread->getTotalUnprocessedMessages();
      if (0 != count)
        boost::this_thread::yield();
    } while (count > 0);

    thread->waitForShutdown();
  }
  BOOST_UNINSTALL_LOGGER()
  zsLib::proxyDump();
  BOOST_EQUAL(zsLib::proxyGetTotalConstructed(), 0);
}
