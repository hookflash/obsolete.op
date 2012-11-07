/*
 
 Copyright (c) 2012, SMB Phone Inc.
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

#include <hookflash/services/IHTTP.h>
#include <hookflash/services/internal/services_HTTP.h>
#include <hookflash/services/internal/services_Helper.h>

#include <zsLib/zsHelpers.h>
#include <zsLib/Stringize.h>
#include <zsLib/Log.h>
#include <zsLib/Event.h>

#include <boost/thread.hpp>

namespace hookflash { namespace services { ZS_DECLARE_SUBSYSTEM(hookflash_services_http) } }

namespace hookflash
{
  namespace services
  {
    namespace internal
    {
#ifndef _WIN32
      typedef timeval TIMEVAL;
#endif //_WIN32

      using zsLib::Stringize;

      typedef zsLib::ULONG ULONG;
      typedef zsLib::DWORD DWORD;
      typedef zsLib::String String;
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::Event Event;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark HTTPGlobalInit
      #pragma mark

      class HTTPGlobalInit
      {
      public:
        HTTPGlobalInit()
        {
          HTTP::singleton();
        }
      };

      static HTTPGlobalInit gHTTPGlobalInit;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark HTTP
      #pragma mark

      //-----------------------------------------------------------------------
      HTTP::HTTP() :
        mID(zsLib::createPUID()),
        mShouldShutdown(false),
        mMultiCurl(NULL)
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void HTTP::init()
      {
      }

      //-----------------------------------------------------------------------
      HTTP::~HTTP()
      {
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark HTTP => IHTTP
      #pragma mark

      //-----------------------------------------------------------------------
      HTTP::HTTPQueryPtr HTTP::get(
                                   IHTTPQueryDelegatePtr delegate,
                                   const char *userAgent,
                                   const char *url,
                                   Duration timeout
                                   )
      {
        HTTPPtr pThis = singleton();
        HTTPQueryPtr query = HTTPQuery::create(pThis, delegate, false, userAgent, url, NULL, 0, NULL, timeout);
        pThis->monitorBegin(query);
        return query;
      }

      //-----------------------------------------------------------------------
      HTTP::HTTPQueryPtr HTTP::post(
                                    IHTTPQueryDelegatePtr delegate,
                                    const char *userAgent,
                                    const char *url,
                                    const BYTE *postData,
                                    ULONG postDataLengthInBytes,
                                    const char *postDataMimeType,
                                    Duration timeout
                                    )
      {
        HTTPPtr pThis = singleton();
        HTTPQueryPtr query = HTTPQuery::create(pThis, delegate, true, userAgent, url, postData, postDataLengthInBytes, postDataMimeType, timeout);
        pThis->monitorBegin(query);
        return query;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark HTTP => friend HTTPQuery
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &HTTP::getLock() const
      {
        return mLock;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark HTTP => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      HTTPPtr HTTP::singleton()
      {
        static HTTPPtr pThis = HTTP::create();
        static HTTPGlobalInit *pBogus = &gHTTPGlobalInit; // create a circular reference to global object
        (void)pBogus; // reference the temporary object (noop)
        return pThis;
      }

      //-----------------------------------------------------------------------
      HTTPPtr HTTP::create()
      {
        HTTPPtr pThis(new HTTP);
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      String HTTP::log(const char *message)
      {
        return String("HTTP [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      void HTTP::cancel()
      {
        AutoRecursiveLock lock();

        mShouldShutdown = true;

        wakeUp();
      }

      //-----------------------------------------------------------------------
      void HTTP::wakeUp()
      {
        int errorCode = 0;

        {
          AutoRecursiveLock lock(mLock);

          if (!mWakeUpSocket)               // is the wakeup socket created?
            return;

          if (!mWakeUpSocket->isValid())
          {
            ZS_LOG_ERROR(Basic, log("Could not wake up socket monitor as wakeup socket was closed. This will cause a delay in the socket monitor response time."))
            return;
          }

          static DWORD gBogus = 0;
          static BYTE *bogus = (BYTE *)&gBogus;

          bool wouldBlock = false;
          mWakeUpSocket->send(bogus, sizeof(gBogus), &wouldBlock, 0, &errorCode);       // send a bogus packet to its own port to wake it up
        }

        if (0 != errorCode) {
          ZS_LOG_ERROR(Basic, log("SocketMonitor: Could not wake up socket monitor. This will cause a delay in the socket monitor response time") + ", error=" + (Stringize<int>(errorCode).string()))
        }
      }

      //-----------------------------------------------------------------------
      void HTTP::createWakeUpSocket()
      {
        AutoRecursiveLock lock(mLock);

        int tries = 0;
        bool useIPv6 = true;
        while (true)
        {
          // bind only on the loopback address
          bool error = false;
          try {
            if (useIPv6)
            {
              mWakeUpAddress = IPAddress::loopbackV6();
              mWakeUpSocket = zsLib::Socket::createUDP(zsLib::Socket::Create::IPv6);
            }
            else
            {
              mWakeUpAddress = IPAddress::loopbackV4();
              mWakeUpSocket = zsLib::Socket::createUDP(zsLib::Socket::Create::IPv4);
            }

            if (((tries > 5) && (tries < 10)) ||
                (tries > 15)) {
              mWakeUpAddress.setPort(5000+(rand()%(65525-5000)));
            } else {
              mWakeUpAddress.setPort(0);
            }

            mWakeUpSocket->setOptionFlag(zsLib::Socket::SetOptionFlag::NonBlocking, true);
            mWakeUpSocket->bind(mWakeUpAddress);
            mWakeUpAddress = mWakeUpSocket->getLocalAddress();
            mWakeUpSocket->connect(mWakeUpAddress);
          } catch (zsLib::Socket::Exceptions::Unspecified &) {
            error = true;
          }
          if (!error)
          {
            break;
          }

          boost::thread::yield();       // do not hammer CPU

          if (tries > 10)
            useIPv6 = (tries%2 == 0);   // after 10 tries, start trying to bind using IPv4

          ZS_THROW_BAD_STATE_MSG_IF(tries > 500, log("Unable to allocate any loopback ports for a wake-up socket"))
        }
      }

      //-----------------------------------------------------------------------
      void HTTP::processWaiting()
      {
        for (EventList::iterator iter = mWaitingForRebuildList.begin(); iter != mWaitingForRebuildList.end(); ++iter)
        {
          (*iter)->notify();
        }
        mWaitingForRebuildList.clear();

        for (HTTPQueryMap::iterator iter = mPendingRemoveQueries.begin(); iter != mPendingRemoveQueries.end(); ++iter)
        {
          HTTPQueryPtr &query = (*iter).second;

          HTTPQueryMap::iterator found = mPendingAddQueries.find(query->getID());
          if (found != mPendingAddQueries.end()) {
            mPendingAddQueries.erase(found);
          }

          found = mQueries.find(query->getID());
          if (found != mQueries.end()) {
            CURL *curl = query->getCURL();
            if (NULL != curl) {
              curl_multi_remove_handle(mMultiCurl, curl);
            }
            query->cleanupCurl();

            if (NULL != curl) {
              HTTPCurlMap::iterator foundCurl = mCurlMap.find(curl);
              if (foundCurl != mCurlMap.end()) {
                mCurlMap.erase(curl);
              }
            }
          }
        }

        mPendingRemoveQueries.clear();

        if (!mShouldShutdown) {
          for (HTTPQueryMap::iterator iter = mPendingAddQueries.begin(); iter != mPendingAddQueries.end(); ++iter)
          {
            HTTPQueryPtr &query = (*iter).second;

            query->prepareCurl();
            CURL *curl = query->getCURL();

            if (curl) {
              if (CURLM_OK != curl_multi_add_handle(mMultiCurl, curl)) {
                curl_multi_remove_handle(mMultiCurl, curl);
                query->cleanupCurl();
                curl = NULL;
              }
            }

            if (curl) {
              mQueries[query->getID()] = query;
              mCurlMap[curl] = query;
            }
          }
          mPendingAddQueries.clear();

        } else {
          for (HTTPQueryMap::iterator iter = mPendingAddQueries.begin(); iter != mPendingAddQueries.end(); ++iter)
          {
            HTTPQueryPtr &query = (*iter).second;
            query->cleanupCurl();

          }
          mPendingAddQueries.clear();

          for (HTTPQueryMap::iterator iter = mQueries.begin(); iter != mQueries.end(); ++iter)
          {
            HTTPQueryPtr &query = (*iter).second;
            CURL *curl = query->getCURL();
            if (curl) {
              curl_multi_remove_handle(mMultiCurl, curl);
            }
            query->cleanupCurl();
          }

          mQueries.clear();
          mCurlMap.clear();
        }
      }

      //-----------------------------------------------------------------------
      void HTTP::monitorBegin(HTTPQueryPtr query)
      {
        EventPtr event;

        {
          AutoRecursiveLock lock();

          if (mShouldShutdown) {
            query->cancel();
            return;
          }

          if (!mThread) {
            mThread = ThreadPtr(new boost::thread(boost::ref(*(HTTP::singleton().get()))));
          }

          mPendingAddQueries[query->getID()] = query;

          event = Event::create();
          mWaitingForRebuildList.push_back(event);                                        // socket handles cane be reused so we must ensure that the socket handles are rebuilt before returning

          wakeUp();
        }
        if (event)
          event->wait();
      }

      //-----------------------------------------------------------------------
      void HTTP::monitorEnd(HTTPQueryPtr query)
      {
        EventPtr event;
        {
          AutoRecursiveLock lock();

          mPendingRemoveQueries[query->getID()] = query;

          event = Event::create();
          mWaitingForRebuildList.push_back(event);                                        // socket handles cane be reused so we must ensure that the socket handles are rebuilt before returning

          wakeUp();
        }
        if (event)
          event->wait();
      }

      //-----------------------------------------------------------------------
      void HTTP::operator()()
      {
#ifndef _LINUX
        pthread_setname_np("com.hookflash.services.http");
#endif
        ZS_LOG_BASIC(log("http thread started"))

        mMultiCurl = curl_multi_init();

        createWakeUpSocket();

        bool shouldShutdown = false;

        TIMEVAL timeout;
        memset(&timeout, 0, sizeof(timeout));

        fd_set fdread;
        fd_set fdwrite;
        fd_set fdexcep;

        do
        {
          SOCKET highestSocket = zsLib::INVALID_SOCKET;

          {
            AutoRecursiveLock lock(mLock);
            processWaiting();

            FD_ZERO(&fdread);
            FD_ZERO(&fdwrite);
            FD_ZERO(&fdexcep);

            // monitor the wakeup socket...
            FD_SET(mWakeUpSocket->getSocket(), &fdread);
            FD_SET(mWakeUpSocket->getSocket(), &fdexcep);

            int maxfd = -1;
            CURLMcode result = curl_multi_fdset(mMultiCurl, &fdread, &fdwrite, &fdexcep, &maxfd);
            if (result != CURLM_OK) {
              ZS_LOG_ERROR(Basic, log("failed multi-select") + ", result=" + Stringize<CURLMcode>(result).string() + ", error=" + curl_multi_strerror(result))
              mShouldShutdown = true;
            }

            long curlTimeout = -1;
            curl_multi_timeout(mMultiCurl, &curlTimeout);

            if (curlTimeout >= 0) {
              timeout.tv_sec = curlTimeout / 1000;
              if (timeout.tv_sec > 1)
                timeout.tv_sec = 1;
              else
                timeout.tv_usec = (curlTimeout % 1000) * 1000;
            }

            int handleCount = 0;
            curl_multi_perform(mMultiCurl, &handleCount);

#ifndef _WIN32
            highestSocket = mWakeUpSocket->getSocket();

            if (-1 != maxfd) {
              if (((SOCKET)maxfd) > highestSocket) {
                highestSocket = (SOCKET)maxfd;
              }
            }
#endif //_WIN32
          }

          timeout.tv_sec = 1;
          timeout.tv_usec = 0;

          int result = select(zsLib::INVALID_SOCKET == highestSocket ? 0 : (highestSocket+1), &fdread, &fdwrite, &fdexcep, &timeout);

          // select completed, do notifications from select
          {
            AutoRecursiveLock lock(mLock);
            shouldShutdown = mShouldShutdown;

            int handleCount = 0;
            curl_multi_perform(mMultiCurl, &handleCount);

            switch (result) {
              
              case zsLib::INVALID_SOCKET:  break;
              case 0:
              default: {
                ULONG totalToProcess = result;

                bool redoWakeupSocket = false;
                if (FD_ISSET(mWakeUpSocket->getSocket(), &fdread)) {
                  --totalToProcess;

                  bool wouldBlock = false;
                  static DWORD gBogus = 0;
                  static BYTE *bogus = (BYTE *)&gBogus;
                  int noThrowError = 0;
                  mWakeUpSocket->receive(bogus, sizeof(gBogus), &wouldBlock, 0, &noThrowError);
                  if (0 != noThrowError) redoWakeupSocket = true;
                }

                if (FD_ISSET(mWakeUpSocket->getSocket(), &fdexcep)) {
                  --totalToProcess;
                  redoWakeupSocket = true;
                }

                if (redoWakeupSocket) {
                  mWakeUpSocket->close();
                  mWakeUpSocket.reset();
                  createWakeUpSocket();
                }

                CURLMsg *msg = NULL;
                int handleCount = 0;

                while ((msg = curl_multi_info_read(mMultiCurl, &handleCount)))
                {
                  if (CURLMSG_DONE == msg->msg) {
                    HTTPCurlMap::iterator found = mCurlMap.find(msg->easy_handle);

                    curl_multi_remove_handle(mMultiCurl, msg->easy_handle);

                    if (found != mCurlMap.end()) {
                      HTTPQueryPtr &query = (*found).second;
                      query->notifyComplete(msg->data.result);

                      HTTPQueryMap::iterator foundQuery = mQueries.find(query->getID());
                      if (foundQuery != mQueries.end()) {
                        mQueries.erase(foundQuery);
                      }

                      mCurlMap.erase(found);
                    }
                  }
                }
              }
            }
          }
        } while (!shouldShutdown);

        {
          AutoRecursiveLock lock(mLock);
          processWaiting();
          mWaitingForRebuildList.clear();
          mWakeUpSocket.reset();

          if (mMultiCurl) {
            curl_multi_cleanup(mMultiCurl);
            mMultiCurl = NULL;
          }
        }

        ZS_LOG_BASIC(log("http thread stopped"))
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark HTTP::HTTPQuery
      #pragma mark

      //-----------------------------------------------------------------------
      HTTP::HTTPQuery::HTTPQuery(
                                 HTTPPtr outer,
                                 IHTTPQueryDelegatePtr delegate,
                                 bool isPost,
                                 const char *userAgent,
                                 const char *url,
                                 const BYTE *postData,
                                 ULONG postDataLengthInBytes,
                                 const char *postDataMimeType,
                                 Duration timeout
                                 ) :
        mID(zsLib::createPUID()),
        mOuter(outer),
        mDelegate(IHTTPQueryDelegateProxy::create(Helper::getServiceQueue(), delegate)),
        mIsPost(isPost),
        mUserAgent(userAgent ? userAgent : ""),
        mURL(url),
        mMimeType(postDataMimeType ? postDataMimeType : ""),
        mTimeout(timeout),
        mCurl(NULL),
        mResponseCode(0),
        mResultCode(CURLE_OK)
      {
        ZS_LOG_DEBUG(log("created"))
        if (0 != postDataLengthInBytes) {
          mPostData.CleanNew(postDataLengthInBytes+sizeof(char)); // add an extra NUL to end of buffer
          memcpy(mPostData.BytePtr(), postData, postDataLengthInBytes);
        }
      }

      //-----------------------------------------------------------------------
      void HTTP::HTTPQuery::init()
      {
      }

      //-----------------------------------------------------------------------
      HTTP::HTTPQuery::~HTTPQuery()
      {
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark HTTP::HTTPQuery => IHTTPQuery
      #pragma mark

      //-----------------------------------------------------------------------
      void HTTP::HTTPQuery::cancel()
      {
        AutoRecursiveLock lock(getLock());

        HTTPQueryPtr pThis = mThisWeak.lock();

        HTTPPtr outer = mOuter.lock();
        if ((outer) &&
            (pThis)) {
          outer->monitorEnd(pThis);
          return;
        }

        if ((pThis) &&
            (mDelegate)) {
          try {
            mDelegate->onHTTPComplete(pThis);
          } catch (IHTTPQueryDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Detail, log("delegate gone"))
          }
        }

        mDelegate.reset();

        if (mCurl) {
          curl_easy_cleanup(mCurl);
          mCurl = NULL;
        }
      }

      //-----------------------------------------------------------------------
      bool HTTP::HTTPQuery::isComplete() const
      {
        AutoRecursiveLock lock(getLock());
        if (!mDelegate) return true;
        return false;
      }

      //-----------------------------------------------------------------------
      bool HTTP::HTTPQuery::wasSuccessful() const
      {
        AutoRecursiveLock lock(getLock());
        if (mDelegate) return false;

        return ((CURLE_OK == mResultCode) &&
                ((mResponseCode >= 200) && (mResponseCode < 400)));
      }

      //-----------------------------------------------------------------------
      long HTTP::HTTPQuery::getResponseCode() const
      {
        AutoRecursiveLock lock(getLock());
        return mResponseCode;
      }

      //-----------------------------------------------------------------------
      ULONG HTTP::HTTPQuery::getHeaderReadSizeAvailableInBtytes() const
      {
        AutoRecursiveLock lock(getLock());
        return mHeader.MaxRetrievable();
      }

      //-----------------------------------------------------------------------
      ULONG HTTP::HTTPQuery::readHeader(
                                        BYTE *outResultData,
                                        ULONG bytesToRead
                                        )
      {
        AutoRecursiveLock lock(getLock());
        return mHeader.Get(outResultData, bytesToRead);
      }

      //-----------------------------------------------------------------------
      ULONG HTTP::HTTPQuery::readHeaderAsString(String &outHeader)
      {
        outHeader.clear();

        AutoRecursiveLock lock(getLock());
        CryptoPP::lword available = mHeader.MaxRetrievable();
        if (0 == available) return 0;

        SecureByteBlock data;
        data.CleanNew(available+sizeof(char));
        mHeader.Get(data.BytePtr(), available);

        outHeader = (const char *)data.BytePtr();
        return strlen(outHeader);
      }

      //-----------------------------------------------------------------------
      ULONG HTTP::HTTPQuery::getReadDataAvailableInBytes() const
      {
        AutoRecursiveLock lock(getLock());
        return mBody.MaxRetrievable();
      }

      //-----------------------------------------------------------------------
      ULONG HTTP::HTTPQuery::readData(
                                      BYTE *outResultData,
                                      ULONG bytesToRead
                                      )
      {
        AutoRecursiveLock lock(getLock());
        return mBody.Get(outResultData, bytesToRead);
      }

      //-----------------------------------------------------------------------
      ULONG HTTP::HTTPQuery::readDataAsString(String &outResultData)
      {
        outResultData.clear();

        AutoRecursiveLock lock(getLock());
        CryptoPP::lword available = mBody.MaxRetrievable();
        if (0 == available) return 0;

        SecureByteBlock data;
        data.CleanNew(available+sizeof(char));
        mBody.Get(data.BytePtr(), available);

        outResultData = (const char *)data.BytePtr();
        return strlen(outResultData);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark HTTP::HTTPQuery => friend HTTP
      #pragma mark

      HTTP::HTTPQueryPtr HTTP::HTTPQuery::create(
                                                 HTTPPtr outer,
                                                 IHTTPQueryDelegatePtr delegate,
                                                 bool isPost,
                                                 const char *userAgent,
                                                 const char *url,
                                                 const BYTE *postData,
                                                 ULONG postDataLengthInBytes,
                                                 const char *postDataMimeType,
                                                 Duration timeout
                                                 )
      {
        HTTPQueryPtr pThis(new HTTPQuery(outer, delegate, isPost, userAgent, url, postData, postDataLengthInBytes, postDataMimeType, timeout));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void HTTP::HTTPQuery::prepareCurl()
      {
        AutoRecursiveLock lock(getLock());

        ZS_THROW_BAD_STATE_IF(mCurl)

        mCurl = curl_easy_init();
        if (!mCurl) {
          ZS_LOG_ERROR(Detail, log("curl failed to initialize"))
          return;
        }

        curl_easy_setopt(mCurl, CURLOPT_URL, mURL.c_str());
        if (!mUserAgent.isEmpty()) {
          curl_easy_setopt(mCurl, CURLOPT_USERAGENT, mUserAgent.c_str());
        }
        if (!mMimeType.isEmpty()) {
          String temp = "Content-Type: " + mMimeType;
          struct curl_slist *slist = curl_slist_append(NULL, temp.c_str());
          curl_easy_setopt(mCurl, CURLOPT_HTTPHEADER, slist);
        }
        {
          /*
           Using POST with HTTP 1.1 implies the use of a "Expect: 100-continue"
           header.  You can disable this header with CURLOPT_HTTPHEADER as usual.
           NOTE: if you want chunked transfer too, you need to combine these two
           since you can only set one list of headers with CURLOPT_HTTPHEADER. */
          
           //please see http://curl.haxx.se/libcurl/c/post-callback.html for example usage
          
          struct curl_slist *slist = NULL;          
          slist = curl_slist_append(slist, "Expect:");
          curl_easy_setopt(mCurl, CURLOPT_HTTPHEADER, slist);
        }
        
        curl_easy_setopt(mCurl, CURLOPT_HEADER, 0);
        if (mIsPost) {
          curl_easy_setopt(mCurl, CURLOPT_POST, 1L);

          if (mPostData.size() > 0) {
            curl_easy_setopt(mCurl, CURLOPT_POSTFIELDS, mPostData.BytePtr());
            curl_easy_setopt(mCurl, CURLOPT_POSTFIELDSIZE, mPostData.size()-sizeof(char)); // added extra NUL to buffer so must subtract to get proper length
          }
        }

        curl_easy_setopt(mCurl, CURLOPT_HEADERFUNCTION, HTTPQuery::writeHeader);
        curl_easy_setopt(mCurl, CURLOPT_WRITEHEADER, this);

        curl_easy_setopt(mCurl, CURLOPT_WRITEFUNCTION, HTTPQuery::writeData);
        curl_easy_setopt(mCurl, CURLOPT_WRITEDATA, this);

        if (Duration() != mTimeout) {
          curl_easy_setopt(mCurl, CURLOPT_TIMEOUT_MS, mTimeout.total_milliseconds());
        }

        if (ZS_IS_LOGGING(Debug)) {
          ZS_LOG_BASIC(log("------------------------------------HTTP INFO----------------------------------"))
          ZS_LOG_BASIC(log("INFO") + ", URL=" + mURL)
          ZS_LOG_BASIC(log("INFO") + ", method=" + (mIsPost ? "POST" : "GET"))
          ZS_LOG_BASIC(log("INFO") + ", user agent=" + mUserAgent)
          if ((mIsPost) &&
              (mPostData.size() > 0)) {
            ZS_LOG_BASIC(log("INFO") + ", content type=" + mMimeType)
            ZS_LOG_BASIC(log("INFO") + ", posted length=" + Stringize<typeof(SecureByteBlock::size_type)>(mPostData.size()-sizeof(char)).string()) // added extra NUL to buffer so must subtract to get proper length
          }
          if (Duration() != mTimeout) {
            ZS_LOG_BASIC(log("INFO") + ", timeout (ms)=" + Stringize<typeof(Duration::tick_type)>(mTimeout.total_milliseconds()).string())
          }
          ZS_LOG_BASIC("------------------------------------HTTP INFO----------------------------------")
          if (mIsPost) {
            if (mPostData.size() > 0) {
              ZS_LOG_BASIC(log("POST DATA=") + "\n" + ((const char *)(mPostData.BytePtr())))  // safe to cast BYTE * as const char * because buffer is NUL terminated
            }
            ZS_LOG_BASIC("------------------------------------HTTP INFO----------------------------------")
          }
        }
      }

      //-----------------------------------------------------------------------
      void HTTP::HTTPQuery::cleanupCurl()
      {
        AutoRecursiveLock lock(getLock());
        mOuter.reset();

        cancel();
      }

      //-----------------------------------------------------------------------
      CURL *HTTP::HTTPQuery::getCURL() const
      {
        AutoRecursiveLock lock(getLock());
        return mCurl;
      }

      //-----------------------------------------------------------------------
      void HTTP::HTTPQuery::notifyComplete(CURLcode result)
      {
        AutoRecursiveLock lock(getLock());

        if ((mCurl) &&
            (0 == mResponseCode)) {
          curl_easy_getinfo(mCurl, CURLINFO_RESPONSE_CODE, &mResponseCode);
        }

        mResultCode = result;

        if (ZS_IS_LOGGING(Debug)) {
          ZS_LOG_BASIC(log("----------------------------------HTTP COMPLETE--------------------------------"))
          bool successful = (((mResponseCode >= 200) && (mResponseCode < 400)) &&
                             (CURLE_OK == mResultCode));
          ZS_LOG_BASIC(log("INFO") + ", success=" + (successful ? "TRUE" : "FALSE"))
          ZS_LOG_BASIC(log("INFO") + ", HTTP response code=" + Stringize<typeof(mResponseCode)>(mResponseCode).string())
          ZS_LOG_BASIC(log("INFO") + ", CURL result code=" + Stringize<typeof(mResultCode)>(mResultCode).string())
          ZS_LOG_BASIC(log("----------------------------------HTTP COMPLETE--------------------------------"))
        }
        cleanupCurl();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark HTTP::HTTPQuery => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String HTTP::HTTPQuery::log(const char *message) const
      {
        return String("HTTPQuery [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      RecursiveLock &HTTP::HTTPQuery::getLock() const
      {
        HTTPPtr outer = mOuter.lock();
        if (outer) return outer->getLock();
        return mBogusLock;
      }

      //-----------------------------------------------------------------------
      size_t HTTP::HTTPQuery::writeHeader(
                                          void *ptr,
                                          size_t size,
                                          size_t nmemb,
                                          void *userdata
                                          )
      {
        HTTPQueryPtr pThis = ((HTTPQuery *)userdata)->mThisWeak.lock();

        AutoRecursiveLock lock(pThis->getLock());

        if (!pThis->mDelegate) {
          return 0;
        }

        pThis->mHeader.Put((BYTE *)ptr, size*nmemb);

        if (ZS_IS_LOGGING(Trace)) {
          ZS_LOG_BASIC(pThis->log("----------------------------HTTP HEADER DATA RECEIVED--------------------------"))

          SecureByteBlock buffer;
          buffer.CleanNew(size * nmemb + sizeof(char));
          memcpy(buffer.BytePtr(), ptr, size * nmemb);

          ZS_LOG_BASIC(pThis->log("HEADER=") + "\n" + ((const char *)(buffer.BytePtr())))
          ZS_LOG_BASIC(pThis->log("----------------------------HTTP HEADER DATA RECEIVED--------------------------"))
        }

        if ((pThis->mCurl) &&
            (0 == pThis->mResponseCode)) {
          curl_easy_getinfo(pThis->mCurl, CURLINFO_RESPONSE_CODE, &(pThis->mResponseCode));
        }

        try {
          pThis->mDelegate->onHTTPReadDataAvailable(pThis);
        } catch(IHTTPQueryDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, pThis->log("delegate gone"))
        }

        return size*nmemb;
      }

      //-----------------------------------------------------------------------
      size_t HTTP::HTTPQuery::writeData(
                                        char *ptr,
                                        size_t size,
                                        size_t nmemb,
                                        void *userdata
                                        )
      {
        HTTPQueryPtr pThis = ((HTTPQuery *)userdata)->mThisWeak.lock();

        AutoRecursiveLock lock(pThis->getLock());

        if (!pThis->mDelegate) {
          return 0;
        }

        //pThis->mBody.LazyPut((BYTE *)ptr, size*nmemb);
        pThis->mBody.Put((BYTE *)ptr, size*nmemb);

        if (ZS_IS_LOGGING(Trace)) {
          ZS_LOG_BASIC(pThis->log("-----------------------------HTTP BODY DATA RECEIVED---------------------------"))

          SecureByteBlock buffer;
          buffer.CleanNew(size * nmemb + sizeof(char));
          memcpy(buffer.BytePtr(), ptr, size * nmemb);

          ZS_LOG_BASIC(pThis->log("BODY=") + "\n" + ((const char *)(buffer.BytePtr())))
          ZS_LOG_BASIC(pThis->log("-----------------------------HTTP BODY DATA RECEIVED---------------------------"))
        }

        if ((pThis->mCurl) &&
            (0 == pThis->mResponseCode)) {
          curl_easy_getinfo(pThis->mCurl, CURLINFO_RESPONSE_CODE, &(pThis->mResponseCode));
        }

        try {
          pThis->mDelegate->onHTTPReadDataAvailable(pThis);
        } catch(IHTTPQueryDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, pThis->log("delegate gone"))
        }

        return size*nmemb;
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IHTTP
    #pragma mark

    //-------------------------------------------------------------------------
    IHTTPQueryPtr IHTTP::get(
                             IHTTPQueryDelegatePtr delegate,
                             const char *userAgent,
                             const char *url,
                             Duration timeout
                             )
    {
      return internal::HTTP::get(delegate, userAgent, url, timeout);
    }

    //-------------------------------------------------------------------------
    IHTTPQueryPtr IHTTP::post(
                              IHTTPQueryDelegatePtr delegate,
                              const char *userAgent,
                              const char *url,
                              const char *postData,
                              const char *postDataMimeType,
                              Duration timeout
                              )
    {
      return IHTTP::post(delegate, userAgent, url, (const BYTE *)postData, (postData ? strlen(postData) : 0), postDataMimeType, timeout);
    }

    //-------------------------------------------------------------------------
    IHTTPQueryPtr IHTTP::post(
                              IHTTPQueryDelegatePtr delegate,
                              const char *userAgent,
                              const char *url,
                              const BYTE *postData,
                              ULONG postDataLengthInBytes,
                              const char *postDataMimeType,
                              Duration timeout
                              )
    {
      return internal::HTTP::post(delegate, userAgent, url, postData, postDataLengthInBytes, postDataMimeType, timeout);
    }
  }
}
