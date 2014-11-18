####
# input files
####

HEADERS = lensed.h config.h kernel.h data.h nested.h quadrature.h log.h \
          version.h constants.h inih/ini.h
SOURCES = lensed.c config.c kernel.c data.c nested.c quadrature.c log.c \
          inih/ini.c


####
# definitions
####

KERNEL_PATH = \"$(PWD)/kernel/\"


####
# compiler and linker settings
####

# general settings
CFLAGS = -std=c99 -Wall -O0 -pedantic -g
CPPFLAGS = -DKERNEL_PATH=$(KERNEL_PATH)
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
	$(RM) lensed $(OBJECTS)


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
