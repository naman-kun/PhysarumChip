CC ?= clang
CFLAGS ?= -std=c11 -Wall -Wextra -O2 -Iinclude -Ithird_party -D_POSIX_C_SOURCE=200809L
LDFLAGS ?= -lm

SRC_DIR = src
INC_DIR = include
TP_DIR = third_party
TEST_DIR = tests
BIN_DIR = bin
BUILD_DIR = build

ENGINE_SRCS = $(SRC_DIR)/component.c \
              $(SRC_DIR)/architecture.c \
              $(SRC_DIR)/placement.c \
              $(SRC_DIR)/graph.c \
              $(SRC_DIR)/dijkstra.c \
              $(SRC_DIR)/physarum.c \
              $(SRC_DIR)/metrics.c \
              $(SRC_DIR)/json_io.c \
              $(TP_DIR)/cJSON.c

ENGINE_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(ENGINE_SRCS))

TARGET = $(BIN_DIR)/physarum_chip
TEST_TARGET = $(BIN_DIR)/test_runner

.PHONY: all clean test sanitize

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/$(SRC_DIR) $(BUILD_DIR)/$(TP_DIR) $(BUILD_DIR)/$(TEST_DIR) $(BIN_DIR)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(ENGINE_OBJS) $(BUILD_DIR)/$(SRC_DIR)/main.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_TARGET): $(ENGINE_OBJS) $(BUILD_DIR)/$(TEST_DIR)/test_main.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

sanitize: CFLAGS += -fsanitize=address,undefined -g
sanitize: clean $(TARGET) $(TEST_TARGET)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
