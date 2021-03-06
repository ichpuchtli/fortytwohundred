SRTP_SRC_DIR=../srtp
SRTP_LIB_DIR=../srtp/lib
SRTP_LIB_ARC=${SRTP_LIB_DIR}/libsrtp.a

GTEST_SRC_DIR=gtest
GTEST_LIB_DIR=build
GTEST_GET=svn checkout http://googletest.googlecode.com/svn/trunk ${GTEST_SRC_DIR}
GTEST_WGET=wget https://googletest.googlecode.com/files/gtest-1.6.0.zip; unzip gtest-1.6.0.zip ; mv gtest-1.6.0 ${GTEST_SRC_DIR}

TEST_SOURCES=$(wildcard *_test.cpp)
TEST_LIBS += ${SRTP_LIB_ARC} ${GTEST_LIB_DIR}/libgtest_main.a ${GTEST_LIB_DIR}/libgtest.a
TEST_TARGET=runtests

CXXFLAGS += -pedantic -Wall -Wextra -std=c++0x -g

all : srtp build_gtest build_tests

srtp:
	make -C ${SRTP_SRC_DIR}

build_tests: ${TEST_SOURCES}
	${CXX} ${CXXFLAGS} -pthread -I${GTEST_SRC_DIR}/include -o ${TEST_TARGET} $^ ${TEST_LIBS} -lssl -lcrypto 

.PHONY: build_gtest
build_gtest: checkout_gtest build_gtest_libs

gtest-all.o:
	${CXX} -I ${GTEST_SRC_DIR}/include -I ${GTEST_SRC_DIR} -c ${GTEST_SRC_DIR}/src/gtest-all.cc 

gtest_main.o:
	${CXX} -I ${GTEST_SRC_DIR}/include -I ${GTEST_SRC_DIR} -c ${GTEST_SRC_DIR}/src/gtest_main.cc

build_gtest_arch: gtest-all.o gtest_main.o
	ar -rv ${GTEST_LIB_DIR}/libgtest.a gtest-all.o
	ar -rv ${GTEST_LIB_DIR}/libgtest_main.a gtest_main.o

build_gtest_libs: mk_gtest_build_dir build_gtest_arch

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
