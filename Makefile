.PHONY : arm build fastrtpsgen


arm : fastrtpsgen
	docker run -it --rm \
		-v $(shell pwd):/build \
		-e HOST_USER=$(shell id -u) \
		vincross/xcompile \
		/bin/sh -c "cd /build && make rtps && make cdr"

rtps :
	mkdir -p build
	cd build && cmake -DCMAKE_TOOLCHAIN_FILE=../arm-gnueabi.toolchain.cmake -DCMAKE_INSTALL_PREFIX:PATH=/build/artifacts -DTHIRDPARTY=ON .. && make && make install
	sudo chown -R ${HOST_USER}:${HOST_USER} .

cdr :
	mkdir -p thirdparty/fastcdr/build
	cd thirdparty/fastcdr/build && cmake -DCMAKE_TOOLCHAIN_FILE=../arm-gnueabi.toolchain.cmake -DCMAKE_INSTALL_PREFIX:PATH=/build/artifacts -DTHIRDPARTY=ON .. && make && make install
	sudo chown -R ${HOST_USER}:${HOST_USER} .

fastrtpsgen : 
	mkdir -p artifacts
	git submodule update --init --recursive
	docker run --rm -v $(shell pwd):/build -w /build --name gradle gradle:3.5-jdk7-alpine /bin/sh -c "cd /build/fastrtpsgen && gradle jar && cp /build/fastrtpsgen/share/fastrtps/fastrtpsgen.jar /build/artifacts"
	sudo chown -R $(shell whoami):$(shell whoami) .
