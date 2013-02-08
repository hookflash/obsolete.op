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

#pragma once

#include <hookflash/stack/hookflashTypes.h>

#include <list>

namespace hookflash
{
  namespace stack
  {
    interaction IPeerFilePrivate
    {
      typedef zsLib::UINT UINT;
      typedef zsLib::String String;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef std::list<String> URIList;

      virtual ElementPtr saveToXML() const = 0;

      virtual IPeerFilesPtr getPeerFiles() const = 0;

      virtual UINT getVersionNumber() const = 0;
      virtual bool containsSection(const char *sectionID) const = 0;

      virtual bool verifyPassword(const char *password) const = 0;

      virtual void getPrivateKeyInPCKS8(
                                        const char *password,
                                        SecureByteBlock &outRaw
                                        ) const = 0;

      virtual String getContactProfileSecret(const char *password) const = 0;
      virtual ElementPtr getCaptcha(const char *password) const = 0;

      virtual void signElement(ElementPtr elementToSign) = 0;
    };
  }
}
