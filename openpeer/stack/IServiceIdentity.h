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

#include <openpeer/stack/types.h>
#include <openpeer/stack/message/types.h>

namespace openpeer
{
  namespace stack
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceIdentity
    #pragma mark

    interaction IServiceIdentity
    {
      static bool isValid(const char *identityURI);
      static bool isValidBase(const char *identityBase);
      static bool isLegacy(const char *identityURI);

      static bool splitURI(
                           const char *identityURI,
                           String &outDomainOrLegacyType,
                           String &outIdentifier,
                           bool *outIsLegacy = NULL
                           );

      static String joinURI(
                            const char *domainOrType,
                            const char *identifier
                            );


      static IServiceIdentityPtr createServiceIdentityFrom(IBootstrappedNetworkPtr bootstrappedNetwork);
      static IServiceIdentityPtr createServiceIdentityFromIdentityProofBundle(ElementPtr identityProofBundleEl);

      virtual PUID getID() const = 0;

      virtual IBootstrappedNetworkPtr getBootstrappedNetwork() const = 0;

      virtual String getDomain() const = 0;

      // PURPOSE: check if an identity proof provided is valid
      // NOTE:    - will use "createServiceIdentityFromIdentityProofBundle" to create an IServiceIdentity
      //          - bootstrapped network returned from "getBootstrappedNetwork()" must return true for its methods
      //            "isPreparationComplete()" and "wasSuccessful()" or this method will return "false".
      static bool isValidIdentityProofBundle(
                                             ElementPtr identityProofBundleEl,
                                             IPeerFilePublicPtr peerFilePublic, // recommended optional check of associated peer file, can pass in IPeerFilePublicPtr() if not known yet
                                             String *outPeerURI = NULL,
                                             String *outIdentityURI = NULL,
                                             String *outStableID = NULL,
                                             Time *outCreated = NULL,
                                             Time *outExpires = NULL
                                             );

      // PURPOSE: query if an identity proof is valid asynchronously
      // NOTE:    bootstrapped network returned from "getBootstrappedNetwork" does not necessarily have to be prepared yet
      static IServiceIdentityProofBundleQueryPtr isValidIdentityProofBundle(
                                                                            ElementPtr identityProofBundleEl,
                                                                            IServiceIdentityProofBundleQueryDelegatePtr delegate,
                                                                            IPeerFilePublicPtr peerFilePublic, // optional recommended check of associated peer file, can pass in IPeerFilePublicPtr() if not known yet
                                                                            String *outPeerURI = NULL,
                                                                            String *outIdentityURI = NULL,
                                                                            String *outStableID = NULL,
                                                                            Time *outCreated = NULL,
                                                                            Time *outExpires = NULL
                                                                            );
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceIdentitySession
    #pragma mark

    interaction IServiceIdentitySession
    {
      typedef message::IdentityInfo IdentityInfo;
      typedef message::IdentityInfoListPtr IdentityInfoListPtr;

      enum SessionStates
      {
        SessionState_Pending,
        SessionState_WaitingAttachmentOfDelegate,
        SessionState_WaitingForBrowserWindowToBeLoaded,
        SessionState_WaitingForBrowserWindowToBeMadeVisible,
        SessionState_WaitingForBrowserWindowToClose,
        SessionState_WaitingForAssociationToLockbox,
        SessionState_Ready,
        SessionState_Shutdown,
      };

      static const char *toString(SessionStates state);

      static String toDebugString(IServiceIdentitySessionPtr session, bool includeCommaPrefix = true);

      static IServiceIdentitySessionPtr loginWithIdentity(
                                                          IServiceIdentitySessionDelegatePtr delegate,
                                                          IServiceIdentityPtr provider,
                                                          IServiceNamespaceGrantSessionPtr grantSession,
                                                          IServiceLockboxSessionPtr existingLockbox,  // pass NULL IServiceLockboxSessionPtr() if none exists
                                                          const char *outerFrameURLUponReload,
                                                          const char *identityURI_or_identityBaseURI  // NULL is legal if wanting to login with just "provider"
                                                          );

      static IServiceIdentitySessionPtr loginWithIdentityPreauthorized(
                                                                       IServiceIdentitySessionDelegatePtr delegate,
                                                                       IServiceIdentityPtr provider,
                                                                       IServiceNamespaceGrantSessionPtr grantSession,
                                                                       IServiceLockboxSessionPtr existingLockbox,  // pass NULL IServiceLockboxSessionPtr() if none exists
                                                                       const char *identityURI,
                                                                       const char *identityAccessToken,
                                                                       const char *identityAccessSecret,
                                                                       Time identityAccessSecretExpires
                                                                       );

      virtual PUID getID() const = 0;

      virtual IServiceIdentityPtr getService() const = 0;

      virtual SessionStates getState(
                                     WORD *outLastErrorCode,
                                     String *outLastErrorReason
                                     ) const = 0;

      virtual bool isDelegateAttached() const = 0;
      virtual void attachDelegate(
                                  IServiceIdentitySessionDelegatePtr delegate,
                                  const char *outerFrameURLUponReload
                                  ) = 0;
      virtual void attachDelegateAndPreauthorizeLogin(
                                                      IServiceIdentitySessionDelegatePtr delegate,
                                                      const char *identityAccessToken,
                                                      const char *identityAccessSecret,
                                                      Time identityAccessSecretExpires
                                                      ) = 0;

      virtual String getIdentityURI() const = 0;
      virtual String getIdentityProviderDomain() const = 0;

      virtual void getIdentityInfo(IdentityInfo &outIdentityInfo) const = 0;

      virtual String getInnerBrowserWindowFrameURL() const = 0;

      virtual void notifyBrowserWindowVisible() = 0;
      virtual void notifyBrowserWindowClosed() = 0;

      virtual DocumentPtr getNextMessageForInnerBrowerWindowFrame() = 0;
      virtual void handleMessageFromInnerBrowserWindowFrame(DocumentPtr unparsedMessage) = 0;

      virtual void startRolodexDownload(const char *inLastDownloadedVersion = NULL) = 0;  // if a previous version of the rolodex was downloaded/stored, pass in the version of the last information downloaded to prevent redownloading infomration again
      virtual void refreshRolodexContacts() = 0;                                          // force a refresh of the contact list
      virtual bool getDownloadedRolodexContacts(                                          // returns "true" if rolodex contacts were obtained, otherwise returns "false"
                                                bool &outFlushAllRolodexContacts,         // if true, all rolodex contacts for this identity must be flushed out entirely
                                                String &outVersionDownloaded,             // returns version information of downloaded rolodex contacts
                                                IdentityInfoListPtr &outRolodexContacts    // output list of contacts of rolodex contacts
                                                ) = 0;

      virtual void cancel() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceIdentitySessionDelegate
    #pragma mark

    interaction IServiceIdentitySessionDelegate
    {
      typedef IServiceIdentitySession::SessionStates SessionStates;

      virtual void onServiceIdentitySessionStateChanged(
                                                        IServiceIdentitySessionPtr session,
                                                        SessionStates state
                                                        ) = 0;

      virtual void onServiceIdentitySessionPendingMessageForInnerBrowserWindowFrame(IServiceIdentitySessionPtr session) = 0;

      virtual void onServiceIdentitySessionRolodexContactsDownloaded(IServiceIdentitySessionPtr identity) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceIdentityProofBundleQuery
    #pragma mark

    interaction IServiceIdentityProofBundleQuery
    {
      virtual bool isComplete() const = 0;
      virtual bool wasSuccessful(
                                 WORD *outErrorCode = NULL,
                                 String *outErrorReason = NULL
                                 ) const = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IServiceIdentityProofBundleQueryDelegate
    #pragma mark

    interaction IServiceIdentityProofBundleQueryDelegate
    {
      virtual void onServiceIdentityProofBundleQueryCompleted(IServiceIdentityProofBundleQueryPtr query) = 0;
    };
    
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::stack::IServiceIdentitySessionDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::stack::IServiceIdentitySessionPtr, IServiceIdentitySessionPtr)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::stack::IServiceIdentitySessionDelegate::SessionStates, SessionStates)
ZS_DECLARE_PROXY_METHOD_2(onServiceIdentitySessionStateChanged, IServiceIdentitySessionPtr, SessionStates)
ZS_DECLARE_PROXY_METHOD_1(onServiceIdentitySessionPendingMessageForInnerBrowserWindowFrame, IServiceIdentitySessionPtr)
ZS_DECLARE_PROXY_METHOD_1(onServiceIdentitySessionRolodexContactsDownloaded, IServiceIdentitySessionPtr)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(openpeer::stack::IServiceIdentityProofBundleQueryDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::stack::IServiceIdentityProofBundleQueryPtr, IServiceIdentityProofBundleQueryPtr)
ZS_DECLARE_PROXY_METHOD_1(onServiceIdentityProofBundleQueryCompleted, IServiceIdentityProofBundleQueryPtr)
ZS_DECLARE_PROXY_END()
