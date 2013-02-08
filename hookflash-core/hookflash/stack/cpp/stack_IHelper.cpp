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

#include <hookflash/stack/internal/stack_MessageRequester.h>
#include <hookflash/stack/internal/stack_MessageRequesterManager.h>
#include <hookflash/stack/message/Message.h>
#include <hookflash/stack/IHelper.h>
#include <hookflash/services/IHelper.h>
#include <zsLib/Log.h>

#include <cryptopp/hex.h>

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

using namespace zsLib::XML;

namespace hookflash
{
  namespace stack
  {
    typedef zsLib::String String;
    typedef zsLib::BYTE BYTE;
    typedef zsLib::ULONG ULONG;
    typedef CryptoPP::HexEncoder HexEncoder;
    typedef CryptoPP::HexDecoder HexDecoder;
    typedef CryptoPP::StringSink StringSink;
    typedef CryptoPP::ByteQueue ByteQueue;

    namespace internal
    {

      String convertToBase64(
                             const BYTE *buffer,
                             ULONG bufferLengthInBytes
                             );
      void convertFromBase64(
                             const zsLib::String &input,
                             SecureByteBlock &output
                             );
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    zsLib::String IHelper::randomString(UINT lengthInChars)
    {
      return services::IHelper::randomString(lengthInChars);
    }

    //-------------------------------------------------------------------------
    String IHelper::convertToBase64(
                                    const BYTE *buffer,
                                    ULONG bufferLengthInBytes
                                    )
    {
      return internal::convertToBase64(buffer, bufferLengthInBytes);
    }

    //-------------------------------------------------------------------------
    String IHelper::convertToBase64(const String &input)
    {
      if (input.isEmpty()) return String();
      return IHelper::convertToBase64((const BYTE *)(input.c_str()), input.length());
    }

    //-------------------------------------------------------------------------
    void IHelper::convertFromBase64(
                                    const String &input,
                                    SecureByteBlock &output
                                    )
    {
      internal::convertFromBase64(input, output);
    }

    //-------------------------------------------------------------------------
    String IHelper::convertFromBase64(const String &input)
    {
      if (input.isEmpty()) return String();

      SecureByteBlock output;
      internal::convertFromBase64(input, output);
      
      SecureByteBlock outputFinal;
      outputFinal.CleanNew(output.SizeInBytes()+sizeof(char));

      memcpy(outputFinal, output, output.SizeInBytes());

      return (const char *)((const BYTE *)(outputFinal));
    }

    //-------------------------------------------------------------------------
    String IHelper::convertToHex(
                                 const BYTE *buffer,
                                 ULONG bufferLengthInBytes
                                 )
    {
      String result;

      HexEncoder encoder(new StringSink(result));
      encoder.Put(buffer, bufferLengthInBytes);
      encoder.MessageEnd();

      return result;
    }

    //-------------------------------------------------------------------------
    void IHelper::convertFromHex(
                                 const String &input,
                                 SecureByteBlock &output
                                 )
    {
      ByteQueue queue;
      queue.Put((BYTE *)input.c_str(), input.size());

      ByteQueue *outputQueue = new ByteQueue;
      HexDecoder decoder(outputQueue);
      queue.CopyTo(decoder);
      decoder.MessageEnd();

      size_t outputLengthInBytes = outputQueue->CurrentSize();
      output.CleanNew(outputLengthInBytes);

      outputQueue->Get(output, outputLengthInBytes);
    }

    //-------------------------------------------------------------------------
    void IHelper::split(
                        const String &input,
                        SplitMap &outResult,
                        char splitChar
                        )
    {
      if (0 == input.size()) return;

      size_t start = input.find(splitChar);
      size_t end = String::npos;

      Index index = 0;
      if (String::npos == start) {
        outResult[index] = input;
        return;
      }

      if (0 != start) {
        // special case where start is not a /
        outResult[index] = input.substr(0, start);
        ++index;
      }

      do {
        end = input.find(splitChar, start+1);

        if (end == String::npos) {
          // there is no more splits left so copy from start / to end
          outResult[index] = input.substr(start+1);
          ++index;
          break;
        }

        // take the mid-point of the string
        if (end != start+1) {
          outResult[index] = input.substr(start+1, end-(start+1));
          ++index;
        }

        // the next starting point will be the current end point
        start = end;
      } while (true);
    }

    //-------------------------------------------------------------------------
    const zsLib::String &IHelper::get(
                                      const SplitMap &inResult,
                                      Index index
                                      )
    {
      static String empty;
      SplitMap::const_iterator found = inResult.find(index);
      if (found == inResult.end()) return empty;
      return (*found).second;
    }
  }
}
