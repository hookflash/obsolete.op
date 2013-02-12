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

#include <hookflash/stack/internal/types.h>
#include <hookflash/stack/IBootstrappedNetwork.h>
#include <hookflash/stack/IServiceCertificates.h>

#include <zsLib/MessageQueueAssociator.h>

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceCertificatesValidateQuery
      #pragma mark

      class ServiceCertificatesValidateQuery : public Noop,
                                               public zsLib::MessageQueueAssociator,
                                               public IServiceCertificatesValidateQuery,
                                               public IBootstrappedNetworkDelegate
      {
      public:
        friend interaction IServiceCertificatesValidateQueryFactory;
        friend interaction IServiceCertificatesValidateQuery;

      protected:
        ServiceCertificatesValidateQuery(
                                        IMessageQueuePtr queue,
                                        IServiceCertificatesValidateQueryDelegatePtr delegate,
                                        ElementPtr signedElement
                                        );
        
        ServiceCertificatesValidateQuery(Noop) : Noop(true), MessageQueueAssociator(IMessageQueuePtr()) {};
        
        void init();

      public:
        ~ServiceCertificatesValidateQuery();

        static ServiceCertificatesValidateQueryPtr convert(IServiceCertificatesValidateQueryPtr query);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceCertificatesValidateQuery => IServiceCertificatesValidateQuery
        #pragma mark

        static String toDebugString(IServiceCertificatesValidateQueryPtr query, bool includeCommaPrefix = true);

        static ServiceCertificatesValidateQueryPtr queryIfValidSignature(
                                                                        IServiceCertificatesValidateQueryDelegatePtr delegate,
                                                                        ElementPtr signedElement
                                                                        );

        virtual PUID getID() const {return mID;}

        virtual IServiceCertificatesPtr getService() const;

        virtual String getSignatureDomain() const;
        virtual String getSignatureService() const;

        virtual bool isComplete() const;
        virtual bool isValidSignature(
                                      WORD *outErrorCode = NULL,
                                      String *outErrorReason = NULL
                                      ) const;

        virtual void cancel();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceCertificatesValidateQuery => IBootstrappedNetworkDelegate
        #pragma mark

        virtual void onBootstrappedNetworkPreparationCompleted(IBootstrappedNetworkPtr bootstrappedNetwork);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => (internal)
        #pragma mark

        RecursiveLock &getLock() const;
        virtual String getDebugStringValue(bool includeCommaPrefix = true) const;

        String log(const char *message) const;


      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark BootstrappedNetwork => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;
        ServiceCertificatesValidateQueryWeakPtr mThisWeak;

        IServiceCertificatesValidateQueryDelegatePtr mDelegate;

        String mCertificateID;
        String mDomain;
        String mService;

        SecureByteBlockPtr mDigest;
        SecureByteBlockPtr mDigestSigned;

        BootstrappedNetworkPtr mBootstrappedNetwork;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IServiceCertificatesValidateQueryFactory
      #pragma mark

      interaction IServiceCertificatesValidateQueryFactory
      {
        static IServiceCertificatesValidateQueryFactory &singleton();

        virtual ServiceCertificatesValidateQueryPtr queryIfValidSignature(
                                                                          IServiceCertificatesValidateQueryDelegatePtr delegate,
                                                                          ElementPtr signedElement
                                                                          );
      };

    }
  }
}
