# ---------------------------------------------------------------------------
# File        : hls/Makefile
# Description : Convenience targets for Vitis HLS csim / synth / cosim
# Author      : Gyusup LEE <gyu2910@waric.co.kr>
# Created     : 2026-09-03
# Copyright   : (c) 2026 Gyusup LEE. All rights reserved.
#
# Invoke from the hls/ directory. First run sources scripts/xilinx_env.sh
# to expose the Vitis HLS binary path.
# ---------------------------------------------------------------------------

HLS_DIR       := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
REPO_ROOT     := $(abspath $(HLS_DIR)/..)
XILINX_ENV    := $(REPO_ROOT)/scripts/xilinx_env.sh
BUILD_DIR     := $(HLS_DIR)/build

PROJECT_NAME  ?= gvp_csim
CSV_PATH      ?= $(REPO_ROOT)/c_model/vectors/fft_dma_vectors.csv

# Source the env script, then execute the unwrapped vitis_hls binary (see
# docs/GVP_HLS_GUIDE.md for why the direct binary is required on this box).
HLS_INVOKE    := bash -c '. $(XILINX_ENV) >/dev/null && \
                          PROJECT_NAME=$(PROJECT_NAME) \
                          CSV_PATH=$(CSV_PATH) \
                          "$$XILINX_HLS_BIN" -f $(1)'

.PHONY: all csim clean help

all: csim

csim:
	@$(call HLS_INVOKE,$(HLS_DIR)/scripts/run_csim.tcl)

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Targets:"
	@echo "  csim        Run C simulation (default target)"
	@echo "  clean       Remove hls/build/"
	@echo ""
	@echo "Overrides:"
	@echo "  PROJECT_NAME  build project name (default: gvp_csim)"
	@echo "  CSV_PATH      csv vectors (default: c_model/vectors/fft_dma_vectors.csv)"
