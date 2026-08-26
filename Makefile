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
SOC_RTL     := $(CORE_RTL) rtl/soc/soc.v $(wildcard rtl/accel/*.v)
TB_DIR      := $(BUILD_DIR)/riscv-tests
SIM_DIR     := $(BUILD_DIR)/core-sim
SIM         := $(SIM_DIR)/core_sim

# --- bare-metal app runner --------------------------------------------------
RUN_DIR     := $(BUILD_DIR)/soc-run
SOC_RUN     := $(RUN_DIR)/soc_run
# Excluded, pre-committed (DESIGN.md): ma_data (misaligned, stricter than
# the ISA mandates) and fence_i (Zifencei, meaningless on split memory).
RVTEST_EXCLUDE := ma_data fence_i
RV32UI_ALL  := $(basename $(notdir $(wildcard $(ISA_DIR)/rv32ui/*.S)))
RV32UI      := $(filter-out $(RVTEST_EXCLUDE),$(RV32UI_ALL))

freeze:
	{ echo "--extra-index-url https://download.pytorch.org/whl/cpu"; $(PIP) freeze; } > requirements.txt

clean:
	rm -rf verif/build

# --- golden model -----------------------------------------------------------
GOLDEN_RUN := model/training/runs/snn_h128_k2_T20
VECTOR_N   := 10

vectors:
	$(PYTHON) model/golden/export_vectors.py --run $(GOLDEN_RUN) --n $(VECTOR_N)

check-golden:
	$(PYTHON) model/golden/check_vs_float.py --run $(GOLDEN_RUN) --n 10000
	$(PYTHON) model/golden/rate_code.py
	$(PYTHON) model/golden/golden.py --run $(GOLDEN_RUN) --check-invariance --n 1000

lint:
	@for f in $(RTL_SOURCES); do \
		echo "== $$f =="; \
		verilator --lint-only -Wall -Irtl/core -Irtl/accel $$f || exit 1; \
	done

test-unit:
	@test -n "$(BLOCK)" || (echo "usage: make test-unit BLOCK=<module_name>"; exit 1)
	$(eval RTL_FILE := $(shell find rtl -name '$(BLOCK).v'))
	$(eval TOP_MODULE := $(if $(filter core,$(BLOCK)),soc,$(BLOCK)))
	$(eval RTL_INPUTS := $(if $(filter core,$(BLOCK)),$(SOC_RTL),$(RTL_FILE)))
	@test -n "$(RTL_FILE)" || (echo "no rtl module named $(BLOCK).v under rtl/"; exit 1)
	mkdir -p $(BUILD_DIR)/$(BLOCK)
	verilator --cc --exe --build -Wall --trace \
		--top-module $(TOP_MODULE) -Irtl/core -Irtl/accel -I$(dir $(RTL_FILE)) \
		-CFLAGS "-I$(CURDIR)/verif/harness" \
		--Mdir $(BUILD_DIR)/$(BLOCK) \
		-o $(BLOCK)_tb \
		$(RTL_INPUTS) verif/unit/$(BLOCK)/$(BLOCK)_tb.cpp
	$(BUILD_DIR)/$(BLOCK)/$(BLOCK)_tb $(ARGS)

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
$(SIM): $(SOC_RTL) verif/harness/riscv_test_runner.cpp verif/harness/tb_harness.h
	@mkdir -p $(SIM_DIR)
	verilator --cc --exe --build -Wall --trace --top-module soc \
		-Irtl/core -Irtl/accel -CFLAGS "-I$(CURDIR)/verif/harness" \
		--Mdir $(SIM_DIR) -o core_sim \
		$(SOC_RTL) verif/harness/riscv_test_runner.cpp

# Verilate the SoC + the bare-metal app runner into one sim executable. Built
# once and reused; loads the program hex at runtime.
$(SOC_RUN): $(SOC_RTL) verif/harness/soc_runner.cpp verif/harness/tb_harness.h
	@mkdir -p $(RUN_DIR)
	verilator --cc --exe --build -Wall --trace --top-module soc \
		-Irtl/core -Irtl/accel -CFLAGS "-I$(CURDIR)/verif/harness" \
		--Mdir $(RUN_DIR) -o soc_run \
		$(SOC_RTL) verif/harness/soc_runner.cpp

# Run a prebuilt program hex on the SoC, e.g.
#   make hex FILE=sw/apps/hello.S OUT=verif/build/hello.hex
#   make run-hex HEX=verif/build/hello.hex
run-hex: $(SOC_RUN)
	@test -n "$(HEX)" || (echo "usage: make run-hex HEX=<program.hex> [MAX=<cycles>]"; exit 1)
	$(SOC_RUN) $(HEX) $(MAX)

# --- bare-metal apps (sw/apps + sw/bsp) -------------------------------------
SW_DIR   := $(BUILD_DIR)/sw
BSP      := sw/bsp
LDSCRIPT := $(BSP)/linker.ld
CRT0     := $(BSP)/crt0.S
# An app may be asm or C; take whichever exists.
APP_SRC   = $(firstword $(wildcard sw/apps/$(APP).S sw/apps/$(APP).c))
LIBSNN    := $(wildcard sw/libsnn/*.c)

# Build crt0 + the app against the bsp, then flatten to $readmemh hex.
# -lgcc goes last: -nostdlib omits it, but C multiply/divide lower to libgcc
# helpers (__mulsi3, __divsi3), which are plain RV32I and run on the core.
sw-build:
	@test -n "$(APP)" || (echo "usage: make sw-build APP=<name>  (sw/apps/<name>.S or .c)"; exit 1)
	@test -n "$(APP_SRC)" || (echo "no sw/apps/$(APP).S or sw/apps/$(APP).c"; exit 1)
	@mkdir -p $(SW_DIR)
	$(CC) $(ARCH_FLAGS) -O1 -nostdlib -nostartfiles -I$(BSP) \
		-T$(LDSCRIPT) $(CRT0) $(APP_SRC) $(LIBSNN) -o $(SW_DIR)/$(APP).elf -lgcc
	$(OBJCOPY) -O binary $(SW_DIR)/$(APP).elf $(SW_DIR)/$(APP).bin
	od -An -tx4 --endian=little -v $(SW_DIR)/$(APP).bin > $(SW_DIR)/$(APP).hex

# Read the ELF before running it: symbol map, section addresses, disassembly.
sw-dump: sw-build
	@echo "== symbols (address order) =="; $(NM) -n $(SW_DIR)/$(APP).elf
	@echo; echo "== sections =="; $(OBJDUMP) -h $(SW_DIR)/$(APP).elf
	@echo; echo "== disassembly =="; $(OBJDUMP) -d $(SW_DIR)/$(APP).elf

# Build and run on the SoC, e.g. `make sw-app APP=hello`.
sw-app: sw-build $(SOC_RUN)
	$(SOC_RUN) $(SW_DIR)/$(APP).hex $(MAX)

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

# --- encoder check ------------------------------------------------------------

VEC_DIR := verif/vectors/snn_h128_k2_T20
ENC_APP := encode
ENC_MAX := 5000000

test-encode:
	@$(MAKE) --no-print-directory sw-build APP=$(ENC_APP) >/dev/null
	@$(MAKE) --no-print-directory $(SOC_RUN) >/dev/null 2>&1
	@img=$$(sed -n 's/^#define IMAGE_INDEX *\([0-9]*\).*/\1/p' sw/libsnn/image.h); \
	$(SOC_RUN) $(SW_DIR)/$(ENC_APP).hex $(ENC_MAX) 2>/dev/null \
		| grep -v '^%' > $(BUILD_DIR)/enc.got; \
	grep -v '^//' $(VEC_DIR)/ev_len.hex | sed -n "$$((img*20+1)),$$((img*20+20))p" \
		| sed 's/^0*//' > $(BUILD_DIR)/enc.want; \
	if diff -u $(BUILD_DIR)/enc.want $(BUILD_DIR)/enc.got > $(BUILD_DIR)/enc.diff; then \
		echo "encoder: image $$img, 20/20 timestep counts match golden"; \
	else \
		echo "encoder: MISMATCH on image $$img  (want | got)"; \
		paste $(BUILD_DIR)/enc.want $(BUILD_DIR)/enc.got | cat -n; \
		exit 1; \
	fi

# --- full system check --------------------------------------------------------
# The C app on the core drives the accelerator over the bus; per-class spike
# totals and the argmax must match the golden model on every image. image.h
# is regenerated per image, so it is saved and put back afterwards.
SYS_APP    := mnist
SYS_MAX    := 5000000
SYS_IMAGES := 10

test-system: $(SOC_RUN)
	@mkdir -p $(BUILD_DIR)
	@cp sw/libsnn/image.h $(BUILD_DIR)/image.h.bak
	@grep -v '^//' $(VEC_DIR)/counts.hex | sed 's/^0*//;s/^$$/0/' > $(BUILD_DIR)/sys.counts
	@grep -v '^//' $(VEC_DIR)/pred.hex   | sed 's/^0*//;s/^$$/0/' > $(BUILD_DIR)/sys.pred
	@pass=0; fail=0; \
	for i in $$(seq 0 $$(($(SYS_IMAGES)-1))); do \
		python3 scripts/gen_image_h.py $$i >/dev/null; \
		if ! out=`$(MAKE) --no-print-directory sw-build APP=$(SYS_APP) 2>&1`; then \
			printf "  \033[31mFAIL\033[0m image %d: sw-build failed\n" $$i; \
			echo "$$out" | grep -m3 -i error; fail=$$((fail+1)); continue; \
		fi; \
		$(SOC_RUN) $(SW_DIR)/$(SYS_APP).hex $(SYS_MAX) 2>/dev/null \
			| grep -v '^%' > $(BUILD_DIR)/sys.got; \
		sed -n "$$((i*10+1)),$$((i*10+10))p" $(BUILD_DIR)/sys.counts > $(BUILD_DIR)/sys.want; \
		sed -n "$$((i+1))p" $(BUILD_DIR)/sys.pred >> $(BUILD_DIR)/sys.want; \
		if diff -q $(BUILD_DIR)/sys.want $(BUILD_DIR)/sys.got >/dev/null; then \
			printf "  \033[32mPASS\033[0m image %d\n" $$i; pass=$$((pass+1)); \
		else \
			printf "  \033[31mFAIL\033[0m image %d  (want | got; rows 1-10 = class counts, row 11 = argmax)\n" $$i; \
			paste $(BUILD_DIR)/sys.want $(BUILD_DIR)/sys.got | cat -n; \
			fail=$$((fail+1)); \
		fi; \
	done; \
	cp $(BUILD_DIR)/image.h.bak sw/libsnn/image.h; \
	echo "-----"; \
	echo "system: $$pass/$(SYS_IMAGES) images match golden counts and argmax"; \
	test $$fail -eq 0

# --- shortcuts ---------------------------------------------------------------
# `make u-accel`      instead of  `make test-unit BLOCK=accel`
# `make app-hello`    instead of  `make sw-app APP=hello`
# `make dump-hello`   instead of  `make sw-dump APP=hello`
u-%:
	@$(MAKE) --no-print-directory test-unit BLOCK=$*

app-%:
	@$(MAKE) --no-print-directory sw-app APP=$* MAX=$(MAX)

dump-%:
	@$(MAKE) --no-print-directory sw-dump APP=$*

# every unit suite, then the riscv-tests suite;
UNIT_BLOCKS := $(sort $(notdir $(wildcard verif/unit/*)))
check:
	@rc=0; \
	for b in $(UNIT_BLOCKS); do \
		printf '%-12s' "$$b"; \
		out=`$(MAKE) --no-print-directory test-unit BLOCK=$$b 2>&1`; st=$$?; \
		line=`echo "$$out" | grep 'checks passed' | tail -1`; \
		test -n "$$line" || line=`echo "$$out" | tail -1`; \
		echo "$$line"; \
		test $$st -eq 0 || rc=1; \
	done; \
	printf '%-12s' "lint"; \
	$(MAKE) --no-print-directory lint >/dev/null 2>&1 && echo "clean" || { echo "FAILED"; rc=1; }; \
	printf '%-12s' "riscv-tests"; \
	out=`$(MAKE) --no-print-directory test-core 2>&1`; st=$$?; \
	echo "$$out" | tail -1; \
	test $$st -eq 0 || rc=1; \
	exit $$rc

.PHONY: test-encode test-system freeze clean lint test-unit dump-asm hex run-hex sw-build sw-dump sw-app test-core test-core-one check

