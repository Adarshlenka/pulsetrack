CXX      ?= g++
CXXSTD   := -std=c++17
WARN     := -Wall -Wextra -Werror -Wpedantic
OPT      := -O2
INCLUDES := -Iinclude
THREADS  := -pthread

BUILD_DIR := build

APP_SRCS := $(wildcard src/*.cpp)
APP_OBJS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(APP_SRCS))
APP_BIN  := pulsetrack

LIB_SRCS := $(filter-out src/main.cpp,$(APP_SRCS))
LIB_OBJS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(LIB_SRCS))

TEST_SRCS := tests/test_core.cpp
TEST_BIN  := test_core

.PHONY: all clean test sanitize-thread sanitize-address memcheck

all: $(APP_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXSTD) $(WARN) $(OPT) $(INCLUDES) $(THREADS) -c $< -o $@

$(APP_BIN): $(APP_OBJS)
	$(CXX) $(CXXSTD) $(WARN) $(OPT) $(THREADS) $(APP_OBJS) -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(LIB_OBJS) $(TEST_SRCS)
	$(CXX) $(CXXSTD) $(WARN) $(OPT) $(INCLUDES) $(THREADS) $(LIB_OBJS) $(TEST_SRCS) -o $@

# Rebuilds and runs the test suite instrumented with ThreadSanitizer, to
# catch data races that a normal build cannot detect on its own.
sanitize-thread:
	$(CXX) $(CXXSTD) $(WARN) -O1 -g -fsanitize=thread $(INCLUDES) $(THREADS) \
		$(LIB_SRCS) $(TEST_SRCS) -o test_core_tsan
	./test_core_tsan

# Rebuilds and runs the test suite instrumented with AddressSanitizer +
# UndefinedBehaviorSanitizer, to catch memory errors and UB.
sanitize-address:
	$(CXX) $(CXXSTD) $(WARN) -O1 -g -fsanitize=address,undefined $(INCLUDES) $(THREADS) \
		$(LIB_SRCS) $(TEST_SRCS) -o test_core_asan
	./test_core_asan

clean:
	rm -rf $(BUILD_DIR) $(APP_BIN) $(TEST_BIN) test_core_tsan test_core_asan *.log
