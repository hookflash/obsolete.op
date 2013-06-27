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

#ifdef _WIN32
#define NOMINMAX
#endif //WIN32

#include <openpeer/services/internal/services_ICESocketSession.h>
#include <openpeer/services/internal/services_ICESocket.h>
#include <openpeer/services/IICESocket.h>
#include <openpeer/services/ISTUNRequester.h>
#include <zsLib/Exception.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>
#include <zsLib/helpers.h>

#include <cryptopp/osrng.h>

#define HOOKFLASH_SERVICES_ICESOCKETSESSION_MAX_WAIT_TIME_FOR_CANDIDATE_TO_ACTIVATE_IF_ALL_DONE (60)
#define HOOKFLASH_SERVICES_ICESOCKETSESSION_DEFAULT_KEEPALIVE_INDICATION_TIME_IN_SECONDS (15)

namespace hookflash { namespace services { ZS_DECLARE_SUBSYSTEM(hookflash_services) } }


namespace hookflash
{
  namespace services
  {
    namespace internal
    {
      using zsLib::Stringize;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      static QWORD randomQWORD()
      {
        BYTE buffer[sizeof(QWORD)];

        CryptoPP::AutoSeededRandomPool rng;
        rng.GenerateBlock(&(buffer[0]), sizeof(buffer));

        return *((QWORD *)(&(buffer[0])));
      }

      //-----------------------------------------------------------------------
      static QWORD calculatePriority(const IICESocket::Candidate &controlling, const IICESocket::Candidate &controlled) {
        QWORD priorityControlling = controlling.mPriority;
        QWORD priorityControlled = controlled.mPriority;

        QWORD priority = ((((QWORD)1) << 32) * (std::min(priorityControlling, priorityControlled))) +
                          (((QWORD)2) * std::max(priorityControlling, priorityControlled)) +
                          (priorityControlling > priorityControlled ? 1 : 0);
        return priority;
      }

      //-----------------------------------------------------------------------
      static bool comparePairControlling(const ICESocketSession::CandidatePairPtr pair1, const ICESocketSession::CandidatePairPtr pair2) {
        QWORD priorityPair1 = calculatePriority(pair1->mLocal, pair1->mRemote);
        QWORD priorityPair2 = calculatePriority(pair2->mLocal, pair2->mRemote);

        return priorityPair1 > priorityPair2; // pair1 comes before pair2 if pair1 priority is greater
      }

      //-----------------------------------------------------------------------
      static bool comparePairControlled(const ICESocketSession::CandidatePairPtr pair1, const ICESocketSession::CandidatePairPtr pair2) {
        QWORD priorityPair1 = calculatePriority(pair1->mRemote, pair1->mLocal);
        QWORD priorityPair2 = calculatePriority(pair2->mRemote, pair2->mLocal);

        return priorityPair1 > priorityPair2; // pair1 comes before pair2 if pair1 priority is greater
      }

