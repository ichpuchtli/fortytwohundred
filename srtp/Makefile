# Example makefile for linking against strp

#SRTPLIB_DIR=srtp/lib
#LDFLAGS += -Wl,-rpath,${SRTPLIB_DIR}
#LDFLAGS += -L${SRTPLIB_DIR} -lsrtp
#
#test : srtp test.c
#		${CC} ${LDFLAGS} test.c -o test

CXXFLAGS += -pedantic -Wall -Wextra -O3 -g

ifeq (${CC}, gcc)
CXXFLAGS += -std=gnu99
else
CXXFLAGS += -std=c++0x
endif

ifeq (${CC}, clang)
CXXFLAGS += -std=gnu99
endif

OUTPUT_DIR=lib
TARGET=libsrtp.so

all : directories ${TARGET}

${TARGET}: srtp.h srtp.c srtp_impl.h
		${CC} ${CXXFLAGS} -fPIC -shared -o ${OUTPUT_DIR}/$@ srtp.c

clean:
		rm -f ${OUTPUT_DIR}/$@

.PHONY: directories
directories: ${OUTPUT_DIR}

${OUTPUT_DIR}:
		mkdir -p ${OUTPUT_DIR}
