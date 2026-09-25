ifneq ($(V),1)
Q := @
endif

DEVICE_FAMILY ?= STM32F4

# Library
LIBRARY = libserial
LIB     = $(LIBRARY)_$(shell echo $(DEVICE_FAMILY) | tr A-Z a-z).a

# Directories
PROJECT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILD_DIR   = $(PROJECT_DIR)/build
OBJS_DIR    = $(BUILD_DIR)/objs
DEPS_DIR    = $(BUILD_DIR)/deps

# Library Directories
OPENCM3_DIR  ?= $(PROJECT_DIR)/lib/libopencm3
NANOCOBS_DIR ?= $(PROJECT_DIR)/lib/nanocobs

# Sources
INCS := inc $(OPENCM3_DIR)/include $(NANOCOBS_DIR)
SRCS := $(NANOCOBS_DIR)/cobs.c $(shell find $(PROJECT_DIR)/src -name "*.c")
OBJS := $(patsubst $(PROJECT_DIR)/%.c, $(OBJS_DIR)/%.o, $(SRCS))
DEPS := $(patsubst $(PROJECT_DIR)/%.c, $(DEPS_DIR)/%.d, $(SRCS))

# Toolchain
PREFIX ?= arm-none-eabi-
CC     := $(PREFIX)gcc
AR     := $(PREFIX)ar

# C Flags
DEF += -D$(DEVICE_FAMILY)
OPT += -Os
DBG += -ggdb3
STD += -std=c99

M_FLAGS += -mcpu=cortex-m4 -mthumb
ifeq ($(DEVICE_FAMILY),STM32F4) # TODO: Support more device families
    M_FLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
endif

W_FLAGS += -Wall -Wextra
F_FLAGS += -ffunction-sections -fdata-sections
I_FLAGS := $(foreach dir, $(INCS), -I$(dir))

CFLAGS += $(DEF) $(OPT) $(DBG) $(STD) $(M_FLAGS) $(W_FLAGS) $(F_FLAGS) $(I_FLAGS)

# Rules

$(BUILD_DIR)/$(LIB): $(OBJS)
	@echo "  AR      $< -> $@"
	$(Q)$(AR) rcs $@ $(OBJS)

$(OBJS): $(OBJS_DIR)/%.o: $(PROJECT_DIR)/%.c
	@echo "  CC	  $< -> $@"
	$(Q)mkdir -p $(dir $@) $(dir $(patsubst $(OBJS_DIR)/%.o, $(DEPS_DIR)/%.d, $@))
	$(Q)$(CC) $(CFLAGS) -MMD -MP -MF $(patsubst $(OBJS_DIR)/%.o, $(DEPS_DIR)/%.d, $@) -c $< -o $@

-include $(DEPS)
