# Project Name
TARGET = SynthV8

USE_DAISYSP_LGPL = 1

# Sources
CPP_SOURCES = SynthV8.cpp 

LDFLAGS += -u _printf_float

# Library Locations
LIBDAISY_DIR = ../lib/libDaisy
DAISYSP_DIR = ../lib/DaisySP


# Core location, and generic makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

