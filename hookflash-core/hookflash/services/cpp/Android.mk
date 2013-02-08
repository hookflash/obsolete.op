LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

MAIN_PATH := $(LOCAL_PATH)


# main shared library
include $(CLEAR_VARS)
#LOCAL_CFLAGS := -D_ANDROID $(LOCAL_CFLAGS)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../.. $(LOCAL_PATH)/../../../../../cmoss/tmp/build/droid/arm-linux-androideabi/include \
		$(LOCAL_PATH)/../../../../../hookflash-libs/zsLib \
		$(LOCAL_PATH)/../../../../../hookflash-libs/cryptopp \
		$(LOCAL_PATH)/../../../../../hookflash-libs/udns
LOCAL_MODULE    := hfservices
LOCAL_SRC_FILES := services_RUDPICESocketSession.cpp services_CanonicalXML.cpp services_RUDPListener.cpp services_DNS.cpp services_RUDPMessaging.cpp services_DNSMonitor.cpp \
	        services_HTTP.cpp services_RUDPPacket.cpp services_Services.cpp services_Helper.cpp services_STUNDiscovery.cpp services_ICESocket.cpp services_STUNPacket.cpp \
		services_ICESocketSession.cpp services_STUNRequester.cpp services_RUDPChannel.cpp services_STUNRequesterManager.cpp services_RUDPChannelStream.cpp services_TURNSocket.cpp \
		services_RUDPICESocket.cpp

include $(BUILD_STATIC_LIBRARY)
