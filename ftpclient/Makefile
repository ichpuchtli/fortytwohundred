all: ftclient ftserver

ftclient: ftpclient.c srtp_common.h list.h
	gcc -o ftclient ftpclient.c -g -std=gnu99 -pedantic -Wall

ftserver: ftpserver.c srtp_common.h list.h
	gcc -o ftserver ftpserver.c -g -std=gnu99 -pedantic -Wall

clean:
	rm -f ftserver ftclient
