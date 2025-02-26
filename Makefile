CC := clang
CFLAGS := -O2 -std=gnu17 -fpic -Wall -Werror -pedantic
CFLAGS_DEBUG := -O0 -g -ggdb -std=gnu17 -fpic -Wall -Werror -pedantic
INCLUDE_DIRS := ./include
SRC_DIR := ./src
TEST_DIR := ./test
BUILD_DIR := ./build
BUILD_DIR_TEST := ./build/test

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
INCLUDE_FLAGS := $(addprefix -I,$(INCLUDE_DIRS))

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
ifdef DEBUG
	$(CC) $(CFLAGS_DEBUG) $(INCLUDE_FLAGS) -c $< -o $@
else
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -c $< -o $@
endif

IA_TEST := test_ia
IA_SRCS := logging.c internal_allocator.c
IA_SRCS := $(addprefix $(SRC_DIR)/,$(IA_SRCS))
IA_OBJS := $(IA_SRCS:%=$(BUILD_DIR)/%.o)
$(BUILD_DIR_TEST)/$(IA_TEST): $(IA_OBJS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_DEBUG) $(INCLUDE_FLAGS) -c $(TEST_DIR)/$(IA_TEST).c -o $(BUILD_DIR)/$(IA_TEST).c.o 
	$(CC) -o $@ $^ $(BUILD_DIR)/$(IA_TEST).c.o

.PHONY: clean test_ia
test_ia: $(BUILD_DIR_TEST)/$(IA_TEST)
clean:
	rm -rf $(BUILD_DIR)

