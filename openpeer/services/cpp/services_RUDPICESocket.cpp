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

#include <openpeer/services/internal/services_RUDPICESocket.h>
#include <openpeer/services/internal/services_RUDPICESocketSession.h>
#include <openpeer/services/internal/services_Helper.h>

#include <zsLib/Exception.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>

#include <algorithm>

namespace openpeer { namespace services { ZS_DECLARE_SUBSYSTEM(openpeer_services) } }


namespace openpeer
{
  namespace services
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPICESocket
      #pragma mark

      //-----------------------------------------------------------------------
      RUDPICESocket::RUDPICESocket(
                                   IMessageQueuePtr queue,
                                   IRUDPICESocketDelegatePtr delegate
                                   ) :
        MessageQueueAssociator(queue),
        mCurrentState(RUDPICESocketState_Pending)
      {
        mDefaultSubscription = mSubscriptions.subscribe(delegate, queue);
        ZS_LOG_BASIC(log("created"))
      }

      //-----------------------------------------------------------------------
      void RUDPICESocket::init(
                               const char *turnServer,
                               const char *turnServerUsername,
                               const char *turnServerPassword,
                               const char *stunServer,
                               WORD port
                               )
      {
        AutoRecursiveLock lock(mLock);    // just in case
        mICESocket = IICESocket::create(
                                        getAssociatedMessageQueue(),
                                        mThisWeak.lock(),
                                        turnServer,
                                        turnServerUsername,
                                        turnServerPassword,
                                        stunServer,
                                        port,
                                        true
                                        );
      }

      //-----------------------------------------------------------------------
      void RUDPICESocket::init(
                               IDNS::SRVResultPtr srvTURNUDP,
                               IDNS::SRVResultPtr srvTURNTCP,
                               const char *turnServerUsername,
                               const char *turnServerPassword,
                               IDNS::SRVResultPtr srvSTUN,
                               WORD port
                               )
      {
        AutoRecursiveLock lock(mLock);    // just in case
        mICESocket = IICESocket::create(
                                        getAssociatedMessageQueue(),
                                        mThisWeak.lock(),
                                        srvTURNUDP,
                                        srvTURNTCP,
                                        turnServerUsername,
                                        turnServerPassword,
                                        srvSTUN,
                                        port,
                                        true
                                        );
      }

