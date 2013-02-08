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

#include <hookflash/stack/types.h>


namespace hookflash
{
  namespace stack
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceCertificates
    #pragma mark

    interaction IServiceCertificates
    {
      static IServiceCertificatesPtr createServiceCertificatesFrom(IBootstrappedNetworkPtr alreadyPreparedBootstrappedNetwork);

      virtual PUID getID() const = 0;

      virtual IBootstrappedNetworkPtr getBootstrappedNetwork() const = 0;

      virtual bool isValidSignature(ElementPtr signedElement) const = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceCertificatesValidateQuery
    #pragma mark

    interaction IServiceCertificatesValidateQuery
    {
      static String toDebugString(IServiceCertificatesValidateQueryPtr query, bool includeCommaPrefix = true);

      static IServiceCertificatesValidateQueryPtr queryIfValidSignature(
                                                                       IServiceCertificatesValidateQueryDelegatePtr delegate,
                                                                       ElementPtr signedElement
                                                                       );

      virtual PUID getID() const = 0;

      virtual IServiceCertificatesPtr getService() const = 0;

      virtual String getSignatureDomain() const = 0;
      virtual String getSignatureService() const = 0;

      virtual bool isComplete() const = 0;
      virtual bool isValidSignature(
                                    WORD *outErrorCode = NULL,
                                    String *outErrorReason = NULL
                                    ) const = 0;

      virtual void cancel() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceCertificatesValidateQueryDelegate
    #pragma mark

    interaction IServiceCertificatesValidateQueryDelegate
    {
      virtual void onServiceCertificatesValidateQueryCompleted(IServiceCertificatesValidateQueryPtr query);
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::IServiceCertificatesValidateQueryDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IServiceCertificatesValidateQueryPtr, IServiceCertificatesValidateQueryPtr)
ZS_DECLARE_PROXY_METHOD_1(onServiceCertificatesValidateQueryCompleted, IServiceCertificatesValidateQueryPtr)
ZS_DECLARE_PROXY_END()
