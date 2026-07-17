PYTHON := .venv/bin/python
PIP    := .venv/bin/pip
CROSS_COMPILE := riscv64-unknown-elf-
ARCH_FLAGS := -march=rv32i -mabi=ilp32

CC      := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump
NM      := $(CROSS_COMPILE)nm

RTL_SOURCES := $(shell find rtl -name '*.v')
BUILD_DIR   := verif/build

# --- riscv-tests (rv32ui) ---------------------------------------------------
ISA_DIR     := verif/riscv-tests/isa
ENV_DIR     := verif/env
MACROS_DIR  := $(ISA_DIR)/macros/scalar
CORE_RTL    := $(shell find rtl/core -name '*.v')
TB_DIR      := $(BUILD_DIR)/riscv-tests
SIM_DIR     := $(BUILD_DIR)/core-sim
SIM         := $(SIM_DIR)/core_sim
# Excluded, pre-committed (docs/DESIGN.md): ma_data (misaligned, stricter than
# the ISA mandates) and fence_i (Zifencei, meaningless on split memory).
RVTEST_EXCLUDE := ma_data fence_i
RV32UI_ALL  := $(basename $(notdir $(wildcard $(ISA_DIR)/rv32ui/*.S)))
RV32UI      := $(filter-out $(RVTEST_EXCLUDE),$(RV32UI_ALL))

freeze:
	{ echo "--extra-index-url https://download.pytorch.org/whl/cpu"; $(PIP) freeze; } > requirements.txt

clean:
	rm -rf verif/build

lint:
	@for f in $(RTL_SOURCES); do \
		echo "== $$f =="; \
		verilator --lint-only -Wall -Irtl/core $$f || exit 1; \
	done

test-unit:
	@test -n "$(BLOCK)" || (echo "usage: make test-unit BLOCK=<module_name>"; exit 1)
	$(eval RTL_FILE := $(shell find rtl -name '$(BLOCK).v'))
	@test -n "$(RTL_FILE)" || (echo "no rtl module named $(BLOCK).v under rtl/"; exit 1)
	mkdir -p $(BUILD_DIR)/$(BLOCK)
	verilator --cc --exe --build -Wall --trace \
		-I$(dir $(RTL_FILE)) \
		-CFLAGS "-I$(CURDIR)/verif/harness" \
		--Mdir $(BUILD_DIR)/$(BLOCK) \
		-o $(BLOCK)_tb \
		$(RTL_FILE) verif/unit/$(BLOCK)/$(BLOCK)_tb.cpp 
	$(BUILD_DIR)/$(BLOCK)/$(BLOCK)_tb

dump-asm:
	@test -n "$(FILE)" || (echo "usage: make dump-asm FILE=<path-to.s>"; exit 1)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(ARCH_FLAGS) -c $(FILE) -o $(BUILD_DIR)/dump.o
	$(OBJDUMP) -d $(BUILD_DIR)/dump.o

# Assembles FILE and emits one 32-bit hex word per instruction, in program order starting at address 0: the format $readmemh expects for imem's word-indexed mem array. Defaults to ./program.hex since that's the literal imem.v currently reads.

hex:
	@test -n "$(FILE)" || (echo "usage: make hex FILE=<path-to.s> [OUT=program.hex]"; exit 1)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(ARCH_FLAGS) -c $(FILE) -o $(BUILD_DIR)/hex.o
	$(OBJCOPY) -O binary $(BUILD_DIR)/hex.o $(BUILD_DIR)/hex.bin
	od -An -tx4 --endian=little -v $(BUILD_DIR)/hex.bin > $(if $(OUT),$(OUT),program.hex)

# Verilate the core + the riscv-tests runner into one sim executable. Loads the
# test hex at runtime, so it's built once and reused across all tests.
$(SIM): $(CORE_RTL) verif/harness/riscv_test_runner.cpp verif/harness/tb_harness.h
	@mkdir -p $(SIM_DIR)
	verilator --cc --exe --build -Wall --trace --top-module core \
		-Irtl/core -CFLAGS "-I$(CURDIR)/verif/harness" \
		--Mdir $(SIM_DIR) -o core_sim \
		$(CORE_RTL) verif/harness/riscv_test_runner.cpp

# Assemble+link one official rv32ui test against our minimal env (0x0 base, no
# CSRs), then flatten to $readmemh hex. Keeps the .elf so we can read tohost.
$(TB_DIR)/%.hex: $(ISA_DIR)/rv32ui/%.S $(ENV_DIR)/link.ld $(ENV_DIR)/riscv_test.h
	@mkdir -p $(TB_DIR)
	$(CC) $(ARCH_FLAGS) -nostdlib -nostartfiles \
		-I$(ENV_DIR) -I$(MACROS_DIR) \
		-T$(ENV_DIR)/link.ld $< -o $(TB_DIR)/$*.elf
	$(OBJCOPY) -O binary $(TB_DIR)/$*.elf $(TB_DIR)/$*.bin
	od -An -tx4 --endian=little -v $(TB_DIR)/$*.bin > $@

# Run a single test end to end, e.g. `make test-core-one TEST=add`.
test-core-one: $(SIM) $(TB_DIR)/$(TEST).hex
	@addr=$$($(NM) $(TB_DIR)/$(TEST).elf | awk '$$3=="tohost"{print "0x"$$1}'); \
	echo "== $(TEST) (tohost=$$addr) =="; \
	$(SIM) $(TB_DIR)/$(TEST).hex $$addr

# Run the whole rv32ui suite (minus the exclusion list) and tally.
test-core: $(SIM) $(addprefix $(TB_DIR)/,$(addsuffix .hex,$(RV32UI)))
	@pass=0; fail=0; failed=""; \
	for t in $(RV32UI); do \
		addr=$$($(NM) $(TB_DIR)/$$t.elf | awk '$$3=="tohost"{print "0x"$$1}'); \
		if $(SIM) $(TB_DIR)/$$t.hex $$addr >/dev/null 2>&1; then \
			printf "  \033[32mPASS\033[0m %s\n" "$$t"; pass=$$((pass+1)); \
		else \
			printf "  \033[31mFAIL\033[0m %s\n" "$$t"; fail=$$((fail+1)); \
			failed="$$failed $$t"; \
		fi; \
	done; \
	echo "-----"; \
	echo "rv32ui: $$pass passed, $$fail failed  (excluded:$(addprefix ,$(RVTEST_EXCLUDE)))"; \
	test -z "$$failed" || { echo "failed:$$failed"; exit 1; }

.PHONY: freeze clean lint test-unit dump-asm hex test-core test-core-one



