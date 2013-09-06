
all: build

build:
	make -C ftpclient

clean:
	make -C ftpclient clean

test:
	make -C tests test
