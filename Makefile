####
# input files
####

HEADERS = lensed.h options.h kernel.h data.h nested.h quadrature.h log.h \
          inih/ini.h config.h version.h constants.h
SOURCES = lensed.c options.c kernel.c data.c nested.c quadrature.c log.c \
          inih/ini.c


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


####
# dependencies
####

MKDIR = mkdir -p
CD = cd
CURL = curl -O
UNZIP = unzip
TOUCH = touch

inih/ini.h:
	$(MKDIR) inih
	$(CD) inih && \
		$(CURL) https://inih.googlecode.com/files/inih_r29.zip && \
		$(UNZIP) inih_r29.zip
	$(TOUCH) $@
