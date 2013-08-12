LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

MAIN_PATH := $(LOCAL_PATH)../../../../../core/openpeer/core/cpp
SOURCE_PATH := ../../../../openpeer/core/cpp
INCLUDE_PATH := $(LOCAL_PATH)../../../../../core


# main shared library
include $(CLEAR_VARS)
#LOCAL_CFLAGS := -D_ANDROID $(LOCAL_CFLAGS)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../.. \
		$(LOCAL_PATH)/../../../../libs/webrtc \
		$(LOCAL_PATH)/../../../../libs/webrtc/webrtc \
		$(LOCAL_PATH)/../../../../libs/webrtc/webrtc/voice_engine/include \
		$(LOCAL_PATH)/../../../../libs/webrtc/webrtc/video_engine/include \
		$(LOCAL_PATH)/../../../../libs/webrtc/webrtc/modules/video_capture/include \
		$(LOCAL_PATH)/../../../../libs/zsLib \
		$(LOCAL_PATH)/../../../../libs \
		$(LOCAL_PATH)/../../../../libs/boost/boost
LOCAL_MODULE    := hfcore
LOCAL_SRC_FILES := $(SOURCE_PATH)/core_Account.cpp \
		   $(SOURCE_PATH)/core_Cache.cpp \
		   $(SOURCE_PATH)/core_Call.cpp \
		   $(SOURCE_PATH)/core_CallTransport.cpp \
		   $(SOURCE_PATH)/core_Contact.cpp \
		   $(SOURCE_PATH)/core_ConversationThread.cpp \
		   $(SOURCE_PATH)/core_ConversationThreadDocumentFetcher.cpp \
		   $(SOURCE_PATH)/core_ConversationThreadHost.cpp \
		   $(SOURCE_PATH)/core_ConversationThreadSlave.cpp \
		   $(SOURCE_PATH)/core_Factory.cpp \
		   $(SOURCE_PATH)/core_Helper.cpp \
		   $(SOURCE_PATH)/core_IConversationThreadParser.cpp \
		   $(SOURCE_PATH)/core_Identity.cpp \
		   $(SOURCE_PATH)/core_IdentityLookup.cpp \
		   $(SOURCE_PATH)/core_Logger.cpp \
		   $(SOURCE_PATH)/core_Stack.cpp \
		   $(SOURCE_PATH)/core.cpp \
		   $(SOURCE_PATH)/core_MediaEngine.cpp


include $(BUILD_STATIC_LIBRARY)
