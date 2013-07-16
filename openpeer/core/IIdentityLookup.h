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

#include <openpeer/core/types.h>

namespace openpeer
{
  namespace core
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIdentityLookup
    #pragma mark

    interaction IIdentityLookup
    {
      static String toDebugString(IIdentityLookupPtr lookup, bool includeCommaPrefix = true);

      struct IdentityLookupInfo
      {
        String mIdentityURI;
        Time mLastUpdated;    // if already have information about this identity, copy the "mLastUpdated" from the IdentityInfo structure, otherwise leave value as Time() if information about this identity is not previously known

        IdentityLookupInfo(const RolodexContact &); // construct from a previous "RolodexContact" structure
        IdentityLookupInfo(const IdentityContact &); // construct from a previous "IdentityInfo" structure
      };
      typedef std::list<IdentityLookupInfo> IdentityLookupInfoList;

      static IIdentityLookupPtr create(
                                       IAccountPtr account,
                                       IIdentityLookupDelegatePtr delegate,
                                       const IdentityLookupInfoList &identityLookupInfos,
                                       const char *identityServiceDomain
                                       );

      virtual PUID getID() const = 0;

      virtual bool isComplete() const = 0;
      virtual bool wasSuccessful(
                                 WORD *outErrorCode,
                                 String *outErrorReason
                                 ) const = 0;

      virtual void cancel() = 0;

      virtual IdentityContactListPtr getIdentities() const = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIdentityLookupDelegate
    #pragma mark

    interaction IIdentityLookupDelegate
    {
      virtual void onIdentityLookupCompleted(IIdentityLookupPtr lookup) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::core::IIdentityLookupDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::core::IIdentityLookupPtr, IIdentityLookupPtr)
ZS_DECLARE_PROXY_METHOD_1(onIdentityLookupCompleted, IIdentityLookupPtr)
ZS_DECLARE_PROXY_END()
