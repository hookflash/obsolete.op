LOCAL_PATH:= $(call my-dir)
TOP_LOCAL_PATH:= $(call my-dir)/../../../../../..

#hookflash-libs projects
#include $(LOCAL_PATH)/udns/Android.mk
#include $(LOCAL_PATH)/cryptopp/Android.mk
#include $(LOCAL_PATH)/zsLib/Android.mk

#hookflash-core projects
include $(call all-subdir-makefiles)
#include $(LOCAL_PATH)/services/Android.mk
#include $(LOCAL_PATH)/stack/Android.mk
#include $(TOP_LOCAL_PATH)/opandroid/openpeer-android-sdk/jni/Android.mk


include $(CLEAR_VARS)
LOCAL_MODULE    := openpeer
MY_STATIC_LIBRARIES := cryptopp udns zslib hfservices hfstack hfcore
LOCAL_STATIC_LIBRARIES := $(MY_STATIC_LIBRARIES)
include $(BUILD_SHARED_LIBRARY)
