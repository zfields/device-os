APPS_MODULE_PATH ?= ../apps
include $(call rwildcard,$(APPS_MODULE_PATH)/,include.mk)
