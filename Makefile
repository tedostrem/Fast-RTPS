.PHONY : arm build

arm :
	docker run -it --rm \
		-v $(shell pwd):/build \
		-e HOST_USER=$(shell id -u) \
		vincross/xcompile \
		/bin/sh -c "cd /build && make rtps && make cdr"

rtps :
	mkdir -p build
	cd build && cmake -DCMAKE_INSTALL_PREFIX:PATH=/build/artifacts -DTHIRDPARTY=ON .. && make && make install
	sudo chown -R ${HOST_USER}:${HOST_USER} .

cdr :
	mkdir -p thirdparty/fastcdr/build
	cd thirdparty/fastcdr/build && cmake -DCMAKE_INSTALL_PREFIX:PATH=/build/artifacts -DTHIRDPARTY=ON .. && make && make install
	sudo chown -R ${HOST_USER}:${HOST_USER} .
