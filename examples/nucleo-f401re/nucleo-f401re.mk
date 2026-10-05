EXAMPLE_OBJS := $(patsubst $(EXAMPLE_DIR)/%.c, $(EXAMPLE_OBJS_DIR)/%.o, $(EXAMPLE_SRCS))
EXAMPLE_DEPS := $(patsubst $(EXAMPLE_DIR)/%.c, $(EXAMPLE_DEPS_DIR)/%.d, $(EXAMPLE_SRCS))

DEVICE_FAMILY  = STM32F4
FLASH_LOCATION = 0x8000000

# Library Defines
# libserial
LIBSERIAL_LIB      := serial_$(shell echo $(DEVICE_FAMILY) | tr A-Z a-z)
LIBSERIAL_LIB_FILE := $(LIBSERIAL_BUILD_DIR)/lib$(LIBSERIAL_LIB).a

# libopencm3
LIBOPENCM3_LIB      := opencm3_$(shell echo $(DEVICE_FAMILY) | tr A-Z a-z)
LIBOPENCM3_LIB_FILE := $(LIBOPENCM3_BUILD_DIR)/lib$(LIBOPENCM3_LIB).a

# Linker Files
LINKER_SCRIPT := $(EXAMPLE_BOARD_DIR)/stm32f401re.ld
LINKER_DIRS   := $(LIBSERIAL_BUILD_DIR) $(LIBOPENCM3_BUILD_DIR)
LINKER_LIBS   := $(LIBSERIAL_LIB) $(LIBOPENCM3_LIB)

# Toolchain
PREFIX  ?= arm-none-eabi-
CC      := $(PREFIX)gcc
LD      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy

# C Flags
DEF += -D$(DEVICE_FAMILY) -DBAUDRATE=$(BAUDRATE) -DPARITY=$(PARITY)
OPT += -Os
DBG += -ggdb3
STD += -std=c99

M_FLAGS += -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
W_FLAGS += -Wall -Wextra
F_FLAGS += -ffunction-sections -fdata-sections
I_FLAGS += $(foreach dir, $(INC_DIRS), -I$(dir))

CFLAGS += $(DEF) $(OPT) $(DBG) $(STD) $(M_FLAGS) $(W_FLAGS) $(F_FLAGS) $(I_FLAGS)

# Linker Flags
LDFLAGS += $(foreach dir, $(LINKER_DIRS), -L$(dir))
LDFLAGS += -T$(LINKER_SCRIPT)
LDFLAGS += --specs=nosys.specs -nostartfiles -Wl,--gc-sections
LDFLAGS += $(M_FLAGS)
LDLIBS  += $(foreach dir, $(LINKER_LIBS), -l$(dir))

$(EXAMPLE_BIN): $(EXAMPLE_ELF) $(CONFIG)
	@echo "  OBJCOPY $< -> $@"
	$(Q)$(OBJCOPY) -Obinary $< $@

$(EXAMPLE_ELF): $(EXAMPLE_OBJS) $(LIBSERIAL_LIB_FILE) $(LIBOPENCM3_LIB_FILE) $(CONFIG)
	@echo "  LD	  $@"
	$(Q)$(LD) $(LDFLAGS) $(EXAMPLE_OBJS) $(LDLIBS) $(LDLIBS) -o $@

$(EXAMPLE_OBJS): $(EXAMPLE_OBJS_DIR)/%.o: $(EXAMPLE_DIR)/%.c $(CONFIG)
	@echo "  CC	  $< -> $@"
	$(Q)mkdir -p $(dir $@) $(dir $(patsubst $(EXAMPLE_OBJS_DIR)/%.o, $(EXAMPLE_DEPS_DIR)/%.d, $@))
	$(Q)$(CC) $(CFLAGS) -MMD -MP -MF $(patsubst $(EXAMPLE_OBJS_DIR)/%.o, $(EXAMPLE_DEPS_DIR)/%.d, $@) -c $< -o $@

-include $(EXAMPLES_DEPS)

$(LIBSERIAL_LIB_FILE): FORCE
	$(Q)$(MAKE) $(SILENT) -C $(PROJECT_DIR) -f build.mk

FORCE:
