TARGET := renderer

SRCS_DIR := src
BUILD_DIR := build
SRCS := $(wildcard $(SRCS_DIR)/*/*.c $(SRCS_DIR)/*.c)
HDRS := $(wildcard $(SRCS_DIR)/*/*.h $(SRCS_DIR)/*.h)
OBJS := $(patsubst $(SRCS_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

CC := cc
CFLAGS := -Wall -Wextra -ansi -pedantic -lm -lSDL3

$(BUILD_DIR)/$(TARGET): $(OBJS) makefile
	$(CC) -o $@ $(OBJS) $(CFLAGS)

$(BUILD_DIR)/%.o: $(SRCS_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BUILD_DIR)/$(TARGET)
	./$(BUILD_DIR)/$(TARGET)

clean:
	$(RM) -r $(BUILD_DIR)

.PHONY: run clean
