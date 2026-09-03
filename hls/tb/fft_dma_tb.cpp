//------------------------------------------------------------------------------
// File        : fft_dma_tb.cpp
// Description : csim testbench for fft_dma_top; reuses c_model CSV vectors
// Author      : Gyusup LEE <gyu2910@waric.co.kr>
// Created     : 2026-09-03
// Copyright   : (c) 2026 Gyusup LEE. All rights reserved.
//------------------------------------------------------------------------------

#include "fft_dma_top.hpp"

#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

static constexpr int      SAMPLE_BYTES    = 4;
static constexpr int      BEAT_BYTES      = BEAT_WIDTH / 8;
static constexpr int      MEMORY_BYTES    = 128 * 1024;
static constexpr int      MEMORY_BEATS    = MEMORY_BYTES / BEAT_BYTES;
static constexpr int      FIXED_TOL       = 96;
static constexpr int      Q214_SCALE_INT  = 16384;
static constexpr int      FFT_POINTS_INT  = 16;
static constexpr int      LINE_MAX_LEN    = 512;
static constexpr int      NAME_MAX_LEN    = 64;
static constexpr const char *DEFAULT_CSV  = "vectors/fft_dma_vectors.csv";

struct case_t {
    int                id;
    std::string        name;
    fft_dma_mode_t     mode;
    uint32_t           num_ffts;
    uint64_t           src_addr;
    uint64_t           dst_addr;
    size_t             n_samples;
    std::vector<float> in_re;
    std::vector<float> in_im;
    std::vector<float> exp_re;
    std::vector<float> exp_im;
};

static int16_t float_to_q214(double v) {
    double scaled  = v * Q214_SCALE_INT;
    double rounded = scaled >= 0.0 ? scaled + 0.5 : scaled - 0.5;
    long   r       = (long)rounded;
    if (r >  32767) return  32767;
    if (r < -32768) return -32768;
    return (int16_t)r;
}

static void mem_write_byte(beat_t *mem, uint64_t byte_addr, uint8_t v) {
    uint64_t beat_idx = byte_addr / BEAT_BYTES;
    unsigned byte_off = byte_addr % BEAT_BYTES;
    mem[beat_idx].range(8 * byte_off + 7, 8 * byte_off) = v;
}

static uint8_t mem_read_byte(const beat_t *mem, uint64_t byte_addr) {
    uint64_t beat_idx = byte_addr / BEAT_BYTES;
    unsigned byte_off = byte_addr % BEAT_BYTES;
    return (uint8_t)mem[beat_idx].range(8 * byte_off + 7, 8 * byte_off);
}

static void mem_write_sample(beat_t *mem, uint64_t byte_addr,
                             int16_t re, int16_t im) {
    uint16_t re_u = (uint16_t)re;
    uint16_t im_u = (uint16_t)im;
    mem_write_byte(mem, byte_addr + 0, (uint8_t)(re_u & 0xFF));
    mem_write_byte(mem, byte_addr + 1, (uint8_t)((re_u >> 8) & 0xFF));
    mem_write_byte(mem, byte_addr + 2, (uint8_t)(im_u & 0xFF));
    mem_write_byte(mem, byte_addr + 3, (uint8_t)((im_u >> 8) & 0xFF));
}

static void mem_read_sample(const beat_t *mem, uint64_t byte_addr,
                            int16_t *re, int16_t *im) {
    uint16_t re_u = (uint16_t)mem_read_byte(mem, byte_addr + 0)
                  | ((uint16_t)mem_read_byte(mem, byte_addr + 1) << 8);
    uint16_t im_u = (uint16_t)mem_read_byte(mem, byte_addr + 2)
                  | ((uint16_t)mem_read_byte(mem, byte_addr + 3) << 8);
    *re = (int16_t)re_u;
    *im = (int16_t)im_u;
}

