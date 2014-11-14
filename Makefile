HEADERS = lensed.h config.h inih/ini.h cubature/cubature.h
SOURCES = lensed.c config.c input.c inih/ini.c fits.c nested.c log.c \
          sersic.c sie.c cubature/hcubature.c
OBJECTS = $(SOURCES:.c=.o)

CFLAGS = -std=c99 -Wall -O3 -pedantic
LDFLAGS = -lcfitsio -lmultinest

all: lensed

lensed: $(HEADERS) $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

clean:
	$(RM) lensed $(OBJECTS)

# dependencies

MKDIR = mkdir -p
CD = cd
MV = mv
CURL = curl -O
UNZIP = unzip
UNTAR = tar xf
TOUCH = touch

inih/ini.h: inih_r29.zip
	$(MKDIR) inih
	$(CD) inih && $(UNZIP) ../inih_r29.zip
	$(TOUCH) $@

inih_r29.zip:
	$(CURL) https://inih.googlecode.com/files/inih_r29.zip

cubature/cubature.h: cubature-1.0.2.tgz
	$(UNTAR) cubature-1.0.2.tgz
	$(MV) cubature-1.0.2 cubature
	$(TOUCH) $@

cubature-1.0.2.tgz:
	$(CURL) http://ab-initio.mit.edu/cubature/cubature-1.0.2.tgz
