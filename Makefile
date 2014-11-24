####
# input files
####

HEADERS = lensed.h input.h kernel.h data.h nested.h quadrature.h log.h \
          input/options.h input/ini.h config.h version.h constants.h
SOURCES = lensed.c input.c kernel.c data.c nested.c quadrature.c log.c \
          input/options.c input/ini.c


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

OBJECTS = $(SOURCES:.c=.o)

all: lensed

lensed: $(HEADERS) $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

clean:
	$(RM) lensed $(OBJECTS) config.h


####
# configuration header
####

config.h: Makefile
	$(RM) config.h
	echo "#pragma once" >> config.h
	echo "" >> config.h
	echo "#define KERNEL_PATH \"$(KERNEL_PATH)/\"" >> config.h
	echo "#define KERNEL_EXT \"$(KERNEL_EXT)\"" >> config.h

