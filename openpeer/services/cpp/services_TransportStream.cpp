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

#include <openpeer/services/internal/services_TransportStream.h>
#include <openpeer/services/internal/services_Helper.h>

#include <zsLib/Log.h>
#include <zsLib/helpers.h>

#include <zsLib/Stringize.h>


namespace openpeer { namespace services { ZS_DECLARE_SUBSYSTEM(openpeer_services) } }

namespace openpeer
{
  namespace services
  {
    using zsLib::Stringize;

    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark TransportStream
      #pragma mark

      //-----------------------------------------------------------------------
      TransportStream::TransportStream(
                                       IMessageQueuePtr queue,
                                       ITransportStreamWriterDelegatePtr writerDelegate,
                                       ITransportStreamReaderDelegatePtr readerDelegate
                                       ) :
        zsLib::MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mShutdown(false)
      {
        ZS_LOG_DEBUG(log("created"))
        if (writerDelegate) {
          mDefaultWriterSubscription = mWriterSubscriptions.subscribe(writerDelegate);
        }
        if (readerDelegate) {
          mDefaultReaderSubscription = mReaderSubscriptions.subscribe(readerDelegate);
        }
      }

      //-----------------------------------------------------------------------
      void TransportStream::init()
      {
      }

      //-----------------------------------------------------------------------
      TransportStream::~TransportStream()
      {
        ZS_LOG_DEBUG(log("destroyed"))
        mThisWeak.reset();
        cancel();
      }

      //-----------------------------------------------------------------------
      TransportStreamPtr TransportStream::convert(ITransportStreamPtr stream)
      {
        return boost::dynamic_pointer_cast<TransportStream>(stream);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark TransportStream => ITransportStream
      #pragma mark

      //-----------------------------------------------------------------------
      String TransportStream::toDebugString(ITransportStreamPtr stream, bool includeCommaPrefix)
      {
        if (!stream) return String(includeCommaPrefix ? ", stream=(null)" : "stream=(null)");

        TransportStreamPtr pThis = TransportStream::convert(stream);
        return pThis->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      TransportStreamPtr TransportStream::create(
                                                 ITransportStreamWriterDelegatePtr writerDelegate,
                                                 ITransportStreamReaderDelegatePtr readerDelegate
                                                 )
      {
        TransportStreamPtr pThis(new TransportStream(IHelper::getServiceQueue(), writerDelegate, readerDelegate));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      ITransportStreamWriterPtr TransportStream::getWriter() const
      {
        return mThisWeak.lock();
      }

      //-----------------------------------------------------------------------
      ITransportStreamReaderPtr TransportStream::getReader() const
      {
        return mThisWeak.lock();
      }

      //-----------------------------------------------------------------------
      void TransportStream::cancel()
      {
        AutoRecursiveLock lock(getLock());
        mShutdown = true;

        mWriterSubscriptions.clear();
        mDefaultWriterSubscription.reset();

        mReaderSubscriptions.clear();
        mDefaultReaderSubscription.reset();

        mBuffers.clear();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark TransportStream => ITransportStreamWriter
      #pragma mark

      //-----------------------------------------------------------------------
      ITransportStreamWriterSubscriptionPtr TransportStream::subscribe(ITransportStreamWriterDelegatePtr originalDelegate)
      {
        AutoRecursiveLock lock(getLock());

        if (!originalDelegate) {
          return mDefaultWriterSubscription;
        }

        ITransportStreamWriterSubscriptionPtr subscription = mWriterSubscriptions.subscribe(originalDelegate);

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("already shutdown"))
          subscription->cancel();
          return subscription;
        }

        if (mBuffers.size() < 1) {
          ITransportStreamWriterDelegatePtr delegate = mWriterSubscriptions.delegate(subscription);
          if (delegate) {
            delegate->onTransportStreamWriterReady(mThisWeak.lock());
          }
        }

        return subscription;
      }

      //-----------------------------------------------------------------------
      ITransportStreamPtr TransportStream::getStream() const
      {
        AutoRecursiveLock lock(getLock());
        return mThisWeak.lock();
      }

      //-----------------------------------------------------------------------
      void TransportStream::write(
                                  const BYTE *inBuffer,
                                  ULONG bufferLengthInBytes,
                                  StreamHeaderPtr header
                                  )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!inBuffer)
        ZS_THROW_INVALID_ARGUMENT_IF(bufferLengthInBytes < 1)

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("already shutdown"))
          return;
        }

