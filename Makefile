LIB_OUTPUT := artifacts/lib/libfastrtps.so

.PHONY : arm build

build : ${LIB_OUTPUT}

${LIB_OUTPUT} :
	mkdir -p build
	cd build && cmake -DTINYXML2_INCLUDE_DIR=../thirdparty/tinyxml2 -DTINYXML2_SOURCE_DIR=../thirdparty/tinyxml2 -DASIO_INCLUDE_DIR=../thirdparty/asio/asio/include -DCMAKE_TOOLCHAIN_FILE=../arm-gnueabi.toolchain.cmake -DCMAKE_INSTALL_PREFIX:PATH=/build/artifacts -DTHIRDPARTY=ON .. && make && make install
	sudo chown -R ${HOST_USER}:${HOST_USER} .

arm : 
	docker run -it --rm \
		-v $(shell pwd):/build \
		-e HOST_USER=$(shell id -u) \
		vincross/xcompile \
		/bin/sh -c "cd /build && make build"

clean :
	rm -frv build artifacts
