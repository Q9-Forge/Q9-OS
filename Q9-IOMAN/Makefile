CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -pedantic -O2

# In-tree Q9 toolchain. Paths default to the SDK's built-in host binaries.
Q9_SDK ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../../..)
Q9_TOOLS_DIR ?= $(Q9_SDK)/macOS/ARM64/CMDS
Q9_QCPP ?= $(Q9_TOOLS_DIR)/qcpp
Q9_QCIR ?= $(Q9_TOOLS_DIR)/qcir
Q9_QIR68K ?= $(Q9_TOOLS_DIR)/qir68k
Q9_QR68K ?= $(Q9_TOOLS_DIR)/qr68k
Q9_QL68K ?= $(Q9_TOOLS_DIR)/ql68k
Q9_RFLAGS ?=
Q9_CPPFLAGS ?= -Iinclude
Q9_C_SOURCES := src/qioman.c src/qioman_system.c src/qioman_kernel.c
Q9_TARGET_C_SOURCES := $(Q9_C_SOURCES) src/qioman_target.c
Q9_ASM := $(patsubst src/%.c,build/%.s68k,$(Q9_TARGET_C_SOURCES))
Q9_IR := $(patsubst src/%.c,build/%.ir,$(Q9_TARGET_C_SOURCES))
Q9_PREPROCESSED := $(patsubst src/%.c,build/%.i,$(Q9_TARGET_C_SOURCES))
Q9_ROF := build/qioman_entry.r $(patsubst src/%.c,build/%.r,$(Q9_TARGET_C_SOURCES))

.PHONY: all test q9-68k q9-module check-q9-tools clean
.SECONDARY: $(Q9_PREPROCESSED) $(Q9_IR) $(Q9_ASM) $(Q9_ROF)

all: test q9-module

build/test_qioman: $(Q9_C_SOURCES) tests/test_qioman.c include/qioman.h include/qioman_system.h include/qioman_kernel.h
	mkdir -p build
	$(CC) $(CFLAGS) -Iinclude $(Q9_C_SOURCES) tests/test_qioman.c -o $@

test: build/test_qioman
	./build/test_qioman

check-q9-tools:
	@test -x "$(Q9_QCPP)" || { echo "Missing Q9 preprocessor: $(Q9_QCPP)"; exit 1; }
	@test -x "$(Q9_QCIR)" || { echo "Missing Q9 C frontend: $(Q9_QCIR)"; exit 1; }
	@test -x "$(Q9_QIR68K)" || { echo "Missing Q9 68K backend: $(Q9_QIR68K)"; exit 1; }
	@test -x "$(Q9_QR68K)" || { echo "Missing Q9 assembler: $(Q9_QR68K)"; exit 1; }
	@test -x "$(Q9_QL68K)" || { echo "Missing Q9 linker: $(Q9_QL68K)"; exit 1; }

q9-68k: $(Q9_ASM)

q9-module: build/qioman

build/%.i: src/%.c include/qioman.h include/qioman_system.h | check-q9-tools
	mkdir -p build
	$(Q9_QCPP) $(Q9_CPPFLAGS) $< $@

build/%.ir: build/%.i
	$(Q9_QCIR) @$< > $@

build/%.s68k: build/%.ir
	$(Q9_QIR68K) $< $@ -os9 -part

build/%.r: build/%.s68k | check-q9-tools
	$(Q9_QR68K) $(Q9_RFLAGS) $< $@

build/qioman_entry.r: 68k/qioman_entry.a | check-q9-tools
	$(Q9_QR68K) $(Q9_RFLAGS) $< $@

build/qioman: $(Q9_ROF)
	$(Q9_QL68K) $(Q9_ROF) -O=$@ -n=ioman

clean:
	rm -f build/test_qioman $(Q9_ASM) $(Q9_IR) $(Q9_PREPROCESSED) $(Q9_ROF) build/qioman
