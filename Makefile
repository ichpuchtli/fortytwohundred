
all: build

build:
	make -C srtp
	make -C ftpclient
	make -C tests

clean:
	make -C srtp clean
	make -C ftpclient clean
	make -C tests clean


test:
	make -C tests test
