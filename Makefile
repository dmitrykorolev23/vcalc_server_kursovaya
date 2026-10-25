CXX ?= c++
CPPFLAGS ?= -Iinclude
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -O2 -g
LDFLAGS ?=
LDLIBS ?= -lcryptopp

TARGET := bin/vcalc_server
APP_SOURCES := src/main.cpp src/config.cpp src/logger.cpp src/user_database.cpp \
               src/authenticator.cpp src/vector_calculator.cpp src/protocol.cpp \
               src/server.cpp src/client_session.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(APP_SOURCES) | bin
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(APP_SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

bin:
	mkdir -p bin

clean:
	rm -rf bin build
