all: ftclient ftserver

ftclient: ftpclient.c debug.h list.h
	gcc -pthread -o ftclient ftpclient.c -g 

ftserver: ftpserver.c debug.h list.h
	gcc -pthread -o ftserver ftpserver.c -g

clean:
	rm -f ftserver ftclient
