default:
	mkdir -p build && cd build && cmake -D STATIC=ON -D ARCH="x86-64" -D BUILD_64=ON -D CMAKE_BUILD_TYPE=Release .. && $(MAKE) VERBOSE=1
debug:
	mkdir -p build && cd build && ccmake .. && make

clean:
	mkdir -p build && cd build && rm -rf *
