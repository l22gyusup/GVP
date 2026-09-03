# ---------------------------------------------------------------------------
# File        : run_csim.tcl
# Description : Vitis HLS csim runner for fft_dma_top
# Author      : Gyusup LEE <gyu2910@waric.co.kr>
# Created     : 2026-09-03
# Copyright   : (c) 2026 Gyusup LEE. All rights reserved.
#
# Usage       :
#   cd hls
#   $XILINX_HLS_BIN -f scripts/run_csim.tcl
#
# Environment overrides (before invoking):
#   PROJECT_NAME  build project name (default: gvp_csim)
#   CSV_PATH      CSV vector file (default: c_model/vectors/fft_dma_vectors.csv)
# ---------------------------------------------------------------------------

# ---- Resolve paths ----
# tcl script's own directory: hls/scripts/. Repo root is two levels up.
set script_dir [file dirname [file normalize [info script]]]
set hls_dir    [file dirname $script_dir]
set repo_root  [file dirname $hls_dir]

# Defaults (env overrides).
set project_name [expr {[info exists ::env(PROJECT_NAME)] ? $::env(PROJECT_NAME) : "gvp_csim"}]
set csv_path     [expr {[info exists ::env(CSV_PATH)]     ? $::env(CSV_PATH)
                                                          : "$repo_root/c_model/vectors/fft_dma_vectors.csv"}]

set build_dir "$hls_dir/build/$project_name"

# ---- Project setup ----
file mkdir $hls_dir/build
cd $hls_dir/build

open_project -reset $project_name
set_top fft_dma_top

# Synthesizable sources (top + primitives).
add_files "$hls_dir/src/fft_dma_top.cpp" -cflags "-std=c++14 -I$hls_dir/src"

# Testbench sources (compiled only for csim/cosim, not synthesis).
add_files -tb "$hls_dir/tb/fft_dma_tb.cpp" -cflags "-std=c++14 -I$hls_dir/src"

open_solution -reset "solution1" -flow_target vivado
set_part {xc7z020clg400-1}
create_clock -period 10 -name default

# ---- Run csim ----
csim_design -argv "$csv_path"

exit
