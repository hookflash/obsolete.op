LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

MAIN_PATH := $(LOCAL_PATH)


# main shared library
include $(CLEAR_VARS)
#LOCAL_CFLAGS := -D_ANDROID $(LOCAL_CFLAGS)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../.. $(LOCAL_PATH)/../../../../cmoss/tmp/build/droid/arm-linux-androideabi/include \
		$(LOCAL_PATH)/../../../../webRTC/webRTC/src \
		$(LOCAL_PATH)/../../../../webRTC/webRTC/src/video_engine/include \
		$(LOCAL_PATH)/../../../../webRTC/webRTC/src/voice_engine/main/interface \
		$(LOCAL_PATH)/../../../../webRTC/webRTC/src/video_engine/main/interface \
		$(LOCAL_PATH)/../../../../webRTC/webRTC/src/common_video/vplib/main/interface \
		$(LOCAL_PATH)/../../../../webRTC/webRTC/src/common_video/interface \
		$(LOCAL_PATH)/../../../../webRTC/webRTC/src/system_wrappers/interface \
		$(LOCAL_PATH)/../../../../webRTC/webRTC/src/modules/interface \
		$(LOCAL_PATH)/../../../../webRTC/webRTC/src/modules/video_render/main/source/iPhone \
		$(LOCAL_PATH)/../../../../webRTC/webRTC/src/modules/video_render/main/interface \
		$(LOCAL_PATH)/../../../../webRTC/webRTC/src/modules/video_capture/main/interface \
		$(LOCAL_PATH)/../../../../webRTC/webRTC/src/modules/video_coding/main/interface \
		$(LOCAL_PATH)/../../../../../hookflash-libs/zsLib \
		$(LOCAL_PATH)/../../../../../hookflash-libs/cryptopp \
		$(LOCAL_PATH)/../../../../../hookflash-libs/boost/boost
LOCAL_MODULE    := hfcore
LOCAL_SRC_FILES := core_ConversationThreadSlave.cpp core_Account.cpp core.cpp ore_Call.cpp core_Helper.cpp core_CallTransport.cpp core_IConversationThreadParser.cpp \
		core_Contact.cpp core_ConversationThread.cpp core_MediaEngine.cpp core_ConversationThreadDocumentFetcher.cpp core_Stack.cpp core_ConversationThreadHost.cpp core_XML.cpp \
		$(LOCAL_PATH)/../provisioning/cpp/provisioning_Account.cpp


include $(BUILD_STATIC_LIBRARY)
