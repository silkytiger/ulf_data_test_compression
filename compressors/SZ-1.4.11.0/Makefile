COMPESS_HOME=./
COMPRESSION_FLAGS = -g -fPIC

#SZ=SZ-2.0.2.1
#SZ_LIB = libSZ.a

SZ=SZ-1.4.11.0
SZ_LIB = libSZ.so

#SZ=SZ-1.4.9-beta
#SZ_LIB = libsz.a

#SZ=SZ-1.3
#SZ_LIB = libsz.a

COMPRESSION_FLAGS += -I$(COMPESS_HOME)/$(SZ)/sz/include -I$(COMPESS_HOME)/$(SZ)/sz -I/$(COMPESS_HOME)/$(SZ)/zlib
COMPRESSION_LIBS = -L$(COMPESS_HOME)/$(SZ)/sz/.libs  -l:$(SZ_LIB) -L$(COMPESS_HOME)/$(SZ)/zlib/.libs -l:libzlib.so

#===========--------- use these for SZ > SZ 2.0 --------===========
#COMPRESSION_FLAGS += -I/$(COMPESS_HOME)/$(SZ)/zstd
#COMPRESSION_LIBS += -L$(COMPESS_HOME)/$(SZ)/zstd/.libs -l:libzstd.a


all:
	gcc $(COMPRESSION_FLAGS) sz_cycle.c $(COMPRESSION_LIBS) -o sz_cycle.dll -shared




