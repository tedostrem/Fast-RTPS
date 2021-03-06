DEB := fastrtps_${VERSION}_armhf.deb
ARTIFACTS_DIR := artifacts/usr
LIB_OUTPUT := ${ARTIFACTS_DIR}/lib/libfastrtps.so
FASTRTPSGEN_JAR := ${ARTIFACTS_DIR}/share/fastrtps/fastrtpsgen.jar

.PHONY : arm build clean deb

build : ${LIB_OUTPUT}

${LIB_OUTPUT} :
	mkdir -p build
	cd build && cmake -DTINYXML2_INCLUDE_DIR=../thirdparty/tinyxml2 \
		-DTINYXML2_SOURCE_DIR=../thirdparty/tinyxml2 \
		-DASIO_INCLUDE_DIR=../thirdparty/asio/asio/include \
		-DCMAKE_TOOLCHAIN_FILE=../arm-gnueabi.toolchain.cmake \
		-DCMAKE_INSTALL_PREFIX:PATH=/build/${ARTIFACTS_DIR} \
		-DTHIRDPARTY=ON .. && make && make install
	sudo chown -R ${HOST_USER}:${HOST_GROUP} .

x86 :
	mkdir -p build
	cd build && cmake \
		-DCMAKE_INSTALL_PREFIX:PATH=/build/${ARTIFACTS_DIR} \
		-DTHIRDPARTY=ON .. && make && make install

arm : ${FASTRTPSGEN_JAR} 
	docker run -it --rm \
		-v $(shell pwd):/build \
		-e VERSION=$(shell git describe --tags --long) \
		-e HOST_USER=$(shell id -u) \
		-e HOST_GROUP=$(shell id -g) \
		vincross/xcompile \
		/bin/sh -c "cd /build && make build deb"

deb : ${DEB}

${DEB} : ${LIB_OUTPUT} ${FASTRTPSGEN_JAR}
	fpm -a armhf -f -s dir -t deb --deb-no-default-config-files -C artifacts --name fastrtps --version ${VERSION} --iteration 1 --description "Fast-RTPS" .
	sudo chown -R ${HOST_USER}:${HOST_GROUP} .

${FASTRTPSGEN_JAR} : 
	mkdir -p ${ARTIFACTS_DIR}/share/fastrtps
	mkdir -p ${ARTIFACTS_DIR}/bin
	git submodule update --init --recursive
	docker run -it --rm -v $(shell pwd):/build -w /build --name gradle gradle:3.5-jdk7-alpine /bin/sh -c "cd /build/fastrtpsgen && gradle jar && cp /build/fastrtpsgen/share/fastrtps/fastrtpsgen.jar /build/${ARTIFACTS_DIR}/share/fastrtps/fastrtpsgen.jar"
	cp fastrtpsgen/scripts/fastrtpsgen ${ARTIFACTS_DIR}/bin/
	sudo chown -R $(shell id -u):$(shell id -g) .

clean :
	rm -frv build artifacts ${DEB}
