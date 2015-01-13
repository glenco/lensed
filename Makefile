####
# input files
####

HEADERS = lensed.h \
          input.h \
          kernel.h \
          data.h \
          nested.h \
          quadrature.h \
          prior.h \
          parse.h \
          log.h \
          constants.h \
          input/objects.h \
          input/options.h \
          input/ini.h \
          prior/delta.h \
          prior/unif.h
SOURCES = lensed.c \
          input.c \
          kernel.c \
          data.c \
          nested.c \
          quadrature.c \
          prior.c \
          parse.c \
          log.c \
          input/objects.c \
          input/options.c \
          input/ini.c \
          prior/delta.c \
          prior/unif.c


####
# config
####

KERNEL_PATH = $(shell pwd)/kernel
KERNEL_EXT = .cl


####
# compiler and linker settings
####

# general settings
CFLAGS = -std=c99 -Wall -Werror -O3 -pedantic
CPPFLAGS = 
LDFLAGS = -lm -lcfitsio -lmultinest

# detect OS
ifndef OS
    OS = $(shell uname -s)
endif

# Linux settings
CFLAGS_Linux = 
LDFLAGS_Linux = -lOpenCL

# Mac OS X settings
CFLAGS_Darwin = 
LDFLAGS_Darwin = -framework OpenCL

# append OS dependent settings
CFLAGS += $(CFLAGS_$(OS))
LDFLAGS += $(LDFLAGS_$(OS))


####
# commands
####

MKDIR = mkdir -p
ECHO = echo


####
# terminal styles
####

ifdef TERM
    STYLE_BOLD=`tput bold`
    STYLE_DARK=`tput dim`
    STYLE_RESET=`tput sgr0`
endif


####
# build rules
####

BUILD_DIR = build
SOURCE_DIR = src
BIN_DIR = bin
CONFIG = $(BUILD_DIR)/config.h
VERSION = $(SOURCE_DIR)/version.h
OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))
LENSED = $(BIN_DIR)/lensed

.PHONY: all clean

all: $(LENSED)

clean:
	@$(ECHO) "cleaning"
	@$(RM) $(CONFIG) $(OBJECTS) $(LENSED) $(RELEASE_TOOL)

$(CONFIG): Makefile
	@$(ECHO) "updating $(STYLE_BOLD)$@$(STYLE_RESET)"
	@$(RM) $@
	@$(MKDIR) $(@D)
	@$(ECHO) "#pragma once" >> $@
	@$(ECHO) "" >> $@
	@$(ECHO) "#define KERNEL_PATH \"$(KERNEL_PATH)/\"" >> $@
	@$(ECHO) "#define KERNEL_EXT \"$(KERNEL_EXT)\"" >> $@

$(OBJECTS): $(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(CONFIG) $(VERSION)
	@$(ECHO) "building $(STYLE_BOLD)$<$(STYLE_RESET)"
	@$(MKDIR) $(@D)
	@$(CC) $(CFLAGS) $(CPPFLAGS) -I$(BUILD_DIR) -c -o $@ $<

$(LENSED): $(OBJECTS)
	@$(ECHO) "linking $(STYLE_BOLD)$@$(STYLE_RESET)"
	@$(MKDIR) $(@D)
	@$(CC) $(LDFLAGS) -o $@ $^
