USER := $(shell whoami)

.PHONY : arm build

arm :
	docker run -it --rm \
		-v $(shell pwd):/build \
		vincross/xcompile \
		/bin/sh -c "cd /build && make build"

build :
	mkdir -p build
	cd build && cmake -DTHIRDPARTY=ON .. && make
	sudo chown -R ${USER}:${USER} .