      //-----------------------------------------------------------------------
      static IICESocket::Types normalize(IICESocket::Types transport)
      {
        if (transport == ICESocket::Type_Relayed)
          return ICESocket::Type_Relayed;
        return ICESocket::Type_Local;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IICESocketSessionForICESocket
      #pragma mark

      //-----------------------------------------------------------------------
      ICESocketSessionPtr IICESocketSessionForICESocket::create(
                                                                IMessageQueuePtr queue,
                                                                IICESocketSessionDelegatePtr delegate,
                                                                ICESocketPtr socket,
                                                                ICEControls control
                                                                )
      {
        return IICESocketSessionFactory::singleton().create(queue, delegate, socket, control);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocketSession::CandidatePair
      #pragma mark

      //-----------------------------------------------------------------------
      ICESocketSession::CandidatePairPtr ICESocketSession::CandidatePair::create()
      {
        CandidatePairPtr pThis(new CandidatePair);
        pThis->mLocal.mType = ICESocket::Type_Unknown;
        pThis->mLocal.mPriority = 0;
        pThis->mLocal.mLocalPreference = 0;
        pThis->mRemote.mType = ICESocket::Type_Unknown;
        pThis->mRemote.mPriority = 0;
        pThis->mRemote.mLocalPreference = 0;
        pThis->mReceivedRequest = false;
        pThis->mReceivedResponse = false;
        pThis->mFailed = false;
        return pThis;
      }

      //-----------------------------------------------------------------------
      ICESocketSession::CandidatePairPtr ICESocketSession::CandidatePair::clone() const
      {
        CandidatePairPtr pThis(new CandidatePair);
        pThis->mLocal = mLocal;
        pThis->mRemote = mRemote;
        pThis->mReceivedRequest = mReceivedRequest;
        pThis->mReceivedResponse = mReceivedResponse;
        pThis->mFailed = mFailed;
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocketSession
      #pragma mark

      //-----------------------------------------------------------------------
      ICESocketSession::ICESocketSession(
                                         IMessageQueuePtr queue,
                                         IICESocketSessionDelegatePtr delegate,
                                         ICESocketPtr socket,
                                         ICEControls control
                                         ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mCurrentState(ICESocketSessionState_Pending),
        mShutdownReason(ICESocketSessionShutdownReason_None),
        mDelegate(IICESocketSessionDelegateProxy::createWeak(queue, delegate)),
        mInformedWriteReady(false),
        mICESocketWeak(socket),
        mControl(control),
        mConflictResolver(randomQWORD()),
        mStartedSearchAt(zsLib::now()),
        mLastSentData(zsLib::now()),
        mLastActivity(zsLib::now()),
        mLastReceivedDataOrSTUN(zsLib::now()),
        mKeepAliveDuration(Seconds(HOOKFLASH_SERVICES_ICESOCKETSESSION_DEFAULT_KEEPALIVE_INDICATION_TIME_IN_SECONDS))
      {
        ZS_LOG_BASIC(log("created"))
      }

      //-----------------------------------------------------------------------
      void ICESocketSession::init()
      {

        AutoRecursiveLock lock(getLock());
        mSocketSubscription = getSocket()->subscribe(mThisWeak.lock());
        step();
      }

      //-----------------------------------------------------------------------
      ICESocketSession::~ICESocketSession()
      {
        if (isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      ICESocketSessionPtr ICESocketSession::create(
                                                   IMessageQueuePtr queue,
                                                   IICESocketSessionDelegatePtr delegate,
                                                   ICESocketPtr socket,
                                                   ICEControls control
                                                   )
      {
        ICESocketSessionPtr pThis(new ICESocketSession(queue, delegate, socket, control));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocketSession => IICESocketSession
      #pragma mark

      //-----------------------------------------------------------------------
      IICESocketPtr ICESocketSession::getSocket()
      {
        ICESocketPtr socket = mICESocketWeak.lock();
        if (!socket) return IICESocketPtr();
        return socket->forICESocketSession().getSocket();
      }

      //-----------------------------------------------------------------------
      ICESocketSession::ICESocketSessionStates ICESocketSession::getState() const
      {
        AutoRecursiveLock lock(getLock());
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      ICESocketSession::ICESocketSessionShutdownReasons ICESocketSession::getShutdownReason() const
      {
        AutoRecursiveLock lock(getLock());
        return mShutdownReason;
      }

      //-----------------------------------------------------------------------
      void ICESocketSession::close()
      {
        ZS_LOG_DEBUG(log("close requested"))
        AutoRecursiveLock lock(getLock());

        setShutdownReason(ICESocketSessionShutdownReason_Closed);
        cancel();
      }

      //-----------------------------------------------------------------------
      void ICESocketSession::getLocalCandidates(CandidateList &outCandidates)
      {
        outCandidates.clear();

        AutoRecursiveLock lock(getLock());
        IICESocketPtr socket = getSocket();
        if (!socket) return;

        socket->getLocalCandidates(outCandidates);
      }

      //-----------------------------------------------------------------------
      void ICESocketSession::updateRemoteCandidates(const CandidateList &remoteCandidates)
      {
        ZS_LOG_DEBUG(log("updating remote candidates"))
        AutoRecursiveLock lock(getLock());

        // remember the remote candidates for later (as long as not doing a "reset" of the candidates)
        if (&mRemoteCandidates != &remoteCandidates)
          mRemoteCandidates = remoteCandidates;

        // nothing is nominated given remote candidates are being replaced...
        if (mNominateRequester) {
          mNominateRequester->cancel();
          mNominateRequester.reset();
        }
        mNominated.reset();
        mStartedSearchAt = zsLib::now();

        ICESocketPtr socket = mICESocketWeak.lock();
        if (socket) {
          socket->forICESocketSession().removeRoute(mThisWeak.lock());
        }

        if (mKeepAliveTimer) {
          mKeepAliveTimer->cancel();
          mKeepAliveTimer.reset();
        }

        if (mExpectingDataTimer) {
          mExpectingDataTimer->cancel();
          mExpectingDataTimer.reset();
        }

        if (mAliveCheckRequester) {
          mAliveCheckRequester->cancel();
          mAliveCheckRequester.reset();
        }

        CandidateList offerList;
        getLocalCandidates(offerList);

        // scope: we have to completely cancel the old searches...
        {
          for (CandidatePairList::iterator iter = mCandidatePairs.begin(); iter != mCandidatePairs.end(); ++iter)
          {
            CandidatePairPtr pairing = (*iter);
            if (pairing->mRequester) {
              pairing->mRequester->cancel();
              pairing->mRequester.reset();
            }
          }

          // bye-bye old pairs...
          mCandidatePairs.clear();
        }

        // scope: assemble the local/remote pairs together
        {
          for (CandidateList::iterator outer = offerList.begin(); outer != offerList.end(); ++outer) {
            for (CandidateList::const_iterator inner = remoteCandidates.begin(); inner != remoteCandidates.end(); ++inner) {
              CandidatePairPtr pairing = CandidatePair::create();

              pairing->mLocal = (*outer);
              pairing->mRemote = (*inner);

              mCandidatePairs.push_back(pairing);
            }
          }
        }


        // sort the list based on priority of the pairs
        if (mControl == IICESocket::ICEControl_Controlling)
          mCandidatePairs.sort(comparePairControlling);
        else
          mCandidatePairs.sort(comparePairControlled);

        // scope: prune the list of non-searchable/redundent candidates
        {
          IICESocket::Types searchArray[] = {ICESocket::Type_Local, ICESocket::Type_ServerReflexive, ICESocket::Type_Relayed, ICESocket::Type_Unknown};

          for (int loop = 0; searchArray[loop] != ICESocket::Type_Unknown; ++loop)
          {
            typedef std::list<ICESocket::Candidate> CandidateList;
            CandidateList foundRemotes;

            for (CandidatePairList::iterator canIter = mCandidatePairs.begin(); canIter != mCandidatePairs.end();)
            {
              CandidatePairList::iterator current = canIter;
              ++canIter;

              CandidatePairPtr &pairing = (*current);

              // only going through one type at a time
              if (pairing->mLocal.mType != searchArray[loop])
                continue;

              if (ICESocket::Type_ServerReflexive == pairing->mLocal.mType) {

                // this is server reflixive which can never be sent "from" so elimate it
                mCandidatePairs.erase(current);
                continue;
              }

              bool foundType = false;
              for (CandidateList::iterator candIter = foundRemotes.begin(); candIter != foundRemotes.end(); ++candIter) {
                Candidate &foundRemoteCandidate = (*candIter);
                if ((foundRemoteCandidate.mIPAddress.isEqualIgnoringIPv4Format(pairing->mRemote.mIPAddress)) &&
                    (foundRemoteCandidate.mUsernameFrag == pairing->mRemote.mUsernameFrag) &&
                    (foundRemoteCandidate.mPassword == pairing->mRemote.mPassword)) {
                  // this is a redundant remote candidate so we will need to eliminate it
                  foundType = true;
                  break;
                }
              }

              if (foundType){
                mCandidatePairs.erase(current);
                continue;
              }
              foundRemotes.push_back(pairing->mRemote);
            }
          }
        }

        if (ZS_IS_LOGGING(Debug)) {
          ZS_LOG_DEBUG(log("--- ICE SESSION CANDIDATES START ---") + ", control=" + (mControl == IICESocket::ICEControl_Controlling ? "CONTROLLING" : "CONTROLLED"))

          for (CandidatePairList::iterator iter = mCandidatePairs.begin(); iter != mCandidatePairs.end(); ++iter) {
            CandidatePairPtr &pairing = (*iter);

            ZS_LOG_DEBUG(log("candidate pair") + ", local ip=" + pairing->mLocal.mIPAddress.string() + " remote=" + pairing->mRemote.mIPAddress.string() + " " + pairing->mLocal.mUsernameFrag + ":" + pairing->mRemote.mUsernameFrag + ", password (local)=" + pairing->mLocal.mPassword + ", password (remote)=" + pairing->mRemote.mPassword)
          }
          ZS_LOG_DEBUG(log("--- ICE SESSION CANDIDATES END ---") + ", control=" + (mControl == IICESocket::ICEControl_Controlling ? "CONTROLLING" : "CONTROLLED"))
        }

        // truncate the list at 100 pairs maximum - RFC says that anything above 100 is unreasonable
        while (mCandidatePairs.size() > 100) {
          mCandidatePairs.pop_back(); // keep popping the end of the list until we reach a maximum of 100 pairs to check
        }

        step();
      }

      //-----------------------------------------------------------------------
      void ICESocketSession::setKeepAliveProperties(
                                                    Duration sendKeepAliveIndications,
                                                    Duration expectSTUNOrDataWithinWithinOrSendAliveCheck,
                                                    Duration keepAliveSTUNRequestTimeout,
                                                    Duration backgroundingTimeout
                                                    )
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("adjusting keep alive propertiess") +
                     ", send keep alive (ms)=" + Stringize<Duration::tick_type>(sendKeepAliveIndications.total_milliseconds()).string() +
                     ", expecting data within (ms)=" + Stringize<Duration::tick_type>(expectSTUNOrDataWithinWithinOrSendAliveCheck.total_milliseconds()).string())

        if (mKeepAliveTimer) {
          ZS_LOG_DEBUG(log("cancelling current keep alive timer"))
          mKeepAliveTimer->cancel();
          mKeepAliveTimer.reset();
        }

        if (mAliveCheckRequester) {
          ZS_LOG_DEBUG(log("cancelling current alive check requester"))
          mAliveCheckRequester->cancel();
          mAliveCheckRequester.reset();
        }

        if (mExpectingDataTimer) {
          ZS_LOG_DEBUG(log("cancelling current expecting data timer"))
          mExpectingDataTimer->cancel();
          mExpectingDataTimer.reset();
        }

        mKeepAliveDuration = sendKeepAliveIndications;
        mExpectSTUNOrDataWithinDuration = expectSTUNOrDataWithinWithinOrSendAliveCheck;
        mKeepAliveSTUNRequestTimeout = keepAliveSTUNRequestTimeout;
        mBackgroundingTimeout = backgroundingTimeout;

        ZS_LOG_DEBUG(log("forcing step to ensure all timers are properly created"))
        (IICESocketSessionAsyncDelegateProxy::create(mThisWeak.lock()))->onStep();
      }

      //-----------------------------------------------------------------------
      bool ICESocketSession::sendPacket(
                                        const BYTE *packet,
                                        ULONG packetLengthInBytes
                                        )
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("unable to send packet as socket is already shutdown"))
          return false;
        }

        mInformedWriteReady = false;  // if this method was called in response to a write-ready event, be sure to clear the write-ready informed flag so future events will fire

        if (mNominateRequester) {
          ZS_LOG_WARNING(Detail, log("not allowed to send data during the ICE nomination process"))
          return false;    // do not allow sending of data during the nomination process
        }
        if (!mNominated) {
          ZS_LOG_WARNING(Detail, log("not allowed to send data as ICE nomination process is not complete"))
          return false;  // do not allow sending when no candidate has been nominated
        }

        mLastSentData = zsLib::now();
        return sendTo(mNominated->mLocal.mType, mNominated->mRemote.mIPAddress, packet, packetLengthInBytes, true);
      }

      //-----------------------------------------------------------------------
      ICESocketSession::ICEControls ICESocketSession::getConnectedControlState()
      {
        AutoRecursiveLock lock(getLock());
        return mControl;
      }

      //-----------------------------------------------------------------------
      IPAddress ICESocketSession::getConnectedRemoteIP()
      {
        AutoRecursiveLock lock(getLock());
        if (!mNominated) return IPAddress();
        return mNominated->mRemote.mIPAddress;
      }

      //-----------------------------------------------------------------------
      bool ICESocketSession::getNominatedCandidateInformation(
                                                              Candidate &outLocal,
                                                              Candidate &outRemote
                                                              )
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) return false;

        if (!mNominated) return false;

        outLocal = mNominated->mLocal;
        outRemote = mNominated->mRemote;
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocketSession => IICESocketSessionForICESocket
      #pragma mark

      //-----------------------------------------------------------------------
      void ICESocketSession::timeout()
      {
        ZS_LOG_DEBUG(log("close requested"))
        AutoRecursiveLock lock(getLock());

        setShutdownReason(ICESocketSessionShutdownReason_Timeout);
        cancel();
      }

      //-----------------------------------------------------------------------
      bool ICESocketSession::handleSTUNPacket(
                                              IICESocket::Types viaTransport,
                                              const IPAddress &source,
                                              STUNPacketPtr stun,
                                              const String &localUsernameFrag,
                                              const String &remoteUsernameFrag
                                              )
      {
        IICESocketSessionDelegatePtr delegate;
        {
          AutoRecursiveLock lock(getLock());
          delegate = mDelegate;
        }
        if (!delegate) {
          ZS_LOG_WARNING(Debug, log("unable to handle STUN packet as delegate is gone"))
          return false;
        }

        // inform that the session is now connected
        if (STUNPacket::Method_Binding != stun->mMethod) {
          try {
            ZS_LOG_DETAIL(log("received incoming STUN which is not ICE related thus handing via delgate"))
            return delegate->handleICESocketSessionReceivedSTUNPacket(mThisWeak.lock(), stun, localUsernameFrag, remoteUsernameFrag);
          } catch(IICESocketSessionDelegateProxy::Exceptions::DelegateGone &) {
            setShutdownReason(ICESocketSessionShutdownReason_DelegateGone);
            cancel();
            return false;
          }
          return false;
        }

        AutoRecursiveLock lock(getLock());
        ZS_THROW_INVALID_USAGE_IF(!stun)

        if (!mNominateRequester) {
          if (mNominated) {
            ZS_LOG_DEBUG(log("expecting nominated pair is sending a bind request/indication"))

            if ((mNominated->mLocal.mUsernameFrag != localUsernameFrag) ||
                (mNominated->mRemote.mUsernameFrag != remoteUsernameFrag) ||
                (normalize(mNominated->mLocal.mType) != normalize(viaTransport)))
            {
              ZS_LOG_TRACE(log("nominated username/password/transport do not match incoming request/indication") +
                              ", local ip=" + mNominated->mLocal.mIPAddress.string() +
                              ", remote ip=" + mNominated->mRemote.mIPAddress.string() +
                              ", username=" + mNominated->mLocal.mUsernameFrag + ":" + mNominated->mRemote.mUsernameFrag)
              return false;
            }

            if (!mNominated->mRemote.mIPAddress.isEqualIgnoringIPv4Format(source)) {
              ZS_LOG_DEBUG(log("requests/indications from inexact source address cannot be considered nominated candidate") + ", source=" + source.string() + ", local=" + mNominated->mLocal.mIPAddress.string() + " remote=" + mNominated->mRemote.mIPAddress.string() + " " + localUsernameFrag + ":" + remoteUsernameFrag)
              return false;
            }

            bool passedIntegrity = stun->isValidMessageIntegrity(mNominated->mLocal.mPassword);

            if (passedIntegrity) {
              mLastReceivedDataOrSTUN = zsLib::now();

              if (mAliveCheckRequester) {
                ZS_LOG_DEBUG(log("alive check reuqester is no longer needed as STUN request/integrity bind was received"))
                mAliveCheckRequester->cancel();
                mAliveCheckRequester.reset();
              }
            }

            if (STUNPacket::Class_Indication == stun->mClass) {
              if (!passedIntegrity) {
                ZS_LOG_WARNING(Detail, log("nominated password integrity failed for indication") + ", local ip=" + mNominated->mLocal.mIPAddress.string() + ", remote ip=" + mNominated->mRemote.mIPAddress.string() + ", username=" + mNominated->mLocal.mUsernameFrag + ":" + mNominated->mRemote.mUsernameFrag)
                return false;
              }
              ZS_LOG_TRACE(log("received keep alive indication"))
              return true;
            }

            STUNPacketPtr response;
            if (!passedIntegrity) {
              stun->mErrorCode = STUNPacket::ErrorCode_Unauthorized;
              ZS_LOG_ERROR(Detail, log("nominated password integrity failed") + ", local ip=" + mNominated->mLocal.mIPAddress.string() + ", remote ip=" + mNominated->mRemote.mIPAddress.string() + ", username=" + mNominated->mLocal.mUsernameFrag + ":" + mNominated->mRemote.mUsernameFrag)

              response = STUNPacket::createErrorResponse(stun);
            } else {
              ZS_LOG_DEBUG(log("nominated pair is sending a bind request") + ", local ip=" + mNominated->mLocal.mIPAddress.string() + ", remote ip=" + mNominated->mRemote.mIPAddress.string() + ", username=" + mNominated->mLocal.mUsernameFrag + ":" + mNominated->mRemote.mUsernameFrag)

              response = STUNPacket::createResponse(stun);
              response->mMappedAddress = source;
            }

            fix(response);

            response->mPassword = mNominated->mLocal.mPassword;
            response->mCredentialMechanism = STUNPacket::CredentialMechanisms_ShortTerm;

            boost::shared_array<BYTE> buffer;
            ULONG bufferLengthInBytes = 0;
            response->packetize(buffer, bufferLengthInBytes, STUNPacket::RFC_5245_ICE);
            sendTo(normalize(mNominated->mLocal.mType), source, buffer.get(), bufferLengthInBytes, false);
            return true;
          }
        }

        if (STUNPacket::Class_Request != stun->mClass) {
          ZS_LOG_WARNING(Detail, log("ignoring ICE binding STUN packet)"))
          return false;
        }

        bool failedIntegrity = false;

        bool foundExactMatch = false;
        CandidatePairPtr inexactMatch;
        CandidatePairPtr requestNow;
        for (CandidatePairList::iterator iter = mCandidatePairs.begin(); iter != mCandidatePairs.end(); ++iter)
        {
          CandidatePairPtr pairing = (*iter);

          if ((pairing->mLocal.mUsernameFrag == localUsernameFrag) &&
              (pairing->mRemote.mUsernameFrag == remoteUsernameFrag) &&
              (normalize(pairing->mLocal.mType) == normalize(viaTransport))) {

            ZS_LOG_TRACE(log("match found on incoming STUN bind request to candidate") + ", incoming local username=" + localUsernameFrag + ", incoming remote username=" + remoteUsernameFrag + ", incoming transport=" + IICESocket::toString(normalize(viaTransport)) + ", candidate local username=" + pairing->mLocal.mUsernameFrag + ", candidate remote username=" + pairing->mRemote.mUsernameFrag + ", candidate transport=" + IICESocket::toString(normalize(viaTransport)))

            // check if it matches a canadidate but does not match the IP address of the remote party we were expecting
            if (!pairing->mRemote.mIPAddress.isEqualIgnoringIPv4Format(source)) {
              ZS_LOG_DEBUG(log("requests from inexact candidate match found") + ", source=" + source.string() + ", local=" + pairing->mLocal.mIPAddress.string() + " remote=" + pairing->mRemote.mIPAddress.string() + " " + localUsernameFrag + ":" + remoteUsernameFrag)
              inexactMatch = pairing;
              continue;
            }

            // we found an exact match... that means we received a request from the remote party
            foundExactMatch = true;
            inexactMatch.reset();
            ZS_LOG_DEBUG(log("request from exact match found") + ", local=" + pairing->mLocal.mIPAddress.string() + " remote=" + pairing->mRemote.mIPAddress.string() + " " + localUsernameFrag + ":" + remoteUsernameFrag)

            if (!stun->isValidMessageIntegrity(pairing->mLocal.mPassword)) {
              ZS_LOG_DEBUG(log("STUN request failed integrity") + ", local=" + pairing->mLocal.mIPAddress.string() + " remote=" + pairing->mRemote.mIPAddress.string() + " " + localUsernameFrag + ":" + remoteUsernameFrag)
              failedIntegrity = true;
              break;
            }

            pairing->mReceivedRequest = true;
            requestNow = pairing;

            if (pairing->mRequester) {
              pairing->mRequester->retryRequestNow();   // retry the request immediately
            } else {
              if ((!pairing->mReceivedResponse) ||
                  (pairing->mFailed)) {
                pairing->mFailed = false;               // even if this previously failed, we are now going to try this request to see if it works
              }
            }
            break;
          } else {
            ZS_LOG_TRACE(log("match NOT found on incoming STUN bind request to candidate") + ", incoming local username=" + localUsernameFrag + ", incoming remote username=" + remoteUsernameFrag + ", incoming transport=" + IICESocket::toString(normalize(viaTransport)) + ", candidate local username=" + pairing->mLocal.mUsernameFrag + ", candidate remote username=" + pairing->mRemote.mUsernameFrag + ", candidate transport=" + IICESocket::toString(normalize(viaTransport)))
          }
        }

        if (inexactMatch) {
          CandidatePairPtr pairing = inexactMatch;

          if (stun->isValidMessageIntegrity(pairing->mLocal.mPassword)) {

            // too many candidates created? ignore this request as if we never saw it...
            if (mCandidatePairs.size() > 100) {
              ZS_LOG_WARNING(Debug, log("unable to accept new found new peer reflexive candidate since too many canadidates where discovered") + ", found ip=" + source.string() + " , local=" + pairing->mLocal.mIPAddress.string() + " remote=" + pairing->mRemote.mIPAddress.string() + " " + localUsernameFrag + ":" + remoteUsernameFrag)
              return true;
            }

            // we have to create a new peer reflexive candidate based on the existing match and activiate it
            CandidatePairPtr newPair = pairing->clone();
            newPair->mLocal.mType = ICESocket::Type_PeerReflexive;  // the only difference is the source and the type
            newPair->mRemote.mIPAddress = source;
            newPair->mLocal.mPriority = ((1 << 24)*(newPair->mLocal.mType)) + ((1 << 8)*(newPair->mLocal.mLocalPreference)) + (256 - 0);
            newPair->mReceivedRequest = true;
            newPair->mReceivedResponse = false;
            newPair->mFailed = false;

            ZS_LOG_DEBUG(log("found new peer reflexive candidate") + ", found ip=" + source.string() + " , local=" + pairing->mLocal.mIPAddress.string() + " remote=" + pairing->mRemote.mIPAddress.string() + " " + localUsernameFrag + ":" + remoteUsernameFrag)

            mCandidatePairs.push_back(newPair);
            requestNow = newPair;

            // sort the list based on priority of the pairs
            if (mControl == IICESocket::ICEControl_Controlling)
              mCandidatePairs.sort(comparePairControlling);
            else
              mCandidatePairs.sort(comparePairControlled);
          }
          else {
            ZS_LOG_WARNING(Debug, log("newly discovered inexact match did not pass integrity check") + ", found ip=" + source.string() + " , local=" + pairing->mLocal.mIPAddress.string() + " remote=" + pairing->mRemote.mIPAddress.string() + " " + localUsernameFrag + ":" + remoteUsernameFrag)
            failedIntegrity = true;
          }
        }

        if (!requestNow) {
          // no match found at all (exact or inexact)...
          ZS_LOG_TRACE(log("did not handle this request"))
          return false;
        }

        CandidatePairPtr pairing = requestNow;

        bool correctRole = true;
        bool wonConflict = false;

        // check to see if the request is in the correct role...
        if ((ICESocket::ICEControl_Controlling == mControl) &&
            (stun->mIceControllingIncluded)) {
          correctRole = false;
          wonConflict = (mConflictResolver >= stun->mIceControlling);
        }

        if ((ICESocket::ICEControl_Controlled == mControl) &&
            (stun->mIceControlledIncluded)) {
          correctRole = false;
          wonConflict = (mConflictResolver < stun->mIceControlled);
        }

        if (!correctRole) { // one of us is in the incorret role?
          if (!wonConflict) {
            // we have to switch roles!
            ZS_LOG_WARNING(Detail, log("candidate role conflict detected thus switching roles") + ", local ip=" + pairing->mLocal.mIPAddress.string() + ", remote ip=" + pairing->mRemote.mIPAddress.string() + ", username=" + pairing->mLocal.mUsernameFrag + ":" + pairing->mRemote.mUsernameFrag)
            switchRole(ICESocket::ICEControl_Controlled == mControl ? IICESocket::ICEControl_Controlling : IICESocket::ICEControl_Controlled);
            return true;
          }

          // we one the conflict but the other party needs to get an error message
        }

        STUNPacketPtr response;

        if ((correctRole) && (!failedIntegrity)) {
          // we need to generate a proper response
          response = STUNPacket::createResponse(stun);
          fix(response);
          response->mMappedAddress = source;
        } else {
          // we need to generate an error response
          if (!correctRole) {
            stun->mErrorCode = STUNPacket::ErrorCode_RoleConflict;
            ZS_LOG_WARNING(Detail, log("candidate role conflict detected thus telling other party to switch roles via an error") + ", local ip=" + pairing->mLocal.mIPAddress.string() + ", remote ip=" + pairing->mRemote.mIPAddress.string() + ", username=" + pairing->mLocal.mUsernameFrag + ":" + pairing->mRemote.mUsernameFrag)
          }
          if (failedIntegrity) {
            stun->mErrorCode = STUNPacket::ErrorCode_Unauthorized;
            ZS_LOG_ERROR(Detail, log("candidate password integrity failed") + ", local ip=" + pairing->mLocal.mIPAddress.string() + ", remote ip=" + pairing->mRemote.mIPAddress.string() + ", username=" + pairing->mLocal.mUsernameFrag + ":" + pairing->mRemote.mUsernameFrag)
          }
          response = STUNPacket::createErrorResponse(stun);
          fix(response);
        }

        response->mPassword = pairing->mLocal.mPassword;
        response->mCredentialMechanism = STUNPacket::CredentialMechanisms_ShortTerm;

        boost::shared_array<BYTE> buffer;
        ULONG bufferLengthInBytes = 0;
        response->packetize(buffer, bufferLengthInBytes, STUNPacket::RFC_5245_ICE);
        sendTo(normalize(pairing->mLocal.mType), source, buffer.get(), bufferLengthInBytes, false);

        if ((stun->mUseCandidateIncluded) &&
            (ICESocket::ICEControl_Controlled == mControl)) {

          if (mNominated != pairing) {
            // the remote party is telling this party that this pair is nominated
            ZS_LOG_DETAIL(log("candidate is nominated by controlling party (i.e. remote party)") + ", local ip=" + pairing->mLocal.mIPAddress.string() + ", remote ip=" + pairing->mRemote.mIPAddress.string() + ", username=" + pairing->mLocal.mUsernameFrag + ":" + pairing->mRemote.mUsernameFrag)

            mNominated = pairing;

            ICESocketPtr socket = mICESocketWeak.lock();
            if (socket) {
              socket->forICESocketSession().addRoute(mThisWeak.lock(), mNominated->mRemote.mIPAddress);
            }

            mInformedWriteReady = false;

            notifyLocalWriteReady();
            notifyRelayWriteReady();

            // yes, okay, it did in fact succeed - we have nominated our candidate!
            // inform that the session is now connected
            ZS_LOG_DETAIL(log("nomination request succeeded") + ", local ip=" + mNominated->mLocal.mIPAddress.string() + ", remote ip=" + mNominated->mRemote.mIPAddress.string() + ", username=" + mNominated->mLocal.mUsernameFrag + ":" + mNominated->mRemote.mUsernameFrag)
            (IICESocketSessionAsyncDelegateProxy::create(mThisWeak.lock()))->onStep();
          }
          return true;
        }

        if (!pairing->mRequester)
        {
          if (mNominated) return true;  // no need to activate another search if the nominated candidate is found
          if (pairing->mReceivedResponse) return true; // have we already determined this candidate is valid? if so, then no need to try again

          ZS_LOG_DETAIL(log("candidate search started on reaction to a request") + ", local ip=" + pairing->mLocal.mIPAddress.string() + ", remote ip=" + pairing->mRemote.mIPAddress.string() + ", username=" + pairing->mLocal.mUsernameFrag + ":" + pairing->mRemote.mUsernameFrag)

          STUNPacketPtr request = STUNPacket::createRequest(STUNPacket::Method_Binding);
          fix(request);
          request->mUsername = pairing->mRemote.mUsernameFrag + ":" + pairing->mLocal.mUsernameFrag;
          request->mPassword = pairing->mRemote.mPassword;
          request->mPriorityIncluded = true;
          request->mPriority = pairing->mLocal.mPriority;
          request->mCredentialMechanism = STUNPacket::CredentialMechanisms_ShortTerm;
          if (IICESocket::ICEControl_Controlling == mControl) {
            request->mIceControllingIncluded = true;
            request->mIceControlling = mConflictResolver;
          } else {
            request->mIceControlledIncluded = true;
            request->mIceControlled = mConflictResolver;
          }

          // activate the pair search now...
          pairing->mRequester = ISTUNRequester::create(getAssociatedMessageQueue(), mThisWeak.lock(), pairing->mRemote.mIPAddress, request, STUNPacket::RFC_5245_ICE);
        }
        return true;
      }

      //-----------------------------------------------------------------------
      bool ICESocketSession::handlePacket(
                                          IICESocket::Types viaTransport,
                                          const IPAddress &source,
                                          const BYTE *packet,
                                          ULONG packetLengthInBytes
                                          )
      {
        // WARNING: This method calls a delegate synchronously thus must
        //          never be called from a method that is within a lock.
        IICESocketSessionDelegatePtr delegate;

        {
          AutoRecursiveLock lock(getLock());
          if ((NULL == packet) ||
              (0 == packetLengthInBytes)) {
            ZS_LOG_WARNING(Trace, log("incoming data packet is NULL or of 0 length thus ignoring"))
            return false;
          }

          if (isShutdown()) {
            ZS_LOG_WARNING(Trace, log("already shutdown thus ignoring incoming data packet"))
            return false;
          }

          if (mNominateRequester) {
            ZS_LOG_WARNING(Trace, log("cannot process data packets while the nominate process is in progress"))
            return false;                                            // can't handle packets during the nomination process
          }
          if (!mNominated) {
            ZS_LOG_WARNING(Trace, log("cannot process data packets without a nominated ice pair"))
            return false;                                          // can't receive if not connected
          }

          if (!mNominated->mRemote.mIPAddress.isEqualIgnoringIPv4Format(source)) {
            ZS_LOG_WARNING(Trace, log("incoming remote IP on data packet does not match nominated IP thus ignoring") + ", remote IP=" + source.string() + ", expected/nominated IP=" + mNominated->mRemote.mIPAddress.string())
            return false;      // the remote IP must match the connected IP
          }
          if (normalize(viaTransport) != normalize(mNominated->mLocal.mType)) {
            ZS_LOG_WARNING(Trace, log("incoming data packet did not arrive via the expected transport thus ignoring") + ", from remote transport" + IICESocket::toString(viaTransport) + ", exoected/nominated transport=" + IICESocket::toString(mNominated->mLocal.mType))
            return false;
          }

          mLastReceivedDataOrSTUN = zsLib::now();

          if (mAliveCheckRequester) {
            ZS_LOG_DEBUG(log("alive check requester is no longer needed as data was received"))
            mAliveCheckRequester->cancel();
            mAliveCheckRequester.reset();
          }

          delegate = mDelegate;
        }

        // we have a match on the packet... send the data to the delegate...
        try {
          delegate->handleICESocketSessionReceivedPacket(mThisWeak.lock(), packet, packetLengthInBytes);
        } catch(IICESocketSessionDelegateProxy::Exceptions::DelegateGone &) {
          setShutdownReason(ICESocketSessionShutdownReason_DelegateGone);
          cancel();
        }
        return true;
      }

      //-----------------------------------------------------------------------
      void ICESocketSession::notifyLocalWriteReady()
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) return;
        if (mInformedWriteReady) return;

        mInformedWriteReady = false;

        if (mNominateRequester) {
          ZS_LOG_TRACE(log("notify local write ready cannot inform delegate since candidates are nominating"))
          return;
        }
        if (!mNominated) {
          ZS_LOG_TRACE(log("notify local write ready cannot inform delegate since nomination process is incomplete"))
          return;
        }

        if (IICESocket::Type_Relayed == normalize(mNominated->mLocal.mType)) {
          ZS_LOG_TRACE(log("notify local write ready cannot inform delegate since relay is being used"))
          return;
        }

        ZS_LOG_TRACE(log("notify local write ready"))
        try {
          mDelegate->onICESocketSessionWriteReady(mThisWeak.lock());
          mInformedWriteReady = true;
        } catch(IICESocketSessionDelegateProxy::Exceptions::DelegateGone &) {
          setShutdownReason(ICESocketSessionShutdownReason_DelegateGone);
          cancel();
        }
      }

