EXAMPLE_SRCS := $(shell find $(EXAMPLE_DIR) -name "*.c")

DEF += -DRX_BUFFER_LEN=$(RX_BUFFER_LEN)

include $(EXAMPLE_BOARD_DIR)/nucleo-f401re.mk
