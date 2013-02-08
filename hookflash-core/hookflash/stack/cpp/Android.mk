LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

MAIN_PATH := $(LOCAL_PATH)


# main shared library
include $(CLEAR_VARS)
#LOCAL_CFLAGS := -D_ANDROID $(LOCAL_CFLAGS)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../.. $(LOCAL_PATH)/../../../../../cmoss/tmp/build/droid/arm-linux-androideabi/include $(LOCAL_PATH)/../../../../../hookflash-libs/zsLib $(LOCAL_PATH)/../../../../../hookflash-libs/cryptopp
LOCAL_MODULE    := hfstack
LOCAL_SRC_FILES := stack_PeerFilePrivate.cpp stack_Account.cpp stack_PeerFilePublic.cpp stack_AccountFinder.cpp stack_PeerFiles.cpp stack_AccountPeerLocation.cpp \
		   stack_PeerSubscription.cpp stack_BootstrappedNetwork.cpp stack_Publication.cpp stack_IHelper.cpp stack_PublicationMetaData.cpp \
		stack_MessageRequester.cpp stack_PublicationRepository.cpp stack_MessageRequesterManager.cpp  stack_stack.cpp stack_PeerContactProfile.cpp stack_XMLDiff.cpp \
		../message/cpp/stack_message_Message.cpp ../message/cpp/stack_message_PeerSubscribeResult.cpp \
		../message/cpp/stack_message_MessageHelper.cpp ../message/cpp/stack_message_PeerToBootstrapperFindersGetRequest.cpp \
		../message/cpp/stack_message_MessageNotify.cpp ../message/cpp/stack_message_PeerToBootstrapperFindersGetResult.cpp \
		../message/cpp/stack_message_MessageReply.cpp ../message/cpp/stack_message_PeerToBootstrapperServicesGetRequest.cpp \
		../message/cpp/stack_message_MessageRequest.cpp ../message/cpp/stack_message_PeerToBootstrapperServicesGetResult.cpp \
		../message/cpp/stack_message_MessageResult.cpp ../message/cpp/stack_message_PeerToFinderPeerLocationFindReply.cpp \
		../message/cpp/stack_message_PeerDeleteRequest.cpp ../message/cpp/stack_message_PeerToFinderPeerLocationFindRequest.cpp \
		../message/cpp/stack_message_PeerDeleteResult.cpp ../message/cpp/stack_message_PeerToFinderPeerLocationFindResult.cpp \
		../message/cpp/stack_message_PeerGetRequest.cpp ../message/cpp/stack_message_PeerToFinderSessionCreateRequest.cpp \
		../message/cpp/stack_message_PeerGetResult.cpp ../message/cpp/stack_message_PeerToFinderSessionCreateResult.cpp \
		../message/cpp/stack_message_PeerKeepAliveRequest.cpp ../message/cpp/stack_message_PeerToFinderSessionDeleteRequest.cpp \
		../message/cpp/stack_message_PeerKeepAliveResult.cpp ../message/cpp/stack_message_PeerToFinderSessionDeleteResult.cpp \
		../message/cpp/stack_message_PeerPublishNotifyRequest.cpp ../message/cpp/stack_message_PeerToPeerPeerIdentifyRequest.cpp \
		../message/cpp/stack_message_PeerPublishNotifyResult.cpp ../message/cpp/stack_message_PeerToPeerPeerIdentifyResult.cpp \
		../message/cpp/stack_message_PeerPublishRequest.cpp ../message/cpp/stack_message_PeerToSaltSignedSaltGetRequest.cpp \
		../message/cpp/stack_message_PeerPublishResult.cpp ../message/cpp/stack_message_PeerToSaltSignedSaltGetResult.cpp \
		../message/cpp/stack_message_PeerSubscribeRequest.cpp ../message/cpp/stack_message_messages.cpp

include $(BUILD_STATIC_LIBRARY)
