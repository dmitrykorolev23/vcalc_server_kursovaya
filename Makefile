CXX ?= c++
CPPFLAGS ?= -Iinclude
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -O2 -g
LDFLAGS ?=
LDLIBS ?= -lcryptopp
TEST_LDLIBS ?= -lUnitTest++ -lcryptopp

TARGET := bin/vcalc_server
UNIT_TESTS := bin/unit_tests
APP_SOURCES := src/main.cpp src/config.cpp src/logger.cpp src/user_database.cpp \
               src/authenticator.cpp src/vector_calculator.cpp src/protocol.cpp \
               src/server.cpp src/client_session.cpp
TEST_SOURCES := tests/unit_tests.cpp src/config.cpp src/logger.cpp src/user_database.cpp \
                src/authenticator.cpp src/vector_calculator.cpp src/protocol.cpp src/server.cpp

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(APP_SOURCES) | bin
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(APP_SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

$(UNIT_TESTS): $(TEST_SOURCES) | bin
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TEST_SOURCES) $(LDFLAGS) $(TEST_LDLIBS) -o $@

bin:
	mkdir -p bin

test: $(UNIT_TESTS)
	./$(UNIT_TESTS)

clean:
	rm -rf bin build
