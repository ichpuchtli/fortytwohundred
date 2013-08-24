SRTP_SRC_DIR=../srtp
SRTP_LIB_DIR=../srtp/lib
SRTP_LIB_ARC=${SRTP_LIB_DIR}/libsrtp.a

GTEST_SRC_DIR=gtest
GTEST_LIB_DIR=build
GTEST_GET=svn checkout http://googletest.googlecode.com/svn/trunk ${GTEST_SRC_DIR}
GTEST_WGET=wget https://googletest.googlecode.com/files/gtest-1.6.0.zip; unzip gtest-1.6.0.zip ; mv gtest-1.6.0 ${GTEST_SRC_DIR}

TEST_SOURCES=$(wildcard *_test.cpp)
TEST_LIBS += ${SRTP_LIB_ARC} ${GTEST_LIB_DIR}/libgtest_main.a ${GTEST_LIB_DIR}/libgtest.a -lssl -lcrypto 
TEST_TARGET=runtests

CXXFLAGS += -pedantic -Wall -Wextra -g -std=c++0x

all : srtp build_gtest build_tests

srtp:
	make -C ${SRTP_SRC_DIR}

build_tests: ${TEST_SOURCES}
	${CXX} ${CXXFLAGS} -pthread -I${GTEST_SRC_DIR}/include -o ${TEST_TARGET} $^ ${TEST_LIBS}

.PHONY: build_gtest
build_gtest: checkout_gtest build_gtest_libs

build_gtest_libs: mk_gtest_build_dir
	${CXX} -I ${GTEST_SRC_DIR}/include -I ${GTEST_SRC_DIR} -c ${GTEST_SRC_DIR}/src/gtest-all.cc 
	${CXX} -I ${GTEST_SRC_DIR}/include -I ${GTEST_SRC_DIR} -c ${GTEST_SRC_DIR}/src/gtest_main.cc
	ar -rv ${GTEST_LIB_DIR}/libgtest.a gtest-all.o
	ar -rv ${GTEST_LIB_DIR}/libgtest_main.a gtest_main.o
	rm -f gtest-all.o gtest_main.o

test:
	./${TEST_TARGET}

clean:
	rm -f ${TEST_LIBS} ${TEST_TARGET}

.PHONY: checkout_gtest
checkout_gtest: ${GTEST_SRC_DIR}

${GTEST_SRC_DIR}:
	${GTEST_WGET}


.PHONY: mk_gtest_build_dir
mk_gtest_build_dir: ${GTEST_LIB_DIR}

${GTEST_LIB_DIR}:
		mkdir -p ${GTEST_LIB_DIR}
