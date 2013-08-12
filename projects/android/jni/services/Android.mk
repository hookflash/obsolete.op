LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

MAIN_PATH := $(LOCAL_PATH)../../../../../core/openpeer/services/cpp
SOURCE_PATH := ../../../../openpeer/services/cpp
INCLUDE_PATH := $(LOCAL_PATH)../../../../../core


# main shared library
include $(CLEAR_VARS)
#LOCAL_CFLAGS := -D_ANDROID $(LOCAL_CFLAGS)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../.. \
		$(LOCAL_PATH)/../../../../libs/zsLib \
		$(LOCAL_PATH)/../../../../libs \
		$(LOCAL_PATH)/../../../../libs/udns \
	       $(LOCAL_PATH)/../../../../libs/boost/boost \
	       $(LOCAL_PATH)/../../../../libs/curl/include
LOCAL_MODULE    := hfservices
LOCAL_SRC_FILES := $(SOURCE_PATH)/services_RUDPICESocketSession.cpp \
		   $(SOURCE_PATH)/services_CanonicalXML.cpp \
		   $(SOURCE_PATH)/services_RUDPListener.cpp \
		   $(SOURCE_PATH)/services_DNS.cpp \
		   $(SOURCE_PATH)/services_RUDPMessaging.cpp \
		   $(SOURCE_PATH)/services_DNSMonitor.cpp \
		   $(SOURCE_PATH)/services_HTTP.cpp \
		   $(SOURCE_PATH)/services_RUDPPacket.cpp \
		   $(SOURCE_PATH)/services_services.cpp \
		   $(SOURCE_PATH)/services_Helper.cpp \
		   $(SOURCE_PATH)/services_STUNDiscovery.cpp \
		   $(SOURCE_PATH)/services_ICESocket.cpp \
		   $(SOURCE_PATH)/services_STUNPacket.cpp \
		   $(SOURCE_PATH)/services_ICESocketSession.cpp \
		   $(SOURCE_PATH)/services_STUNRequester.cpp \
		   $(SOURCE_PATH)/services_RUDPChannel.cpp \
		   $(SOURCE_PATH)/services_STUNRequesterManager.cpp \
		   $(SOURCE_PATH)/services_RUDPChannelStream.cpp \
		   $(SOURCE_PATH)/services_TURNSocket.cpp \
		   $(SOURCE_PATH)/services_RUDPICESocket.cpp \
		  $(SOURCE_PATH)/services_Factory.cpp

include $(BUILD_STATIC_LIBRARY)
