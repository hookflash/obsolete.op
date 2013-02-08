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
#include <hookflash/services/IICESocket.h>
#include <hookflash/services/IHelper.h>
#include <zsLib/zsTypes.h>
#include <zsLib/String.h>
#include <zsLib/IPAddress.h>
#include <hookflash/stack/IPeerFilePublic.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      typedef std::list<zsLib::String> RouteList;
      typedef std::list<zsLib::String> StringList;
      typedef hookflash::services::IICESocket::Candidate Candidate;
      typedef hookflash::services::IICESocket::CandidateList CandidateList;

      typedef stack::Location Location;
      typedef stack::LocationList LocationList;

      struct Service
      {
        typedef zsLib::String String;

        String mID;
        String mType;
        String mURL;
        String mVersion;
        String mX509Certificate;

        bool hasData() const
        {
            return !mID.isEmpty() || !mType.isEmpty() || !mVersion.isEmpty() || !mURL.isEmpty() || !mX509Certificate.isEmpty(); //|| !mIPAddress.isPortEmpty() || !mIPAddress.isAddressEmpty();
        }
      };
      typedef std::list<Service> ServiceList;


      struct Finder
      {
        typedef zsLib::String String;
        typedef zsLib::WORD WORD;
        typedef zsLib::IPAddress IPAddress;

        String mID;
        String mTransport;
        String mSRV;
        String mX509Certificate;
        WORD  mPriority;
        WORD  mWeight;

        Finder() :
          mPriority(0),
          mWeight(0)
        {}

        bool hasData() const
        {
          return !mID.isEmpty() || !mTransport.isEmpty() || !mSRV.isEmpty() || !mX509Certificate.isEmpty() || (mPriority != 0) || (mWeight != 0); //mService.hasData()
        }
      };

      typedef std::list<Finder> FinderList;

      typedef zsLib::XML::ElementPtr SignedSaltElementPtr;
      typedef std::list<SignedSaltElementPtr> SaltBundleList;

      class Message;
      typedef boost::shared_ptr<Message> MessagePtr;
      typedef boost::weak_ptr<Message> MessageWeakPtr;

      interaction IMessageHelper;
      typedef boost::shared_ptr<IMessageHelper> IMessageHelperPtr;
      typedef boost::weak_ptr<IMessageHelper> IMessageHelperWeakPtr;

      interaction IMessageFactory;
      typedef boost::shared_ptr<IMessageFactory> IMessageFactoryPtr;
      typedef boost::weak_ptr<IMessageFactory> IMessageFactoryWeakPtr;

      class MessageFactoryStack;
      typedef boost::shared_ptr<MessageFactoryStack> MessageFactoryStackPtr;
      typedef boost::weak_ptr<MessageFactoryStack> MessageFactoryStackWeakPtr;

      class MessageRequest;
      typedef boost::shared_ptr<MessageRequest> MessageRequestPtr;
      typedef boost::weak_ptr<MessageRequest> MessageRequestWeakPtr;

      class MessageResult;
      typedef boost::shared_ptr<MessageResult> MessageResultPtr;
      typedef boost::weak_ptr<MessageResult> MessageResultWeakPtr;

      class MessageNotify;
      typedef boost::shared_ptr<MessageNotify> MessageNotifyPtr;
      typedef boost::weak_ptr<MessageNotify> MessageNotifyWeakPtr;

      class MessageReply;
      typedef boost::shared_ptr<MessageReply> MessageReplyPtr;
      typedef boost::weak_ptr<MessageReply> MessageReplyWeakPtr;

      class PeerPublishRequest;
      typedef boost::shared_ptr<PeerPublishRequest> PeerPublishRequestPtr;
      typedef boost::weak_ptr<PeerPublishRequest> PeerPublishRequestWeakPtr;

      class PeerPublishResult;
      typedef boost::shared_ptr<PeerPublishResult> PeerPublishResultPtr;
      typedef boost::weak_ptr<PeerPublishResult> PeerPublishResultWeakPtr;

      class PeerGetRequest;
      typedef boost::shared_ptr<PeerGetRequest> PeerGetRequestPtr;
      typedef boost::weak_ptr<PeerGetRequest> PeerGetRequestWeakPtr;

      class PeerGetResult;
      typedef boost::shared_ptr<PeerGetResult> PeerGetResultPtr;
      typedef boost::weak_ptr<PeerGetResult> PeerGetResultWeakPtr;

      class PeerDeleteRequest;
      typedef boost::shared_ptr<PeerDeleteRequest> PeerDeleteRequestPtr;
      typedef boost::weak_ptr<PeerDeleteRequest> PeerDeleteRequestWeakPtr;

      class PeerDeleteResult;
      typedef boost::shared_ptr<PeerDeleteResult> PeerDeleteResultPtr;
      typedef boost::weak_ptr<PeerDeleteResult> PeerDeleteResultWeakPtr;

      class PeerSubscribeRequest;
      typedef boost::shared_ptr<PeerSubscribeRequest> PeerSubscribeRequestPtr;
      typedef boost::weak_ptr<PeerSubscribeRequest> PeerSubscribeRequestWeakPtr;

      class PeerSubscribeResult;
      typedef boost::shared_ptr<PeerSubscribeResult> PeerSubscribeResultPtr;
      typedef boost::weak_ptr<PeerSubscribeResult> PeerSubscribeResultWeakPtr;

      class PeerPublishNotifyRequest;
      typedef boost::shared_ptr<PeerPublishNotifyRequest> PeerPublishNotifyRequestPtr;
      typedef boost::weak_ptr<PeerPublishNotifyRequest> PeerPublishNotifyRequestWeakPtr;

      class PeerPublishNotifyResult;
      typedef boost::shared_ptr<PeerPublishNotifyResult> PeerPublishNotifyResultPtr;
      typedef boost::weak_ptr<PeerPublishNotifyResult> PeerPublishNotifyResultWeakPtr;

      class PeerToFinderSessionCreateRequest;
      typedef boost::shared_ptr<PeerToFinderSessionCreateRequest> PeerToFinderSessionCreateRequestPtr;
      typedef boost::weak_ptr<PeerToFinderSessionCreateRequest> PeerToFinderSessionCreateRequestWeakPtr;

      class PeerToFinderSessionCreateResult;
      typedef boost::shared_ptr<PeerToFinderSessionCreateResult> PeerToFinderSessionCreateResultPtr;
      typedef boost::weak_ptr<PeerToFinderSessionCreateResult> PeerToFinderSessionCreateResultWeakPtr;

      class PeerToFinderSessionDeleteRequest;
      typedef boost::shared_ptr<PeerToFinderSessionDeleteRequest> PeerToFinderSessionDeleteRequestPtr;
      typedef boost::weak_ptr<PeerToFinderSessionDeleteRequest> PeerToFinderSessionDeleteRequestWeakPtr;

      class PeerToFinderSessionDeleteResult;
      typedef boost::shared_ptr<PeerToFinderSessionDeleteResult> PeerToFinderSessionDeleteResultPtr;
      typedef boost::weak_ptr<PeerToFinderSessionDeleteResult> PeerToFinderSessionDeleteResultWeakPtr;

      class PeerKeepAliveRequest;
      typedef boost::shared_ptr<PeerKeepAliveRequest> PeerKeepAliveRequestPtr;
      typedef boost::weak_ptr<PeerKeepAliveRequest> PeerKeepAliveRequestWeakPtr;

      class PeerKeepAliveResult;
      typedef boost::shared_ptr<PeerKeepAliveResult> PeerKeepAliveResultPtr;
      typedef boost::weak_ptr<PeerKeepAliveResult> PeerKeepAliveResultWeakPtr;

      class PeerToFinderPeerLocationFindRequest;
      typedef boost::shared_ptr<PeerToFinderPeerLocationFindRequest> PeerToFinderPeerLocationFindRequestPtr;
      typedef boost::weak_ptr<PeerToFinderPeerLocationFindRequest> PeerToFinderPeerLocationFindRequestWeakPtr;

      class PeerToFinderPeerLocationFindResult;
      typedef boost::shared_ptr<PeerToFinderPeerLocationFindResult> PeerToFinderPeerLocationFindResultPtr;
      typedef boost::weak_ptr<PeerToFinderPeerLocationFindResult> PeerToFinderPeerLocationFindResultWeakPtr;

      class PeerToFinderPeerLocationFindReply;
      typedef boost::shared_ptr<PeerToFinderPeerLocationFindReply> PeerToFinderPeerLocationFindReplyPtr;
      typedef boost::weak_ptr<PeerToFinderPeerLocationFindReply> PeerToFinderPeerLocationFindReplyWeakPtr;

      class PeerToPeerPeerIdentifyRequest;
      typedef boost::shared_ptr<PeerToPeerPeerIdentifyRequest> PeerToPeerPeerIdentifyRequestPtr;
      typedef boost::weak_ptr<PeerToPeerPeerIdentifyRequest> PeerToPeerPeerIdentifyRequestWeakPtr;

      class PeerToPeerPeerIdentifyResult;
      typedef boost::shared_ptr<PeerToPeerPeerIdentifyResult> PeerToPeerPeerIdentifyResultPtr;
      typedef boost::weak_ptr<PeerToPeerPeerIdentifyResult> PeerToPeerPeerIdentifyResultWeakPtr;

      class PeerToBootstrapperFindersGetResult;
      typedef boost::shared_ptr<PeerToBootstrapperFindersGetResult> PeerToBootstrapperFindersGetResultPtr;
      typedef boost::weak_ptr<PeerToBootstrapperFindersGetResult> PeerToBootstrapperFindersGetResultWeakPtr;

      class PeerToBootstrapperFindersGetRequest;
      typedef boost::shared_ptr<PeerToBootstrapperFindersGetRequest> PeerToBootstrapperFindersGetRequestPtr;
      typedef boost::weak_ptr<PeerToBootstrapperFindersGetRequest> PeerToBootstrapperFindersGetRequestWeakPtr;

      class PeerToBootstrapperServicesGetRequest;
      typedef boost::shared_ptr<PeerToBootstrapperServicesGetRequest> PeerToBootstrapperServicesGetRequestPtr;
      typedef boost::weak_ptr<PeerToBootstrapperServicesGetRequest> PeerToBootstrapperServicesGetRequestWeakPtr;

      class PeerToBootstrapperServicesGetResult;
      typedef boost::shared_ptr<PeerToBootstrapperServicesGetResult> PeerToBootstrapperServicesGetResultPtr;
      typedef boost::weak_ptr<PeerToBootstrapperServicesGetResult> PeerToBootstrapperServicesGetResultWeakPtr;

      class PeerToSaltSignedSaltGetResult;
      typedef boost::shared_ptr<PeerToSaltSignedSaltGetResult> PeerToSaltSignedSaltGetResultPtr;
      typedef boost::weak_ptr<PeerToSaltSignedSaltGetResult> PeerToSaltSignedSaltGetResultWeakPtr;

      class PeerToSaltSignedSaltGetRequest;
      typedef boost::shared_ptr<PeerToSaltSignedSaltGetRequest> PeerToSaltSignedSaltGetRequestPtr;
      typedef boost::weak_ptr<PeerToSaltSignedSaltGetRequest> PeerToSaltSignedSaltGetRequestWeakPtr;
    }
  }
}
