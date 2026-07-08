PYTHON := .venv/bin/python
PIP    := .venv/bin/pip
CROSS_COMPILE := riscv64-unknown-elf-
ARCH_FLAGS := -march=rv32i -mabi=ilp32

CC      := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

RTL_SOURCES := $(shell find rtl -name '*.v')
BUILD_DIR   := verif/build

freeze:
	{ echo "--extra-index-url https://download.pytorch.org/whl/cpu"; $(PIP) freeze; } > requirements.txt

clean:
	rm -rf verif/build

lint:
	@for f in $(RTL_SOURCES); do \
		echo "== $$f =="; \
		verilator --lint-only -Wall $$f || exit 1; \
	done

test-unit:
	@test -n "$(BLOCK)" || (echo "usage: make test-unit BLOCK=<module_name>"; exit 1)
	$(eval RTL_FILE := $(shell find rtl -name '$(BLOCK).v'))
	@test -n "$(RTL_FILE)" || (echo "no rtl module named $(BLOCK).v under rtl/"; exit 1)
	mkdir -p $(BUILD_DIR)/$(BLOCK)
	verilator --cc --exe --build -Wall --trace \
		-CFLAGS "-I$(CURDIR)/verif/harness" \
		--Mdir $(BUILD_DIR)/$(BLOCK) \
		-o $(BLOCK)_tb \
		$(RTL_FILE) verif/unit/$(BLOCK)/$(BLOCK)_tb.cpp 
	$(BUILD_DIR)/$(BLOCK)/$(BLOCK)_tb

.PHONY: freeze clean lint test-unit



