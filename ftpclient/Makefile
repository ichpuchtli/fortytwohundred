all: srtp ftpclient ftpserver

SRTP_SRC_DIR=../srtp
SRTP_LIB_DIR=../srtp/lib
SRTP_LIB_ARC=../srtp/lib/libsrtp.a

srtp:
		make -C ${SRTP_SRC_DIR}

ftpclient: ftpclient.cpp
	${CXX} -o $@ $^ ${SRTP_LIB_ARC}

ftpserver: ftpserver.cpp
	${CXX} -o $@ $^ ${SRTP_LIB_ARC}

clean:
	rm -f ftpserver ftpclient