      //-----------------------------------------------------------------------
      void ICESocketSession::notifyRelayWriteReady()
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) return;
        if (mInformedWriteReady) return;

        mInformedWriteReady = false;

        if (mNominateRequester) {
          ZS_LOG_DEBUG(log("notify relay write ready cannot inform delegate since candidates are nominating"))
          return;
        }
        if (!mNominated) {
          ZS_LOG_DEBUG(log("notify relay write ready cannot inform delegate since nomination process is incomplete"))
          return;
        }

        if (IICESocket::Type_Relayed != normalize(mNominated->mLocal.mType)) {
          ZS_LOG_DEBUG(log("notify relay write ready cannot inform delegate since local socket is being used"))
          return;
        }

        ZS_LOG_TRACE(log("notify relay write ready"))

        try {
          mDelegate->onICESocketSessionWriteReady(mThisWeak.lock());
          mInformedWriteReady = true;
        } catch(IICESocketSessionDelegateProxy::Exceptions::DelegateGone &) {
          setShutdownReason(ICESocketSessionShutdownReason_DelegateGone);
          cancel();
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocketSession => IICESocketSessionAsyncDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ICESocketSession::onStep()
      {
        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocketSession => IICESocketDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ICESocketSession::onICESocketStateChanged(
                                                     IICESocketPtr socket,
                                                     ICESocketStates state
                                                     )
      {
        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocketSession => ISTUNRequesterDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ICESocketSession::onSTUNRequesterSendPacket(
                                                       ISTUNRequesterPtr requester,
                                                       IPAddress destination,
                                                       boost::shared_array<BYTE> packet,
                                                       ULONG packetLengthInBytes
                                                       )
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) return;

        if ((requester == mNominateRequester) ||
            (requester == mAliveCheckRequester)) {
          ZS_THROW_BAD_STATE_IF(!mNominated)
          sendTo(mNominated->mLocal.mType, destination, packet.get(), packetLengthInBytes, false);
          return;
        }

        // scope: search the candidates to see which one is sending the request
        {
          for (CandidatePairList::iterator iter = mCandidatePairs.begin(); iter != mCandidatePairs.end(); ++iter)
          {
            CandidatePairPtr pairing = (*iter);
            if (requester == pairing->mRequester) {
              sendTo(pairing->mLocal.mType, destination, packet.get(), packetLengthInBytes, false);
              return;
            }
          }
        }

        // must be from an old requester to get here...
      }

      //-----------------------------------------------------------------------
      bool ICESocketSession::handleSTUNRequesterResponse(
                                                         ISTUNRequesterPtr requester,
                                                         IPAddress fromIPAddress,
                                                         STUNPacketPtr response
                                                         )
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) return false;

