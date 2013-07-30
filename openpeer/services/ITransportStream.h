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

#include <openpeer/services/types.h>

namespace openpeer
{
  namespace services
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ITransportStream
    #pragma mark

    interaction ITransportStream
    {
      interaction StreamHeader
      {
        virtual ~StreamHeader() {}  // needs virtual method to ensure dynamic casting works
      };
      typedef boost::shared_ptr<StreamHeader> StreamHeaderPtr;
      typedef boost::weak_ptr<StreamHeader> StreamHeaderWeakPtr;
      typedef ULONG BufferLength;

      static String toDebugString(ITransportStreamPtr stream, bool includeCommaPrefix = true);

      static ITransportStreamPtr create(
                                        ITransportStreamWriterDelegatePtr writerDelegate = ITransportStreamWriterDelegatePtr(),
                                        ITransportStreamReaderDelegatePtr readerDelegate = ITransportStreamReaderDelegatePtr()
                                        );

      virtual PUID getID() const = 0;

      virtual ITransportStreamWriterPtr getWriter() const = 0;
      virtual ITransportStreamReaderPtr getReader() const = 0;

      virtual void cancel() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ITransportStreamWriter
    #pragma mark

    interaction ITransportStreamWriter
    {
      typedef ITransportStream::StreamHeader StreamHeader;
      typedef ITransportStream::StreamHeaderPtr StreamHeaderPtr;
      typedef ITransportStream::StreamHeaderWeakPtr StreamHeaderWeakPtr;

      virtual PUID getID() const = 0; // returns the same ID as the stream

      //-----------------------------------------------------------------------
      // PURPOSE: Subscribe to receive events when the write buffer is empty
      //          and available for more data to be written
      virtual ITransportStreamWriterSubscriptionPtr subscribe(ITransportStreamWriterDelegatePtr delegate) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: obtain the associated transport stream
      virtual ITransportStreamPtr getStream() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: clears out all data pending in the stream and prevents any
      //          more data being read/written
      virtual void cancel() = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Write buffer into the stream
      virtual void write(
                         const BYTE *buffer,
                         ULONG bufferLengthInBytes,
                         StreamHeaderPtr header = StreamHeaderPtr()   // not always needed
                         ) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Write buffer into the stream
      virtual void write(
                         SecureByteBlockPtr bufferToAdopt,
                         StreamHeaderPtr header = StreamHeaderPtr()   // not always needed
                         ) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ITransportStreamWriterDelegate
    #pragma mark

    interaction ITransportStreamWriterDelegate
    {
      virtual void onTransportStreamWriterReady(ITransportStreamWriterPtr writer) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ITransportStreamWriterSubscription
    #pragma mark

    interaction ITransportStreamWriterSubscription
    {
      virtual PUID getID() const = 0;

      virtual void cancel() = 0;

      virtual void background() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ITransportStreamReader
    #pragma mark

    interaction ITransportStreamReader
    {
      typedef ITransportStream::StreamHeader StreamHeader;
      typedef ITransportStream::StreamHeaderPtr StreamHeaderPtr;
      typedef ITransportStream::StreamHeaderWeakPtr StreamHeaderWeakPtr;

      virtual PUID getID() const = 0; // returns the same ID as the stream

      //-----------------------------------------------------------------------
      // PURPOSE: Subscribe to receive events when read data is available
      virtual ITransportStreamReaderSubscriptionPtr subscribe(ITransportStreamReaderDelegatePtr delegate) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: obtain the associated transport stream
      virtual ITransportStreamPtr getStream() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: clears out all data pending in the stream and prevents any
      //          more data being read/written
      virtual void cancel() = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Obtains the size of the next "written" buffer available to
      //          be read.
      // NOTE:    This buffer will match the next FIFO size buffer written to
      //          the ITransportWriter (and not the total amount written thus
      //          far).
      virtual ULONG getNextReadSizeInBytes() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Obtains the header for the next FIFO buffer written
      // NOTE:    Will return StreamHeaderPtr() if no header is available (or
      //          no buffer is available).
      virtual StreamHeaderPtr getNextReadHeader() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Obtains the size of all data "written" buffer available to
      //          be read.
      // NOTE:    This buffer will match the sum of all FIFO buffered data
      //          written to the ITransportWriter (and not any individual
      //          written buffer).
      virtual ULONG getTotalReadSizeAvailableInBytes() const = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Reads buffered data written to the stream.
      // NOTE:    If the buffer size is equal to the next FIFO buffer written
      //          this method will read the entire written buffer at once
      //          and the header value will point to that buffer's header.
      //          If the read size is less than the next written FIFO buffer,
      //          this method will partially read part of the next FIFO buffer
      //          and return the written header of that buffer. If the read
      //          size is greater than the next FIFO buffer, it will read
      //          beyond the next available FIFO buffer but return the header
      //          of only the first available FIFO buffer.
      virtual ULONG read(
                        BYTE *outBuffer,
                        ULONG bufferLengthInBytes,
                        StreamHeaderPtr *outHeader = NULL
                        ) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Reads buffered data written to the write buffer with exactly
      //          the size written.
      virtual SecureByteBlockPtr read(
                                      StreamHeaderPtr *outHeader = NULL
                                      ) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ITransportStreamReaderDelegate
    #pragma mark

    interaction ITransportStreamReaderDelegate
    {
      virtual void onTransportStreamReaderReady(ITransportStreamReaderPtr reader) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ITransportReaderSubscription
    #pragma mark

    interaction ITransportStreamReaderSubscription
    {
      virtual PUID getID() const = 0;

      virtual void cancel() = 0;

      virtual void background() = 0;
    };
    
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::services::ITransportStreamWriterDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::services::ITransportStreamWriterPtr, ITransportStreamWriterPtr)
ZS_DECLARE_PROXY_METHOD_1(onTransportStreamWriterReady, ITransportStreamWriterPtr)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_SUBSCRIPTIONS_BEGIN(openpeer::services::ITransportStreamWriterDelegate, openpeer::services::ITransportStreamWriterSubscription)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_TYPEDEF(openpeer::services::ITransportStreamWriterPtr, ITransportStreamWriterPtr)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_METHOD_1(onTransportStreamWriterReady, ITransportStreamWriterPtr)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_END()

ZS_DECLARE_PROXY_BEGIN(openpeer::services::ITransportStreamReaderDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::services::ITransportStreamReaderPtr, ITransportStreamReaderPtr)
ZS_DECLARE_PROXY_METHOD_1(onTransportStreamReaderReady, ITransportStreamReaderPtr)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_SUBSCRIPTIONS_BEGIN(openpeer::services::ITransportStreamReaderDelegate, openpeer::services::ITransportStreamReaderSubscription)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_TYPEDEF(openpeer::services::ITransportStreamReaderPtr, ITransportStreamReaderPtr)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_METHOD_1(onTransportStreamReaderReady, ITransportStreamReaderPtr)
ZS_DECLARE_PROXY_SUBSCRIPTIONS_END()
