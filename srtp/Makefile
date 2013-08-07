
# Linking example
#SRTP_LIB=srtp/lib/libsrtp.a
#test : test.c
#		${CXX} ${LDFLAGS} $^ -o $@ ${SRTP_LIB}

OUTPUT_DIR=lib

CXXFLAGS += -pedantic -Wall -Wextra -g -std=c++0x -fPIC

all : mk_out_dir srtp.o

srtp.o: srtp.h srtp.cpp srtp_impl.h
	${CXX} ${CXXFLAGS} -c -o ${OUTPUT_DIR}/srtp.o srtp.cpp
	ar -rv ${OUTPUT_DIR}/libsrtp.a ${OUTPUT_DIR}/srtp.o

clean:
	rm -f ${OUTPUT_DIR}/srtp.o ${OUTPUT_DIR}/libsrtp.a
	rmdir ${OUTPUT_DIR}

.PHONY: mk_out_dir
mk_out_dir: ${OUTPUT_DIR}

${OUTPUT_DIR}:
		mkdir -p ${OUTPUT_DIR}
