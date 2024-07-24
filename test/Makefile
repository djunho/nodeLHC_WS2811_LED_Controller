#---------
#
# CppUTest Examples Makefile
#
#----------

#Set this to @ to keep the makefile quiet
ifndef SILENCE
	SILENCE = @
endif

#--- Inputs ----#
COMPONENT_NAME = LEDPanel
PROJECT_ROOT = ..
CPPUTEST_HOME = $(PROJECT_ROOT)/libs/CppUTest

CPPUTEST_USE_VPATH = Y
CPPUTEST_OBJS_DIR = objs
CPPUTEST_EXE_FLAGS = -v -c

CPPUTEST_USE_EXTENSIONS = Y
CPPUTEST_USE_GCOV = Y
CPP_PLATFORM = Gcc

CPPUTEST_CPPFLAGS = -DUT

SRC_DIRS =

SRC_FILES = \
	$(PROJECT_ROOT)/src/main/artnet.c \

TEST_SRC_DIRS = \
    $(PROJECT_ROOT)/test/

INCLUDE_DIRS =\
    .\
    $(CPPUTEST_HOME)/include \
    $(PROJECT_ROOT)/test/inc \
    $(PROJECT_ROOT)/test/inc/FreeRTOS \
    $(PROJECT_ROOT)/src/main/ \

include $(CPPUTEST_HOME)/build/MakefileWorker.mk

