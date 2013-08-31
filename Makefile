
all: build

build:
	make -C srtp
	make -C ftpclient
	make -C tests
	make -C doc; exit 0

clean:
	make -C srtp clean
	make -C ftpclient clean
	make -C tests clean
	make -C doc clean

test:
	make -C tests test

debug:
	@make -C srtp debug --no-print-directory

release:
	@make -C srtp release --no-print-directory
