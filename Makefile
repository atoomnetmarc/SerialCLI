CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror

build/test_serial_cli: test/test_serial_cli.cpp src/SerialCli.cpp src/SerialCli.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) -I src test/test_serial_cli.cpp src/SerialCli.cpp -o $@

.PHONY: test clean
test: build/test_serial_cli
	./build/test_serial_cli

clean:
	rm -rf build
