
all : build test

build :
	make -C srtp
	make -C ftpclient
	make -C tests

test:
	make -C tests test
