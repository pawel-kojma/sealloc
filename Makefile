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
ifdef DEBUG
	$(CC) $(CFLAGS_DEBUG) $(INCLUDE_FLAGS) -c $< -o $@
else
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -c $< -o $@
endif

objs: $(OBJS)

# Test targets

IA_TEST := test_ia_1 test_ia_2 test_ia_3 test_ia_4
IA_SRCS := internal_allocator.c
IA_SRCS := $(addprefix $(SRC_DIR)/,$(IA_SRCS))
IA_OBJS := $(IA_SRCS:%=$(BUILD_DIR)/%.o)

test_ia: $(IA_OBJS)
	mkdir -p $(BUILD_DIR_TEST)
	for test in $(IA_TEST); do \
		$(CC) $(CFLAGS_DEBUG) $(INCLUDE_FLAGS) -c $(TEST_DIR)/tests_ia/$${test}.c -o $(BUILD_DIR)/$${test}.o ; \
		$(CC) -o $(BUILD_DIR_TEST)/$${test} $^ $(BUILD_DIR)/$${test}.o ; \
	done

RUN_TEST := test_run_1
RUN_SRCS := run.c random.c
RUN_SRCS := $(addprefix $(SRC_DIR)/,$(RUN_SRCS))
RUN_OBJS := $(RUN_SRCS:%=$(BUILD_DIR)/%.o)

test_run: $(RUN_OBJS)
	mkdir -p $(BUILD_DIR_TEST)
	for test in $(RUN_TEST); do \
		$(CC) $(CFLAGS_DEBUG) $(INCLUDE_FLAGS) -c $(TEST_DIR)/tests_run/$${test}.c -o $(BUILD_DIR)/$${test}.o ; \
		$(CC) -o $(BUILD_DIR_TEST)/$${test} $^ $(BUILD_DIR)/$${test}.o ; \
	done

.PHONY: objs help clean test_ia
clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Available targets:"
	@echo "objs    - Build objects"
	@echo "test_ia - Build test for internal allocator."
	@echo "test_run - Build test for run utils."
	@echo "clean   - Clean build files."

