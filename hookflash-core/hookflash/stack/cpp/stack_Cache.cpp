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

#include <hookflash/stack/internal/stack_Cache.h>

#include <hookflash/stack/message/Message.h>
#include <hookflash/stack/IMessageMonitor.h>

#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>
#include <zsLib/Log.h>
#include <zsLib/XML.h>

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;
      using namespace zsLib::XML;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICacheForServices
      #pragma mark

      //-----------------------------------------------------------------------
      bool ICacheForServices::handledFromCache(
                                               const char *cookieNamePath,
                                               message::MessagePtr originalMessage
                                               )
      {
        ICachePtr singleton = ICache::singleton();

        String str = singleton->fetch(cookieNamePath);
        if (str.isEmpty()) return false;

        DocumentPtr doc = Document::createFromParsedJSON(str);
        if (!doc) return false;

        ElementPtr rootEl = doc->getFirstChildElement();
        if (!rootEl) return false;

        String messageID = originalMessage->messageID();
        String appID = originalMessage->appID();

        if (messageID.hasData()) {
          rootEl->setAttribute("id", messageID);
        }
        if (appID.hasData()) {
          rootEl->setAttribute("appid", appID);
        }

        MessagePtr message = Message::create(rootEl, IMessageSourcePtr());
        if (!message) return false;

        return IMessageMonitor::handleMessageReceived(message);
      }

      //-----------------------------------------------------------------------
      void ICacheForServices::storeMessage(
                                           const char *cookieNamePath,
                                           Time expires,
                                           message::MessagePtr originalMessage
                                           )
      {
        if (!originalMessage) return;

        ElementPtr el = originalMessage->creationElement();
        if (!el) return;

        GeneratorPtr generator = Generator::createJSONGenerator();
        boost::shared_array<char> output = generator->write(el);

        String result = (const char *)output.get();

        ICachePtr singleton = ICache::singleton();

        singleton->store(cookieNamePath, expires, result);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Cache
      #pragma mark

      //-----------------------------------------------------------------------
      Cache::Cache() :
        mID(zsLib::createPUID())
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      Cache::~Cache()
      {
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
      }

      //-----------------------------------------------------------------------
      CachePtr Cache::convert(ICachePtr cache)
      {
        return boost::dynamic_pointer_cast<Cache>(cache);
      }

      //-----------------------------------------------------------------------
      CachePtr Cache::create()
      {
        CachePtr pThis(new Cache());
        pThis->mThisWeak = pThis;
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Cache => ICache
      #pragma mark

      //-----------------------------------------------------------------------
      void Cache::setup(ICacheDelegatePtr delegate)
      {
        singleton()->actualSetup(delegate);
      }

      //-----------------------------------------------------------------------
      CachePtr Cache::singleton()
      {
        static CachePtr singleton = Cache::create();
        return singleton;
      }

      //-----------------------------------------------------------------------
      String Cache::fetch(const char *cookieNamePath) const
      {
        if (!cookieNamePath) return String();

        ICacheDelegatePtr delegate;

        {
          AutoRecursiveLock lock(mLock);
          delegate = mDelegate;
        }

        if (delegate) return delegate->fetch(cookieNamePath);
        return String();
      }

      //-----------------------------------------------------------------------
      void Cache::store(
                        const char *cookieNamePath,
                        Time expires,
                        const char *str
                        )
      {
        if (!cookieNamePath) return;
        if (!str) clear(cookieNamePath);
        if (!(*str)) clear(cookieNamePath);

        ICacheDelegatePtr delegate;

        {
          AutoRecursiveLock lock(mLock);
          delegate = mDelegate;
        }

        if (delegate) return delegate->store(cookieNamePath, expires, str);
      }

      //-----------------------------------------------------------------------
      void Cache::clear(const char *cookieNamePath)
      {
        if (!cookieNamePath) return;

        ICacheDelegatePtr delegate;

        {
          AutoRecursiveLock lock(mLock);
          delegate = mDelegate;
        }

        if (delegate) return delegate->clear(cookieNamePath);
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Cache => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String Cache::log(const char *message) const
      {
        return String("stack::Cache [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      void Cache::actualSetup(ICacheDelegatePtr delegate)
      {
        AutoRecursiveLock lock(mLock);
        mDelegate = delegate;

        ZS_LOG_DEBUG(log("setup called") + ", has delegate=" + (delegate ? "true":"false"))
      }

    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ICache
    #pragma mark

    //-------------------------------------------------------------------------
    void ICache::setup(ICacheDelegatePtr delegate)
    {
      internal::Cache::setup(delegate);
    }

    //-------------------------------------------------------------------------
    ICachePtr ICache::singleton()
    {
      return internal::Cache::singleton();
    }

  }
}
