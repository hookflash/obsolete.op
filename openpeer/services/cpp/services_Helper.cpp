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

#include <openpeer/services/internal/services_Helper.h>
#include <openpeer/services/IDNS.h>
#include <cryptopp/osrng.h>
#include <zsLib/Stringize.h>
#include <zsLib/helpers.h>
#include <zsLib/Log.h>
//#include <zsLib/ISocket.h>
//#include <zsLib/Socket.h>
#include <zsLib/MessageQueueThread.h>
//#include <zsLib/Timer.h>
//#include <zsLib/Numeric.h>

#include <iostream>
#include <fstream>
#ifndef _WIN32
#include <pthread.h>
#endif //ndef _WIN32

#include <boost/shared_array.hpp>

namespace openpeer { namespace services { ZS_DECLARE_SUBSYSTEM(openpeer_services) } }


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
      #pragma mark GlobalLock
      #pragma mark

      //-----------------------------------------------------------------------
      class GlobalLock
      {
      public:
        GlobalLock() {}
        ~GlobalLock() {}

        //---------------------------------------------------------------------
        static GlobalLock &singleton()
        {
          static GlobalLock lock;
          return lock;
        }

        //---------------------------------------------------------------------
        RecursiveLock &getLock() const
        {
          return mLock;
        }

      private:
        mutable RecursiveLock mLock;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark GlobalLockInit
      #pragma mark

      //-----------------------------------------------------------------------
      class GlobalLockInit
      {
      public:
        //---------------------------------------------------------------------
        GlobalLockInit()
        {
          singleton();
        }

        //---------------------------------------------------------------------
        RecursiveLock &singleton()
        {
          return (GlobalLock::singleton()).getLock();
        }
      };

      static GlobalLockInit gGlobalLockInit;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceThread
      #pragma mark

      //-----------------------------------------------------------------------
      class ServiceThread;
      typedef boost::shared_ptr<ServiceThread> ServiceThreadPtr;
      typedef boost::weak_ptr<ServiceThread> ServiceThreadWeakPtr;

      class ServiceThread
      {
        //---------------------------------------------------------------------
        ServiceThread() {}

        //---------------------------------------------------------------------
        void init()
        {
          mThread = MessageQueueThread::createBasic("org.openpeer.services.serviceThread");
        }

      public:
        //---------------------------------------------------------------------
        ~ServiceThread()
        {
          if (!mThread) return;
          mThread->waitForShutdown();
        }

        //---------------------------------------------------------------------
        static ServiceThreadPtr create()
        {
          ServiceThreadPtr pThis(new ServiceThread);
          pThis->mThisWeak = pThis;
          pThis->init();
          return pThis;
        }

        //---------------------------------------------------------------------
        static ServiceThreadPtr singleton()
        {
          AutoRecursiveLock lock(Helper::getGlobalLock());
          static ServiceThreadPtr thread = ServiceThread::create();
          return thread;
        }

        //---------------------------------------------------------------------
        MessageQueueThreadPtr getThread() const
        {
          return mThread;
        }

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceThread => (data)
        #pragma mark

        ServiceThreadWeakPtr mThisWeak;

        MessageQueueThreadPtr mThread;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Helper
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &Helper::getGlobalLock()
      {
        return gGlobalLockInit.singleton();
      }

      //-----------------------------------------------------------------------
      String Helper::getDebugValue(const char *name, const String &value, bool &firstTime)
      {
        if (value.isEmpty()) return String();
        if (firstTime) {
          firstTime = false;
          return String(name) + "=" + value;
        }
        return String(", ") + name + "=" + value;
      }

      //-----------------------------------------------------------------------
      String Helper::randomString(UINT lengthInChars)
      {
        static const char *randomCharArray = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static size_t randomSize = strlen(randomCharArray);

        BYTE staticBuffer[256];
        char staticOutputBuffer[sizeof(staticBuffer)+1];

        boost::shared_array<BYTE> allocatedBuffer;
        boost::shared_array<char> allocatedOutputBuffer;

        BYTE *buffer = &(staticBuffer[0]);
        char *output = &(staticOutputBuffer[0]);
        if (lengthInChars > sizeof(staticBuffer)) {
          // use the allocated buffer instead
          allocatedBuffer = boost::shared_array<BYTE>(new BYTE[lengthInChars]);
          allocatedOutputBuffer = boost::shared_array<char>(new char[lengthInChars+1]);
          buffer = allocatedBuffer.get();
          output = allocatedOutputBuffer.get();
        }

        AutoSeededRandomPool rng;
        rng.GenerateBlock(&(buffer[0]), lengthInChars);

        memset(&(output[0]), 0, sizeof(char)*(lengthInChars+1));

        for (UINT loop = 0; loop < lengthInChars; ++loop) {
          output[loop] = randomCharArray[((buffer[loop])%randomSize)];
        }
        return String((CSTR)(&(output[0])));
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::random(UINT lengthInBytes)
      {
        SecureByteBlockPtr output(new SecureByteBlock);
        AutoSeededRandomPool rng;
        output->CleanNew(lengthInBytes);
        rng.GenerateBlock(*output, lengthInBytes);
        return output;
      }

      //-----------------------------------------------------------------------
      ULONG Helper::random(ULONG minValue, ULONG maxValue)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(minValue > maxValue)
        if (minValue == maxValue) return minValue;

        ULONG range = maxValue - minValue;

        ULONG value = 0;
        
        AutoSeededRandomPool rng;
        rng.GenerateBlock((BYTE *) &value, sizeof(ULONG));

        value = minValue + (value % range);

        return value;
      }

      //-----------------------------------------------------------------------
      IMessageQueuePtr Helper::getServiceQueue()
      {
        ServiceThreadPtr thread = ServiceThread::singleton();
        return thread->getThread();
      }

    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark services::IHelper
    #pragma mark

    //-------------------------------------------------------------------------
    RecursiveLock &IHelper::getGlobalLock()
    {
      return internal::Helper::getGlobalLock();
    }

    //-------------------------------------------------------------------------
    String IHelper::randomString(UINT lengthInChars)
    {
      return internal::Helper::randomString(lengthInChars);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::random(UINT lengthInBytes)
    {
      return internal::Helper::random(lengthInBytes);
    }

    //-------------------------------------------------------------------------
    ULONG IHelper::random(ULONG minValue, ULONG maxValue)
    {
      return internal::Helper::random(minValue, maxValue);
    }

    //-------------------------------------------------------------------------
    IMessageQueuePtr IHelper::getServiceQueue()
    {
      return internal::Helper::getServiceQueue();
    }

  }
}
