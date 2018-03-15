LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_POWERHAL_VARIANT),bullhead4c)

include $(CLEAR_VARS)

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MULTILIB := both
LOCAL_SHARED_LIBRARIES := liblog libcutils libdl
LOCAL_SRC_FILES := power.c metadata-parser.c utils.c list.c hint-data.c

LOCAL_SRC_FILES += power-bullhead4c.c

LOCAL_MODULE := power.bullhead
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

endif # TARGET_POWERHAL_VARIANT == bullhead4c
