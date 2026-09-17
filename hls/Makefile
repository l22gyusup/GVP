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
VITIS_HLS     := $(REPO_ROOT)/scripts/vitis_hls
BUILD_DIR     := $(HLS_DIR)/build

PROJECT_NAME  ?= gvp_csim
CSV_PATH      ?= $(REPO_ROOT)/c_model/vectors/fft_dma_vectors.csv

# scripts/vitis_hls sources xilinx_env.sh and CDs into $XILINX_VITIS before
# exec'ing the unwrapped binary (needed so scripts/builtin.tcl resolves).
HLS_INVOKE    := PROJECT_NAME=$(PROJECT_NAME) CSV_PATH=$(CSV_PATH) $(VITIS_HLS) -f

.PHONY: all csim clean help

all: csim

csim:
	@mkdir -p $(BUILD_DIR)
	@$(HLS_INVOKE) $(HLS_DIR)/scripts/run_csim.tcl

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
