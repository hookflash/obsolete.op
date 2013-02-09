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

#include <hookflash/services/internal/services_RUDPICESocket.h>
#include <hookflash/services/internal/services_RUDPICESocketSession.h>

#include <zsLib/Exception.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>

#include <algorithm>

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
      #pragma mark RUDPICESocket
      #pragma mark

      //-----------------------------------------------------------------------
      RUDPICESocket::RUDPICESocket(
                                   IMessageQueuePtr queue,
                                   IRUDPICESocketDelegatePtr delegate
                                   ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mCurrentState(RUDPICESocketState_Pending)
      {
        mDelegates[0] = IRUDPICESocketDelegateProxy::createWeak(queue, delegate);
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
        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPICESocket => IRUDPICESocket
      #pragma mark

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
      IRUDPICESocket::RUDPICESocketStates RUDPICESocket::getState() const
      {
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      IRUDPICESocketSubscriptionPtr RUDPICESocket::subscribe(IRUDPICESocketDelegatePtr delegate)
      {
        AutoRecursiveLock lock(mLock);
        IRUDPICESocket::RUDPICESocketStates state = getState();

        delegate = IRUDPICESocketDelegateProxy::createWeak(getAssociatedMessageQueue(), delegate);

        SubscriptionPtr subscription = Subscription::create(mThisWeak.lock());

        if (RUDPICESocketState_Pending != state) {
          try {
            delegate->onRUDPICESocketStateChanged(mThisWeak.lock(), state);
          } catch (IRUDPICESocketDelegateProxy::Exceptions::DelegateGone &) {
          }
        }

        if (RUDPICESocketState_Shutdown == state) return subscription;
        mDelegates[subscription->mID] = delegate;
        return subscription;
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
                                                                                const CandidateList &remoteCandidates,
                                                                                ICEControls control
                                                                                )
      {
        AutoRecursiveLock lock(mLock);
        RUDPICESocketSessionPtr session = IRUDPICESocketSessionForRUDPICESocket::create(getAssociatedMessageQueue(), mThisWeak.lock(), delegate, remoteCandidates, control);

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
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPICESocket => friend Subscription
      #pragma mark

      //-----------------------------------------------------------------------
      void RUDPICESocket::cancelSubscription(Subscription &subscription)
      {
        AutoRecursiveLock lock(mLock);
        DelegateMap::iterator found = mDelegates.find(subscription.getID());
        if (found == mDelegates.end()) return;

        mDelegates.erase(found);
        if (isShuttingDown()) {
          cancel();
        }
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
        return String("RUDPICESocket [") + Stringize<PUID>(mID).string() + "] " + message;
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
        mDelegates.clear();

        setState(RUDPICESocketState_Shutdown);

        if (mICESocket) {
          mICESocket->shutdown();
          mICESocket.reset();
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
          // notify the delegates
          for (DelegateMap::iterator delIter = mDelegates.begin(); delIter != mDelegates.end(); )
          {
            DelegateMap::iterator current = delIter;
            ++delIter;

            try {
              (*current).second->onRUDPICESocketStateChanged(mThisWeak.lock(), (RUDPICESocketStates)state);
            } catch(IRUDPICESocketDelegateProxy::Exceptions::DelegateGone &) {
              ZS_LOG_WARNING(Detail, log("delegate gone"))
              mDelegates.erase(current);
            }
          }
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RUDPICESocket::Subscription
      #pragma mark

      //-----------------------------------------------------------------------
      RUDPICESocket::Subscription::Subscription(RUDPICESocketPtr outer) :
        mOuter(outer),
        mID(zsLib::createPUID())
      {
      }

      //-----------------------------------------------------------------------
      RUDPICESocket::Subscription::~Subscription()
      {
        cancel();
      }

      //-----------------------------------------------------------------------
      RUDPICESocket::SubscriptionPtr RUDPICESocket::Subscription::create(RUDPICESocketPtr outer)
      {
        SubscriptionPtr pThis(new Subscription(outer));
        return pThis;
      }

      //-----------------------------------------------------------------------
      void RUDPICESocket::Subscription::cancel()
      {
        RUDPICESocketPtr outer = mOuter.lock();
        if (!outer) return;

        outer->cancelSubscription(*this);
        mOuter.reset();
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