        if ((requester == mNominateRequester) ||
            (requester == mAliveCheckRequester)) {
          ZS_THROW_BAD_STATE_IF(!mNominated)

          if ((0 != response->mErrorCode) ||
              (response->mClass != STUNPacket::Class_Response)) {
            // some kind of error happened during the nominate
            switch (response->mErrorCode) {
              case STUNPacket::ErrorCode_RoleConflict: {
                // this request better be signed properly or we will ignore the conflict...
                if (!response->isValidMessageIntegrity(mNominated->mRemote.mPassword)) {
                  ZS_LOG_WARNING(Detail, log("nomination caused role conflict reply did not pass integtiry check") + ", local ip=" + mNominated->mLocal.mIPAddress.string() + ", remote ip=" + mNominated->mRemote.mIPAddress.string() + ", username=" + mNominated->mLocal.mUsernameFrag + ":" + mNominated->mRemote.mUsernameFrag)
                  return false;
                }

                if (requester == mAliveCheckRequester) {
                  ZS_LOG_WARNING(Detail, log("nomination caused role conflict reply cannot be issued for alive check request (since already nominated)") + ", local ip=" + mNominated->mLocal.mIPAddress.string() + ", remote ip=" + mNominated->mRemote.mIPAddress.string() + ", username=" + mNominated->mLocal.mUsernameFrag + ":" + mNominated->mRemote.mUsernameFrag)
                  return false;
                }

                ZS_LOG_WARNING(Detail, log("nomination request caused role conflict") + ", local ip=" + mNominated->mLocal.mIPAddress.string() + ", remote ip=" + mNominated->mRemote.mIPAddress.string() + ", username=" + mNominated->mLocal.mUsernameFrag + ":" + mNominated->mRemote.mUsernameFrag)

                // we have a role conflict... switch roles now...
                STUNPacketPtr originalRequest = requester->getRequest();
                switchRole(originalRequest->mIceControlledIncluded ? IICESocket::ICEControl_Controlling : IICESocket::ICEControl_Controlled);
                return true;
              }
              default: break; // well.. that sucks... why would it be errored???
            }

            // handle this in the same way a timeout would be handled (what else can we do?)
            onSTUNRequesterTimedOut(requester);
            return true;
          }

          if (mAliveCheckRequester) {
            if (mNominated->mRemote.mUsernameFrag.isEmpty()) {
              ZS_LOG_DEBUG(log("alive check request to a server succeeded") + ", local ip=" + mNominated->mLocal.mIPAddress.string() + ", remote ip=" + mNominated->mRemote.mIPAddress.string() + ", username=" + mNominated->mLocal.mUsernameFrag)
              mLastReceivedDataOrSTUN = zsLib::now();

              mAliveCheckRequester.reset();
              return true;
            }
          }

          // the nomination request succeeded (or so we think - make sure it was signed properly)!
          if (!response->isValidMessageIntegrity(mNominated->mRemote.mPassword)) {
            ZS_LOG_WARNING(Detail, log("response from nomination or alive check failed message integrity") + ", was nominate requester=" + (requester == mNominateRequester  ? "true" : "false"))
            return false;
          }

          if (requester == mAliveCheckRequester) {
            ZS_LOG_DEBUG(log("alive check request succeeded") + ", local ip=" + mNominated->mLocal.mIPAddress.string() + ", remote ip=" + mNominated->mRemote.mIPAddress.string() + ", username=" + mNominated->mLocal.mUsernameFrag + ":" + mNominated->mRemote.mUsernameFrag)
            mLastReceivedDataOrSTUN = zsLib::now();

            mAliveCheckRequester.reset();
            return true;
          }

          // yes, okay, it did in fact succeed - we have nominated our candidate!
          // inform that the session is now connected
          ZS_LOG_DETAIL(log("nomination request succeeded") + ", local ip=" + mNominated->mLocal.mIPAddress.string() + ", remote ip=" + mNominated->mRemote.mIPAddress.string() + ", username=" + mNominated->mLocal.mUsernameFrag + ":" + mNominated->mRemote.mUsernameFrag)

          // we are now established to the remote party

          mNominateRequester.reset();

          mInformedWriteReady = false;

          notifyLocalWriteReady();
          notifyRelayWriteReady();

          (IICESocketSessionAsyncDelegateProxy::create(mThisWeak.lock()))->onStep();
          return true;
        }

