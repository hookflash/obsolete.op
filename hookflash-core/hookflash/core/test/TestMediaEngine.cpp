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

#include "TestMediaEngine.h"
#include "config.h"
#include "boost_replacement.h"

namespace hookflash { namespace core { namespace test { ZS_DECLARE_SUBSYSTEM(hookflash_core_test) } } }

namespace hookflash { namespace core { namespace test { ZS_IMPLEMENT_SUBSYSTEM(hookflash_core_test) } } }

//#define HOOKFLASH_MEDIA_ENGINE_DEBUG_LOG_LEVEL
//#define HOOKFLASH_MEDIA_ENGINE_ENABLE_TIMER

namespace hookflash
{
  namespace core
  {
    namespace test
    {
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark TestMediaEngine
      #pragma mark
      
      //-----------------------------------------------------------------------
      TestMediaEngine::~TestMediaEngine()
      {
        destroyMediaEngine();
      }
      
      //-----------------------------------------------------------------------
      String TestMediaEngine::log(const char *message) const
      {
        return String("TestMediaEngine [") + zsLib::Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      internal::MediaEnginePtr TestMediaEngine::create(IMediaEngineDelegatePtr delegate)
      {
        TestMediaEnginePtr pThis(new TestMediaEngine());
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void TestMediaEngine::setLogLevel()
      {
#ifndef HOOKFLASH_MEDIA_ENGINE_DEBUG_LOG_LEVEL
        ILogger::setLogLevel("hookflash_webrtc", ILogger::Basic);
#else
        ILogger::setLogLevel("hookflash_webrtc", ILogger::Debug);
#endif
      }
      
      //-----------------------------------------------------------------------
      void TestMediaEngine::setReceiverAddress(String receiverAddress)
      {
        internal::AutoRecursiveLock lock(mLock);
        
        ZS_LOG_DEBUG(log("set receiver address - value: ") + receiverAddress)
        
        mReceiverAddress = receiverAddress;
      }
      
      //-----------------------------------------------------------------------
      String TestMediaEngine::getReceiverAddress() const
      {
        internal::AutoRecursiveLock lock(mLock);
        
        ZS_LOG_DEBUG(log("get receiver address - value: ") + mReceiverAddress)
        
        return mReceiverAddress;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark TestMediaEngine => IMediaEngineForCallTransport
      #pragma mark
      
      //-----------------------------------------------------------------------
      int TestMediaEngine::registerVoiceExternalTransport(Transport &transport)
      {
        ZS_THROW_INVALID_USAGE("external transport is disabled - cannot be registered")
      }
      
      //-----------------------------------------------------------------------
      int TestMediaEngine::deregisterVoiceExternalTransport()
      {
        ZS_THROW_INVALID_USAGE("external transport is disabled - cannot be deregistered")
      }
      
      //-----------------------------------------------------------------------
      int TestMediaEngine::receivedVoiceRTPPacket(const void *data, unsigned int length)
      {
        ZS_THROW_INVALID_USAGE("external transport is disabled - cannot receive data")
      }
      
      //-----------------------------------------------------------------------
      int TestMediaEngine::receivedVoiceRTCPPacket(const void* data, unsigned int length)
      {
        ZS_THROW_INVALID_USAGE("external transport is disabled - cannot receive data")
      }
      
      //-----------------------------------------------------------------------
      int TestMediaEngine::registerVideoExternalTransport(Transport &transport)
      {
        ZS_THROW_INVALID_USAGE("external transport is disabled - cannot be registered")
      }
      
      //-----------------------------------------------------------------------
      int TestMediaEngine::deregisterVideoExternalTransport()
      {
        ZS_THROW_INVALID_USAGE("external transport is disabled - cannot be deregistered")
      }
      
      //-----------------------------------------------------------------------
      int TestMediaEngine::receivedVideoRTPPacket(const void *data, const int length)
      {
        ZS_THROW_INVALID_USAGE("external transport is disabled - cannot receive data")
      }
      
      //-----------------------------------------------------------------------
      int TestMediaEngine::receivedVideoRTCPPacket(const void *data, const int length)
      {
        ZS_THROW_INVALID_USAGE("external transport is disabled - cannot receive data")
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaEngine => ITimerDelegate
      #pragma mark
      
      //-----------------------------------------------------------------------
      void TestMediaEngine::onTimer(zsLib::TimerPtr timer)
      {
        internal::AutoRecursiveLock lock(mLock);
        
        if (timer == mVoiceStatisticsTimer)
        {
          unsigned int averageJitterMs;
          unsigned int maxJitterMs;
          unsigned int discardedPackets;
          
          mError = mVoiceRtpRtcp->GetRTPStatistics(mVoiceChannel, averageJitterMs, maxJitterMs, discardedPackets);
          if (0 != mError) {
            ZS_LOG_ERROR(Detail, log("failed to get RTP statistics for voice (error: ") + zsLib::Stringize<internal::INT>(mVoiceBase->LastError()).string() + ")")
            return;
          }
          
          //printf("=== Jitter buffer - Average jitter: %d, Max jitter: %d, Discarded pacets: %d\n", averageJitterMs, maxJitterMs, discardedPackets);
        }
      }
      
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaEngine => TraceCallback
      #pragma mark
      //-----------------------------------------------------------------------
      void TestMediaEngine::Print(const webrtc::TraceLevel level, const char *traceString, const int length)
      {
        printf("%s\n", traceString);
        MediaEngine::Print(level, traceString, length);
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark TestMediaEngine => (internal)
      #pragma mark
      
      //-----------------------------------------------------------------------
      void TestMediaEngine::internalStartVoice()
      {
        MediaEngine::internalStartVoice();
        
#ifdef HOOKFLASH_MEDIA_ENGINE_ENABLE_TIMER
        mVoiceStatisticsTimer = zsLib::Timer::create(mThisWeak.lock(), zsLib::Seconds(1));
#endif
      }
      
      //-----------------------------------------------------------------------
      void TestMediaEngine::internalStopVoice()
      {
#ifdef HOOKFLASH_MEDIA_ENGINE_ENABLE_TIMER
        if (mVoiceStatisticsTimer) {
          mVoiceStatisticsTimer->cancel();
          mVoiceStatisticsTimer.reset();
        }
#endif
        MediaEngine::internalStopVoice();
      }

      //-----------------------------------------------------------------------
      int TestMediaEngine::registerVoiceTransport()
      {
        return 0;
      }
      
      //-----------------------------------------------------------------------
      int TestMediaEngine::setVoiceTransportParameters()
      {
        mError = mVoiceBase->SetSendDestination(mVoiceChannel, 20010, mReceiverAddress.c_str());
        mError = mVoiceBase->SetLocalReceiver(mVoiceChannel, 20010);
        return mError;
      }
      
      //-----------------------------------------------------------------------
      void TestMediaEngine::internalStartVideoCapture()
      {
        MediaEngine::internalStartVideoCapture();
      }
      
      //-----------------------------------------------------------------------
      void TestMediaEngine::internalStopVideoCapture()
      {
        MediaEngine::internalStopVideoCapture();
      }
      
      //-----------------------------------------------------------------------
      void TestMediaEngine::internalStartVideoChannel()
      {
        MediaEngine::internalStartVideoChannel();
      }
      
      //-----------------------------------------------------------------------
      void TestMediaEngine::internalStopVideoChannel()
      {
        MediaEngine::internalStopVideoChannel();
      }

      //-----------------------------------------------------------------------
      int TestMediaEngine::registerVideoTransport()
      {
        return 0;
      }
      
      //-----------------------------------------------------------------------
      int TestMediaEngine::deregisterVideoTransport()
      {
        return 0;
      }
      
      //-----------------------------------------------------------------------
      int TestMediaEngine::setVideoTransportParameters()
      {
        mError = mVideoNetwork->SetSendDestination(mVideoChannel, mReceiverAddress.c_str(), 20000);
        mError = mVideoNetwork->SetLocalReceiver(mVideoChannel, 20000);
        return mError;
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark TestMediaEngineFactory
      #pragma mark
      
      //-----------------------------------------------------------------------
      internal::MediaEnginePtr TestMediaEngineFactory::createMediaEngine(IMediaEngineDelegatePtr delegate)
      {
        return TestMediaEngine::create(delegate);
      }
    }
  }
}

using hookflash::core::test::TestMediaEngineFactory;
using hookflash::core::test::TestMediaEngineFactoryPtr;
using hookflash::core::test::TestMediaEngine;
using hookflash::core::test::TestMediaEnginePtr;


void doMediaEngineTest()
{
  if (!HOOKFLASH_CORE_TEST_DO_MEDIA_ENGINE_TEST) return;
  
  BOOST_INSTALL_LOGGER();
  
  TestMediaEngineFactoryPtr overrideFactory(new TestMediaEngineFactory);
  
  hookflash::core::internal::Factory::override(overrideFactory);
  

}
