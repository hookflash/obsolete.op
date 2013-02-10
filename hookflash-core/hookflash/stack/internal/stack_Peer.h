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

#include <hookflash/stack/IPeer.h>
#include <hookflash/stack/internal/types.h>

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
      #pragma mark IPeerForAccount
      #pragma mark

      interaction IPeerForAccount
      {
        IPeerForAccount &forAccount() {return *this;}
        const IPeerForAccount &forAccount() const {return *this;}

        static PeerPtr create(
                              AccountPtr account,
                              IPeerFilePublicPtr peerFilePublic
                              );

        virtual String getPeerURI() const = 0;

        virtual IPeerFilePublicPtr getPeerFilePublic() const = 0;

        virtual void setPeerFilePublic(IPeerFilePublicPtr peerFilePublic) = 0;

        virtual String getDebugValueString(bool includeCommaPrefix = true) const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPeerForLocation
      #pragma mark

      interaction IPeerForLocation
      {
        IPeerForLocation &forLocation() {return *this;}
        const IPeerForLocation &forLocation() const {return *this;}

        static PeerPtr create(
                              AccountPtr account,
                              const char *peerURI
                              );

        static bool isValid(const char *peerURI);

        virtual AccountPtr getAccount() const = 0;

        virtual String getPeerURI() const = 0;

        virtual String getDebugValueString(bool includeCommaPrefix = true) const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPeerForMessages
      #pragma mark

      interaction IPeerForMessages
      {
        IPeerForMessages &forMessages() {return *this;}
        const IPeerForMessages &forMessages() const {return *this;}

        static PeerPtr create(
                              AccountPtr account,
                              IPeerFilePublicPtr peerFilePublic
                              );

        static PeerPtr getFromSignature(
                                        AccountPtr account,
                                        ElementPtr signedElement
                                        );

        virtual PUID getID() const = 0;

        virtual String getPeerURI() const = 0;

        virtual IPeerFilePublicPtr getPeerFilePublic() const = 0;

        virtual bool verifySignature(ElementPtr signedElement) const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPeerForPeerSubscription
      #pragma mark

      interaction IPeerForPeerSubscription
      {
        IPeerForPeerSubscription &forPeerSubscription() {return *this;}
        const IPeerForPeerSubscription &forPeerSubscription() const {return *this;}

        virtual AccountPtr getAccount() const = 0;

        virtual String getPeerURI() const = 0;

        virtual String getDebugValueString(bool includeCommaPrefix = true) const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPeerForPeerPublicationRepository
      #pragma mark

      interaction IPeerForPeerPublicationRepository
      {
        IPeerForPeerPublicationRepository &forRepo() {return *this;}
        const IPeerForPeerPublicationRepository &forRepo() const {return *this;}

        virtual String getPeerURI() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Peer
      #pragma mark

      class Peer : public IPeer,
                   public IPeerForAccount,
                   public IPeerForLocation,
                   public IPeerForMessages,
                   public IPeerForPeerSubscription,
                   public IPeerForPeerPublicationRepository
      {
      public:
        friend interaction IPeerFactory;

        friend interaction IPeer;
        friend interaction IPeerForLocation;

      protected:
        Peer(
             AccountPtr account,
             IPeerFilePublicPtr peerFilePublic,
             const String &peerURI
             );

        void init();

      public:
        ~Peer();

        static PeerPtr convert(IPeerPtr peer);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Peer => IPeer
        #pragma mark

        static bool isValid(const char *peerURI);

        static bool splitURI(
                             const char *peerURI,
                             String &outDomain,
                             String &outContactID
                             );

        static String joinURI(
                              const char *domain,
                              const char *contactID
                              );

        static String toDebugString(IPeerPtr peer, bool includeCommaPrefix = true);

        static PeerPtr create(
                              IAccountPtr account,
                              IPeerFilePublicPtr peerFilePublic
                              );

        // (duplicate) static IPeerPtr create(
        //                                    IAccountPtr account,
        //                                    const char *peerURI
        //                                    );

        static PeerPtr getFromSignature(
                                        IAccountPtr account,
                                        ElementPtr signedElement
                                        );

        virtual PUID getID() const  {return mID;}

        virtual String getPeerURI() const;

        virtual IPeerFilePublicPtr getPeerFilePublic() const;
        virtual bool verifySignature(ElementPtr signedElement) const;

        virtual PeerFindStates getFindState() const;

        virtual LocationListPtr getLocationsForPeer(bool includeOnlyConnectedLocations) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Peer => IPeerForAccount
        #pragma mark

        // (duplicate) virtual String getPeerURI() const;

        // (duplicate) virtual IPeerFilePublicPtr getPeerFilePublic() const;

        virtual void setPeerFilePublic(IPeerFilePublicPtr peerFilePublic);

        // (duplciate) virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Peer => IPeerForLocation
        #pragma mark

        static PeerPtr create(
                              AccountPtr account,
                              const char *peerURI
                              );

        virtual AccountPtr getAccount() const;

        // (duplicate) virtual String getPeerURI() const;

        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Peer => IPeerForMessages
        #pragma mark

        // (duplicate) static PeerPtr create(
        //                                   AccountPtr account,
        //                                   const char *peerURI
        //                                   );

        // (duplicate) virtual PUID getID() const;

        // (duplicate) virtual String getPeerURI() const;

        // (duplicate) virtual IPeerFilePublicPtr getPeerFilePublic() const;

        // (duplicate) virtual bool verifySignature(ElementPtr signedElement) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark IPeerForPeerSubscription
        #pragma mark

        // (duplicate) virtual AccountPtr getAccount() const;

        // (duplicate) virtual String getPeerURI() const;

        // (duplciate) virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark IPeerForPeerPublicationRepository
        #pragma mark

        // (duplicate) virtual String getPeerURI() const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Peer => (internal)
        #pragma mark

        RecursiveLock &getLock() const;
        String log(const char *message) const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Peer => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mBogusLock;
        PeerWeakPtr mThisWeak;

        AccountWeakPtr mAccount;

        IPeerFilePublicPtr mPeerFilePublic;

        String mPeerURI;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPeerFactory
      #pragma mark

      interaction IPeerFactory
      {
        static IPeerFactory &singleton();

        virtual PeerPtr create(
                               IAccountPtr account,
                               IPeerFilePublicPtr peerFilePublic
                               );
        virtual PeerPtr getFromSignature(
                                         IAccountPtr account,
                                         ElementPtr signedElement
                                         );
        virtual PeerPtr create(
                               AccountPtr account,
                               const char *peerURI
                               );
      };

    }
  }
}