        // this could be for one of the candidates...
        for (CandidatePairList::iterator iter = mCandidatePairs.begin(); iter != mCandidatePairs.end(); ++iter)
        {
          CandidatePairPtr pairing = (*iter);
          if (requester != pairing->mRequester)
            continue;

          // see what kind of issue we have on the packet
          if ((0 != response->mErrorCode) ||
              (response->mClass != STUNPacket::Class_Response)) {
            // some kind of error happened during the nominate
            switch (response->mErrorCode) {
              case STUNPacket::ErrorCode_RoleConflict: {
                // this request better be signed properly or we will ignore the conflict...
                if (!response->isValidMessageIntegrity(pairing->mRemote.mPassword)) return false;

                ZS_LOG_WARNING(Detail, log("candidate role conflict error received") + ", local ip=" + pairing->mLocal.mIPAddress.string() + ", remote ip=" + pairing->mRemote.mIPAddress.string() + ", username=" + pairing->mLocal.mUsernameFrag + ":" + pairing->mRemote.mUsernameFrag)

                // we have a role conflict... switch roles now...
                STUNPacketPtr originalRequest = requester->getRequest();
                switchRole(originalRequest->mIceControlledIncluded ? IICESocket::ICEControl_Controlling : IICESocket::ICEControl_Controlled);
                return true;
              }
              default: break;
            }
            // we will ignore all other issues
            return true;
          }

          pairing->mFailed = false;
          pairing->mReceivedResponse = true;
          pairing->mRequester.reset();
          if (pairing->mRemote.mUsernameFrag.isEmpty()) {
            // fake that we received a request since we will never receive in this case
            pairing->mReceivedRequest = true;
          }
          step();
          return true;
        }

