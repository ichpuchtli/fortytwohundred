
# Linking example
#SRTP_LIB=srtp/lib/libsrtp.a
#test : test.c
#		${CXX} ${LDFLAGS} $^ -o $@ ${SRTP_LIB}

OUTPUT_DIR=lib

CXXFLAGS += -pedantic -Wall -Wextra -g -std=c++0x -fPIC

all : mk_out_dir ${OUTPUT_DIR}/libsrtp.a

${OUTPUT_DIR}/srtp_impl.o: srtp_impl.cpp
	${CXX} ${CXXFLAGS} -c -o $@ $<
	
${OUTPUT_DIR}/srtp_debug.o: srtp_debug.cpp
	${CXX} ${CXXFLAGS} -c -o $@ $<

${OUTPUT_DIR}/srtp.o: srtp.cpp
	${CXX} ${CXXFLAGS} -c -o $@ $<

${OUTPUT_DIR}/libsrtp.a: ${OUTPUT_DIR}/srtp.o ${OUTPUT_DIR}/srtp_impl.o ${OUTPUT_DIR}/srtp_debug.o
	ar -rv $@ $^

clean:
	rm -rf ${OUTPUT_DIR}

.PHONY: mk_out_dir
mk_out_dir: ${OUTPUT_DIR}

${OUTPUT_DIR}:
		mkdir -p ${OUTPUT_DIR}

debug:
	@sed -i 's/^\/\/#define SRTP_DEBUG/#define SRTP_DEBUG/g' srtp_debug.h
	@echo "Switched to DEBUG mode"

release:
	@sed -i 's/^#define SRTP_DEBUG/\/\/#define SRTP_DEBUG/g' srtp_debug.h
	@echo "Switched to RELEASE mode"
