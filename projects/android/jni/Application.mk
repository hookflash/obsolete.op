#APP_BUILD_SCRIPT := $(call my-dir)/Android.mk
APP_ABI := armeabi armeabi-v7a
#APP_OPTIM := release
APP_STL := gnustl_static
#APP_CFLAGS := -D__GLIBC__ -D__GLIBCPP__ -DBOOST_HAS_THREADS -DANDROID -fexceptions -frtti
APP_CFLAGS := -fexceptions -frtti -D__GLIBC__ -D__GLIBCPP__ -D_ANDROID -DCURL_STATICLIB

