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

#include <openpeer/stack/internal/stack.h>
#include <openpeer/stack/stack.h>

#include <zsLib/Log.h>
#include <zsLib/Stringize.h>

namespace openpeer { namespace stack { ZS_IMPLEMENT_SUBSYSTEM(openpeer_stack) } }


namespace openpeer
{
  namespace stack
  {
    using internal::Helper;
    using zsLib::string;

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Candidate
    #pragma mark

    //-------------------------------------------------------------------------
    Candidate::Candidate() :
      IICESocket::Candidate()
    {
    }

    //-------------------------------------------------------------------------
    Candidate::Candidate(const Candidate &candidate) :
      IICESocket::Candidate(candidate)
    {
      mNamespace = candidate.mNamespace;
      mTransport = candidate.mTransport;

      mAccessToken = candidate.mAccessToken;
      mAccessSecretProof = candidate.mAccessSecretProof;
    }

    //-------------------------------------------------------------------------
    Candidate::Candidate(const IICESocket::Candidate &candidate) :
      IICESocket::Candidate(candidate)
    {
    }

    //-------------------------------------------------------------------------
    bool Candidate::hasData() const
    {
      bool hasData = IICESocket::Candidate::hasData();
      if (hasData) return true;

      return ((mNamespace.hasData()) ||
              (mTransport.hasData()) ||
              (mAccessToken.hasData()) ||
              (mAccessSecretProof.hasData()));
    }

    //-------------------------------------------------------------------------
    String Candidate::getDebugValueString(bool includeCommaPrefix) const
    {
      bool firstTime = false;
      String result = IICESocket::Candidate::toDebugString(includeCommaPrefix);

      return
      result +
      Helper::getDebugValue("class", mNamespace, firstTime) +
      Helper::getDebugValue("transport", mTransport, firstTime) +
      Helper::getDebugValue("access token", mAccessToken, firstTime) +
      Helper::getDebugValue("access secret proof", mAccessSecretProof, firstTime);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark LocationInfo
    #pragma mark

    //-------------------------------------------------------------------------
    bool LocationInfo::hasData() const
    {
      return (((bool)mLocation) ||
              (!mIPAddress.isEmpty()) ||
              (mDeviceID.hasData()) ||
              (mUserAgent.hasData()) ||
              (mOS.hasData()) ||
              (mSystem.hasData()) ||
              (mHost.hasData()) ||
              (mCandidates.size() > 0));
    }

    //-------------------------------------------------------------------------
    String LocationInfo::getDebugValueString(bool includeCommaPrefix) const
    {
      bool firstTime = false;
      return ILocation::toDebugString(mLocation, includeCommaPrefix) +
             Helper::getDebugValue("IP addres", !mIPAddress.isEmpty() ? mIPAddress.string() : String(), firstTime) +
             Helper::getDebugValue("device ID", mDeviceID, firstTime) +
             Helper::getDebugValue("user agent", mUserAgent, firstTime) +
             Helper::getDebugValue("os", mOS, firstTime) +
             Helper::getDebugValue("system", mSystem, firstTime) +
             Helper::getDebugValue("host", mHost, firstTime) +
             Helper::getDebugValue("candidates", mCandidates.size() > 0 ? string(mCandidates.size()) : String(), firstTime);
    }
  }
}
