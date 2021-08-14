TARGET := lime

CC := gcc
CPPFLAGS := -g 
#`pkg-config --cflags --libs gtk3`
LDFLAGS :=  -lm -lpthread -lX11

BUILD_DIR := ./build
SRC_DIRS := ./src
SRCS := $(shell find ${SRC_DIRS} -name *.c)

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

$(BUILD_DIR)/${TARGET}: $(OBJS)
	$(CC) $(OBJS) -o $@  $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)


