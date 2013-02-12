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

#include <hookflash/services/internal/services_RUDPICESocketSession.h>
#include <hookflash/services/internal/services_RUDPICESocket.h>
#include <hookflash/services/internal/services_RUDPChannel.h>
#include <hookflash/services/RUDPPacket.h>
#include <zsLib/Exception.h>
#include <zsLib/helpers.h>
#include <zsLib/Log.h>
#include <zsLib/Stringize.h>

#include <cryptopp/osrng.h>

#include <algorithm>

#define HOOKFLASH_SERVICES_RUDPICESOCKETSESSION_MAX_ATTEMPTS_TO_FIND_FREE_CHANNEL_NUMBER (5)

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
      #pragma mark IRUDPICESocketSessionForRUDPICESocket
      #pragma mark

      //-----------------------------------------------------------------------
      RUDPICESocketSessionPtr IRUDPICESocketSessionForRUDPICESocket::create(
                                                                            IMessageQueuePtr queue,
                                                                            RUDPICESocketPtr parent,
                                                                            IRUDPICESocketSessionDelegatePtr delegate,
                                                                            const CandidateList &remoteCandidates,
                                                                            ICEControls control
                                                                            )
      {
        return IRUDPICESocketSessionFactory::singleton().create(queue, parent, delegate, remoteCandidates, control);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPICESocketSession
      #pragma mark

      //-----------------------------------------------------------------------
      RUDPICESocketSession::RUDPICESocketSession(
                                                 IMessageQueuePtr queue,
                                                 RUDPICESocketPtr parent,
                                                 IRUDPICESocketSessionDelegatePtr delegate
                                                 ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mCurrentState(RUDPICESocketSessionState_Pending),
        mShutdownReason(RUDPICESocketSessionShutdownReason_None),
        mOuter(parent),
        mDelegate(IRUDPICESocketSessionDelegateProxy::createWeak(queue, delegate))
      {
        ZS_LOG_BASIC(log("created"))
      }

      //-----------------------------------------------------------------------
      void RUDPICESocketSession::init(
                                      const CandidateList &remoteCandidates,
                                      ICEControls control
                                      )
      {
        AutoRecursiveLock lock(getLock());
        mICESession = (mOuter.lock())->forSession().getICESocket()->createSessionFromRemoteCandidates(mThisWeak.lock(), remoteCandidates, control);
      }

      //-----------------------------------------------------------------------
      RUDPICESocketSession::~RUDPICESocketSession()
      {
        if (isNoop()) return;
        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      RUDPICESocketSessionPtr RUDPICESocketSession::create(
                                                           IMessageQueuePtr queue,
                                                           RUDPICESocketPtr parent,
                                                           IRUDPICESocketSessionDelegatePtr delegate,
                                                           const CandidateList &remoteCandidates,
                                                           ICEControls control
                                                           )
      {
        RUDPICESocketSessionPtr pThis(new RUDPICESocketSession(queue, parent, delegate));
        pThis->mThisWeak = pThis;
        pThis->init(remoteCandidates, control);
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPICESocketSession => RUDPICESocketSession
      #pragma mark

      //-----------------------------------------------------------------------
      IRUDPICESocketPtr RUDPICESocketSession::getSocket()
      {
        AutoRecursiveLock lock(getLock());
        RUDPICESocketPtr socket = mOuter.lock();
        if (!socket) return IRUDPICESocketPtr();
        return socket->forSession().getRUDPICESocket();
      }

      //-----------------------------------------------------------------------
      IRUDPICESocketSession::RUDPICESocketSessionStates RUDPICESocketSession::getState() const
      {
        AutoRecursiveLock lock(getLock());
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      IRUDPICESocketSession::RUDPICESocketSessionShutdownReasons RUDPICESocketSession::getShutdownReason() const
      {
        AutoRecursiveLock lock(getLock());
        if (RUDPICESocketSessionShutdownReason_None != mShutdownReason) {
          return mShutdownReason;
        }
        if (mICESession) {
          IICESocketSession::ICESocketSessionShutdownReasons iceReason = mICESession->getShutdownReason();
          if (IICESocketSession::ICESocketSessionShutdownReason_None != iceReason) {
            // normally I don't like doing this but it makes sence from an API perspective that getShutdownReason
            // doesn't change the state of the object - and it isn't because ICE session's reason overrides
            // the reason of the RUDPICESocketSession when the RUDPSocketSession is not set.
            RUDPICESocketSession *pThis = const_cast<RUDPICESocketSession *>(this);
            pThis->mShutdownReason = ((RUDPICESocketSessionShutdownReasons)iceReason);
          }
        }

        return mShutdownReason;
      }

      //-----------------------------------------------------------------------
      void RUDPICESocketSession::shutdown()
      {
        AutoRecursiveLock lock(getLock());
        setShutdownReason(RUDPICESocketSessionShutdownReason_Closed);
        cancel();
      }

      //-----------------------------------------------------------------------
      void RUDPICESocketSession::getLocalCandidates(CandidateList &outCandidates)
      {
        outCandidates.clear();

        IICESocketSessionPtr session = getICESession();
        if (!session) return;
        session->getLocalCandidates(outCandidates);
      }

      //-----------------------------------------------------------------------
      void RUDPICESocketSession::updateRemoteCandidates(const CandidateList &remoteCandidates)
      {
        IICESocketSessionPtr session = getICESession();
        if (!session) return;
        session->updateRemoteCandidates(remoteCandidates);
      }

      //-----------------------------------------------------------------------
      void RUDPICESocketSession::setKeepAliveProperties(
                                                        Duration sendKeepAliveIndications,
                                                        Duration expectSTUNOrDataWithinWithinOrSendAliveCheck,
                                                        Duration keepAliveSTUNRequestTimeout,
                                                        Duration backgroundingTimeout
                                                        )
      {
        IICESocketSessionPtr session = getICESession();
        if (!session) {
          ZS_LOG_WARNING(Detail, log("unable to obtain ICE socket session to set keep alive properties"))
          return;
        }
        session->setKeepAliveProperties(sendKeepAliveIndications, expectSTUNOrDataWithinWithinOrSendAliveCheck, keepAliveSTUNRequestTimeout, backgroundingTimeout);
      }

      //-----------------------------------------------------------------------
      RUDPICESocketSession::ICEControls RUDPICESocketSession::getConnectedControlState()
      {
        IICESocketSessionPtr session = getICESession();
        if (!session) return IICESocket::ICEControl_Controlled;

        return session->getConnectedControlState();
      }

      //-----------------------------------------------------------------------
      IPAddress RUDPICESocketSession::getConnectedRemoteIP()
      {
        IICESocketSessionPtr session = getICESession();
        if (!session) return IPAddress();

        return session->getConnectedRemoteIP();
      }

      //-----------------------------------------------------------------------
      bool RUDPICESocketSession::getNominatedCandidateInformation(
                                                                  Candidate &outLocal,
                                                                  Candidate &outRemote
                                                                  )
      {
        IICESocketSessionPtr session = getICESession();
        if (!session) return false;

        return session->getNominatedCandidateInformation(outLocal, outRemote);
      }

      //-----------------------------------------------------------------------
      IRUDPChannelPtr RUDPICESocketSession::openChannel(
                                                        IRUDPChannelDelegatePtr delegate,
                                                        const char *connectionInfo
                                                        )
      {
        AutoRecursiveLock lock(getLock());
        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_WARNING(Detail, log("attempting to open a channel during the shutdown thus returning NULL channel object"))
          return IRUDPChannelPtr();
        }

        if (!mICESession) {
          ZS_LOG_WARNING(Detail, log("attempting to open a channel without an ICE session thus returning NULL channel object"))
          return IRUDPChannelPtr();
        }

        CryptoPP::AutoSeededRandomPool rng;
        // we have a valid nonce, we will open the channel, but first - pick an unused channel number
        UINT tries = 0;

        WORD channelNumber = 0;
        bool valid = false;
        do
        {
          ++tries;
          if (tries > HOOKFLASH_SERVICES_RUDPICESOCKETSESSION_MAX_ATTEMPTS_TO_FIND_FREE_CHANNEL_NUMBER) return IRUDPChannelPtr();

          rng.GenerateBlock((BYTE *)(&channelNumber), sizeof(channelNumber));
          channelNumber = (channelNumber % (HOOKFLASH_SERVICES_RUDPICESOCKETSESSION_CHANNEL_RANGE_END - HOOKFLASH_SERVICES_RUDPICESOCKETSESSION_CHANNEL_RANGE_START)) + HOOKFLASH_SERVICES_RUDPICESOCKETSESSION_CHANNEL_RANGE_START;

          // check to see if the channel was used for this IP before...
          SessionMap::iterator found = mLocalChannelNumberSessions.find(channelNumber);
          valid = (found == mLocalChannelNumberSessions.end());
        } while (!valid);

        if (!valid) {
          ZS_LOG_WARNING(Detail, log("unable to find a free channel number within a reasonable number of attempts"))
          return IRUDPChannelPtr();
        }

        Candidate nominatedLocal;
        Candidate nominatedRemote;
        bool hasCandidate = getNominatedCandidateInformation(nominatedLocal, nominatedRemote);
        if (!hasCandidate) {
          ZS_LOG_WARNING(Detail, log("failed to find any candidates thus returning NULL channel object"))
          return IRUDPChannelPtr();
        }

        mLocalUsernameFrag = nominatedLocal.mUsernameFrag;
        mRemoteUsernameFrag = nominatedRemote.mUsernameFrag;

        ZS_LOG_DEBUG(log("channel openned with nominated pair") + ", local username frag=" + mLocalUsernameFrag + ", remote username frag=" + mRemoteUsernameFrag)

        // found a useable channel number therefor create a new session
        RUDPChannelPtr session = IRUDPChannelForRUDPICESocketSession::createForRUDPICESocketSessionOutgoing(
                                                                                                            getAssociatedMessageQueue(),
                                                                                                            mThisWeak.lock(),
                                                                                                            delegate,
                                                                                                            getConnectedRemoteIP(),
                                                                                                            channelNumber,
                                                                                                            nominatedLocal.mUsernameFrag,
                                                                                                            nominatedRemote.mUsernameFrag,
                                                                                                            nominatedLocal.mPassword,
                                                                                                            nominatedRemote.mPassword,
                                                                                                            connectionInfo
                                                                                                            );

        mLocalChannelNumberSessions[channelNumber] = session;
        issueChannelConnectIfPossible();
        return session;
      }

      //-----------------------------------------------------------------------
      IRUDPChannelPtr RUDPICESocketSession::acceptChannel(IRUDPChannelDelegatePtr delegate)
      {
        AutoRecursiveLock lock(getLock());

        if (mPendingSessions.size() < 1) return IRUDPChannelPtr();

        RUDPChannelPtr found = mPendingSessions.front();
        found->forSession().setDelegate(delegate);
        mPendingSessions.pop_front();
        return found;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPICESocketSession => IICESocketSessionDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void RUDPICESocketSession::onICESocketSessionStateChanged(
                                                                IICESocketSessionPtr session,
                                                                ICESocketSessionStates state
                                                                )
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Debug, log("notified of ICE socket session changed while shutdown") + ", ICE session ID=" + Stringize<PUID>(session->getID()).string())
          return;
        }

        if (session != mICESession) {
          ZS_LOG_WARNING(Debug, log("received notification of ICE socket session state changed from obsolete session") + ", ICE session ID=" + Stringize<PUID>(session->getID()).string())
          return;
        }

        if (IICESocketSession::ICESocketSessionState_Shutdown == state) {
          ZS_LOG_WARNING(Detail, log("ICE socket session reported itself shutdown so must shutdown RUDP session") + ", ICE session ID=" + Stringize<PUID>(session->getID()).string())
          getShutdownReason(); // fix the shutdown reason if not set already
          cancel();
          return;
        }

        if (IICESocketSession::ICESocketSessionState_Nominated == state) {
          Candidate nominatedLocal;
          Candidate nominatedRemote;
          bool hasCandidate = getNominatedCandidateInformation(nominatedLocal, nominatedRemote);
          if (!hasCandidate) {
            ZS_LOG_WARNING(Debug, log("told is was connected but it has no nominations (session closed quickly??)") + ", ICE session ID=" + Stringize<PUID>(session->getID()).string())
            return;
          }

          mLocalUsernameFrag = nominatedLocal.mUsernameFrag;
          mRemoteUsernameFrag = nominatedRemote.mUsernameFrag;

          ZS_LOG_DEBUG(log("notified that socket session state is nominated") + ", ICE session ID=" + Stringize<PUID>(session->getID()).string() + ", local username frag=" + mLocalUsernameFrag + ", remote username frag=" + mRemoteUsernameFrag)

          issueChannelConnectIfPossible();
        }

        step();
      }

      //-----------------------------------------------------------------------
      void RUDPICESocketSession::handleICESocketSessionReceivedPacket(
                                                                      IICESocketSessionPtr ignore,
                                                                      const BYTE *buffer,
                                                                      ULONG bufferLengthInBytes
                                                                      )
      {
        RUDPPacketPtr rudp = RUDPPacket::parseIfRUDP(buffer, bufferLengthInBytes);

        if (!rudp) {
          ZS_LOG_WARNING(Trace, log("failed to parse data packet as RUDP thus ignoring packet"))
          return;
        }

        RUDPChannelPtr session;

        // scope: figure out which session this belongs
        {
          AutoRecursiveLock lock(getLock());
          SessionMap::iterator found = mLocalChannelNumberSessions.find(rudp->mChannelNumber);
          if (found == mLocalChannelNumberSessions.end()) {
            ZS_LOG_WARNING(Trace, log("RUDP packet does not belong to any known channel thus igoring the packet"))
            return;  // doesn't belong to any session so ignore it
          }

          session = (*found).second;
          ZS_THROW_INVALID_ASSUMPTION_IF(!session)
        }

        // push the RUDP packet to the session to handle
        session->forSession().handleRUDP(rudp, buffer, bufferLengthInBytes);
      }

      //-----------------------------------------------------------------------
      bool RUDPICESocketSession::handleICESocketSessionReceivedSTUNPacket(
                                                                          IICESocketSessionPtr session,
                                                                          STUNPacketPtr stun,
                                                                          const String &localUsernameFrag,
                                                                          const String &remoteUsernameFrag
                                                                          )
      {
        // next we ignore all responses/error responses because they would have been handled by a requster
        if ((STUNPacket::Class_Response == stun->mClass) ||
            (STUNPacket::Class_ErrorResponse == stun->mClass)) {
          ZS_LOG_TRACE(log("this is a response which would be handled from requester, thus ignoring"))
          return false;
        }

        // can only be one of these two methods
        if ((STUNPacket::Method_ReliableChannelOpen != stun->mMethod) &&
            (STUNPacket::Method_ReliableChannelACK != stun->mMethod)) {
          ZS_LOG_TRACE(log("the request or indication is not a channel open or ACK, thus ignoring"))
          return false;
        }

        // must have username and channel number or it is illegal
        if ((!stun->hasAttribute(STUNPacket::Attribute_Username)) ||
            (!stun->hasAttribute(STUNPacket::Attribute_ChannelNumber))) {
          ZS_LOG_ERROR(Debug, log("the request or indication does not have a username, thus ignoring"))
          return false;
        }

        STUNPacketPtr response;

        do
        {
          RUDPChannelPtr session;

          // scope: next we attempt to see if there is already a session that handles this IP/channel pairing
          {
            AutoRecursiveLock lock(getLock());
            if (localUsernameFrag != mLocalUsernameFrag) {
              ZS_LOG_TRACE(log("the request local username frag does not match, thus ignoring") + ", local username frag=" + localUsernameFrag + ", expected local username frag=" + mLocalUsernameFrag)
              return false;
            }

            SessionMap::iterator found = mRemoteChannelNumberSessions.find(stun->mChannelNumber);
            if (found != mRemoteChannelNumberSessions.end()) {
              session = (*found).second;
            } else {
              if (remoteUsernameFrag != mRemoteUsernameFrag) {
                ZS_LOG_TRACE(log("the request remote username frag does not match (thus ignoring - might be for another session)") + ", remote username frag=" + remoteUsernameFrag + ", expected remote username frag=" + mRemoteUsernameFrag)
                return false;
              }
            }
          }

          if (session) {
            bool handled = session->forSession().handleSTUN(stun, response, localUsernameFrag, remoteUsernameFrag);
            if ((handled) && (!response)) return true;
          } else {
            bool handled =  handleUnknownChannel(stun, response);
            if ((handled) && (!response)) return true;
          }

          if (!response) {
            // not handled
            if (STUNPacket::Class_Request == stun->mClass) {
              stun->mErrorCode = STUNPacket::ErrorCode_BadRequest;
              response = STUNPacket::createErrorResponse(stun);
              fix(response);
            }
          }

          // make sure there is a response, if not then we abort since it was a STUN packet but we may or may not have responded
          if (!response) {
            ZS_LOG_TRACE(log("no response to send (already sent?)"))
            return false;
          }
        } while (false);  // using as a scope rather than as a loop

        if (response) {
          IICESocketSessionPtr session = getICESession();
          if (!session) return false;

          boost::shared_array<BYTE> packetized;
          ULONG packetizedLength = 0;
          response->packetize(packetized, packetizedLength, STUNPacket::RFC_draft_RUDP);

          session->sendPacket(packetized.get(), packetizedLength);
        }
        return true;
      }

      //-----------------------------------------------------------------------
      void RUDPICESocketSession::onICESocketSessionWriteReady(IICESocketSessionPtr session)
      {
        AutoRecursiveLock lock(getLock());
        for (SessionMap::iterator iter = mLocalChannelNumberSessions.begin(); iter != mLocalChannelNumberSessions.end(); ++iter) {
          (*iter).second->forSession().notifyWriteReady();
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPICESocketSession => IRUDPChannelDelegateForSessionAndListener
      #pragma mark

      //-----------------------------------------------------------------------
      void RUDPICESocketSession::onRUDPChannelStateChanged(
                                                           RUDPChannelPtr channel,
                                                           RUDPChannelStates state
                                                           )
      {
        AutoRecursiveLock lock(getLock());

        switch (state) {
          case IRUDPChannel::RUDPChannelState_Connecting: break;
          case IRUDPChannel::RUDPChannelState_Connected:
          {
            WORD channelNumber = channel->forSession().getIncomingChannelNumber();
            SessionMap::iterator found = mLocalChannelNumberSessions.find(channelNumber);
            if (found == mLocalChannelNumberSessions.end()) return;

            mRemoteChannelNumberSessions[channel->forSession().getOutgoingChannelNumber()] = channel;
            break;
          }
          case IRUDPChannel::RUDPChannelState_ShuttingDown: break;
          case IRUDPChannel::RUDPChannelState_Shutdown:
          {
            ZS_LOG_DEBUG(log("channel closed notification") + ", channel ID=" + Stringize<PUID>(channel->forSession().getID()).string())
            for (SessionMap::iterator iter = mLocalChannelNumberSessions.begin(); iter != mLocalChannelNumberSessions.end(); ++iter)
            {
              if ((*iter).second != channel) continue;
              ZS_LOG_TRACE(log("clearing out local channel number") + ", local channel number=" + Stringize<WORD>(channel->forSession().getIncomingChannelNumber()).string())
              mLocalChannelNumberSessions.erase(iter);
              break;
            }
            for (SessionMap::iterator iter = mRemoteChannelNumberSessions.begin(); iter != mRemoteChannelNumberSessions.end(); ++iter)
            {
              if ((*iter).second != channel) continue;
              ZS_LOG_TRACE(log("clearing out remote channel number") + ", remote channel number=" + Stringize<WORD>(channel->forSession().getOutgoingChannelNumber()).string())
              mRemoteChannelNumberSessions.erase(iter);
              break;
            }
            for (PendingSessionList::iterator iter = mPendingSessions.begin(); iter != mPendingSessions.end(); ++iter)
            {
              if ((*iter) != channel) continue;
              ZS_LOG_TRACE(String("clearing out pending socket session"))
              mPendingSessions.erase(iter);
              break;
            }
          }
        }
        step();
      }

      //-----------------------------------------------------------------------
      bool RUDPICESocketSession::notifyRUDPChannelSendPacket(
                                                             RUDPChannelPtr channel,
                                                             const IPAddress &remoteIP,
                                                             const BYTE *packet,
                                                             ULONG packetLengthInBytes
                                                             )
      {
        IICESocketSessionPtr session = getICESession();
        if (!session) {
          ZS_LOG_WARNING(Detail, log("send packet failed as ICE session object destroyed"))
          return false;
        }

        return session->sendPacket(packet, packetLengthInBytes);  // no need to call within a lock
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPICESocketSession => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &RUDPICESocketSession::getLock() const
      {
        RUDPICESocketPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->forSession().getLock();
      }

      //-----------------------------------------------------------------------
      String RUDPICESocketSession::log(const char *message) const
      {
        return String("RUDPICESocketSession [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      void RUDPICESocketSession::fix(STUNPacketPtr stun) const
      {
        stun->mLogObject = "RUDPICESocketSession";
        stun->mLogObjectID = mID;
      }

      //-----------------------------------------------------------------------
      IICESocketSessionPtr RUDPICESocketSession::getICESession() const
      {
        AutoRecursiveLock lock(getLock());
        return mICESession;
      }

      //-----------------------------------------------------------------------
      void RUDPICESocketSession::cancel()
      {
        AutoRecursiveLock lock(getLock());  // just in case
        if (isShutdown()) return;

        if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

        setState(RUDPICESocketSessionState_ShuttingDown);

        RUDPICESocketSessionShutdownReasons reason = getShutdownReason();

        for (SessionMap::iterator iter = mLocalChannelNumberSessions.begin(); iter != mLocalChannelNumberSessions.end(); ++iter) {
          if (RUDPICESocketSessionShutdownReason_Closed != reason) {
            (*iter).second->forSession().shutdownFromTimeout();
          } else {
            (*iter).second->forSession().shutdown();
          }
        }

        if (mGracefulShutdownReference) {
          if (mLocalChannelNumberSessions.size() > 0) {
            ZS_LOG_DEBUG(log("waiting for channels to shutdown"))
            return;
          }
        }

        setState(RUDPICESocketSessionState_Shutdown);

        mGracefulShutdownReference.reset();

        mDelegate.reset();

        if (mICESession) {
          mICESession->close();
          mICESession.reset();
        }

        IRUDPICESocketForRUDPICESocketSessionPtr outerProxy = IRUDPICESocketForRUDPICESocketSessionProxy::create(mOuter.lock());
        if (outerProxy) {
          outerProxy->onRUDPICESessionClosed(mID);
        }

        mLocalChannelNumberSessions.clear();
        mRemoteChannelNumberSessions.clear();
        mPendingSessions.clear();
      }

      //-----------------------------------------------------------------------
      void RUDPICESocketSession::step()
      {
        if ((isShuttingDown()) ||
            (isShutdown())) {
          cancel();
          return;
        }

        IICESocketSession::ICESocketSessionStates state = mICESession->getState();
        switch (state) {
          case IICESocketSession::ICESocketSessionState_Pending:  break;
          case IICESocketSession::ICESocketSessionState_Prepared:   setState(RUDPICESocketSessionState_Prepared); break;
          case IICESocketSession::ICESocketSessionState_Searching:  setState(RUDPICESocketSessionState_Searching); break;
          case IICESocketSession::ICESocketSessionState_Nominating: setState(RUDPICESocketSessionState_Searching); break;
          case IICESocketSession::ICESocketSessionState_Nominated:  setState(RUDPICESocketSessionState_Ready); break;
          case IICESocketSession::ICESocketSessionState_Shutdown:   cancel(); break;
        }
      }

      //-----------------------------------------------------------------------
      void RUDPICESocketSession::setState(RUDPICESocketSessionStates state)
      {
        if (state == mCurrentState) return;

        ZS_LOG_BASIC(log("state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state))

        mCurrentState = state;

        if (!mDelegate) return;

        RUDPICESocketSessionPtr pThis = mThisWeak.lock();

        if (pThis) {
          try {
            mDelegate->onRUDPICESocketSessionStateChanged(pThis, mCurrentState);
          } catch(IRUDPICESocketSessionDelegateProxy::Exceptions::DelegateGone &) {
          }
        }
      }

      //-----------------------------------------------------------------------
      void RUDPICESocketSession::setShutdownReason(RUDPICESocketSessionShutdownReasons reason)
      {
        AutoRecursiveLock lock(getLock());

        if (mShutdownReason == reason) return;

        getShutdownReason();  // fix the shutdown reason to the ICE socket session reason first

        if (RUDPICESocketSessionShutdownReason_None != mShutdownReason) {
          ZS_LOG_WARNING(Detail, log("shutdown reason already set") + ", current reason=" + toString(reason) + ", trying to set to reason=" + toString(reason))
          return;
        }

        ZS_LOG_DEBUG(log("shutdown reason set") + ", reason=" + toString(reason))
        mShutdownReason = reason;
      }

      //-----------------------------------------------------------------------
      bool RUDPICESocketSession::handleUnknownChannel(
                                                      STUNPacketPtr &stun,
                                                      STUNPacketPtr &response
                                                      )
      {
        AutoRecursiveLock lock(getLock());
        if ((isShuttingDown()) ||
            (isShutdown())) return false;

        if (!mICESession) return false;

        do
        {
          if (STUNPacket::Class_Indication == stun->mClass) return false;   // we don't respond to indications

          // only channel open can be used
          if (STUNPacket::Method_ReliableChannelOpen != stun->mMethod) {
            // sorry, this channel was not found therefor we discard the request
            stun->mErrorCode = STUNPacket::ErrorCode_Unauthorized;
            response = STUNPacket::createErrorResponse(stun);
            fix(response);
            break;
          }

          if ((!stun->hasAttribute(STUNPacket::Attribute_Username)) ||
              (!stun->hasAttribute(STUNPacket::Attribute_MessageIntegrity)) ||
              (!stun->hasAttribute(STUNPacket::Attribute_NextSequenceNumber)) ||
              (!stun->hasAttribute(STUNPacket::Attribute_CongestionControl)) ||
              (stun->mLocalCongestionControl.size() < 1) ||
              (stun->mRemoteCongestionControl.size() < 1)) {
            // all of these attributes are manditory otherwise the request is considered bad
            stun->mErrorCode = STUNPacket::ErrorCode_BadRequest;
            response = STUNPacket::createErrorResponse(stun);
            fix(response);
            break;
          }

          // make sure the username has the right format
          size_t pos = stun->mUsername.find(":");
          if (String::npos == pos) {
            stun->mErrorCode = STUNPacket::ErrorCode_Unauthorized;
            response = STUNPacket::createErrorResponse(stun);
            fix(response);
            break;
          }

          CryptoPP::AutoSeededRandomPool rng;
          // we have a valid nonce, we will open the channel, but first - pick an unused channel number
          UINT tries = 0;

          WORD channelNumber = 0;
          bool valid = false;
          do
          {
            ++tries;
            if (tries > HOOKFLASH_SERVICES_RUDPICESOCKETSESSION_MAX_ATTEMPTS_TO_FIND_FREE_CHANNEL_NUMBER) {
              stun->mErrorCode = STUNPacket::ErrorCode_InsufficientCapacity;
              response = STUNPacket::createErrorResponse(stun);
              fix(response);
              break;
            }

            rng.GenerateBlock((BYTE *)(&channelNumber), sizeof(channelNumber));
            channelNumber = (channelNumber % (HOOKFLASH_SERVICES_RUDPICESOCKETSESSION_CHANNEL_RANGE_END - HOOKFLASH_SERVICES_RUDPICESOCKETSESSION_CHANNEL_RANGE_START)) + HOOKFLASH_SERVICES_RUDPICESOCKETSESSION_CHANNEL_RANGE_START;

            // check to see if the channel was used for this IP before...
            SessionMap::iterator found = mLocalChannelNumberSessions.find(channelNumber);
            valid = (found == mLocalChannelNumberSessions.end());
          } while (!valid);

          if (!valid) break;

          Candidate nominatedLocal;
          Candidate nominatedRemote;
          bool hasCandidate = getNominatedCandidateInformation(nominatedLocal, nominatedRemote);
          if (!hasCandidate) break;

          // found a useable channel number therefor create a new session
          RUDPChannelPtr session = IRUDPChannelForRUDPICESocketSession::createForRUDPICESocketSessionIncoming(
                                                                                                              getAssociatedMessageQueue(),
                                                                                                              mThisWeak.lock(),
                                                                                                              getConnectedRemoteIP(),
                                                                                                              channelNumber,
                                                                                                              nominatedLocal.mUsernameFrag,
                                                                                                              nominatedRemote.mUsernameFrag,
                                                                                                              nominatedLocal.mPassword,
                                                                                                              nominatedRemote.mPassword,
                                                                                                              stun,
                                                                                                              response
                                                                                                              );
          if (!response) {
            // there must be a response or it is an error
            stun->mErrorCode = STUNPacket::ErrorCode_BadRequest;
            response = STUNPacket::createErrorResponse(stun);
            fix(response);
            break;
          }
          if (response) {
            if (STUNPacket::Class_ErrorResponse == response->mClass) {
              // do not add the session if there was an error response
              break;
            }
          }

          mLocalChannelNumberSessions[channelNumber] = session;
          mRemoteChannelNumberSessions[stun->mChannelNumber] = session;
          mPendingSessions.push_back(session);

          // inform the delegate of the new session waiting...
          try {
            mDelegate->onRUDPICESocketSessionChannelWaiting(mThisWeak.lock());
          } catch(IRUDPICESocketSessionDelegateProxy::Exceptions::DelegateGone &) {
            setShutdownReason(RUDPICESocketSessionShutdownReason_DelegateGone);
            cancel();
            return true;
          }
        } while (false);  // using as a scope rather than as a loop

        return response;
      }

      //-----------------------------------------------------------------------
      void RUDPICESocketSession::issueChannelConnectIfPossible()
      {
        AutoRecursiveLock lock(getLock());
        if (!isReady()) return;

        for (SessionMap::iterator iter = mLocalChannelNumberSessions.begin(); iter != mLocalChannelNumberSessions.end(); ++iter) {
          SessionMap::iterator found = mRemoteChannelNumberSessions.find((*iter).second->forSession().getOutgoingChannelNumber());
          if (found == mRemoteChannelNumberSessions.end()) {
            (*iter).second->forSession().issueConnectIfNotIssued();
          }
        }
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IRUDPICESocketSession
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IRUDPICESocketSession::toString(RUDPICESocketSessionStates states)
    {
      switch (states) {
        case RUDPICESocketSessionState_Pending:       return "Preparing";
        case RUDPICESocketSessionState_Prepared:      return "Prepared";
        case RUDPICESocketSessionState_Searching:     return "Searching";
        case RUDPICESocketSessionState_Ready:         return "Ready";
        case RUDPICESocketSessionState_ShuttingDown:  return "Shutting down";
        case RUDPICESocketSessionState_Shutdown:      return "Shutdown";
        default: break;
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    const char *IRUDPICESocketSession::toString(RUDPICESocketSessionShutdownReasons reason)
    {
      return IICESocketSession::toString((IICESocketSession::ICESocketSessionShutdownReasons)reason);
    }
  }
}
