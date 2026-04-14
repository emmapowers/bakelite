.PHONY: all cmake build clean run-server run-client

all: build

build: build/Makefile
	$(MAKE) -C build

build/Makefile:
	cmake -B build

cmake:
	rm -rf build && cmake -B build

clean:
	rm -rf build

run-server: build
	./build/server

run-client: build
	./build/client