        write(IHelper::convertToBuffer(inBuffer, bufferLengthInBytes), header);
      }

      //-----------------------------------------------------------------------
      void TransportStream::write(
                                  SecureByteBlockPtr bufferToAdopt,
                                  StreamHeaderPtr header
                                  )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!bufferToAdopt)
        ZS_THROW_INVALID_ARGUMENT_IF(bufferToAdopt->SizeInBytes() < 1)

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("already shutdown"))
          return;
        }

        Buffer buffer;
        buffer.mBuffer = bufferToAdopt;
        buffer.mHeader = header;

        ZS_LOG_TRACE(log("buffer written") + ", written=" + Stringize<SecureByteBlock::size_type>(bufferToAdopt->SizeInBytes()).string() )

        mBuffers.push_back(buffer);

        // only notify if this is the first buffer added (as have to wait until read called before notifying again)
        if (mBuffers.size() == 1) {
          ZS_LOG_TRACE(log("notifying buffer ready to read"))
          mReaderSubscriptions.delegate()->onTransportStreamReaderReady(mThisWeak.lock());
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark TransportStream => ITransportStreamReader
      #pragma mark

      //-----------------------------------------------------------------------
      ITransportStreamReaderSubscriptionPtr TransportStream::subscribe(ITransportStreamReaderDelegatePtr originalDelegate)
      {
        AutoRecursiveLock lock(getLock());

        if (!originalDelegate) {
          return mDefaultReaderSubscription;
        }

        ITransportStreamReaderSubscriptionPtr subscription = mReaderSubscriptions.subscribe(originalDelegate);

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("already shutdown"))
          subscription->cancel();
          return subscription;
        }

        if (mBuffers.size() > 0) {
          ITransportStreamReaderDelegatePtr delegate = mReaderSubscriptions.delegate(subscription);
          if (delegate) {
            delegate->onTransportStreamReaderReady(mThisWeak.lock());
          }
        }

        return subscription;
      }

      //-----------------------------------------------------------------------
      ULONG TransportStream::TransportStream::getNextReadSizeInBytes() const
      {
        AutoRecursiveLock lock(getLock());

        if (mBuffers.size() < 1) {
          ZS_LOG_TRACE(log("no read buffers available") + ", read size=0")
          return 0;
        }

        const Buffer &buffer = mBuffers.front();

        ULONG readSize = (buffer.mBuffer->SizeInBytes() - buffer.mRead);

        ZS_LOG_TRACE(log("read size") + ", read size=" + Stringize<typeof(readSize)>(readSize).string())

        return readSize;
      }

      //-----------------------------------------------------------------------
      TransportStream::StreamHeaderPtr TransportStream::getNextReadHeader() const
      {
        AutoRecursiveLock lock(getLock());

        if (mBuffers.size() < 1) {
          ZS_LOG_TRACE(log("no read buffers available") + ", header returned=false")
          return StreamHeaderPtr();
        }

        const Buffer &buffer = mBuffers.front();

        ZS_LOG_TRACE(log("header returned") + ", header=" + (buffer.mHeader ? "true":"false"))
        
        return buffer.mHeader;
      }

      //-----------------------------------------------------------------------
      ULONG TransportStream::getTotalReadSizeAvailableInBytes() const
      {
        AutoRecursiveLock lock(getLock());

        ULONG total = 0;
        for (BufferList::const_iterator iter = mBuffers.begin(); iter != mBuffers.end(); ++iter)
        {
          const Buffer &buffer = (*iter);
          total += (buffer.mBuffer->SizeInBytes() - buffer.mRead);
        }

        ZS_LOG_TRACE(log("total read size available") + ", read size=" + Stringize<typeof(total)>(total).string() + ", buffers=" + Stringize<BufferList::size_type>(mBuffers.size()).string())

        return total;
      }

      //-----------------------------------------------------------------------
      ULONG TransportStream::read(
                                  BYTE *outBuffer,
                                  ULONG bufferLengthInBytes,
                                  StreamHeaderPtr *outHeader
                                  )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!outBuffer)

        if (outHeader) {
          *outHeader = StreamHeaderPtr();
        }

        AutoRecursiveLock lock(getLock());

        BYTE *dest = outBuffer;

        ULONG totalRead = 0;
        bool first = true;

        while (0 != bufferLengthInBytes)
        {
          if (mBuffers.size() < 1) {
            ZS_LOG_TRACE(log("no more buffered data available to read"))
            break;
          }

          Buffer &buffer = mBuffers.front();

          if (first) {
            if (outHeader) {
              *outHeader = buffer.mHeader;
            }
            first = false;
          }

          ULONG available = (buffer.mBuffer->SizeInBytes() - buffer.mRead);

          ULONG consume = (bufferLengthInBytes > available ? available : bufferLengthInBytes);

          const BYTE *source = buffer.mBuffer->BytePtr() + buffer.mRead;

          memcpy(dest, source, consume);

          buffer.mRead += consume;
          totalRead += consume;
          bufferLengthInBytes -= consume;
          dest += consume;

          ZS_LOG_TRACE(log("buffer read") + ", read=" + Stringize<typeof(consume)>(consume).string() + ", buffer available=" + Stringize<typeof(available)>(available).string() + ", remaining=" + Stringize<typeof(bufferLengthInBytes)>(bufferLengthInBytes).string())

          if (buffer.mRead == buffer.mBuffer->SizeInBytes()) {
            // entire buffer has not been consumed, remove it
            ZS_LOG_TRACE(log("entire buffer consumed") + ", buffer size=" + Stringize<typeof(buffer.mRead)>(buffer.mRead).string())
            mBuffers.pop_front();
            continue;
          }
        }

        if (mBuffers.size() < 1) {
          ZS_LOG_TRACE(log("notifying buffer ready to write"))
          mWriterSubscriptions.delegate()->onTransportStreamWriterReady(mThisWeak.lock());
        }

        if (mBuffers.size() > 0) {
          ZS_LOG_TRACE(log("notifying buffer ready to read"))
          mReaderSubscriptions.delegate()->onTransportStreamReaderReady(mThisWeak.lock());
        }

        return totalRead;
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr TransportStream::read(StreamHeaderPtr *outHeader)
      {
        if (outHeader) {
          *outHeader = StreamHeaderPtr();
        }

        AutoRecursiveLock lock(getLock());
        if (mBuffers.size() < 1) return SecureByteBlockPtr();

        SecureByteBlockPtr result;
        // scope: read one writtenn buffer
        {
          Buffer &buffer = mBuffers.front();

          result = buffer.mBuffer;
          if (outHeader) {
            *outHeader = buffer.mHeader;
          }

          ZS_LOG_TRACE(log("buffer read") + ", read=" + Stringize<SecureByteBlock::size_type>(result->SizeInBytes()).string() )

          mBuffers.pop_front();
        }

        return result;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark TransportStream  => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &TransportStream::getLock() const
      {
        return mLock;
      }

      //-----------------------------------------------------------------------
      String TransportStream::log(const char *message) const
      {
        return String("TransportStream [" + Stringize<typeof(mID)>(mID).string() + "] " + message);
      }

      //-----------------------------------------------------------------------
      String TransportStream::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("transport stream id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("shutdown", mShutdown ? String("true") : String(), firstTime) +
               Helper::getDebugValue("writer subscriptions", mWriterSubscriptions.size() > 0 ? Stringize<ITransportStreamWriterDelegateSubscriptions::size_type>(mWriterSubscriptions.size()).string() : String(), firstTime) +
               Helper::getDebugValue("default writer subscription", mDefaultWriterSubscription ? String("true") : String(), firstTime) +
               Helper::getDebugValue("reader subscriptions", mReaderSubscriptions.size() > 0 ? Stringize<ITransportStreamReaderDelegateSubscriptions::size_type>(mReaderSubscriptions.size()).string() : String(), firstTime) +
               Helper::getDebugValue("default reader subscription", mDefaultReaderSubscription ? String("true") : String(), firstTime) +
               Helper::getDebugValue("buffers", mBuffers.size() > 0 ? Stringize<BufferList::size_type>(mBuffers.size()).string() : String(), firstTime);
      }


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
    }
    
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    #pragma mark
    #pragma mark IStreamTransport
    #pragma mark

    //-----------------------------------------------------------------------
    String ITransportStream::toDebugString(ITransportStreamPtr stream, bool includeCommaPrefix)
    {
      return internal::TransportStream::toDebugString(stream, includeCommaPrefix);
    }
    
    //-----------------------------------------------------------------------
    ITransportStreamPtr ITransportStream::create(
                                                 ITransportStreamWriterDelegatePtr writerDelegate,
                                                 ITransportStreamReaderDelegatePtr readerDelegate
                                                 )
    {
      return internal::ITransportStreamFactory::singleton().create(writerDelegate, readerDelegate);
    }
  }
}
