all: ftclient ftserver

ftclient: ftpclient.c srtp_common.h list.h
	gcc -pthread -o ftclient ftpclient.c -g -std=gnu99

ftserver: ftpserver.c srtp_common.h list.h
	gcc -pthread -o ftserver ftpserver.c -g -std=gnu99

clean:
	rm -f ftserver ftclient
