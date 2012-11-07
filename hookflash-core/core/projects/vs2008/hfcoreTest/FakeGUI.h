
#pragma once

#include <hookflash/IStack.h>
#include <hookflash/IConversationThread.h>
#include <hookflash/IParticipant.h>
#include <hookflash/ICall.h>
#include <hookflash/IClient.h>
#include <hookflash/hookflashTypes.h>
#include <hookflash/IAccount.h>
#include <hookflash/IAuthorizationList.h>
#include <hookflash/IContact.h>
#include <hookflash/IFileTransfer.h>
#include <hookflash/provisioning/IAccount.h>
#include <hookflash/provisioning/IAccountLinkSocialSignin.h>
#include <hookflash/provisioning/IAccountSocialInformation.h>

#include <hookflash/hookflashTypes.h>

#include <map>

namespace hfcoretest
{
  class FakeGUI;
  typedef boost::shared_ptr<FakeGUI> FakeGUIPtr;
  typedef boost::weak_ptr<FakeGUI> FakeGUIWeakPtr;

  // notice that this object is intentionally NOT inheritenting from zsLib::MessageQueueAssociator? That's because
  // we want to make sure that the stack will automatically assign our object to the message queue of the main
  // GUI thread.
  class FakeGUI : public hookflash::IStackDelegate,
                  public hookflash::IConversationThreadDelegate,
                  public hookflash::IParticipantDelegate,
                  public hookflash::ICallDelegate,
                  public hookflash::IClientLogDelegate,
                  public hookflash::IAccountDelegate,
                  public hookflash::IContactDelegate,
                  public hookflash::IAuthorizationListDelegate,
                  public hookflash::IFileTransferDelegate,
                  public hookflash::provisioning::IAccountDelegate,
                  public hookflash::provisioning::IAccountLinkSocialSigninDelegate,
                  public hookflash::provisioning::IAccountSocialInformationDelegate
                  //public hookflash::IClientDelegate
  {
  protected:
    FakeGUI();

  public:
    static FakeGUIPtr create();

    void operator()();
    void waitShutdown();

    // IStackDelegate
    virtual void onShutdownReady();

    // IConversationThreadDelegate
    virtual void onConversationThreadNew(hookflash::IConversationThreadPtr conversationThread) {}

    virtual void onConversationThreadParticipantsChanged(hookflash::IConversationThreadPtr conversationThread) {}

    virtual void onConversationThreadStateChanged(hookflash::IConversationThreadPtr conversationThread) {}

    virtual void onConversationThreadMessageDelivered(
                                                      hookflash::IConversationThreadPtr conversationThread,
                                                      const char *messageID
                                                      ) {}

    virtual void onConversationThreadParticipantHasFloor(
                                                         hookflash::IConversationThreadPtr call,
                                                         hookflash::IParticipantPtr participant
                                                         ) {}
    virtual void onConversationThreadParticipantLostFloor(
                                                          hookflash::IConversationThreadPtr call,
                                                          hookflash::IParticipantPtr participant
                                                          ) {}

    // IParticipantDelegate
    virtual void onParticipantContactUpdated(hookflash::IParticipantPtr participant) {}

    virtual void onParticipantRequestingAddContactChanged(hookflash::IParticipantPtr participant) {}

    virtual void onParticipantMediaStateChanged(hookflash::IParticipantPtr participant) {}

    virtual void onParticipantMessageReceived(
                                              hookflash::IParticipantPtr participant,
                                              const char *messageID,
                                              const char *message,
                                              double receivedEpoch
                                              ) {}

    // ICallDelegate
    virtual void onCallRequesting(hookflash::ICallPtr call) {}

    virtual void onCallStateChanged(hookflash::ICallPtr call) {}


    // IClientLogDelegate
    virtual void onNewSubsystem(
                                zsLib::PTRNUMBER subsystemID,
                                const char *subsystemName
                                );

    virtual void onLog(
                       zsLib::PTRNUMBER subsystemID,
                       const char *subsystemName,
                       hookflash::IClient::Log::Severity inSeverity,
                       hookflash::IClient::Log::Level inLevel,
                       const char *inMessage,
                       const char *inFunction,
                       const char *inFilePath,
                       zsLib::ULONG inLineNumber
                       );


    virtual void onParticipantStateChanged(hookflash::IParticipantPtr participant) {}

    virtual void onParticipantCallListChanged(hookflash::IParticipantPtr participant) {}

    virtual void onParticipantMessageReceived(
      hookflash::IParticipantPtr participant,
      const char *messageID,
      const char *message,
      time_t receivedEpoch
      ) {}


    //openpeer::IAccountDelegate
    virtual void onAccountStateChanged(hookflash::IAccountPtr account) {}
    virtual void onAccountGroupsChanged(hookflash::IAccountPtr account) {}
    virtual void onAccountParkedConversationThreadsChanged(hookflash::IAccountPtr) {}


    //IContactDelegate
    virtual void onMultipleContactsUpdated(hookflash::IContactDelegate::ContactList contacts){}

    //IAuthorizationListDelegate
    virtual void onAuthorizationListChanged(hookflash::IAuthorizationListPtr list){}

    //IFileTransferDelegate
    virtual void onFileTransferNew(hookflash::IFileTransferPtr){}
    virtual void onFileTransferStateChanged(hookflash::IFileTransferPtr){}

    //IClientDelegate
    //virtual void onMessagePutInGUIQueue(){}

    //provisioning::IAccountDelegate
    virtual void onProvisioningAccountCaptchaChallenge(hookflash::provisioning::IAccountPtr provisioning, hookflash::ICaptchaPtr captcha){}

    virtual void onProvisioningAccountStateChanged(hookflash::provisioning::IAccount::ProvisioningStates state, hookflash::provisioning::IAccountDelegate::ProvisioningErrors error, zsLib::String errormsg){
    int i = 1;
    }

    virtual void onProvisioningAccountPeerFileReceived(zsLib::String peerFile, zsLib::String userID, zsLib::String peerFilePassword){
      int i = 1;
    }

   /* virtual void onProvisioningAccountErrorOccured(ProvisioningErrors errorCode, zsLib::String errormsg){
      int i = 1;
    }*/

    virtual void onProvisioningAccountEmailValidationResent(hookflash::provisioning::IAccountPtr provisioning){}
    virtual void onProvisioningAccountEmailValidationComplete(hookflash::provisioning::IAccountPtr provisioning){}
    virtual void onProvisioningAccountUserIDReceived(zsLib::String userID){}

//provisioning::IAccountLinkSocialSigninDelegate

 virtual void onProvisioningAccountLinkSocialSigninFailed(
                                                               hookflash::provisioning::IAccountLinkSocialSignin::ProvisioningStates state, 
															   AccountLinkSocialSigningErrors error = hookflash::provisioning::IAccountLinkSocialSignin::AccountLinkSocialSigningError_None, 
															   zsLib::String errormsg = ""
															   ) {}

 virtual void onProvisioningAccountLinkSocialSigninComplete(hookflash::provisioning::IAccountLinkSocialSigninPtr provisioning) {}
 virtual void onProvisioningAccountLinkSocialSigninAssociate(zsLib::String mMessage) {}



//provisioning::IAccountSocialInformationDelegate

 virtual void onProvisioningAccountSocialInformationFailed(
		                                                        hookflash::provisioning::IAccountSocialInformation::ProvisioningStates state, 
															    AccountSocialInformationErrors error = hookflash::provisioning::IAccountSocialInformation::AccountSocialInformationError_None, 
															    zsLib::String errormsg = ""
																) {}

 virtual void onProvisioningAccountSocialInformationComplete(hookflash::provisioning::IAccountSocialInformationPtr provisioning) {}
 virtual void onProvisioningAccountSocialInformationUserExist(bool userExist) {}
 virtual void onProvisioningAccountSocialInformationUserProfile(zsLib::String mProviderProfile) {}


  private:
    void spawn();

  protected:
    typedef std::map<zsLib::PTRNUMBER, const char *> SubsystemMap;

    zsLib::RecursiveLock mLock;
    FakeGUIWeakPtr mThisWeak;
    zsLib::ThreadPtr mThread;

    bool mShouldShutdown;
#ifdef _WIN32
    DWORD mThreadID;
#endif //_WIN32

    zsLib::ULONG mLoopRun;
    hookflash::IStackPtr mStack;

    SubsystemMap mSubsystemMap;
  };
}
