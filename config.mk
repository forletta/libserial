# Project Directories
PROJECT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILD_DIR   = $(PROJECT_DIR)/build
TESTS_DIR   = $(PROJECT_DIR)/tests

# Library directories
# libserial
LIBSERIAL_DIR       := $(PROJECT_DIR)
LIBSERIAL_BUILD_DIR := $(LIBSERIAL_DIR)/build
INC_DIRS            += $(LIBSERIAL_DIR)/inc

# libopencm3
LIBOPENCM3_DIR       := $(PROJECT_DIR)/lib/libopencm3
LIBOPENCM3_BUILD_DIR := $(LIBOPENCM3_DIR)/lib
INC_DIRS             += $(LIBOPENCM3_DIR)/include

# Python
PYTHON_VERSION = 3.14
VENV           = .venv
PIP            = $(VENV)/bin/pip

# Tools
FLASH  := $(shell which st-flash)
REMOTE := $(shell which st-util)
GDB    := $(shell which arm-none-eabi-gdb)
PYTEST = $(VENV)/bin/pytest

# Required Environment Variables:
# DEFAULT_BOARD=<board> (e.g. nucleo-f401re)
# DEFAULT_EXAMPLE=<example> (e.g. echo)
#
# Optional:
# DEFAULT_COMMAND=<command> (default: flash)
# DEFAULT_BAUDRATE=<baudrat> (default: 115200)
# DEFAULT_RX_BUFFER_LEN=<rx_buffer_len> (default: 128)
include .env

BOARD             ?= $(DEFAULT_BOARD)
EXAMPLE           ?= $(DEFAULT_EXAMPLE)
EXAMPLE_BOARD_DIR := $(PROJECT_DIR)/examples/$(BOARD)
EXAMPLE_DIR       := $(EXAMPLE_BOARD_DIR)/$(EXAMPLE)

EXAMPLE_BUILD_DIR := $(PROJECT_DIR)/build/examples/$(BOARD)/$(EXAMPLE)
EXAMPLE_OBJS_DIR  := $(EXAMPLE_BUILD_DIR)/objs
EXAMPLE_DEPS_DIR  := $(EXAMPLE_BUILD_DIR)/deps

EXAMPLE_BIN := $(EXAMPLE_BUILD_DIR)/$(EXAMPLE).bin
EXAMPLE_ELF := $(EXAMPLE_BUILD_DIR)/$(EXAMPLE).elf

DEFAULT_BAUDRATE      ?= 115200
DEFAULT_PARITY        ?= USART_PARITY_NONE
DEFAULT_RX_BUFFER_LEN ?= 128

BAUDRATE      ?= $(DEFAULT_BAUDRATE)
PARITY        ?= $(DEFAULT_PARITY)
RX_BUFFER_LEN ?= $(DEFAULT_RX_BUFFER_LEN)

TRACKED_VARS   = BAUDRATE PARITY
CURRENT_CONFIG := $(foreach v,$(TRACKED_VARS),$(v)=$($(v)))

CONFIG = $(BUILD_DIR)/.example-config

.PHONY: check-config
check-config:
	@echo "$(CURRENT_CONFIG)" > .current-config.tmp
	@if ! test -f $(CONFIG) || ! cmp -s .current-config.tmp $(CONFIG); then \
		echo "  CONFIG  $(CONFIG)"; \
		mkdir -p $(dir $(CONFIG)) && mv .current-config.tmp $(CONFIG); \
	fi
	@rm -f .current-config.tmp
