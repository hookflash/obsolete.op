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

#include <hookflash/hookflashTypes.h>

#include <zsLib/Proxy.h>
#include <zsLib/String.h>
#include <list>

namespace hookflash
{
  interaction IContact
  {
    typedef zsLib::String String;
    typedef zsLib::ULONG ULONG;
    typedef std::list<IContactPtr> ContactList;

    enum ContactTypes
    {
      ContactType_OpenPeer,
      ContactType_External
    };

    virtual IAccountPtr getAccount() const = 0;

    virtual String getContactID() const = 0;

    static IContactPtr createFromPeerFilePublic(
                                                IAccountPtr account,
                                                const char *publicPeerFile
                                                );

    virtual bool isSelf() = 0;                        // returns true if this contact represents yourself
    virtual ContactTypes getContactType() = 0;

    virtual bool isEditable() = 0;                    // only "external" contacts are editable and "self"
    virtual bool isPublicXMLEditable() = 0;           // only allow editing of the public XML if this contact is "you", same as "isSelf"

    virtual String getPublicXML() = 0;
    virtual String getPrivateXML() = 0;

    virtual bool updateProfile(
                               const char *publicXML,                           // for external contacts this MUST be null
                               const char *privateXML
                               ) = 0;

    // change version hints
    virtual ULONG getProfileVersion() = 0;
  };
}
