CC := clang
CFLAGS := -O2 -std=gnu17 -fpic -Wall -Werror -pedantic
CFLAGS_DEBUG := -O0 -g -ggdb -DLOGGING -std=gnu17 -fpic -Wall -Werror -pedantic -Wno-format-pedantic
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
ifdef LOGGING
	$(CC) $(CFLAGS_DEBUG) $(INCLUDE_FLAGS) -c $< -o $@
else
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -c $< -o $@
endif

# Test targets

IA_TEST := test_ia_1 test_ia_2
IA_SRCS := internal_allocator.c
IA_SRCS := $(addprefix $(SRC_DIR)/,$(IA_SRCS))
IA_OBJS := $(IA_SRCS:%=$(BUILD_DIR)/%.o)

.PHONY: help clean test_ia
test_ia: $(IA_OBJS)
	mkdir -p $(BUILD_DIR_TEST)
	for test in $(IA_TEST); do \
		$(CC) $(CFLAGS_DEBUG) $(INCLUDE_FLAGS) -c $(TEST_DIR)/$${test}.c -o $(BUILD_DIR)/$${test}.o ; \
		$(CC) -o $(BUILD_DIR_TEST)/$${test} $^ $(BUILD_DIR)/$${test}.o ; \
	done

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Available targets:"
	@echo "test_ia - Build test for internal allocator."
	@echo "clean   - Clean build files."