static int load_cases(const char *path, std::vector<case_t> &cases) {
    FILE *fp = std::fopen(path, "r");
    if (!fp) {
        std::fprintf(stderr, "load_cases: cannot open '%s'\n", path);
        return -1;
    }

    char line[LINE_MAX_LEN];
    if (!std::fgets(line, sizeof(line), fp)) {
        std::fclose(fp);
        return -1;
    }

    while (std::fgets(line, sizeof(line), fp)) {
        int      id, mode, sample_idx;
        char     name[NAME_MAX_LEN];
        uint32_t num_ffts;
        uint64_t src_addr, dst_addr;
        float    in_re, in_im, exp_re, exp_im;

        int n = std::sscanf(line,
                            "%d,%63[^,],%d,%" SCNu32 ",%" SCNu64 ",%" SCNu64
                            ",%d,%f,%f,%f,%f",
                            &id, name, &mode, &num_ffts,
                            &src_addr, &dst_addr, &sample_idx,
                            &in_re, &in_im, &exp_re, &exp_im);
        if (n != 11) {
            std::fprintf(stderr, "load_cases: malformed row: %s", line);
            std::fclose(fp);
            return -1;
        }

        case_t *c = nullptr;
        for (auto &existing : cases) {
            if (existing.id == id) {
                c = &existing;
                break;
            }
        }
        if (!c) {
            case_t fresh;
            fresh.id        = id;
            fresh.name      = name;
            fresh.mode      = (fft_dma_mode_t)mode;
            fresh.num_ffts  = num_ffts;
            fresh.src_addr  = src_addr;
            fresh.dst_addr  = dst_addr;
            fresh.n_samples = (size_t)FFT_POINTS_INT * num_ffts;
            fresh.in_re.assign(fresh.n_samples, 0.0f);
            fresh.in_im.assign(fresh.n_samples, 0.0f);
            fresh.exp_re.assign(fresh.n_samples, 0.0f);
            fresh.exp_im.assign(fresh.n_samples, 0.0f);
            cases.push_back(std::move(fresh));
            c = &cases.back();
        }
        if ((size_t)sample_idx >= c->n_samples) {
            std::fprintf(stderr,
                         "load_cases: sample_idx %d out of range for case %d\n",
                         sample_idx, id);
            std::fclose(fp);
            return -1;
        }
        c->in_re[sample_idx]  = in_re;
        c->in_im[sample_idx]  = in_im;
        c->exp_re[sample_idx] = exp_re;
        c->exp_im[sample_idx] = exp_im;
    }
    std::fclose(fp);
    return (int)cases.size();
}

static int check_fft(const case_t &c, const beat_t *mem) {
    int fails = 0;
    for (size_t n = 0; n < c.n_samples; n++) {
        int16_t got_re, got_im;
        mem_read_sample(mem, c.dst_addr + n * SAMPLE_BYTES, &got_re, &got_im);
        int16_t exp_re = float_to_q214(c.exp_re[n] / (double)FFT_POINTS_INT);
        int16_t exp_im = float_to_q214(c.exp_im[n] / (double)FFT_POINTS_INT);
        int dre = std::abs((int)got_re - (int)exp_re);
        int dim = std::abs((int)got_im - (int)exp_im);
        if (dre > FIXED_TOL || dim > FIXED_TOL) {
            if (fails < 5) {
                std::printf("  [%s] sample %zu: got (%6d,%6d), "
                            "exp (%6d,%6d), diff (%d,%d)\n",
                            c.name.c_str(), n, got_re, got_im,
                            exp_re, exp_im, dre, dim);
            }
            fails++;
        }
    }
    return fails;
}

static int check_readonly(const case_t &c, const beat_t *mem) {
    int fails = 0;
    for (size_t n = 0; n < c.n_samples; n++) {
        int16_t got_re, got_im;
        mem_read_sample(mem, c.dst_addr + n * SAMPLE_BYTES, &got_re, &got_im);
        if (got_re != 0 || got_im != 0) {
            if (fails < 5) {
                std::printf("  [%s] sample %zu: got (%d,%d), expected (0,0)\n",
                            c.name.c_str(), n, got_re, got_im);
            }
            fails++;
        }
    }
    return fails;
}

