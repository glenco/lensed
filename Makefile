####
# input files
####

HEADERS = lensed.h \
          input.h \
          kernel.h \
          data.h \
          nested.h \
          quadrature.h \
          log.h \
          version.h \
          constants.h \
          input/options.h \
          input/ini.h
SOURCES = lensed.c \
          input.c \
          kernel.c \
          data.c \
          nested.c \
          quadrature.c \
          log.c \
          input/options.c \
          input/ini.c


####
# config
####

KERNEL_PATH = $(shell pwd)/kernel
KERNEL_EXT = .cl


####
# compiler and linker settings
####

# general settings
CFLAGS = -std=c99 -Wall -O3 -pedantic
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
# build rules
####

.PHONY: all clean

SOURCE_DIR = src
BUILD_DIR = build
BIN_DIR = bin
CONFIG = $(BUILD_DIR)/config.h
OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))
LENSED = $(BIN_DIR)/lensed

MKDIR = mkdir -p

all: $(LENSED)

clean:
	$(RM) $(CONFIG) $(OBJECTS) $(LENSED)

$(LENSED): $(OBJECTS)
	@$(MKDIR) $(@D)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJECTS):$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(CONFIG)
	@$(MKDIR) $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -I$(BUILD_DIR) -c -o $@ $<


####
# configuration header
####

ECHO = echo

$(CONFIG): Makefile
	@$(ECHO) "generate config.h"
	@$(RM) $@
	@$(MKDIR) $(@D)
	@$(ECHO) "#pragma once" >> $@
	@$(ECHO) "" >> $@
	@$(ECHO) "#define KERNEL_PATH \"$(KERNEL_PATH)/\"" >> $@
	@$(ECHO) "#define KERNEL_EXT \"$(KERNEL_EXT)\"" >> $@
