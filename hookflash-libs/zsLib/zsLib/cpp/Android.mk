LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

MAIN_PATH := $(LOCAL_PATH)


# main shared library
include $(CLEAR_VARS)
#LOCAL_CFLAGS := -D_ANDROID $(LOCAL_CFLAGS)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../.. $(LOCAL_PATH)/../../../../cmoss/tmp/build/droid/arm-linux-androideabi/include
LOCAL_MODULE    := zslib
LOCAL_SRC_FILES := zsLib.cpp zsLib_Event.cpp zsLib_helpers.cpp zsLib_IPAddress.cpp zsLib_Log.cpp zsLib_MessageQueue.cpp zsLib_MessageQueueThreadBasic.cpp \
		zsLib_MessageQueueThread.cpp zsLib_MessageQueueThreadUsingCurrentGUIMessageQueueForWindows.cpp zsLib_MessageQueueThreadUsingMainThreadMessageQueueForApple.cpp \
		zsLib_Numeric.cpp zsLib_Proxy.cpp zsLib_Socket.cpp zsLib_SocketMonitor.cpp zsLib_String.cpp zsLib_Stringize.cpp zsLib_Timer.cpp zsLib_TimerMonitor.cpp \
		zsLib_XMLAttribute.cpp zsLib_XMLComment.cpp zsLib_XML.cpp zsLib_XMLDeclaration.cpp zsLib_XMLDocument.cpp zsLib_XMLElement.cpp zsLib_XMLGenerator.cpp \
		zsLib_XMLNode.cpp zsLib_XMLParser.cpp zsLib_XMLParserPos.cpp zsLib_XMLParserWarningTypes.cpp zsLib_XMLText.cpp zsLib_XMLUnknown.cpp

include $(BUILD_STATIC_LIBRARY)