static int check_writeonly(const case_t &c, const beat_t *mem) {
    int fails = 0;
    for (size_t n = 0; n < c.n_samples; n++) {
        int16_t got_re, got_im;
        mem_read_sample(mem, c.dst_addr + n * SAMPLE_BYTES, &got_re, &got_im);
        int16_t exp_re = float_to_q214(c.exp_re[n]);
        int16_t exp_im = float_to_q214(c.exp_im[n]);
        if (got_re != exp_re || got_im != exp_im) {
            if (fails < 5) {
                std::printf("  [%s] sample %zu: got (%6d,%6d), exp (%6d,%6d)\n",
                            c.name.c_str(), n, got_re, got_im, exp_re, exp_im);
            }
            fails++;
        }
    }
    return fails;
}

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : DEFAULT_CSV;

    std::vector<case_t> cases;
    int ncases = load_cases(path, cases);
    if (ncases < 0) {
        std::fprintf(stderr, "Failed to load vectors from '%s'\n", path);
        return 2;
    }

    std::vector<beat_t> mem(MEMORY_BEATS, beat_t(0));
    std::printf("Loaded %d test case(s) from %s\n", ncases, path);
    std::printf("Running fft_dma_top against golden vectors...\n");

    int failed   = 0;
    int pass_fft = 0, tot_fft = 0;
    int pass_ro  = 0, tot_ro  = 0;
    int pass_wo  = 0, tot_wo  = 0;

    for (const auto &c : cases) {
        if (c.src_addr % BEAT_BYTES != 0 || c.dst_addr % BEAT_BYTES != 0) {
            std::printf("  SKIP: %s (unaligned addr src=%" PRIu64
                        " dst=%" PRIu64 ")\n",
                        c.name.c_str(), c.src_addr, c.dst_addr);
            continue;
        }
        if (c.src_addr + c.n_samples * SAMPLE_BYTES > (uint64_t)MEMORY_BYTES ||
            c.dst_addr + c.n_samples * SAMPLE_BYTES > (uint64_t)MEMORY_BYTES) {
            std::printf("  SKIP: %s (addr exceeds memory)\n", c.name.c_str());
            continue;
        }

        std::fill(mem.begin(), mem.end(), beat_t(0));

        if (c.mode == MODE_FFT || c.mode == MODE_READ_ONLY) {
            for (size_t n = 0; n < c.n_samples; n++) {
                int16_t re_raw = float_to_q214(c.in_re[n]);
                int16_t im_raw = float_to_q214(c.in_im[n]);
                mem_write_sample(mem.data(),
                                 c.src_addr + n * SAMPLE_BYTES,
                                 re_raw, im_raw);
            }
        }

        beat_t *rd_ptr = mem.data() + (c.src_addr / BEAT_BYTES);
        beat_t *wr_ptr = mem.data() + (c.dst_addr / BEAT_BYTES);
        fft_dma_top(rd_ptr, wr_ptr, c.num_ffts, (uint32_t)c.mode);

        int bad = 0;
        switch (c.mode) {
        case MODE_FFT:
            bad = check_fft(c, mem.data());
            tot_fft++;
            if (bad == 0) pass_fft++;
            break;
        case MODE_READ_ONLY:
            bad = check_readonly(c, mem.data());
            tot_ro++;
            if (bad == 0) pass_ro++;
            break;
        case MODE_WRITE_ONLY:
            bad = check_writeonly(c, mem.data());
            tot_wo++;
            if (bad == 0) pass_wo++;
            break;
        }

        if (bad != 0) {
            std::printf("  FAIL: %s (mode=%d, %d mismatch(es))\n",
                        c.name.c_str(), (int)c.mode, bad);
            failed++;
        }
    }

    std::printf("---------------------------------\n");
    std::printf("MODE_FFT       : %d / %d PASS\n", pass_fft, tot_fft);
    std::printf("MODE_READ_ONLY : %d / %d PASS\n", pass_ro,  tot_ro);
    std::printf("MODE_WRITE_ONLY: %d / %d PASS\n", pass_wo,  tot_wo);

    return failed == 0 ? 0 : 1;
}
