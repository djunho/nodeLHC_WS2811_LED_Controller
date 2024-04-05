#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := nodeLHC_ledController
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(notdir $(patsubst %/,%,$(dir $(mkfile_path))))

IDF_PATH = $(abspath $(current_dir)/../../libs/ESP8266_RTOS_SDK)

COMPONENT_INCLUDES += $(abspath $(current_dir)/../../libs/)

EXTRA_COMPONENT_DIRS = $(abspath $(current_dir)/../libs/)
#COMPONENT_DIRS := $(filter-out $(subst ",,$(COMPONENT_DIRS)), $(abspath $(BUILD_DIR_BASE)/libs/))

include $(IDF_PATH)/make/project.mk

