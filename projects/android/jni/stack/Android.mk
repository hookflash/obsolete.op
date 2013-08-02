LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

MAIN_PATH := $(LOCAL_PATH)../../../../../core/openpeer/stack/cpp
SOURCE_PATH := ../../../../openpeer/stack/cpp
MESSAGE_SOURCE_PATH := ../../../../openpeer/stack/message
INCLUDE_PATH := $(LOCAL_PATH)../../../../../core


# main shared library
include $(CLEAR_VARS)
#LOCAL_CFLAGS := -D_ANDROID $(LOCAL_CFLAGS)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../.. \
		    $(LOCAL_PATH)/../../../../libs/zsLib \
		    $(LOCAL_PATH)/../../../../libs \
		    $(LOCAL_PATH)/../../../../libs/boost/boost
LOCAL_MODULE    := hfstack
LOCAL_SRC_FILES := $(SOURCE_PATH)/stack.cpp \
		   $(SOURCE_PATH)/stack_Account.cpp \
		   $(SOURCE_PATH)/stack_AccountFinder.cpp \
		   $(SOURCE_PATH)/stack_AccountPeerLocation.cpp \
		   $(SOURCE_PATH)/stack_BootstrappedNetwork.cpp \
		   $(SOURCE_PATH)/stack_BootstrappedNetworkManager.cpp \
		  $(SOURCE_PATH)/stack_Cache.cpp \
		   $(SOURCE_PATH)/stack_Diff.cpp \
		   $(SOURCE_PATH)/stack_Factory.cpp \
		   $(SOURCE_PATH)/stack_Helper.cpp \
		   $(SOURCE_PATH)/stack_Location.cpp \
		   $(SOURCE_PATH)/stack_MessageIncoming.cpp \
		   $(SOURCE_PATH)/stack_MessageMonitor.cpp \
		   $(SOURCE_PATH)/stack_MessageMonitorManager.cpp \
		   $(SOURCE_PATH)/stack_Peer.cpp \
		   $(SOURCE_PATH)/stack_PeerFilePrivate.cpp \
		   $(SOURCE_PATH)/stack_PeerFilePublic.cpp \
		   $(SOURCE_PATH)/stack_PeerFiles.cpp \
		   $(SOURCE_PATH)/stack_PeerSubscription.cpp \
		   $(SOURCE_PATH)/stack_Publication.cpp \
		   $(SOURCE_PATH)/stack_PublicationMetaData.cpp \
		   $(SOURCE_PATH)/stack_PublicationRepository.cpp \
		   $(SOURCE_PATH)/stack_RSAPrivateKey.cpp \
		   $(SOURCE_PATH)/stack_RSAPublicKey.cpp \
		   $(SOURCE_PATH)/stack_ServiceCertificatesValidateQuery.cpp \
		   $(SOURCE_PATH)/stack_ServiceIdentitySession.cpp \
		   $(SOURCE_PATH)/stack_ServiceLockboxSession.cpp \
		  $(SOURCE_PATH)/stack_ServiceNamespaceGrantSession.cpp \
		   $(SOURCE_PATH)/stack_ServiceSaltFetchSignedSaltQuery.cpp \
		   $(SOURCE_PATH)/stack_stack.cpp \
		   $(MESSAGE_SOURCE_PATH)/cpp/stack_message_Message.cpp \
		   $(MESSAGE_SOURCE_PATH)/cpp/stack_message_MessageFactoryManager.cpp \
		   $(MESSAGE_SOURCE_PATH)/cpp/stack_message_MessageFactoryUnknown.cpp \
		   $(MESSAGE_SOURCE_PATH)/cpp/stack_message_MessageHelper.cpp \
		   $(MESSAGE_SOURCE_PATH)/cpp/stack_message_MessageNotify.cpp \
		   $(MESSAGE_SOURCE_PATH)/cpp/stack_message_MessageNotifyUnknown.cpp \
		   $(MESSAGE_SOURCE_PATH)/cpp/stack_message_MessageReply.cpp \
		   $(MESSAGE_SOURCE_PATH)/cpp/stack_message_MessageRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/cpp/stack_message_MessageRequestUnknown.cpp \
		   $(MESSAGE_SOURCE_PATH)/cpp/stack_message_MessageResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/cpp/stack_message_messages.cpp \
		   $(MESSAGE_SOURCE_PATH)/bootstrapped-finder/cpp/FindersGetRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/bootstrapped-finder/cpp/FindersGetResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/bootstrapped-finder/cpp/MessageFactoryBootstrappedFinder.cpp \
		   $(MESSAGE_SOURCE_PATH)/bootstrapper/cpp/MessageFactoryBootstrapper.cpp \
		   $(MESSAGE_SOURCE_PATH)/bootstrapper/cpp/ServicesGetRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/bootstrapper/cpp/ServicesGetResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/certificates/cpp/CertificatesGetRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/certificates/cpp/CertificatesGetResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/certificates/cpp/MessageFactoryCertificates.cpp \
		   $(MESSAGE_SOURCE_PATH)/identity/cpp/IdentityAccessCompleteNotify.cpp \
		   $(MESSAGE_SOURCE_PATH)/identity/cpp/IdentityAccessLockboxUpdateRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/identity/cpp/IdentityAccessLockboxUpdateResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/identity/cpp/IdentityAccessRolodexCredentialsGetRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/identity/cpp/IdentityAccessRolodexCredentialsGetResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/identity/cpp/IdentityAccessStartNotify.cpp \
		   $(MESSAGE_SOURCE_PATH)/identity/cpp/IdentityAccessWindowRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/identity/cpp/IdentityAccessWindowResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/identity/cpp/IdentityLookupUpdateRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/identity/cpp/IdentityLookupUpdateResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/identity/cpp/MessageFactoryIdentity.cpp \
		  $(MESSAGE_SOURCE_PATH)/identity-lockbox/cpp/LockboxAccessRequest.cpp \
		  $(MESSAGE_SOURCE_PATH)/identity-lockbox/cpp/LockboxAccessResult.cpp \
		  $(MESSAGE_SOURCE_PATH)/identity-lockbox/cpp/LockboxContentGetRequest.cpp \
		  $(MESSAGE_SOURCE_PATH)/identity-lockbox/cpp/LockboxContentGetResult.cpp \
		  $(MESSAGE_SOURCE_PATH)/identity-lockbox/cpp/LockboxContentSetRequest.cpp \
		  $(MESSAGE_SOURCE_PATH)/identity-lockbox/cpp/LockboxContentSetResult.cpp \
		  $(MESSAGE_SOURCE_PATH)/identity-lockbox/cpp/LockboxIdentitiesUpdateRequest.cpp \
		  $(MESSAGE_SOURCE_PATH)/identity-lockbox/cpp/LockboxIdentitiesUpdateResult.cpp \
		  $(MESSAGE_SOURCE_PATH)/identity-lockbox/cpp/LockboxNamespaceGrantChallengeValidateRequest.cpp \
		  $(MESSAGE_SOURCE_PATH)/identity-lockbox/cpp/LockboxNamespaceGrantChallengeValidateResult.cpp \
		  $(MESSAGE_SOURCE_PATH)/identity-lockbox/cpp/MessageFactoryIdentityLockbox.cpp \
		  $(MESSAGE_SOURCE_PATH)/identity-lookup/cpp/IdentityLookupCheckRequest.cpp \
		  $(MESSAGE_SOURCE_PATH)/identity-lookup/cpp/IdentityLookupCheckResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/identity-lookup/cpp/IdentityLookupRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/identity-lookup/cpp/IdentityLookupResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/identity-lookup/cpp/MessageFactoryIdentityLookup.cpp \
		  $(MESSAGE_SOURCE_PATH)/namespace-grant/cpp/MessageFactoryNamespaceGrant.cpp \
		  $(MESSAGE_SOURCE_PATH)/namespace-grant/cpp/NamespaceGrantCompleteNotify.cpp \
		  $(MESSAGE_SOURCE_PATH)/namespace-grant/cpp/NamespaceGrantStartNotify.cpp \
		  $(MESSAGE_SOURCE_PATH)/namespace-grant/cpp/NamespaceGrantWindowRequest.cpp \
		  $(MESSAGE_SOURCE_PATH)/namespace-grant/cpp/NamespaceGrantWindowResult.cpp \
		  $(MESSAGE_SOURCE_PATH)/peer/cpp/MessageFactoryPeer.cpp \
		  $(MESSAGE_SOURCE_PATH)/peer/cpp/PeerServicesGetRequest.cpp \
		  $(MESSAGE_SOURCE_PATH)/peer/cpp/PeerServicesGetResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-common/cpp/MessageFactoryPeerCommon.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-common/cpp/PeerDeleteRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-common/cpp/PeerDeleteResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-common/cpp/PeerGetRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-common/cpp/PeerGetResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-common/cpp/PeerPublishNotifyRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-common/cpp/PeerPublishNotifyResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-common/cpp/PeerPublishRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-common/cpp/PeerPublishResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-common/cpp/PeerSubscribeRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-common/cpp/PeerSubscribeResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-finder/cpp/MessageFactoryPeerFinder.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-finder/cpp/PeerLocationFindReply.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-finder/cpp/PeerLocationFindRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-finder/cpp/PeerLocationFindResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-finder/cpp/SessionCreateRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-finder/cpp/SessionCreateResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-finder/cpp/SessionDeleteRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-finder/cpp/SessionDeleteResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-finder/cpp/SessionKeepAliveRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-finder/cpp/SessionKeepAliveResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-salt/cpp/MessageFactoryPeerSalt.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-salt/cpp/SignedSaltGetRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-salt/cpp/SignedSaltGetResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-to-peer/cpp/MessageFactoryPeerToPeer.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-to-peer/cpp/PeerIdentifyRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-to-peer/cpp/PeerIdentifyResult.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-to-peer/cpp/PeerKeepAliveRequest.cpp \
		   $(MESSAGE_SOURCE_PATH)/peer-to-peer/cpp/PeerKeepAliveResult.cpp \
		  $(MESSAGE_SOURCE_PATH)/rolodex/cpp/MessageFactoryRolodex.cpp \
		  $(MESSAGE_SOURCE_PATH)/rolodex/cpp/RolodexAccessRequest.cpp \
		  $(MESSAGE_SOURCE_PATH)/rolodex/cpp/RolodexAccessResult.cpp \
		  $(MESSAGE_SOURCE_PATH)/rolodex/cpp/RolodexContactsGetRequest.cpp \
		  $(MESSAGE_SOURCE_PATH)/rolodex/cpp/RolodexContactsGetResult.cpp \
		  $(MESSAGE_SOURCE_PATH)/rolodex/cpp/RolodexNamespaceGrantChallengeValidateRequest.cpp \
		  $(MESSAGE_SOURCE_PATH)/rolodex/cpp/RolodexNamespaceGrantChallengeValidateResult.cpp

include $(BUILD_STATIC_LIBRARY)
