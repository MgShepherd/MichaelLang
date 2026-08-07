CC := clang
CFLAGS := -Wall -Wextra -g -std=c17 -I./include
SRC_DIR := src
TEST_DIR := tests
BUILD_DIR := build

TARGET := $(BUILD_DIR)/Compiler
TEST_TARGET := $(BUILD_DIR)/CompilerTests

SRCS := $(wildcard $(SRC_DIR)/*.c)
TEST_SRCS := $(filter-out src/main.c, $(SRCS))
TEST_FILE := $(TEST_DIR)/tests.c
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TEST_OBJS := $(TEST_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o) $(TEST_FILE:$(TEST_DIR)/%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@

$(TEST_TARGET): $(TEST_OBJS)
	$(CC) $(TEST_OBJS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: $(TEST_TARGET)
	$(TEST_TARGET)

run: $(TARGET)
	$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
