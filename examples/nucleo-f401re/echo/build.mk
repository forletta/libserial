EXAMPLE_SRCS := $(shell find $(EXAMPLE_DIR) -name "*.c")

include $(EXAMPLE_BOARD_DIR)/nucleo-f401re.mk