        return false;
      }

      //-----------------------------------------------------------------------
      void ICESocketSession::onSTUNRequesterTimedOut(ISTUNRequesterPtr requester)
      {
        AutoRecursiveLock lock(getLock());

        if (requester == mAliveCheckRequester) {
          ZS_LOG_WARNING(Detail, log("alive connectivity check failed (probably a connection timeout)") + ", local ip=" + mNominated->mLocal.mIPAddress.string() + ", remote ip=" + mNominated->mRemote.mIPAddress.string() + ", username=" + mNominated->mLocal.mUsernameFrag + ":" + mNominated->mRemote.mUsernameFrag)

          mAliveCheckRequester.reset();
          setShutdownReason(ICESocketSessionShutdownReason_Timeout);
          cancel();
          return;
        }

        if (requester == mNominateRequester) {
          mNominateRequester.reset();

          // we were nominating this candidate but it isn't responding! We will not nominate this pair but instead will start the scan again...
          for (CandidatePairList::iterator iter = mCandidatePairs.begin(); iter != mCandidatePairs.end(); ++iter)
          {
            CandidatePairPtr pairing = (*iter);
            if (mNominated == pairing) {
              ZS_LOG_ERROR(Detail, log("nomination of candidate failed") + ", local ip=" + pairing->mLocal.mIPAddress.string() + ", remote ip=" + pairing->mRemote.mIPAddress.string() + ", username=" + pairing->mLocal.mUsernameFrag + ":" + pairing->mRemote.mUsernameFrag)

              // we found the candidate that was going to be nomiated but it can't be since the nomination failed...
              pairing->mFailed = false;
              pairing->mReceivedResponse = false; // mark that we haven't received the reply yet so the scan has to restart
              if (pairing->mRequester) {
                pairing->mRequester->cancel();
                pairing->mRequester.reset();
              }
              break;
            }
          }

          // forget which was nominated since it failed...
          mNominated.reset();

          // try something else instead...
          (IICESocketSessionAsyncDelegateProxy::create(mThisWeak.lock()))->onStep();
          return;
        }

        // scope: try to figure out which candidate this requested belonged to...
        {
          for (CandidatePairList::iterator iter = mCandidatePairs.begin(); iter != mCandidatePairs.end(); ++iter)
          {
            CandidatePairPtr pairing = (*iter);
            if (requester == pairing->mRequester) {
              // mark this pair as failed
              ZS_LOG_DETAIL(log("candidate timeout") + ", local ip=" + pairing->mLocal.mIPAddress.string() + ", remote ip=" + pairing->mRemote.mIPAddress.string() + ", username=" + pairing->mLocal.mUsernameFrag + ":" + pairing->mRemote.mUsernameFrag)

              pairing->mRequester.reset();
              pairing->mFailed = true;

              step();
              return;
            }
          }
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocketSession => ITimerDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ICESocketSession::onTimer(TimerPtr timer)
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) return;

        Time tick = zsLib::now();
        if (Duration() != mBackgroundingTimeout) {
          Duration diff = tick - mLastActivity;
          if (diff > mBackgroundingTimeout) {
            ZS_LOG_WARNING(Detail, log("backgrounding timeout forced this session to close") + ", time diff (ms)=" + Stringize<Duration::tick_type>(diff.total_milliseconds()).string())
            setShutdownReason(ICESocketSessionShutdownReason_BackgroundingTimeout);
            cancel();
            return;
          }
          mLastActivity = tick;
        }

        if (timer == mStepTimer) {
          step();
          return;
        }

        if (timer == mActivateTimer)
        {
          if (mNominateRequester) return;  // already in the process of nominating

          if (0 == mCandidatePairs.size()) return;

          // we are going to activate the next candidate pair now...
          for (CandidatePairList::iterator iter = mCandidatePairs.begin(); iter != mCandidatePairs.end(); ++iter)
          {
            CandidatePairPtr pairing = (*iter);
            if (pairing->mRequester)
              continue;

            if (pairing->mReceivedResponse)
              continue; // no need to activate a second time if a response has been received

            if (pairing->mFailed)
              continue; // do not activate a pair that has already failed

            ZS_LOG_DETAIL(log("activating search on candidate") + ", local ip=" + pairing->mLocal.mIPAddress.string() + ", remote ip=" + pairing->mRemote.mIPAddress.string() + ", username=" + pairing->mLocal.mUsernameFrag + ":" + pairing->mRemote.mUsernameFrag)

            // activate this pair right now... if there is no remote username then treat this as a regular STUN request/response situation (plus will automatically nominate if successful)
            STUNPacketPtr request = STUNPacket::createRequest(STUNPacket::Method_Binding);
            fix(request);
            bool isICE = false;

            if (!pairing->mRemote.mUsernameFrag.isEmpty()) {
              isICE = true;
              request->mUsername = pairing->mRemote.mUsernameFrag + ":" + pairing->mLocal.mUsernameFrag;
              request->mPassword = pairing->mRemote.mPassword;
              request->mCredentialMechanism = STUNPacket::CredentialMechanisms_ShortTerm;
              request->mPriorityIncluded = true;
              request->mPriority = pairing->mLocal.mPriority;
              if (IICESocket::ICEControl_Controlling == mControl) {
                request->mIceControllingIncluded = true;
                request->mIceControlling = mConflictResolver;
              } else {
                request->mIceControlledIncluded = true;
                request->mIceControlled = mConflictResolver;
              }
            }

            // activate the pair search now...
            pairing->mRequester = ISTUNRequester::create(getAssociatedMessageQueue(), mThisWeak.lock(), pairing->mRemote.mIPAddress, request, (isICE ? STUNPacket::RFC_5245_ICE : STUNPacket::RFC_5389_STUN));
            break;  // only activate one pair at this time
          }
          return;
        }

        if (timer == mKeepAliveTimer)
        {
          if (mNominateRequester) return;  // can't do keep alives during a nomination process
          if (!mNominated) return;  // can't do keep alives if not connected

          // we are going to check the ICE socket to see if it can shutdown TURN at this time...
          if (mLastSentData + mKeepAliveDuration > tick) {
            ZS_LOG_TRACE(log("no need to fire keep alive timer as data was sent within keep alive window"))
            return;  // not enough time has passed since sending data to send more...
          }

          ZS_LOG_DETAIL(log("keep alive") + ", local ip=" + mNominated->mLocal.mIPAddress.string() + ", remote ip=" + mNominated->mRemote.mIPAddress.string() + ", username=" + mNominated->mLocal.mUsernameFrag + ":" + mNominated->mRemote.mUsernameFrag)
          STUNPacketPtr indication = STUNPacket::createIndication(STUNPacket::Method_Binding);
          fix(indication);

          if (!mNominated->mRemote.mUsernameFrag.isEmpty()) {
            indication->mUsername = mNominated->mRemote.mUsernameFrag + ":" + mNominated->mLocal.mUsernameFrag;
            indication->mPassword = mNominated->mRemote.mPassword;
            indication->mCredentialMechanism = STUNPacket::CredentialMechanisms_ShortTerm;
          }

          boost::shared_array<BYTE> buffer;
          ULONG length = 0;
          indication->packetize(buffer, length, STUNPacket::RFC_5245_ICE);
          sendTo(mNominated->mLocal.mType, mNominated->mRemote.mIPAddress, buffer.get(), length, true);
        }

        if (timer == mExpectingDataTimer)
        {
          if (mNominateRequester) return;  // can't do keep alives during a nomination process
          if (!mNominated) return;  // can't do keep alives if not connected

          if (mLastReceivedDataOrSTUN + mExpectSTUNOrDataWithinDuration > tick) {
            ZS_LOG_TRACE(log("received STUN request or indication or data within the expected window so no need to test if remote party is alive"))
            return;
          }

          if (mAliveCheckRequester) {
            ZS_LOG_WARNING(Detail, log("alive check requester already activated"))
            return;
          }

          ZS_LOG_TRACE(log("expecting data timer fired"))

          //...................................................................
          // NOTE: Servers will *NOT* send regular connectivity checks to
          // their clients. This responsibility is leftto the client so
          // the client cannot expect to receive data within a time frame
          // when connecting to a server.
          //
          // However the keep alive check mechanism can be used to probe if
          // a server is still alive as the server will respond to the request
          // albeit without security credentials.

          STUNPacketPtr request = STUNPacket::createRequest(STUNPacket::Method_Binding);
          fix(request);
          bool isICE = false;

          if (!mNominated->mRemote.mUsernameFrag.isEmpty()) {
            ZS_LOG_WARNING(Detail, log("expected STUN request or indication or data within the expected window but did not receive (thus will attempt to do a connectivity check)"))
            isICE = true;
            request->mUsername = mNominated->mRemote.mUsernameFrag + ":" + mNominated->mLocal.mUsernameFrag;
            request->mPassword = mNominated->mRemote.mPassword;
            request->mCredentialMechanism = STUNPacket::CredentialMechanisms_ShortTerm;
            request->mIceControllingIncluded = true;
            request->mIceControlling = mConflictResolver;
            request->mPriorityIncluded = true;
            request->mPriority = mNominated->mLocal.mPriority;
          }

          mAliveCheckRequester = ISTUNRequester::create(getAssociatedMessageQueue(), mThisWeak.lock(), mNominated->mRemote.mIPAddress, request, (isICE ? STUNPacket::RFC_5245_ICE : STUNPacket::RFC_5389_STUN), mKeepAliveSTUNRequestTimeout);
          return;
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICESocketSession => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &ICESocketSession::getLock() const
      {
        ICESocketPtr socket = mICESocketWeak.lock();
        if (!socket)
          return mBogusLock;
        return socket->forICESocketSession().getLock();
      }

      //-----------------------------------------------------------------------
      String ICESocketSession::log(const char *message) const
      {
        return String("ICESocketSession [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      void ICESocketSession::fix(STUNPacketPtr stun) const
      {
        stun->mLogObject = "ICESocketSession";
        stun->mLogObjectID = mID;
      }

      //-----------------------------------------------------------------------
      void ICESocketSession::cancel()
      {
        AutoRecursiveLock lock(getLock());  // just in case
        if (isShutdown()) return;

        ZS_LOG_DETAIL(log("closing"))

        setState(ICESocketSessionState_Shutdown);

        mDelegate.reset();

        if (mSocketSubscription) {
          mSocketSubscription->cancel();
          mSocketSubscription.reset();
        }

        IICESocketForICESocketSessionPtr socketProxy = IICESocketForICESocketSessionProxy::create(mICESocketWeak.lock());
        if (socketProxy) {
          socketProxy->onICESocketSessionClosed(getID());
        }

        if (mActivateTimer) {
          mActivateTimer->cancel();
          mActivateTimer.reset();
        }

        if (mKeepAliveTimer) {
          mKeepAliveTimer->cancel();
          mKeepAliveTimer.reset();
        }

        if (mExpectingDataTimer) {
          mExpectingDataTimer->cancel();
          mExpectingDataTimer.reset();
        }

        if (mAliveCheckRequester) {
          mAliveCheckRequester->cancel();
          mAliveCheckRequester.reset();
        }

        if (mStepTimer) {
          mStepTimer->cancel();
          mStepTimer.reset();
        }

        if (mNominateRequester) {
          mNominateRequester->cancel();
          mNominateRequester.reset();
        }

        // scope: we have to completely cancel the old searches...
        {
          for (CandidatePairList::iterator iter = mCandidatePairs.begin(); iter != mCandidatePairs.end(); ++iter)
          {
            CandidatePairPtr pairing = (*iter);
            if (pairing->mRequester) {
              pairing->mRequester->cancel();
              pairing->mRequester.reset();
            }
          }

          // bye-bye old pairs...
          mCandidatePairs.clear();
        }
      }

      //-----------------------------------------------------------------------
      void ICESocketSession::step()
      {
        if (isShutdown()) {
          cancel();
          return;
        }

        Time tick = zsLib::now();

        IICESocketPtr socket = getSocket();
        if (!socket) {
          setShutdownReason(ICESocketSessionShutdownReason_SocketGone);
          cancel();
          return;
        }

        IICESocket::ICESocketStates state = socket->getState();
        switch (state) {
          case IICESocket::ICESocketState_ShuttingDown:
          case IICESocket::ICESocketState_Shutdown:
          {
            // socket is self-destructing or is destroyed...
            setShutdownReason(ICESocketSessionShutdownReason_SocketGone);
            cancel();
            return;
          }
          default: break;
        }

        if ((mNominateRequester) ||
            (mNominated)) {

          // we don't need the step timer anymore (since the step timer is used to make sure that candidates eventually give up...
          if (mStepTimer) {
            mStepTimer->cancel();
            mStepTimer.reset();
          }

          // we dont' need an activate time since we already picked the nomination pair
          if (mActivateTimer) {
            mActivateTimer->cancel();
            mActivateTimer.reset();
          }
        }

        if (mNominateRequester) {
          setState(ICESocketSessionState_Nominating);
          return;                  // already in the process of nominating
        }

        if (mNominated) {
          setState(ICESocketSessionState_Nominated);

          // we need a keep alive timer to keep the UDP ports alive
          if ((!mKeepAliveTimer) &&
              (Duration() != mKeepAliveDuration)) {
            ZS_LOG_DEBUG(log("creating keep alive timer"))
            mLastActivity = zsLib::now();
            mKeepAliveTimer = Timer::create(mThisWeak.lock(), mKeepAliveDuration);  // start a keep alive counter
          }

          if ((!mExpectingDataTimer) &&
              (Duration() != mExpectSTUNOrDataWithinDuration)) {
            ZS_LOG_DEBUG(log("creating expecting data timer"))
            mLastActivity = zsLib::now();
            mExpectingDataTimer = Timer::create(mThisWeak.lock(), mExpectSTUNOrDataWithinDuration);  // start a keep alive counter
          }

          // we have a connected IP and no activate timer, that means we are done with all the candidates since one has been nominated...
          for (CandidatePairList::iterator iter = mCandidatePairs.begin(); iter != mCandidatePairs.end(); ++iter) {
            CandidatePairPtr pairing = (*iter);
            if (pairing->mRequester) {
              pairing->mRequester->cancel();
              pairing->mRequester.reset();
            }
          }

          // we don't need candidate pairs anymore since we have our noinated candidate!
          mCandidatePairs.clear();

          return; // nothing more to do if already nominated all pairs
        }

        switch (state) {
          case IICESocket::ICESocketState_Pending:    return;   // ICE socket is not ready yet...
          case IICESocket::ICESocketState_GoingToSleep:
          case IICESocket::ICESocketState_Sleeping:
          {
            // make sure the socket is in the ready state before even searching for candidates
            socket->wakeup();
            return;
          }
          default:  break;
        }

        if (mCandidatePairs.size() < 1) {
          // no remote candidates offered so can't start the process... the socket is ready at least...
          setState(ICESocketSessionState_Prepared);
          return;
        }

        if (!mStepTimer) {
          // the step timer is used to ensure candidates eventually give up...
          mLastActivity = zsLib::now();
          mStepTimer = Timer::create(mThisWeak.lock(), Seconds(2));
        }

        if (!mActivateTimer) {
          // the activation timer is used to activiate candidates...
          mLastActivity = zsLib::now();
          mActivateTimer = Timer::create(mThisWeak.lock(), Milliseconds(20)); // this will cause candidates to start searching right away
        }

        // scope: cancel any requests after a lower priority valid request (only if in the controlling role)
        {
          CandidatePairPtr foundPair;

          int foundValidIndex = -1;
          int foundOutgoingRequestPending = -1;
          int incomingRequestPending = -1;
          int foundNonActivated = -1;
          int loop = 0;
          for (CandidatePairList::iterator iter = mCandidatePairs.begin(); iter != mCandidatePairs.end(); ++iter, ++loop)
          {
            CandidatePairPtr &pairing = (*iter);

            if (!pairing->mFailed) {
              // hasn't failed yet...
              if ((pairing->mReceivedRequest) &&
                  (pairing->mReceivedResponse)) {
                // we found a valid pair
                if (-1 != foundValidIndex) continue;  // already found valid

                foundValidIndex = loop;
                foundPair = pairing;
                continue;
              }

              if (-1 != foundValidIndex) {
                // found a higher priority valid than this one...

                // we will pretend the request failed if the response wasn't
                // received to prevent the pair from being activitated
                if (!pairing->mReceivedResponse)
                  pairing->mFailed = true;

                // perform the cancellation of this request
                if (pairing->mRequester) {
                  ZS_LOG_DETAIL(log("cancelling pair search (as higher priority found valid)") + ", local ip=" + pairing->mLocal.mIPAddress.string() + ", remote ip=" + pairing->mRemote.mIPAddress.string() + ", username=" + pairing->mLocal.mUsernameFrag + ":" + pairing->mRemote.mUsernameFrag)
                  pairing->mRequester->cancel();
                  pairing->mRequester.reset();
                }
                // this can't be pending, nor non-activated...
                continue;
              }

              if (pairing->mRequester) {
                if (-1 != foundOutgoingRequestPending) continue;     // already found pending
                foundOutgoingRequestPending = loop;
                continue;
              }

              if (pairing->mReceivedResponse) {
                // we have already received a response, but not a request
                // thus it is pending and but it isn't valid yet but it has
                // already been activated
                if (-1 != incomingRequestPending) continue;     // already found pending
                incomingRequestPending = loop;
                continue;
              }

              // this is non activated candidate (hasn't failed, no requester
              // found yet hasn't received both request and response)

              if (-1 != foundNonActivated) continue;     // already found non-activated
              foundNonActivated = loop;
              continue;
            }

            // this request failed...
          }

          if (IICESocket::ICEControl_Controlling == mControl)
          {
            CandidatePairPtr usePair;
            if (-1 != foundValidIndex) {

              if ((0 == foundValidIndex) ||
                  ((-1 == foundOutgoingRequestPending) &&
                   (-1 == incomingRequestPending) &&
                   (-1 == foundNonActivated))) {
                // we definately want to go with this pair since it's the higest possible priority
                usePair = foundPair;
              } else {
                // at a certain point we have to stop searching...
                if (mStartedSearchAt + Seconds(4) < tick) {
                  // time to stop searching - use the top found candidate
                  ZS_LOG_DEBUG(log("will not wait any longer for any other candidates since a match is available (even though it's not ideal)"))
                  usePair = foundPair;
                }
              }
            }

            if (usePair) {
              mNominated = usePair;

              mLastReceivedDataOrSTUN = tick;

              // we are going to nominate this pair...
              ICESocketPtr socket = mICESocketWeak.lock();
              if (socket) {
                socket->forICESocketSession().addRoute(mThisWeak.lock(), mNominated->mRemote.mIPAddress);
              }

              ZS_LOG_DETAIL(log("nominating pair") + ", local ip=" + mNominated->mLocal.mIPAddress.string() + ", remote ip=" + mNominated->mRemote.mIPAddress.string() + ", username=" + mNominated->mLocal.mUsernameFrag + ":" + mNominated->mRemote.mUsernameFrag)

              if (mNominated->mRemote.mUsernameFrag.isEmpty()) {
                ZS_LOG_DEBUG(log("remote username is not set thus this pair can be immediately nominated (i.e. server mode)"))
                // if we never had a username then we were just doing a regular STUN request to the server to detect connectivity

                // we are now connected to this IP address...
                (IICESocketSessionAsyncDelegateProxy::create(mThisWeak.lock()))->onStep();
                return;
              }

              // this is done to inform the remote party of the nomination since the nominiation process has completed
              STUNPacketPtr request = STUNPacket::createRequest(STUNPacket::Method_Binding);
              fix(request);
              request->mUsername = mNominated->mRemote.mUsernameFrag + ":" + mNominated->mLocal.mUsernameFrag;
              request->mPassword = mNominated->mRemote.mPassword;
              request->mCredentialMechanism = STUNPacket::CredentialMechanisms_ShortTerm;
              request->mIceControllingIncluded = true;
              request->mIceControlling = mConflictResolver;
              request->mPriorityIncluded = true;
              request->mPriority = mNominated->mLocal.mPriority;
              request->mUseCandidateIncluded = true;

              // form a new request
              mNominateRequester = ISTUNRequester::create(getAssociatedMessageQueue(), mThisWeak.lock(), mNominated->mRemote.mIPAddress, request, STUNPacket::RFC_5245_ICE);
              setState(ICESocketSessionState_Nominating);

              (IICESocketSessionAsyncDelegateProxy::create(mThisWeak.lock()))->onStep();
              return;
            }
          }

          if ((-1 != foundValidIndex) &&
              (-1 == foundOutgoingRequestPending) &&
              (-1 == incomingRequestPending) &&
              (-1 == foundNonActivated)) {
            // we found a valid candiate but for some reason have not activated
            // it yet... wait around for it to activate... or give up if its
            // been a ridiculously long time...
            if (mStartedSearchAt + Seconds(HOOKFLASH_SERVICES_ICESOCKETSESSION_MAX_WAIT_TIME_FOR_CANDIDATE_TO_ACTIVATE_IF_ALL_DONE) < tick) {
              ZS_LOG_ERROR(Basic, log("candidate found valid candidate but was never activated") + ", timeout=" + Stringize<int>(HOOKFLASH_SERVICES_ICESOCKETSESSION_MAX_WAIT_TIME_FOR_CANDIDATE_TO_ACTIVATE_IF_ALL_DONE).string())
              setShutdownReason(ICESocketSessionShutdownReason_Timeout);
              cancel(); // we waited as long as we could and it seems that we failed to connect...
              return;
            }
          }

          if ((-1 == foundValidIndex) &&
              (-1 == foundOutgoingRequestPending) &&
              (-1 != incomingRequestPending) &&
              (-1 == foundNonActivated)) {
            // we never found a valid candidate and we did receive some
            // response but an incoming request was never received
            if (mStartedSearchAt + Seconds(HOOKFLASH_SERVICES_ICESOCKETSESSION_MAX_WAIT_TIME_FOR_CANDIDATE_TO_ACTIVATE_IF_ALL_DONE) < tick) {
              ZS_LOG_ERROR(Basic, log("all outgoing request received a response but some incoming request has not arrived yet") + ", timeout=" + Stringize<int>(HOOKFLASH_SERVICES_ICESOCKETSESSION_MAX_WAIT_TIME_FOR_CANDIDATE_TO_ACTIVATE_IF_ALL_DONE).string())
              setShutdownReason(ICESocketSessionShutdownReason_Timeout);
              cancel(); // we waited as long as we could and it seems that we failed to connect...
              return;
            }
          }

          if ((-1 == foundValidIndex) &&
              (-1 == foundOutgoingRequestPending) &&
              (-1 == incomingRequestPending) &&
              (-1 == foundNonActivated)) {
            // nothing pending, nothing non-activated, nothing valid, give up!
            ZS_LOG_ERROR(Basic, log("candidate search has failed!"))
            setShutdownReason(ICESocketSessionShutdownReason_CandidateSearchFailed);
            cancel();
            return;
          }
        }
      }

      //-----------------------------------------------------------------------
      void ICESocketSession::setState(ICESocketSessionStates state)
      {
        if (mCurrentState == state) return;

        ZS_LOG_BASIC(log("state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state))

        mCurrentState = state;

        if (!mDelegate) return;

        ICESocketSessionPtr pThis = mThisWeak.lock();

        if (pThis) {
          // inform the delegate of the state change
          try {
            mDelegate->onICESocketSessionStateChanged(pThis, mCurrentState);
          } catch(IICESocketSessionDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_DEBUG(log("delegate gone"))
          }
        }
      }

      //-----------------------------------------------------------------------
      void ICESocketSession::setShutdownReason(ICESocketSessionShutdownReasons reason)
      {
        AutoRecursiveLock lock(getLock());
        if (ICESocketSessionShutdownReason_None != mShutdownReason) {
          if (reason == mShutdownReason) return;

          ZS_LOG_WARNING(Detail, log("shutdown reason already set") + ", current reason=" + toString(reason) + ", trying to set to reason=" + toString(reason))
          return;
        }

        ZS_LOG_DEBUG(log("shutdown reason set") + ", reason=" + toString(reason))
        mShutdownReason = reason;
      }

      //-----------------------------------------------------------------------
      void ICESocketSession::switchRole(ICEControls newRole)
      {
        if (isShutdown()) return;
        if (newRole == mControl) return; // role did not switch

        // switch roles now...
        mControl = newRole;

        // redo all the testing from scratch!
        updateRemoteCandidates(mRemoteCandidates);
      }

      //-----------------------------------------------------------------------
      bool ICESocketSession::sendTo(
                                    IICESocket::Types viaTransport,
                                    const IPAddress &destination,
                                    const BYTE *buffer,
                                    ULONG bufferLengthInBytes,
                                    bool isUserData
                                    )
      {
        if (isShutdown()) {
          ZS_LOG_WARNING(Debug, log("cannot send packet as ICE session is closed") + ", via=" + IICESocket::toString(viaTransport) + " to ip=" + destination.string() + ", buffer=" + (buffer ? "true" : "false") + ", buffer length=" + Stringize<ULONG>(bufferLengthInBytes).string() + ", user data=" + (isUserData ? "true" : "false"))
          return false;
        }
        ICESocketPtr socket = mICESocketWeak.lock();
        if (!socket) {
          ZS_LOG_WARNING(Debug, log("cannot send packet as ICE socket is closed") + ", via=" + IICESocket::toString(viaTransport) + " to ip=" + destination.string() + ", buffer=" + (buffer ? "true" : "false") + ", buffer length=" + Stringize<ULONG>(bufferLengthInBytes).string() + ", user data=" + (isUserData ? "true" : "false"))
          return false;
        }

        ZS_LOG_TRACE(log("sending packet") + ", via=" + IICESocket::toString(viaTransport) + " to ip=" + destination.string() + ", buffer=" + (buffer ? "true" : "false") + ", buffer length=" + Stringize<ULONG>(bufferLengthInBytes).string() + ", user data=" + (isUserData ? "true" : "false"))
        return socket->forICESocketSession().sendTo(viaTransport, destination, buffer, bufferLengthInBytes, isUserData);
      }

    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IICESocketSession
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IICESocketSession::toString(ICESocketSessionStates state)
    {
      switch (state) {
        case ICESocketSessionState_Pending:    return "Preparing";
        case ICESocketSessionState_Prepared:   return "Prepared";
        case ICESocketSessionState_Searching:  return "Searching";
        case ICESocketSessionState_Nominating: return "Nominating";
        case ICESocketSessionState_Nominated:  return "Nominated";
        case ICESocketSessionState_Shutdown:   return "Shutdown";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *IICESocketSession::toString(ICESocketSessionShutdownReasons reason)
    {
      switch (reason) {
        case ICESocketSessionShutdownReason_None:                   return "None";
        case ICESocketSessionShutdownReason_Closed:                 return "Closed";
        case ICESocketSessionShutdownReason_Timeout:                return "Timeout";
        case ICESocketSessionShutdownReason_BackgroundingTimeout:   return "Backgrounding timeout";
        case ICESocketSessionShutdownReason_CandidateSearchFailed:  return "Candidate search failed";
        case ICESocketSessionShutdownReason_DelegateGone:           return "Delegate gone";
        case ICESocketSessionShutdownReason_SocketGone:             return "Socket gone";
      }
      return "UNDEFINED";
    }
  }
}
