/*
 *  Created by Robin Raymond.
 *  Copyright 2009-2013. Robin Raymond. All rights reserved.
 *
 * This file is part of zsLib.
 *
 * zsLib is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License (LGPL) as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * zsLib is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with zsLib; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <zsLib/Timer.h>
#include <zsLib/MessageQueueThread.h>

//#include <boost/test/unit_test_suite.hpp>
//#include <boost/test/unit_test.hpp>
//#include <boost/test/test_tools.hpp>

#include "boost_replacement.h"

using zsLib::ULONG;

class TestTimerCallback;
typedef boost::shared_ptr<TestTimerCallback> TestTimerCallbackPtr;

class TestTimerCallback : public zsLib::ITimerDelegate,
                          public zsLib::MessageQueueAssociator
{
private:
  TestTimerCallback(zsLib::IMessageQueuePtr queue) : zsLib::MessageQueueAssociator(queue), mCount(0)
  {
  }
public:
  static TestTimerCallbackPtr create(zsLib::IMessageQueuePtr queue)
  {
    return TestTimerCallbackPtr(new TestTimerCallback(queue));
  }

  virtual void onTimer(zsLib::TimerPtr timer)
  {
    ++mCount;
    std::cout << "ONTIMER:      " << ((zsLib::PTRNUMBER)timer.get()) << "\n";
  }

  ~TestTimerCallback()
  {
  }

public:
  ULONG mCount;
};



BOOST_AUTO_TEST_SUITE(zsLibTimer)

  BOOST_AUTO_TEST_CASE(TestTimer)
  {
    zsLib::MessageQueueThreadPtr thread(zsLib::MessageQueueThread::createBasic());

    TestTimerCallbackPtr testObject = TestTimerCallback::create(thread);
    TestTimerCallbackPtr testObject2 = TestTimerCallback::create(thread);
    TestTimerCallbackPtr testObject3 = TestTimerCallback::create(thread);
    TestTimerCallbackPtr testObject4 = TestTimerCallback::create(thread);

    zsLib::TimerPtr timer1(zsLib::Timer::create(testObject, zsLib::Seconds(3)));
    zsLib::TimerPtr timer2(zsLib::Timer::create(testObject2, zsLib::Seconds(1), false));
    zsLib::TimerPtr timer3(zsLib::Timer::create(testObject3, zsLib::Seconds(4), false));
    zsLib::TimerPtr timer4(zsLib::Timer::create(testObject4, zsLib::Seconds(4), false));

    timer3.reset();         // this should cause the timer to be cancelled as if it fell out of scope before it has a chance to fire
    timer4->background();   // this should cause the timer to not be cancelled (but it will cancel itself after being fired)
    timer4.reset();

    boost::this_thread::sleep(zsLib::Seconds(10));
    timer1->cancel();

    BOOST_EQUAL(testObject->mCount, 3);
    BOOST_EQUAL(testObject2->mCount, 1);
    BOOST_EQUAL(testObject3->mCount, 0);
    BOOST_EQUAL(testObject4->mCount, 1);

    std::cout << "WAITING:      To ensure the timers have truly stopped firing events.\n";
    boost::this_thread::sleep(zsLib::Seconds(10));

    timer1.reset();
    timer2.reset();
    timer3.reset();
    timer4.reset();

    BOOST_EQUAL(testObject->mCount, 3);
    BOOST_EQUAL(testObject2->mCount, 1);
    BOOST_EQUAL(testObject3->mCount, 0);
    BOOST_EQUAL(testObject4->mCount, 1);

    ULONG count = 0;
    do
    {
      count = thread->getTotalUnprocessedMessages();
      if (0 != count)
        boost::this_thread::yield();
    } while (count > 0);
    thread->waitForShutdown();

    BOOST_EQUAL(zsLib::proxyGetTotalConstructed(), 0);
  }

BOOST_AUTO_TEST_SUITE_END()
