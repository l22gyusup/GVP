//------------------------------------------------------------------------------
// File        : fft_dma_top.hpp
// Description : Public interface for the FFT+DMA HLS top function
// Author      : Gyusup LEE <gyu2910@waric.co.kr>
// Created     : 2026-09-03
// Copyright   : (c) 2026 Gyusup LEE. All rights reserved.
//------------------------------------------------------------------------------

#ifndef FFT_DMA_TOP_HPP_
#define FFT_DMA_TOP_HPP_

#include <ap_int.h>
#include <stdint.h>

// AXI data-bus width (spec: DATA_WIDTH = 256). One beat = 32 bytes = 8 samples.
constexpr int BEAT_WIDTH = 256;
typedef ap_uint<BEAT_WIDTH> beat_t;

// MODE register encoding. Values match c_model/fft_dma.h.
typedef enum {
    MODE_FFT        = 0,
    MODE_READ_ONLY  = 1,
    MODE_WRITE_ONLY = 2
} fft_dma_mode_t;

// Top function. Address / control regs are exposed through an AXI4-Lite
// slave (bundle=ctrl); data ports through two AXI4 masters (bundle=gmem_rd,
// gmem_wr). See fft_dma_top.cpp for the full interface pragma set.
void fft_dma_top(beat_t   *rd_ptr,
                 beat_t   *wr_ptr,
                 uint32_t  num_ffts,
                 uint32_t  mode);

#endif  // FFT_DMA_TOP_HPP_
