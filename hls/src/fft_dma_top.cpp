//------------------------------------------------------------------------------
// File        : fft_dma_top.cpp
// Description : FFT+DMA HLS top implementation (AXI4-Lite ctrl, 2x AXI4 masters)
// Author      : Gyusup LEE <gyu2910@waric.co.kr>
// Created     : 2026-09-03
// Copyright   : (c) 2026 Gyusup LEE. All rights reserved.
//------------------------------------------------------------------------------

#include "fft_dma_top.hpp"
#include "fft16_hls.hpp"

// One 256-bit beat → 8 Q2.14 complex samples (LSB=sample 0, MSB=sample 7).
// Layout matches c_model/fft_dma.c (read_sample / write_sample) and the
// diagram in docs/GVP_DIAGRAMS.md Section 7.
static inline void unpack_beat(beat_t beat, cplx_t samples[8]) {
#pragma HLS INLINE
    for (int n = 0; n < 8; n++) {
#pragma HLS UNROLL
        samples[n].re.range() = beat.range(32 * n + 15, 32 * n);
        samples[n].im.range() = beat.range(32 * n + 31, 32 * n + 16);
    }
}

static inline beat_t pack_beat(const cplx_t samples[8]) {
#pragma HLS INLINE
    beat_t beat = 0;
    for (int n = 0; n < 8; n++) {
#pragma HLS UNROLL
        beat.range(32 * n + 15, 32 * n)      = samples[n].re.range();
        beat.range(32 * n + 31, 32 * n + 16) = samples[n].im.range();
    }
    return beat;
}

void fft_dma_top(beat_t   *rd_ptr,
                 beat_t   *wr_ptr,
                 uint32_t  num_ffts,
                 uint32_t  mode) {
#pragma HLS INTERFACE m_axi     port=rd_ptr    bundle=gmem_rd    offset=slave \
                                num_read_outstanding=16                       \
                                max_read_burst_length=16
#pragma HLS INTERFACE m_axi     port=wr_ptr    bundle=gmem_wr    offset=slave \
                                num_write_outstanding=16                      \
                                max_write_burst_length=16
#pragma HLS INTERFACE s_axilite port=rd_ptr    bundle=ctrl
#pragma HLS INTERFACE s_axilite port=wr_ptr    bundle=ctrl
#pragma HLS INTERFACE s_axilite port=num_ffts  bundle=ctrl
#pragma HLS INTERFACE s_axilite port=mode      bundle=ctrl
#pragma HLS INTERFACE s_axilite port=return    bundle=ctrl

    switch (mode) {
    case MODE_FFT:
        for (uint32_t k = 0; k < num_ffts; k++) {
#pragma HLS PIPELINE
            cplx_t in[FFT_POINTS];
#pragma HLS ARRAY_PARTITION variable=in complete dim=1
            cplx_t out[FFT_POINTS];
#pragma HLS ARRAY_PARTITION variable=out complete dim=1

            unpack_beat(rd_ptr[k * 2 + 0], &in[0]);
            unpack_beat(rd_ptr[k * 2 + 1], &in[8]);
            fft16_hls(in, out);
            wr_ptr[k * 2 + 0] = pack_beat(&out[0]);
            wr_ptr[k * 2 + 1] = pack_beat(&out[8]);
        }
        break;

    case MODE_READ_ONLY:
        // m_axi reads have externally observable side effects (AXI traffic).
        // The values are consumed by (void) casts to make the intent explicit.
        for (uint32_t k = 0; k < num_ffts; k++) {
#pragma HLS PIPELINE
            beat_t rb0 = rd_ptr[k * 2 + 0];
            beat_t rb1 = rd_ptr[k * 2 + 1];
            (void)rb0;
            (void)rb1;
        }
        break;

    case MODE_WRITE_ONLY:
        // Deterministic counter pattern: raw sample n = global sample index.
        // Matches c_model/fft_dma.c write_counter().
        for (uint32_t k = 0; k < num_ffts; k++) {
#pragma HLS PIPELINE
            uint32_t n_base = k * FFT_POINTS;
            beat_t wb0 = 0;
            beat_t wb1 = 0;
            for (int n = 0; n < 8; n++) {
#pragma HLS UNROLL
                wb0.range(32 * n + 31, 32 * n) = ap_uint<32>(n_base + n);
                wb1.range(32 * n + 31, 32 * n) = ap_uint<32>(n_base + 8 + n);
            }
            wr_ptr[k * 2 + 0] = wb0;
            wr_ptr[k * 2 + 1] = wb1;
        }
        break;
    }
}
