CC := clang
SRC_DIR := src
TEST_DIR := tests
BUILD_DIR := build

LLVM_C_FLAGS := $(shell llvm-config --cflags)
LLVM_LD_FLAGS := $(shell llvm-config --ldflags)
LLVM_LIBS := $(shell llvm-config --libs)
LLVM_SYSLIBS := $(shell llvm-config --system-libs)
CFLAGS := -Wall -Wextra -g -std=c23 -I./include $(LLVM_C_FLAGS)

LIBS := $(LLVM_LIBS) $(LLVM_SYSLIBS)
LD_FLAGS := $(LLVM_LD_FLAGS)

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
	$(CC) $(LD_FLAGS) $(OBJS) -o $@ $(LIBS)

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
