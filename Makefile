# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall
LDFLAGS =

# Main application settings
TARGET = reconstruction_aayush
SRC = src/reconstruction_aayush.cpp
OUT = $(TARGET)

# Test application settings
TEST_SRC = test/test_orderbook.cpp
TEST_OUT = test_runner
GTEST_LIBS = -lgtest -lgtest_main -pthread

# Default target: build the main application
all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(OUT) $(SRC)

# Target to build and run tests
test: $(TEST_OUT)
	./$(TEST_OUT)

$(TEST_OUT): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -o $(TEST_OUT) $(TEST_SRC) $(LDFLAGS) $(GTEST_LIBS)

# Clean up build artifacts
clean:
	rm -f $(OUT) $(TEST_OUT)

.PHONY: all test clean
