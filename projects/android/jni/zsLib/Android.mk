LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

MAIN_PATH := $(LOCAL_PATH)../../../../libs/zsLib/zsLib/cpp
SOURCE_PATH := ../../../../libs/zsLib/zsLib/cpp
INCLUDE_PATH := $(LOCAL_PATH)../../../../libs/zsLib


# main shared library
include $(CLEAR_VARS)
#LOCAL_CFLAGS := -D_ANDROID $(LOCAL_CFLAGS)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../libs/zsLib \
		    $(LOCAL_PATH)/../../../../libs/boost/boost

LOCAL_MODULE    := zslib
LOCAL_SRC_FILES := $(SOURCE_PATH)/zsLib.cpp \
		   $(SOURCE_PATH)/zsLib_Event.cpp \
		   $(SOURCE_PATH)/zsLib_helpers.cpp \
		   $(SOURCE_PATH)/zsLib_IPAddress.cpp \
		   $(SOURCE_PATH)/zsLib_Log.cpp \
		   $(SOURCE_PATH)/zsLib_MessageQueue.cpp \
		   $(SOURCE_PATH)/zsLib_MessageQueueThreadBasic.cpp \
		   $(SOURCE_PATH)/zsLib_MessageQueueThread.cpp \
		   $(SOURCE_PATH)/zsLib_MessageQueueThreadUsingCurrentGUIMessageQueueForWindows.cpp \
		   $(SOURCE_PATH)/zsLib_MessageQueueThreadUsingMainThreadMessageQueueForApple.cpp \
		   $(SOURCE_PATH)/zsLib_Numeric.cpp \
		   $(SOURCE_PATH)/zsLib_Proxy.cpp \
		  $(SOURCE_PATH)/zsLib_RegEx.cpp \
		   $(SOURCE_PATH)/zsLib_Socket.cpp \
		   $(SOURCE_PATH)/zsLib_SocketMonitor.cpp \
		   $(SOURCE_PATH)/zsLib_String.cpp \
		   $(SOURCE_PATH)/zsLib_Stringize.cpp \
		   $(SOURCE_PATH)/zsLib_Timer.cpp \
		   $(SOURCE_PATH)/zsLib_TimerMonitor.cpp \
		   $(SOURCE_PATH)/zsLib_XMLAttribute.cpp \
		   $(SOURCE_PATH)/zsLib_XMLComment.cpp \
		   $(SOURCE_PATH)/zsLib_XML.cpp \
		   $(SOURCE_PATH)/zsLib_XMLDeclaration.cpp \
		   $(SOURCE_PATH)/zsLib_XMLDocument.cpp \
		   $(SOURCE_PATH)/zsLib_XMLElement.cpp \
		   $(SOURCE_PATH)/zsLib_XMLGenerator.cpp \
		   $(SOURCE_PATH)/zsLib_XMLNode.cpp \
		   $(SOURCE_PATH)/zsLib_XMLParser.cpp \
		   $(SOURCE_PATH)/zsLib_XMLParserPos.cpp \
		   $(SOURCE_PATH)/zsLib_XMLParserWarningTypes.cpp \
		   $(SOURCE_PATH)/zsLib_XMLText.cpp \
		   $(SOURCE_PATH)/zsLib_XMLUnknown.cpp

include $(BUILD_STATIC_LIBRARY)
