# Simple build configuration
CXX = g++
CXXFLAGS = -std=c++20 -Wall -O2

# Build the main executable
all:
	$(CXX) $(CXXFLAGS) src/main.cpp src/wal/wal_writer.cpp src/wal/wal_reader.cpp -o tickdb -lz

# Run the test suite execution binary
run: all
	./tickdb

# Clean up generated build artifacts and test wal logs
clean:
	rm -f tickdb
	rm -f test_suite_exec.wal