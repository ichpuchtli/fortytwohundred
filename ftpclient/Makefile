all: srtp ftclient ftserver

SRTP_SRC_DIR=../srtp
SRTP_LIB_DIR=../srtp/lib
SRTP_LIB_ARC=../srtp/lib/libsrtp.a

srtp:
		make -C ${SRTP_SRC_DIR}

ftclient: ftpclient.cpp debug.h
	${CXX} -pthread -o $@ ftpclient.cpp ${SRTP_LIB_ARC}

ftserver: ftpserver.cpp debug.h
	${CXX} -pthread -o $@ ftpserver.cpp ${SRTP_LIB_ARC}

clean:
	rm -f ftserver ftclient
