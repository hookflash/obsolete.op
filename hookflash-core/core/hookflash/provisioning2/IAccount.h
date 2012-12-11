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
#include <hookflash/provisioning2/hookflashTypes.h>
#include <zsLib/Proxy.h>
#include <zsLib/XML.h>

#include <CryptoPP/secblock.h>

#include <list>

namespace hookflash
{
  namespace provisioning2
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning2::IAccount
    #pragma mark
    
    interaction IAccount
    {
      typedef zsLib::WORD WORD;
      typedef zsLib::UINT UINT;
      typedef zsLib::Time Time;
      typedef zsLib::String String;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef CryptoPP::SecByteBlock SecureByteBlock;

      enum AccountStates
      {
        AccountState_Pending,
        AccountState_Ready,
        AccountState_ShuttingDown,
        AccountState_Shutdown
      };

      static const char *toString(AccountStates state);

      enum AccountErrorCodes
      {
        AccountErrorCode_None,
        AccountErrorCode_ServerCommunicationError,
        AccountErrorCode_InternalFailure,

        AccountErrorCode_TBD,
      };

      static const char *toString(AccountErrorCodes errorCode);

      typedef String IdentityURI;
      typedef std::list<IdentityURI> IdentityURIList;
      
      typedef String PeerContactURI;
      typedef std::list<PeerContactURI> PeerContactURIList;

      typedef String ContactProfileSecret;
      typedef std::pair<PeerContactURI, ContactProfileSecret> PeerContactURIAndProfileSecretPair;
      typedef std::list<PeerContactURIAndProfileSecretPair> PeerContactAndProfileSecretList;

      struct IdentityProfileAvatarInfo
      {
        String mAvatarName;
        String mAvatarURL;
        UINT mPixelWidth;
        UINT mPixelHeight;
      };

      typedef std::list<IdentityProfileAvatarInfo> IdentityProfileAvatarInfoList;

      struct Identity
      {
        IdentityURI mIdentityURI;
        String mIdentityProvider;
        
        WORD mPriority;
        WORD mWeight;
      };
      
      typedef std::list<Identity> IdentityList;

      struct IdentityLookupInfo : public Identity
      {
        String mPeerContactURI;
        String mPublicPeerFileSecret;
        Time mInfoTTL;
        
        Time mProfileLastUpdated;
        String mDisplayName;
        String mProfileRenderedURL;     // the profile as renderable in a browser window
        String mProfileProgrammaticURL; // the profile as readable by a computer

        IdentityProfileAvatarInfoList mAvatars;
      };

      // PURPOSE: Construct a provisioning account object and optionally login
      //          to the peer contact service if the private peer file
      //          information is already known in advance.
      static IAccountPtr create(
                                IStackPtr stack,
                                provisioning2::IAccountDelegatePtr provisioningDelegate,
                                hookflash::IAccountDelegatePtr openpeerDelegate,
                                const char *peerContactServiceBootstrappedDomain,       // service used to host the peer contact information, e.g. "bar.com"
                                const char *privatePeerFileSecret = NULL,               // specify if known
                                ElementPtr privatePeerFileEl = ElementPtr()             // specify if known
                                );

      // PURPOSE: Login to an existing identity or add a new identity to the
      //          identity list associated to the peer contact.
      // NOTE:    Until at least one identity login process is completed or
      //          a previous private peer file information was passed into the
      //          "IAccount::create" method, the account creation process will
      //          likely remain in the "Pending" state.
      virtual IIdentityLoginSessionPtr identityLogin(
                                                     IIdentityLoginSessionDelegatePtr delegate,
                                                     const char *identityBaseURI,     // (or full identity URI if known) (e.g. "identity://foo.com/")
                                                     const char *identityProvider     // required if identity is legacy type, otherwise domain from base URI is assumed
                                                     ) = 0;

      // PURPOSE: When the account is no longer needed it should be shutdown.
      virtual void shutdown();

      // PURPOSE: Gets a list of identities already associated to the peer
      //          contact.
      // NOTE:    Identities not previously known but found here can then be
      //          logged into using the "identityLogin" method and will likely
      //          be auto-logged during the login process (unless auto-login
      //          is not available for the identity type).
      virtual void getKnownIdentities(IdentityList &outIdentities) const = 0;

      // PURPOSE: Sets the identities associated to the peer contact (but only
      //          contacts that are already logged in or were obtained from
      //          "getKnownIdentities"
      // NOTE:    Can not add identities but can change the priority/weight or
      //          remove identities through this method.
      virtual void setKnownIdentities(const IdentityList &inIdentities) const = 0;

      virtual String getPrivatePeerFileSecret() const = 0;
      virtual ElementPtr exportPrivatePeerFile() const = 0;

      // PURPOSE: Start a lookup process to resolve identity URIs into
      //          "identityLookupInfo" structures.
      virtual IAccountIdentityLookupQueryPtr lookup(
                                                    IAccountIdentityLookupQueryDelegatePtr delegate,
                                                    const IdentityURIList &identities
                                                    ) = 0;

      // PURPOSE: Start a lookup process to convert peer contact URIs into
      //          public peer files so that contacts can be created from the
      //          peer contacts.
      virtual IAccountPeerFileLookupQueryPtr lookup(
                                                    IAccountPeerFileLookupQueryDelegatePtr delegate,
                                                    const PeerContactAndProfileSecretList &peerContacts
                                                    ) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning2::IAccountDelegate
    #pragma mark

    interaction IAccountDelegate
    {
      typedef IAccount::AccountStates AccountStates;
      typedef IAccount::AccountErrorCodes AccountErrorCodes;

      virtual void onProvisioningAccountStateChanged(
                                                     IAccountPtr account,
                                                     AccountStates state
                                                     ) = 0;

      virtual void onProvisioningAccountError(
                                              IAccountPtr account,
                                              AccountErrorCodes error
                                              ) = 0;

      virtual void onProvisioningAccountPrivatePeerFileChanged(IAccountPtr account) = 0;

      virtual void onProvisioningAccountIdentitiesChanged(IAccountPtr account) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning2::IIdentityLoginSession
    #pragma mark

    interaction IIdentityLoginSession
    {
      typedef zsLib::UINT UINT;
      typedef zsLib::Time Time;
      typedef zsLib::String String;
      typedef zsLib::XML::ElementPtr ElementPtr;

      virtual String getIdentityBase() const = 0;               // returns URL of identity base being logged in (or full identity if already known)
      virtual String getIdentityProvider() const = 0;           // returns provider providing service for the identity
      virtual String getIdentityURI() const = 0;                // returns logged in identity upon successful identity login
      
      virtual String getClientToken() const = 0;                // this is the generated client key to use for the user login webpage (see Open Peer - Protocol Specification "Identity Login Notification")
      virtual String getServerToken() const = 0;                // this is the generated server key to use for the user login webpage (see Open Peer - Protocol Specification "Identity Login Notification")
      virtual String getClientLoginSecret() const = 0;          // this is the generated encryption key to use for the user login webpage (see Open Peer - Protocol Specification "Identity Login Notification")

      virtual bool isComplete() const = 0;                      // returns true if identity login is complete
      virtual bool wasSuccessful() const = 0;                   // returns true if identity login is complete and successful
      virtual UINT getErrorCode() const = 0;                    // if not successful, this will return HTTP-like error code

      virtual String getHiddenWindowBrowserURL() const = 0;     // once the URL is known for the hidden browswer window this will return that URL
      virtual Time getLoginExpires() const = 0;                 // when the login process must complete by before it is cancelled

      virtual String getIdentityReloginToken() const = 0;       // if successful login is complete, this token can be used to relogin in as the identity
      virtual ElementPtr getCustomLoginElement() const = 0;     // optional identity specific custom login XML data for the identity

      virtual void cancel() = 0;                                // cancel the login process

      virtual void complete() = 0;                              // when the webpage login process is done, call this method to compelte the process
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning2::IIdentityLoginSessionDelegate
    #pragma mark

    interaction IIdentityLoginSessionDelegate
    {
      virtual void onIdentityLoginSessionBrowserWindowRequired(IIdentityLoginSessionPtr session) = 0;

      virtual void onIdentityLoginSessionCompleted(IIdentityLoginSessionPtr session) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning2::IAccountIdentityLookupQuery
    #pragma mark

    interaction IAccountIdentityLookupQuery
    {
      typedef IAccount::IdentityURI IdentityURI;
      typedef IAccount::IdentityURIList IdentityURIList;
      typedef IAccount::IdentityLookupInfo IdentityLookupInfo;
      
      virtual bool isComplete() const = 0;
      virtual bool didSucceed() const = 0;

      virtual void cancel() = 0;

      virtual void getIdentities(IdentityURIList &outIdentities) const = 0;
      virtual bool getLookupInfo(
                                 const IdentityURI &inIdentity,
                                 IdentityLookupInfo &outInfo
                                 ) const = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning2::IAccountIdentityLookupQueryDelegate
    #pragma mark

    interaction IAccountIdentityLookupQueryDelegate
    {
      virtual void onAccountIdentityLookupQueryComplete(IAccountIdentityLookupQueryPtr query) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning2::IAccountPeerFileLookupQuery
    #pragma mark

    interaction IAccountPeerFileLookupQuery
    {
      typedef zsLib::String String;
      typedef IAccount::PeerContactURI PeerContactURI;
      typedef IAccount::PeerContactURIList PeerContactURIList;

      virtual bool isComplete() const = 0;
      virtual bool didSucceed() const = 0;

      virtual void cancel() = 0;

      virtual void getPeerContactURIs(PeerContactURIList &outPeerContactURIs) const = 0;
      virtual String getPublicPeerFileAsString(const PeerContactURI &userID) const = 0;
    };
    
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark provisioning2::IAccountPeerFileLookupQueryDelegate
    #pragma mark
    
    interaction IAccountPeerFileLookupQueryDelegate
    {
      virtual void onAccountPeerFileLookupQueryComplete(IAccountPeerFileLookupQueryPtr query) = 0;
    };
  }
}
