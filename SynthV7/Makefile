# Project Name
TARGET = SynthV7

# Sources
CPP_SOURCES = SynthV7.cpp

USE_DAISYSP_LGPL=1
LDFLAGS += -u _printf_float

# Library Locations
LIBDAISY_DIR = ../../DaisyExamples/libDaisy/
DAISYSP_DIR = ../../DaisyExamples/DaisySP/

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
