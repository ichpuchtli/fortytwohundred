all: ftclient ftserver

ftclient: ftpclient.c debug.h list.h
	gcc -pthread -o ftclient ftpclient.c -g -std=gnu99

ftserver: ftpserver.c debug.h list.h
	gcc -pthread -o ftserver ftpserver.c -g -std=gnu99

clean:
	rm -f ftserver ftclient
