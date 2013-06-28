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

#include <openpeer/core/internal/types.h>
#include <openpeer/core/IIdentity.h>

#include <openpeer/stack/IServiceIdentity.h>

#include <zsLib/MessageQueueAssociator.h>

namespace openpeer
{
  namespace core
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IIdentityForAccount
      #pragma mark

      interaction IIdentityForAccount
      {
        IIdentityForAccount &forAccount() {return *this;}
        const IIdentityForAccount &forAccount() const {return *this;}

        static IdentityPtr createFromExistingSession(IServiceIdentitySessionPtr session);

        virtual String getIdentityURI() const = 0;

        virtual IServiceIdentitySessionPtr getSession() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Identity
      #pragma mark

      class Identity : public Noop,
                       public MessageQueueAssociator,
                       public IIdentity,
                       public IIdentityForAccount,
                       public IServiceIdentitySessionDelegate
      {
      public:
        friend interaction IIdentityFactory;
        friend interaction IIdentity;

      protected:
        Identity(IMessageQueuePtr queue);
        
        Identity(Noop) : Noop(true), MessageQueueAssociator(IMessageQueuePtr()) {};

        void init();

      public:
        ~Identity();

        static IdentityPtr convert(IIdentityPtr identity);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Identity => IIdentity
        #pragma mark

        static String toDebugString(IIdentityPtr identity, bool includeCommaPrefix = true);

        static IdentityPtr login(
                                 IAccountPtr account,
                                 IIdentityDelegatePtr delegate,
                                 const char *outerFrameURLUponReload,
                                 const char *identityURI_or_identityBaseURI,
                                 const char *identityProviderDomain
                                 );

        virtual IdentityStates getState(
                                        WORD *outLastErrorCode,
                                        String *outLastErrorReason
                                        ) const;

        virtual PUID getID() const {return mID;}

        virtual bool isDelegateAttached() const;
        virtual void attachDelegate(
                                    IIdentityDelegatePtr delegate,
                                    const char *outerFrameURLUponReload
                                    );

        virtual String getIdentityURI() const;
        virtual String getIdentityProviderDomain() const;
        virtual ElementPtr getSignedIdentityBundle() const;

        virtual String getInnerBrowserWindowFrameURL() const;

        virtual void notifyBrowserWindowVisible();
        virtual void notifyBrowserWindowClosed();

        virtual ElementPtr getNextMessageForInnerBrowerWindowFrame();
        virtual void handleMessageFromInnerBrowserWindowFrame(ElementPtr message);

        virtual void cancel();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Identity => IIdentityForAccount
        #pragma mark

        static IdentityPtr createFromExistingSession(IServiceIdentitySessionPtr session);

        // (duplicate) virtual String getIdentityURI() const;

        virtual IServiceIdentitySessionPtr getSession() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Identity => IServiceIdentitySessionDelegate
        #pragma mark

        virtual void onServiceIdentitySessionStateChanged(
                                                          IServiceIdentitySessionPtr session,
                                                          SessionStates state
                                                          );

        virtual void onServiceIdentitySessionPendingMessageForInnerBrowserWindowFrame(IServiceIdentitySessionPtr session);

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Identity => (internal)
        #pragma mark

        String log(const char *message) const;
        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Identity => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;
        IdentityWeakPtr mThisWeak;

        IIdentityDelegatePtr mDelegate;

        IServiceIdentitySessionPtr mSession;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IIdentityFactory
      #pragma mark

      interaction IIdentityFactory
      {
        static IIdentityFactory &singleton();

        virtual IdentityPtr login(
                                  IAccountPtr account,
                                  IIdentityDelegatePtr delegate,
                                  const char *outerFrameURLUponReload,
                                  const char *identityURI_or_identityBaseURI,
                                  const char *identityProviderDomain
                                  );

        virtual IdentityPtr createFromExistingSession(IServiceIdentitySessionPtr session);
      };

    }
  }
}
