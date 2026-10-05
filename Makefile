ifneq ($(V),1)
Q      := @
NULL   = > .tmp 2>&1 || (cat .tmp && rm .tmp && exit 1); rm -f .tmp
SILENT = --silent
endif

.PHONY: default build flash debug test pyclean
default: flash

include config.mk
include examples/$(BOARD)/$(EXAMPLE)/build.mk

build: check-config $(EXAMPLE_BIN)

flash: check-config $(EXAMPLE_BIN)
	@echo "  FLASH   $(EXAMPLE_BIN)"
	$(Q)$(FLASH) write $(EXAMPLE_BIN) $(FLASH_LOCATION) $(NULL)
	$(Q)$(FLASH) reset $(NULL)

debug:
	@echo "  REMOTE"
	$(Q)$(REMOTE) --semihosting > /dev/null 2>&1 & REMOTE_PID=$$!; \
	echo "  GDB  $(EXAMPLE_ELF)"; \
	$(GDB) --quiet $(EXAMPLE_ELF); \
	kill $$REMOTE_PID || true

test: $(VENV)
	$(Q)$(PYTEST) -s $(TESTS_DIR)/$(EXAMPLE)_test.py

$(VENV):
	@echo "  VENV"
	$(Q)test -d $(VENV) || python$(PYTHON_VERSION) -m venv $(VENV) $(NULL)
	@echo "  PIP-UPGRADE"
	$(Q)$(PIP) install --upgrade pip $(NULL)
	@echo "  PIP-INSTALL"
	$(Q)$(PIP) install -r $(TESTS_DIR)/requirements.txt $(NULL)
	$(Q)$(PIP) install -r $(PROJECT_DIR)/requirements-dev.txt $(NULL)

pyclean:
	@./tools/pyclean.sh

mfmt:
	@./tools/make-format.sh

clean:
	rm -rf $(BUILD_DIR)
