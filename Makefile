######################################################################
# Makefile for Lensed                                                #
# -------------------                                                #
# you can pass the following variables to make:                      #
#                                                                    #
#   CFITSIO_DIR                                                      #
#     path to a local CFITSIO build                                  #
#                                                                    #
#   CFITSIO_INCLUDE_DIR                                              #
#     path to `fitsio.h`                                             #
#                                                                    #
#   CFITSIO_LIB_DIR                                                  #
#     path to `libcfitsio`                                           #
#                                                                    #
#   CFITSIO_LIB                                                      #
#     CFITSIO library (e.g. `-lcfitsio`)                             #
#                                                                    #
#   MULTINEST_DIR                                                    #
#     path to a local MultiNest CMake build                          #
#                                                                    #
#   MULTINEST_INCLUDE_DIR                                            #
#     path to `multinest.h`                                          #
#                                                                    #
#   MULTINEST_LIB_DIR                                                #
#     path to the MultiNest library                                  #
#                                                                    #
#   MULTINEST_LIB                                                    #
#     MultiNest library (e.g. `-lmultinest` or `-lnest3`)            #
#                                                                    #
#   OPENCL_DIR                                                       #
#     path to the OpenCL implementation                              #
#                                                                    #
#   OPENCL_INCLUDE_DIR                                               #
#     path to the `CL/cl.h` header                                   #
#                                                                    #
#   OPENCL_LIB_DIR                                                   #
#     path to the OpenCL library                                     #
#                                                                    #
#   OPENCL_LIB                                                       #
#     OpenCL runtime library (e.g. `-lOpenCL`)                       #
#                                                                    #
#   EXTRA_LIBS                                                       #
#     additional libraries                                           #
#                                                                    #
#   DEBUG                                                            #
#     build with debug symbols and no optimisation                   #
#                                                                    #
# variables are cached in build/cache.mk                             #
######################################################################

####
# input files
####

HEADERS = lensed.h \
          input.h \
          kernel.h \
          data.h \
          nested.h \
          opencl.h \
          quadrature.h \
          prior.h \
          parse.h \
          path.h \
          profile.h \
          log.h \
          input/objects.h \
          input/options.h \
          input/ini.h \
          prior/delta.h \
          prior/unif.h \
          prior/norm.h \
          quad/point.h \
          quad/sub2.h \
          quad/sub4.h \
          quad/g3k7.h \
          quad/g5k11.h \
          quad/g7k15.h \
          quad/gm75.h
SOURCES = lensed.c \
          input.c \
          kernel.c \
          data.c \
          nested.c \
          opencl.c \
          quadrature.c \
          prior.c \
          parse.c \
          path.c \
          profile.c \
          log.c \
          input/objects.c \
          input/options.c \
          input/ini.c \
          prior/delta.c \
          prior/unif.c \
          prior/norm.c \
          quad/point.c \
          quad/sub2.c \
          quad/sub4.c \
          quad/g3k7.c \
          quad/g5k11.c \
          quad/g7k15.c \
          quad/gm75.c


####
# environment
####

# detect OS
ifndef OS
OS = $(shell uname -s)
endif


####
# compiler and linker settings
####

# include cached settings
-include build/cache.mk

# general settings
CFLAGS = -std=c99 -Wall -Werror -pedantic
LDFLAGS = 
LDLIBS = 

# debug or release settings
ifdef DEBUG
CFLAGS += -O0 -g -DLENSED_DEBUG
DEBUG_TAG = " [debug]"
else
CFLAGS += -O3
DEBUG_TAG = 
endif

# CFITSIO library
CFITSIO_LIB ?= -lcfitsio

ifdef CFITSIO_DIR
CFITSIO_INCLUDE_DIR = $(CFITSIO_DIR)
CFITSIO_LIB_DIR = $(CFITSIO_DIR)
endif
ifdef CFITSIO_INCLUDE_DIR
CFLAGS += -I$(CFITSIO_INCLUDE_DIR)
endif
ifdef CFITSIO_LIB_DIR
LDFLAGS += -L$(CFITSIO_LIB_DIR) -Wl,-rpath,$(CFITSIO_LIB_DIR)
endif
LDLIBS += $(CFITSIO_LIB)

# MultiNest library
MULTINEST_LIB ?= -lmultinest

ifdef MULTINEST_DIR
MULTINEST_INCLUDE_DIR = $(MULTINEST_DIR)/include
MULTINEST_LIB_DIR = $(MULTINEST_DIR)/lib
endif
ifdef MULTINEST_INCLUDE_DIR
CFLAGS += -I$(MULTINEST_INCLUDE_DIR)
endif
ifdef MULTINEST_LIB_DIR
LDFLAGS += -L$(MULTINEST_LIB_DIR) -Wl,-rpath,$(MULTINEST_LIB_DIR)
endif
LDLIBS += $(MULTINEST_LIB)

# system-dependent OpenCL library
OPENCL_LIB_Linux = -lOpenCL
OPENCL_LIB_Darwin = -framework OpenCL
OPENCL_LIB ?= $(OPENCL_LIB_$(OS))

ifdef OPENCL_DIR
OPENCL_INCLUDE_DIR = $(OPENCL_DIR)/include
OPENCL_LIB_DIR = $(OPENCL_DIR)/lib
endif
ifdef OPENCL_INCLUDE_DIR
CFLAGS += -I$(OPENCL_INCLUDE_DIR)
endif
ifdef OPENCL_LIB_DIR
LDFLAGS += -L$(OPENCL_LIB_DIR) -Wl,-rpath,$(OPENCL_LIB_DIR)
endif
LDLIBS += $(OPENCL_LIB)

# append extra libraries
LDLIBS += $(EXTRA_LIBS) -lm


####
# commands
####

MKDIR = mkdir -p
ECHO = echo
CAT = cat
CD = cd
CP = cp
TAR_Linux = tar
TAR_Darwin = COPYFILE_DISABLE=1 tar
TAR = $(TAR_$(OS))


####
# terminal styles
####

ifdef TERM
STYLE_BOLD  = $(shell tput bold)
STYLE_DARK  = $(shell tput dim)
STYLE_RESET = $(shell tput sgr0)
endif


####
# build rules
####

BUILD_DIR  = build
SOURCE_DIR = src
BIN_DIR = bin
CACHE = $(BUILD_DIR)/cache.mk
VERSION = $(SOURCE_DIR)/version.h
OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))
LENSED = $(BIN_DIR)/lensed

.PHONY: all test clean distclean

all: $(LENSED)

test:
	@$(ECHO) "testing"
	@$(MAKE) -C tests

clean:
	@$(ECHO) "cleaning"
	@$(RM) $(OBJECTS) $(LENSED)

distclean: clean
	@$(RM) -r $(BUILD_DIR) $(BIN_DIR)

$(OBJECTS): $(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(VERSION)
	@$(ECHO) "building $(STYLE_BOLD)$<$(STYLE_RESET)$(DEBUG_TAG)"
	@$(MKDIR) $(@D)
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(LENSED): $(OBJECTS)
	@$(ECHO) "linking $(STYLE_BOLD)$@$(STYLE_RESET)"
	@$(MKDIR) $(@D)
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)


####
# cache for make
####

.PHONY: cache show-cache

cache:
	@$(MKDIR) $(BUILD_DIR)
	@$(ECHO) "# cached settings for make" > $(CACHE)
	@$(ECHO) "CFITSIO_INCLUDE_DIR = $(CFITSIO_INCLUDE_DIR)" >> $(CACHE)
	@$(ECHO) "CFITSIO_LIB_DIR = $(CFITSIO_LIB_DIR)" >> $(CACHE)
	@$(ECHO) "CFITSIO_LIB = $(CFITSIO_LIB)" >> $(CACHE)
	@$(ECHO) "MULTINEST_INCLUDE_DIR = $(MULTINEST_INCLUDE_DIR)" >> $(CACHE)
	@$(ECHO) "MULTINEST_LIB_DIR = $(MULTINEST_LIB_DIR)" >> $(CACHE)
	@$(ECHO) "MULTINEST_LIB = $(MULTINEST_LIB)" >> $(CACHE)
	@$(ECHO) "OPENCL_INCLUDE_DIR = $(OPENCL_INCLUDE_DIR)" >> $(CACHE)
	@$(ECHO) "OPENCL_LIB_DIR = $(OPENCL_LIB_DIR)" >> $(CACHE)
	@$(ECHO) "OPENCL_LIB = $(OPENCL_LIB)" >> $(CACHE)
	@$(ECHO) "EXTRA_LIBS = $(EXTRA_LIBS)" >> $(CACHE)
	@$(ECHO) "DEBUG = $(DEBUG)" >> $(CACHE)

show-cache:
	@$(CAT) $(CACHE)

$(CACHE): cache


####
# deploy rules
####

# files to include in release
RELEASE_FILES = \
    $(LENSED) \
    README.md LICENSE.txt CHANGELOG.md \
    $(wildcard docs/*.md) docs/lensed.js docs/lensed.css \
    $(wildcard docs/figures/*.png) \
    $(wildcard examples/*.ini) $(wildcard examples/*.fits) \
    examples/chains/chains.txt \
    $(wildcard extras/*.*) \
    $(wildcard kernel/*.cl) $(wildcard objects/*.cl)

# release version from git
ifndef RELEASE_VERSION
RELEASE_VERSION = release
endif

# release tag
ifndef RELEASE_TAG
RELEASE_TAG_Linux = linux
RELEASE_TAG_Darwin = osx
RELEASE_TAG = $(RELEASE_TAG_$(OS))
endif

# name of release
RELEASE_NAME = lensed-$(RELEASE_VERSION)

# release product
RELEASE = $(BUILD_DIR)/$(RELEASE_NAME).$(RELEASE_TAG).tar.gz

.PHONY: release $(RELEASE)

release: $(RELEASE)

$(RELEASE): $(RELEASE_FILES:%=$(BUILD_DIR)/$(RELEASE_NAME)/%)
	@$(RM) $@
	@$(ECHO) "release $(STYLE_BOLD)$@$(STYLE_RESET)"
	@$(CD) $(@D) && $(TAR) -czf $(@F) $(RELEASE_NAME)
	@$(RM) -r $(BUILD_DIR)/$(RELEASE_NAME)

$(BUILD_DIR)/$(RELEASE_NAME)/%: %
	@$(MKDIR) $(@D)
	@$(CP) -a $< $@
