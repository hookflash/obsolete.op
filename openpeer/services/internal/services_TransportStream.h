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

#include <openpeer/services/ITransportStream.h>
#include <openpeer/services/internal/types.h>

#include <list>
#include <map>

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
      #pragma mark TransportStream
      #pragma mark

      class TransportStream : public Noop,
                              public zsLib::MessageQueueAssociator,
                              public ITransportStream,
                              public ITransportStreamWriter,
                              public ITransportStreamReader
      {
      public:
        friend interaction ITransportStreamFactory;
        friend interaction ITransportStream;

        typedef ITransportStream::StreamHeader StreamHeader;
        typedef ITransportStream::StreamHeaderPtr StreamHeaderPtr;
        typedef ITransportStream::StreamHeaderWeakPtr StreamHeaderWeakPtr;

        struct Buffer
        {
          Buffer() : mRead(0) {}

          SecureByteBlockPtr mBuffer;
          ULONG mRead;
          StreamHeaderPtr mHeader;
        };

        typedef std::list<Buffer> BufferList;

      protected:
        TransportStream(
                        IMessageQueuePtr queue,
                        ITransportStreamWriterDelegatePtr writerDelegate = ITransportStreamWriterDelegatePtr(),
                        ITransportStreamReaderDelegatePtr readerDelegate = ITransportStreamReaderDelegatePtr()
                        );

        TransportStream(Noop) :
          Noop(true),
          zsLib::MessageQueueAssociator(IMessageQueuePtr()) {}

        void init();

      public:
        ~TransportStream();

        static TransportStreamPtr convert(ITransportStreamPtr stream);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark TransportStream => ITransportStream
        #pragma mark

        static String toDebugString(ITransportStreamPtr stream, bool includeCommaPrefix = true);

        static TransportStreamPtr create(
                                         ITransportStreamWriterDelegatePtr writerDelegate = ITransportStreamWriterDelegatePtr(),
                                         ITransportStreamReaderDelegatePtr readerDelegate = ITransportStreamReaderDelegatePtr()
                                         );

        virtual PUID getID() const {return mID;}

        virtual ITransportStreamWriterPtr getWriter() const;
        virtual ITransportStreamReaderPtr getReader() const;

        virtual void cancel();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark TransportStream => ITransportStreamWriter
        #pragma mark

        // (duplicate) virtual PUID getID() const;

        virtual ITransportStreamWriterSubscriptionPtr subscribe(ITransportStreamWriterDelegatePtr delegate);

        virtual ITransportStreamPtr getStream() const;

        virtual void write(
                           const BYTE *buffer,
                           ULONG bufferLengthInBytes,
                           StreamHeaderPtr header = StreamHeaderPtr()   // not always needed
                           );

        virtual void write(
                           SecureByteBlockPtr bufferToAdopt,
                           StreamHeaderPtr header = StreamHeaderPtr()   // not always needed
                           );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark TransportStream => ITransportStreamReader
        #pragma mark

        // (duplicate) virtual PUID getID() const;

        virtual ITransportStreamReaderSubscriptionPtr subscribe(ITransportStreamReaderDelegatePtr delegate);

        // (duplicate) virtual ITransportStreamPtr getStream() const;

        virtual void notifyReaderReadyToRead();

        virtual ULONG getNextReadSizeInBytes() const;

        virtual StreamHeaderPtr getNextReadHeader() const;

        virtual ULONG getTotalReadSizeAvailableInBytes() const;

        virtual ULONG read(
                          BYTE *outBuffer,
                          ULONG bufferLengthInBytes,
                          StreamHeaderPtr *outHeader = NULL
                          );

        virtual SecureByteBlockPtr read(StreamHeaderPtr *outHeader = NULL);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark StreamTransport => (internal)
        #pragma mark

        RecursiveLock &getLock() const;
        String log(const char *message) const;

        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        bool isShutdown() const {return mShutdown;}

        void notifySubscribers(
                               bool afterRead,
                               bool afterWrite
                               );

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark StreamTransport => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;
        TransportStreamWeakPtr mThisWeak;

        bool mShutdown;
        bool mReaderReady;

        bool mReadReadyNotified;
        bool mWriteReadyNotified;

        ITransportStreamWriterDelegateSubscriptions mWriterSubscriptions;
        ITransportStreamWriterSubscriptionPtr mDefaultWriterSubscription;

        ITransportStreamReaderDelegateSubscriptions mReaderSubscriptions;
        ITransportStreamReaderSubscriptionPtr mDefaultReaderSubscription;

        BufferList mBuffers;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ITransportStreamFactor
      #pragma mark

      interaction ITransportStreamFactory
      {
        static ITransportStreamFactory &singleton();

        virtual TransportStreamPtr create(
                                          ITransportStreamWriterDelegatePtr writerDelegate = ITransportStreamWriterDelegatePtr(),
                                          ITransportStreamReaderDelegatePtr readerDelegate = ITransportStreamReaderDelegatePtr()
                                          );
      };
      
    }
  }
}