      //-----------------------------------------------------------------------
      RUDPICESocket::~RUDPICESocket()
      {
        if (isNoop()) return;
        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      RUDPICESocketPtr RUDPICESocket::convert(IRUDPICESocketPtr socket)
      {
        return boost::dynamic_pointer_cast<RUDPICESocket>(socket);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPICESocket => IRUDPICESocket
      #pragma mark

      //-----------------------------------------------------------------------
      String RUDPICESocket::toDebugString(IRUDPICESocketPtr socket, bool includeCommaPrefix)
      {
        if (!socket) return String(includeCommaPrefix ? ", rudp ice socket=(null)" : "rudp ice socket=(null)");

        RUDPICESocketPtr pThis = RUDPICESocket::convert(socket);
        return pThis->getDebugValueString(includeCommaPrefix);
      }
      
      //-----------------------------------------------------------------------
      RUDPICESocketPtr RUDPICESocket::create(
                                             IMessageQueuePtr queue,
                                             IRUDPICESocketDelegatePtr delegate,
                                             const char *turnServer,
                                             const char *turnServerUsername,
                                             const char *turnServerPassword,
                                             const char *stunServer,
                                             WORD port
                                             )
      {
        RUDPICESocketPtr pThis(new RUDPICESocket(queue, delegate));
        pThis->mThisWeak = pThis;
        pThis->init(turnServer, turnServerUsername, turnServerPassword, stunServer, port);
        return pThis;
      }

      //-----------------------------------------------------------------------
      RUDPICESocketPtr RUDPICESocket::create(
                                             IMessageQueuePtr queue,
                                             IRUDPICESocketDelegatePtr delegate,
                                             IDNS::SRVResultPtr srvTURNUDP,
                                             IDNS::SRVResultPtr srvTURNTCP,
                                             const char *turnServerUsername,
                                             const char *turnServerPassword,
                                             IDNS::SRVResultPtr srvSTUN,
                                             WORD port
                                             )
      {
        RUDPICESocketPtr pThis(new RUDPICESocket(queue, delegate));
        pThis->mThisWeak = pThis;
        pThis->init(srvTURNUDP, srvTURNTCP, turnServerUsername, turnServerPassword, srvSTUN, port);
        return pThis;
      }

      //-----------------------------------------------------------------------
      IRUDPICESocket::RUDPICESocketStates RUDPICESocket::getState(
                                                                  WORD *outLastErrorCode,
                                                                  String *outLastErrorReason
                                                                  ) const
      {
        AutoRecursiveLock lock(getLock());
        if (outLastErrorCode) {
          *outLastErrorCode = 0;
        }
        if (outLastErrorReason) {
          *outLastErrorReason = String();
        }

        if (mICESocket) {
          mICESocket->getState(outLastErrorCode, outLastErrorReason);
        }

        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      IRUDPICESocketSubscriptionPtr RUDPICESocket::subscribe(IRUDPICESocketDelegatePtr originalDelegate)
      {
        ZS_LOG_DETAIL(log("subscribing to socket state"))

        AutoRecursiveLock lock(getLock());
        if (!originalDelegate) return mDefaultSubscription;

        IRUDPICESocketSubscriptionPtr subscription = mSubscriptions.subscribe(originalDelegate);

        IRUDPICESocketDelegatePtr delegate = mSubscriptions.delegate(subscription);

        if (delegate) {
          RUDPICESocketPtr pThis = mThisWeak.lock();

          if (RUDPICESocketState_Pending != mCurrentState) {
            delegate->onRUDPICESocketStateChanged(pThis, mCurrentState);
          }
          if (mNotifiedCandidateChanged) {
            delegate->onRUDPICESocketCandidatesChanged(pThis);
          }
        }

        if (isShutdown()) {
          mSubscriptions.clear();
        }

        return subscription;
      }

      //-----------------------------------------------------------------------
      String RUDPICESocket::getUsernameFrag() const
      {
        AutoRecursiveLock lock(getLock());
        if (!mICESocket) return String();
        return mICESocket->getUsernameFrag();
      }

      //-----------------------------------------------------------------------
      String RUDPICESocket::getPassword() const
      {
        AutoRecursiveLock lock(getLock());
        if (!mICESocket) return String();
        return mICESocket->getPassword();
      }

      //-----------------------------------------------------------------------
      void RUDPICESocket::shutdown()
      {
        AutoRecursiveLock lock(mLock);
        cancel();
      }

      //-----------------------------------------------------------------------
      void RUDPICESocket::wakeup(Duration minimumTimeCandidatesMustRemainValidWhileNotUsed)
      {
        IICESocketPtr socket = getICESocket();
        if (!socket) return;

        socket->wakeup(minimumTimeCandidatesMustRemainValidWhileNotUsed); // no need to do while inside a lock
      }

      //-----------------------------------------------------------------------
      void RUDPICESocket::getLocalCandidates(CandidateList &outCandidates)
      {
        outCandidates.clear();

        IICESocketPtr socket = getICESocket();
        if (!socket) return;

        socket->getLocalCandidates(outCandidates); // no need to do while inside a lock
      }

      //-----------------------------------------------------------------------
      IRUDPICESocketSessionPtr RUDPICESocket::createSessionFromRemoteCandidates(
                                                                                IRUDPICESocketSessionDelegatePtr delegate,
                                                                                const char *remoteUsernameFrag,
                                                                                const char *remotePassword,
                                                                                const CandidateList &remoteCandidates,
                                                                                ICEControls control
                                                                                )
      {
        AutoRecursiveLock lock(mLock);
        RUDPICESocketSessionPtr session = IRUDPICESocketSessionForRUDPICESocket::create(getAssociatedMessageQueue(), mThisWeak.lock(), delegate, remoteUsernameFrag, remotePassword, remoteCandidates, control);

        if ((isShuttingDown()) ||
            (isShutdown())) {
          session->forSocket().shutdown();
          return session;
        }

        mSessions[session->forSocket().getID()] = session;
        return session;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPICESocket => IRUDPICESocketForRUDPICESocketSession
      #pragma mark

      //-----------------------------------------------------------------------
      IICESocketPtr RUDPICESocket::getICESocket() const
      {
        return mICESocket;
      }

      //-----------------------------------------------------------------------
      IRUDPICESocketPtr RUDPICESocket::getRUDPICESocket() const
      {
        return mThisWeak.lock();
      }

      //-----------------------------------------------------------------------
      void RUDPICESocket::onRUDPICESessionClosed(PUID sessionID)
      {
        AutoRecursiveLock lock(mLock);
        do {
          SessionMap::iterator found = mSessions.find(sessionID);
          if (found == mSessions.end()) break;

          mSessions.erase(found);
        } while(false); // using as a scope rather than as a loop

        if ((isShuttingDown()) ||
            (isShutdown()) ||
            (mGracefulShutdownReference)) {
          cancel();
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPICESocket => IICESocketDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void RUDPICESocket::onICESocketStateChanged(
                                                  IICESocketPtr socket,
                                                  ICESocketStates state
                                                  )
      {
        AutoRecursiveLock lock(mLock);

        setState((RUDPICESocketStates)state);

        if ((isShutdown()) ||
            (isShuttingDown())) {
          cancel();
        }
      }

      //-----------------------------------------------------------------------
      void RUDPICESocket::onICESocketCandidatesChanged(IICESocketPtr socket)
      {
        AutoRecursiveLock lock(mLock);

        get(mNotifiedCandidateChanged) = true;

        mSubscriptions.delegate()->onRUDPICESocketCandidatesChanged(mThisWeak.lock());
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPICESocket => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String RUDPICESocket::log(const char *message) const
      {
        return String("RUDPICESocket [") + string(mID) + "] " + message;
      }

      //-----------------------------------------------------------------------
      bool RUDPICESocket::isShuttingDown()
      {
        return RUDPICESocketState_ShuttingDown == mCurrentState;
      }

      //-----------------------------------------------------------------------
      bool RUDPICESocket::isShutdown()
      {
        return RUDPICESocketState_Shutdown == mCurrentState;
      }

      //-----------------------------------------------------------------------
      String RUDPICESocket::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return

        Helper::getDebugValue("rudp ice socket id", string(mID), firstTime) +

        Helper::getDebugValue("graceful shutdown", mGracefulShutdownReference ? String("true") : String(), firstTime) +

        Helper::getDebugValue("state", IRUDPICESocket::toString(mCurrentState), firstTime) +

        Helper::getDebugValue("subscriptions", mSubscriptions.size() > 0 ? string(mSubscriptions.size()) : String(), firstTime) +
        Helper::getDebugValue("default subscription", mDefaultSubscription ? String("true") : String(), firstTime) +

        Helper::getDebugValue("notified candidates changed", mNotifiedCandidateChanged ? String("true") : String(), firstTime) +

        Helper::getDebugValue("sessions", mSessions.size() > 0 ? string(mSessions.size()) : String(), firstTime) +

        IICESocket::toDebugString(mICESocket, firstTime);
      }
      
      //-----------------------------------------------------------------------
      void RUDPICESocket::cancel()
      {
        AutoRecursiveLock lock(mLock);    // just in case
        if (isShutdown()) return;

        setState(RUDPICESocketState_ShuttingDown);

        if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

        if (mSessions.size() > 0) {
          for (SessionMap::iterator iter = mSessions.begin(); iter != mSessions.end(); ++iter) {
            (*iter).second->forSocket().shutdown();
          }
        }

        if (mGracefulShutdownReference) {
          if (mSessions.size() > 0) {
            ZS_LOG_DEBUG(log("waiting for sessions to shutdown"))
            return;
          }

          if (mICESocket) {
            mICESocket->shutdown();
          }

          if (IICESocket::ICESocketState_Shutdown != mICESocket->getState()) {
            ZS_LOG_DEBUG(log("waiting for object to shutdown"))
            return;
          }
        }

        setState(RUDPICESocketState_Shutdown);

        mGracefulShutdownReference.reset();

        mSubscriptions.clear();
        mDefaultSubscription.reset();

        setState(RUDPICESocketState_Shutdown);

        if (mICESocket) {
          mICESocket->shutdown();
        }
      }

      //-----------------------------------------------------------------------
      void RUDPICESocket::setState(RUDPICESocketStates state)
      {
        if (isShutdown()) return; // prevent going backwards to "shutting down" or other states from "shutdown"
        if (mCurrentState == state) return;

        ZS_LOG_BASIC(log("state changed") + ", state=" + IRUDPICESocket::toString(state))

        mCurrentState = state;

        RUDPICESocketPtr pThis = mThisWeak.lock();

        if (pThis) {
          mSubscriptions.delegate()->onRUDPICESocketStateChanged(pThis, mCurrentState);
        }
      }

    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IRUDPICESocket
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IRUDPICESocket::toString(RUDPICESocketStates state)
    {
      return IICESocket::toString((IICESocket::ICESocketStates)state);
    }

    //-------------------------------------------------------------------------
    String IRUDPICESocket::toDebugString(IRUDPICESocketPtr socket, bool includeCommaPrefix)
    {
      return internal::RUDPICESocket::toDebugString(socket, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    IRUDPICESocketPtr IRUDPICESocket::create(
                                             IMessageQueuePtr queue,
                                             IRUDPICESocketDelegatePtr delegate,
                                             const char *turnServer,
                                             const char *turnServerUsername,
                                             const char *turnServerPassword,
                                             const char *stunServer,
                                             WORD port
                                             )
    {
      return internal::IRUDPICESocketFactory::singleton().create(queue, delegate, turnServer, turnServerUsername, turnServerPassword, stunServer, port);
    }

    //-------------------------------------------------------------------------
    IRUDPICESocketPtr IRUDPICESocket::create(
                                             IMessageQueuePtr queue,
                                             IRUDPICESocketDelegatePtr delegate,
                                             IDNS::SRVResultPtr srvTURNUDP,
                                             IDNS::SRVResultPtr srvTURNTCP,
                                             const char *turnServerUsername,
                                             const char *turnServerPassword,
                                             IDNS::SRVResultPtr srvSTUN,
                                             WORD port
                                             )
    {
      return internal::IRUDPICESocketFactory::singleton().create(queue, delegate, srvTURNUDP, srvTURNTCP, turnServerUsername, turnServerPassword, srvSTUN, port);
    }
  }
}
