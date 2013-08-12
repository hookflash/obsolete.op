LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

MAIN_PATH := $(LOCAL_PATH)../../../../libs/udns
SOURCE_PATH := ../../../../libs/udns
INCLUDE_PATH := $(LOCAL_PATH)../../../../libs/udns


# main shared library
include $(CLEAR_VARS)
LOCAL_CFLAGS    := -DHAVE_CONFIG_H
LOCAL_MODULE    := udns
LOCAL_SRC_FILES := $(SOURCE_PATH)/udns_dn.c \
		   $(SOURCE_PATH)/udns_dntosp.c \
		   $(SOURCE_PATH)/udns_parse.c \
		   $(SOURCE_PATH)/udns_resolver.c \
		   $(SOURCE_PATH)/udns_init.c \
		   $(SOURCE_PATH)/udns_misc.c \
		   $(SOURCE_PATH)/udns_XtoX.c \
		   $(SOURCE_PATH)/udns_rr_a.c \
		   $(SOURCE_PATH)/udns_rr_ptr.c \
		   $(SOURCE_PATH)/udns_rr_mx.c \
		   $(SOURCE_PATH)/udns_rr_txt.c \
		   $(SOURCE_PATH)/udns_bl.c \
		   $(SOURCE_PATH)/udns_rr_srv.c \
		   $(SOURCE_PATH)/udns_rr_naptr.c \
		   $(SOURCE_PATH)/udns_codes.c \
		   $(SOURCE_PATH)/udns_jran.c
 
include $(BUILD_STATIC_LIBRARY)

