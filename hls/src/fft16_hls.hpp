//------------------------------------------------------------------------------
// File        : fft16_hls.hpp
// Description : 16-point radix-2 DIT FFT primitives (Q2.14 fixed-point, HLS)
// Author      : Gyusup LEE <gyu2910@waric.co.kr>
// Created     : 2026-09-03
// Copyright   : (c) 2026 Gyusup LEE. All rights reserved.
//------------------------------------------------------------------------------

#ifndef FFT16_HLS_HPP_
#define FFT16_HLS_HPP_

#include <ap_fixed.h>
#include <ap_int.h>

constexpr int FFT_POINTS = 16;

// Q2.14 signed fixed-point sample. Range: [-2, 2 - 2^-14].
typedef ap_fixed<16, 2> sample_t;

// Q1.15 signed fixed-point twiddle. Range: [-1, 1 - 2^-15].
typedef ap_fixed<16, 1> twiddle_t;

typedef struct {
    sample_t re;
    sample_t im;
} cplx_t;

typedef struct {
    twiddle_t re;
    twiddle_t im;
} cplx_tw_t;

// Twiddle W16^k = exp(-j * 2*pi * k / 16), k = 0..7, in Q1.15.
// Raw values match c_model/fft16.c W16_FX exactly; the fraction form
// (raw / 32768.0) is exactly representable in Q1.15, so any quantization
// mode gives the same bits. +1.0 is unreachable in Q1.15, so k=0 uses
// 32767/32768 (max positive), matching the C reference.
static const cplx_tw_t W16_FX[8] = {
    { twiddle_t( 32767.0 / 32768.0), twiddle_t(     0.0 / 32768.0) },
    { twiddle_t( 30274.0 / 32768.0), twiddle_t(-12539.0 / 32768.0) },
    { twiddle_t( 23170.0 / 32768.0), twiddle_t(-23170.0 / 32768.0) },
    { twiddle_t( 12539.0 / 32768.0), twiddle_t(-30274.0 / 32768.0) },
    { twiddle_t(     0.0 / 32768.0), twiddle_t(-32767.0 / 32768.0) },
    { twiddle_t(-12539.0 / 32768.0), twiddle_t(-30274.0 / 32768.0) },
    { twiddle_t(-23170.0 / 32768.0), twiddle_t(-23170.0 / 32768.0) },
    { twiddle_t(-30274.0 / 32768.0), twiddle_t(-12539.0 / 32768.0) },
};

// Radix-2 DIT butterfly with per-stage >>1 block scaling.
// Bit-exact translation of c_model/fft16.c fft16_forward_fx:
//   Q1.15 * Q2.14 auto-widens to Q3.29 (ap_fixed<32, 3>); subtraction of two
//   Q3.29 values gives Q4.29 (ap_fixed<33, 4>). Assigning to sample_t (Q2.14)
//   uses AP_TRN + AP_WRAP by default, matching C's arithmetic right shift.
static inline void butterfly(const cplx_t &a,
                             const cplx_t &b,
                             const cplx_tw_t &w,
                             cplx_t &out_a,
                             cplx_t &out_b) {
#pragma HLS INLINE

    ap_fixed<33, 4> wb_re = w.re * b.re - w.im * b.im;
    ap_fixed<33, 4> wb_im = w.re * b.im + w.im * b.re;

    out_a.re = (a.re + wb_re) >> 1;
    out_a.im = (a.im + wb_im) >> 1;
    out_b.re = (a.re - wb_re) >> 1;
    out_b.im = (a.im - wb_im) >> 1;
}

// Bit-reversal permutation for 4-bit indices (N=16). Reads sequential input,
// writes to bit-reversed position. Matches c_model/fft16.c BIT_REV_4.
static const ap_uint<4> BIT_REV_4[FFT_POINTS] = {
    0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15
};

// 16-point radix-2 DIT forward FFT (Q2.14 in, Q2.14 out).
// Bit-exact translation of c_model/fft16.c fft16_forward_fx.
// Intended to be inlined at the caller (fft_dma_top).
static inline void fft16_hls(const cplx_t in[FFT_POINTS],
                             cplx_t out[FFT_POINTS]) {
#pragma HLS INLINE

    cplx_t stage[FFT_POINTS];
#pragma HLS ARRAY_PARTITION variable=stage complete dim=1

    for (int i = 0; i < FFT_POINTS; i++) {
#pragma HLS UNROLL
        stage[BIT_REV_4[i]] = in[i];
    }

    for (int s = 1; s <= 4; s++) {
#pragma HLS UNROLL
        const int m            = 1 << s;
        const int half_m       = m >> 1;
        const int twiddle_step = FFT_POINTS / m;

        for (int k = 0; k < FFT_POINTS; k += m) {
#pragma HLS UNROLL
            for (int j = 0; j < half_m; j++) {
#pragma HLS UNROLL
                const int idx_a = k + j;
                const int idx_b = k + j + half_m;
                cplx_t a = stage[idx_a];
                cplx_t b = stage[idx_b];
                butterfly(a, b, W16_FX[j * twiddle_step],
                          stage[idx_a], stage[idx_b]);
            }
        }
    }

    for (int i = 0; i < FFT_POINTS; i++) {
#pragma HLS UNROLL
        out[i] = stage[i];
    }
}

#endif  // FFT16_HLS_HPP_
