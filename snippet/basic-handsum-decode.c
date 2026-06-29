// Copyright 2026 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

// ----------------

// This is a small, self-contained, single-file C library to decode basic
// Handsum image files. Basic means that it only supports Handsum's default
// configuration, c3q4: color=3 (RGB, no Alpha) and quality=4 (best).
//
// It is a C port of some of the github.com/google/wuffs/lib/handsum library.
// Decoding only, no encoding, and c3q4 only.
//
// To decode other configurations, like c1q1 (Gray, worst quality) or c4q2
// (RGBA, medium-low quality), use the github.com/google/wuffs Go or
// C-transpiled-from-memory-safe-Wuffs implementations.
//
// To use this file as a "foo.c"-like implementation, instead of a "foo.h"-like
// header, #define BASIC_HANDSUM_DECODE_IMPLEMENTATION before #include'ing or
// compiling it.
//
// As an option, you may also #define
// BASIC_HANDSUM_DECODE_CONFIG__STATIC_FUNCTIONS to make these functions have
// static storage. This can help the compiler ignore or discard unused code,
// which can produce faster compiles and smaller binaries.

#ifndef BASIC_HANDSUM_DECODE_INCLUDE_GUARD
#define BASIC_HANDSUM_DECODE_INCLUDE_GUARD

#if defined(BASIC_HANDSUM_DECODE_CONFIG__STATIC_FUNCTIONS)
#define BASIC_HANDSUM_DECODE__MAYBE_STATIC static
#else
#define BASIC_HANDSUM_DECODE__MAYBE_STATIC
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct basic_handsum_decode__pixel_buffer_type {
  uint32_t width;   // Measured in pixels, not bytes.
  uint32_t height;  // Measured in pixels, not bytes.
  uint8_t bgra_pixels[1024];
} basic_handsum_decode__pixel_buffer;

// BASIC_HANDSUM_DECODE__RESULT__ETC can be returned by
// basic_handsum_decode__decode. Zero means OK. Non-zero means an error.
#define BASIC_HANDSUM_DECODE__RESULT__OK 0
#define BASIC_HANDSUM_DECODE__RESULT__DST_IS_NULL 1
#define BASIC_HANDSUM_DECODE__RESULT__SRC_IS_TOO_SHORT 2
#define BASIC_HANDSUM_DECODE__RESULT__SRC_IS_NOT_HANDSUM_C3Q4 3

// Decodes the source bytes into the destination pixel buffer, producing 4
// bytes per pixel (in B, G, R, A order). The bgra_pixels' row stride will be
// (4 * width) bytes.
//
// It only supports c3q4 Handsum files: color=3 (so alpha is always 0xFF) and
// quality=4 (best). Every valid c3q4 Handsum file is exactly 147 bytes long.
//
// On success, it returns zero and dst's width and height will range between 1
// and 16, inclusive.
//
// On failure, it returns non-zero and, if dst is non-NULL, its width and
// height will be set to zero.
//
// This function is thread-safe.
BASIC_HANDSUM_DECODE__MAYBE_STATIC int  //
basic_handsum_decode__decode(basic_handsum_decode__pixel_buffer* dst,
                             const uint8_t* src_ptr,
                             const size_t src_len);

// --------

#ifdef BASIC_HANDSUM_DECODE_IMPLEMENTATION

static void  //
basic_handsum_decode__smooth_block_seams_16x16(uint8_t* b) {
  static const uint8_t smoothing_pairs_16x16[28][2] = {
      {0x07, 0x08}, {0x17, 0x18}, {0x27, 0x28}, {0x37, 0x38},
      {0x47, 0x48}, {0x57, 0x58}, {0x67, 0x68},

      {0x70, 0x80}, {0x71, 0x81}, {0x72, 0x82}, {0x73, 0x83},
      {0x74, 0x84}, {0x75, 0x85}, {0x76, 0x86},

      {0x79, 0x89}, {0x7A, 0x8A}, {0x7B, 0x8B}, {0x7C, 0x8C},
      {0x7D, 0x8D}, {0x7E, 0x8E}, {0x7F, 0x8F},

      {0x97, 0x98}, {0xA7, 0xA8}, {0xB7, 0xB8}, {0xC7, 0xC8},
      {0xD7, 0xD8}, {0xE7, 0xE8}, {0xF7, 0xF8},
  };

  for (int i = 0; i < 28; i++) {
    uint8_t p0 = smoothing_pairs_16x16[i][0];
    uint8_t p1 = smoothing_pairs_16x16[i][1];
    uint32_t v0 = b[p0];
    uint32_t v1 = b[p1];
    b[p0] = ((3 * v0) + v1 + 2) / 4;
    b[p1] = ((3 * v1) + v0 + 2) / 4;
  }

  uint32_t v77 = b[0x77];
  uint32_t v78 = b[0x78];
  uint32_t v88 = b[0x88];
  uint32_t v87 = b[0x87];

  b[0x77] = ((9 * v77) + (3 * v78) + v88 + (3 * v87) + 8) / 16;
  b[0x78] = ((9 * v78) + (3 * v88) + v87 + (3 * v77) + 8) / 16;
  b[0x88] = ((9 * v88) + (3 * v87) + v77 + (3 * v78) + 8) / 16;
  b[0x87] = ((9 * v87) + (3 * v77) + v78 + (3 * v88) + 8) / 16;
}

static void  //
basic_handsum_decode__smooth_block_seams_8x8_q4(uint8_t* b) {
  for (int y = 1; y < 7; y += 2) {
    for (int x = 1; x < 7; x += 2) {
      int o = (y * 8) + x;

      uint32_t v0 = b[o + 0];
      uint32_t v1 = b[o + 1];
      uint32_t v9 = b[o + 9];
      uint32_t v8 = b[o + 8];

      b[o + 0] = ((9 * v0) + (3 * v1) + v9 + (3 * v8) + 8) / 16;
      b[o + 1] = ((9 * v1) + (3 * v9) + v8 + (3 * v0) + 8) / 16;
      b[o + 9] = ((9 * v9) + (3 * v8) + v0 + (3 * v1) + 8) / 16;
      b[o + 8] = ((9 * v8) + (3 * v0) + v1 + (3 * v9) + 8) / 16;
    }

    {
      uint32_t v0 = b[000 + y];
      uint32_t v1 = b[001 + y];
      b[000 + y] = ((3 * v0) + v1 + 2) / 4;
      b[001 + y] = ((3 * v1) + v0 + 2) / 4;
    }

    {
      uint32_t v0 = b[070 + y];
      uint32_t v1 = b[071 + y];
      b[070 + y] = ((3 * v0) + v1 + 2) / 4;
      b[071 + y] = ((3 * v1) + v0 + 2) / 4;
    }

    int y8 = y * 8;

    {
      uint32_t v0 = b[000 + y8];
      uint32_t v1 = b[010 + y8];
      b[000 + y8] = ((3 * v0) + v1 + 2) / 4;
      b[010 + y8] = ((3 * v1) + v0 + 2) / 4;
    }

    {
      uint32_t v0 = b[007 + y8];
      uint32_t v1 = b[017 + y8];
      b[007 + y8] = ((3 * v0) + v1 + 2) / 4;
      b[017 + y8] = ((3 * v1) + v0 + 2) / 4;
    }
  }
}

static int  //
basic_handsum_decode__decode_block_q4(uint8_t* dst_ptr,
                                      size_t dst_stride,
                                      const uint8_t* src_ptr,
                                      size_t bit_offset) {
  static const uint8_t bias_and_clamp[1024] = {
      0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B,
      0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
      0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3,
      0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
      0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB,
      0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
      0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3,
      0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
      0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB,
      0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
      0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,

      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
      0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
      0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23,
      0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
      0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
      0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
      0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53,
      0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
      0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B,
      0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
      0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
  };

  uint8_t tmp[64];
  for (int i = 0; i < 16; i++) {
    int nibble0 = (src_ptr[bit_offset >> 3] >> (bit_offset & 4)) & 15;
    int dct0 = (nibble0 * 0x22) - 256;
    bit_offset += 4;

    int nibble1 = (src_ptr[bit_offset >> 3] >> (bit_offset & 4)) & 15;
    int dct1 = (nibble1 * 0x10) - 128;
    bit_offset += 4;

    int nibble2 = (src_ptr[bit_offset >> 3] >> (bit_offset & 4)) & 15;
    int dct2 = (nibble2 * 0x10) - 128;
    bit_offset += 4;

    int x2 = 2 * (i & 3);
    int y2 = 2 * (i >> 2);
    int o2 = (y2 * 8) + x2;

    tmp[o2 + 0] = bias_and_clamp[((+dct0 + dct1 + dct2 + 1) >> 1) & 1023];
    tmp[o2 + 1] = bias_and_clamp[((+dct0 - dct1 + dct2 + 1) >> 1) & 1023];
    tmp[o2 + 8] = bias_and_clamp[((+dct0 + dct1 - dct2 + 1) >> 1) & 1023];
    tmp[o2 + 9] = bias_and_clamp[((+dct0 - dct1 - dct2 + 1) >> 1) & 1023];
  }

  basic_handsum_decode__smooth_block_seams_8x8_q4(&tmp[0]);

  for (size_t i = 0; i < 8; i++) {
    size_t di = i * dst_stride;
    size_t ti = i * 8;
    memcpy(&dst_ptr[di], &tmp[ti], 8);
  }

  return bit_offset;
}

static void  //
basic_handsum_decode__scale_and_bias_chroma_down(uint8_t* b) {
  for (int i = 0; i < 64; i++) {
    b[i] = (b[i] >> 1) + 0x3C;
  }
}

static void  //
basic_handsum_decode__upsample_from(uint8_t* restrict dst,
                                    const uint8_t* restrict src) {
  // clang-format off

  dst[0x00] = src[0x00];
  dst[0x01] = ((3 * (int)(src[0x00])) + (int)(src[0x01]) + 2) / 4;
  dst[0x02] = ((3 * (int)(src[0x01])) + (int)(src[0x00]) + 2) / 4;
  dst[0x03] = ((3 * (int)(src[0x01])) + (int)(src[0x02]) + 2) / 4;
  dst[0x04] = ((3 * (int)(src[0x02])) + (int)(src[0x01]) + 2) / 4;
  dst[0x05] = ((3 * (int)(src[0x02])) + (int)(src[0x03]) + 2) / 4;
  dst[0x06] = ((3 * (int)(src[0x03])) + (int)(src[0x02]) + 2) / 4;
  dst[0x07] = ((3 * (int)(src[0x03])) + (int)(src[0x04]) + 2) / 4;
  dst[0x08] = ((3 * (int)(src[0x04])) + (int)(src[0x03]) + 2) / 4;
  dst[0x09] = ((3 * (int)(src[0x04])) + (int)(src[0x05]) + 2) / 4;
  dst[0x0A] = ((3 * (int)(src[0x05])) + (int)(src[0x04]) + 2) / 4;
  dst[0x0B] = ((3 * (int)(src[0x05])) + (int)(src[0x06]) + 2) / 4;
  dst[0x0C] = ((3 * (int)(src[0x06])) + (int)(src[0x05]) + 2) / 4;
  dst[0x0D] = ((3 * (int)(src[0x06])) + (int)(src[0x07]) + 2) / 4;
  dst[0x0E] = ((3 * (int)(src[0x07])) + (int)(src[0x06]) + 2) / 4;
  dst[0x0F] = src[0x07];
  dst[0x10] = ((3 * (int)(src[0x00])) + (int)(src[0x08]) + 2) / 4;
  dst[0x11] = ((9 * (int)(src[0x00])) + (3 * (int)(src[0x01])) + (3 * (int)(src[0x08])) + (int)(src[0x09]) + 8) / 16;
  dst[0x12] = ((9 * (int)(src[0x01])) + (3 * (int)(src[0x00])) + (3 * (int)(src[0x09])) + (int)(src[0x08]) + 8) / 16;
  dst[0x13] = ((9 * (int)(src[0x01])) + (3 * (int)(src[0x02])) + (3 * (int)(src[0x09])) + (int)(src[0x0A]) + 8) / 16;
  dst[0x14] = ((9 * (int)(src[0x02])) + (3 * (int)(src[0x01])) + (3 * (int)(src[0x0A])) + (int)(src[0x09]) + 8) / 16;
  dst[0x15] = ((9 * (int)(src[0x02])) + (3 * (int)(src[0x03])) + (3 * (int)(src[0x0A])) + (int)(src[0x0B]) + 8) / 16;
  dst[0x16] = ((9 * (int)(src[0x03])) + (3 * (int)(src[0x02])) + (3 * (int)(src[0x0B])) + (int)(src[0x0A]) + 8) / 16;
  dst[0x17] = ((9 * (int)(src[0x03])) + (3 * (int)(src[0x04])) + (3 * (int)(src[0x0B])) + (int)(src[0x0C]) + 8) / 16;
  dst[0x18] = ((9 * (int)(src[0x04])) + (3 * (int)(src[0x03])) + (3 * (int)(src[0x0C])) + (int)(src[0x0B]) + 8) / 16;
  dst[0x19] = ((9 * (int)(src[0x04])) + (3 * (int)(src[0x05])) + (3 * (int)(src[0x0C])) + (int)(src[0x0D]) + 8) / 16;
  dst[0x1A] = ((9 * (int)(src[0x05])) + (3 * (int)(src[0x04])) + (3 * (int)(src[0x0D])) + (int)(src[0x0C]) + 8) / 16;
  dst[0x1B] = ((9 * (int)(src[0x05])) + (3 * (int)(src[0x06])) + (3 * (int)(src[0x0D])) + (int)(src[0x0E]) + 8) / 16;
  dst[0x1C] = ((9 * (int)(src[0x06])) + (3 * (int)(src[0x05])) + (3 * (int)(src[0x0E])) + (int)(src[0x0D]) + 8) / 16;
  dst[0x1D] = ((9 * (int)(src[0x06])) + (3 * (int)(src[0x07])) + (3 * (int)(src[0x0E])) + (int)(src[0x0F]) + 8) / 16;
  dst[0x1E] = ((9 * (int)(src[0x07])) + (3 * (int)(src[0x06])) + (3 * (int)(src[0x0F])) + (int)(src[0x0E]) + 8) / 16;
  dst[0x1F] = ((3 * (int)(src[0x07])) + (int)(src[0x0F]) + 2) / 4;
  dst[0x20] = ((3 * (int)(src[0x08])) + (int)(src[0x00]) + 2) / 4;
  dst[0x21] = ((9 * (int)(src[0x08])) + (3 * (int)(src[0x09])) + (3 * (int)(src[0x00])) + (int)(src[0x01]) + 8) / 16;
  dst[0x22] = ((9 * (int)(src[0x09])) + (3 * (int)(src[0x08])) + (3 * (int)(src[0x01])) + (int)(src[0x00]) + 8) / 16;
  dst[0x23] = ((9 * (int)(src[0x09])) + (3 * (int)(src[0x0A])) + (3 * (int)(src[0x01])) + (int)(src[0x02]) + 8) / 16;
  dst[0x24] = ((9 * (int)(src[0x0A])) + (3 * (int)(src[0x09])) + (3 * (int)(src[0x02])) + (int)(src[0x01]) + 8) / 16;
  dst[0x25] = ((9 * (int)(src[0x0A])) + (3 * (int)(src[0x0B])) + (3 * (int)(src[0x02])) + (int)(src[0x03]) + 8) / 16;
  dst[0x26] = ((9 * (int)(src[0x0B])) + (3 * (int)(src[0x0A])) + (3 * (int)(src[0x03])) + (int)(src[0x02]) + 8) / 16;
  dst[0x27] = ((9 * (int)(src[0x0B])) + (3 * (int)(src[0x0C])) + (3 * (int)(src[0x03])) + (int)(src[0x04]) + 8) / 16;
  dst[0x28] = ((9 * (int)(src[0x0C])) + (3 * (int)(src[0x0B])) + (3 * (int)(src[0x04])) + (int)(src[0x03]) + 8) / 16;
  dst[0x29] = ((9 * (int)(src[0x0C])) + (3 * (int)(src[0x0D])) + (3 * (int)(src[0x04])) + (int)(src[0x05]) + 8) / 16;
  dst[0x2A] = ((9 * (int)(src[0x0D])) + (3 * (int)(src[0x0C])) + (3 * (int)(src[0x05])) + (int)(src[0x04]) + 8) / 16;
  dst[0x2B] = ((9 * (int)(src[0x0D])) + (3 * (int)(src[0x0E])) + (3 * (int)(src[0x05])) + (int)(src[0x06]) + 8) / 16;
  dst[0x2C] = ((9 * (int)(src[0x0E])) + (3 * (int)(src[0x0D])) + (3 * (int)(src[0x06])) + (int)(src[0x05]) + 8) / 16;
  dst[0x2D] = ((9 * (int)(src[0x0E])) + (3 * (int)(src[0x0F])) + (3 * (int)(src[0x06])) + (int)(src[0x07]) + 8) / 16;
  dst[0x2E] = ((9 * (int)(src[0x0F])) + (3 * (int)(src[0x0E])) + (3 * (int)(src[0x07])) + (int)(src[0x06]) + 8) / 16;
  dst[0x2F] = ((3 * (int)(src[0x0F])) + (int)(src[0x07]) + 2) / 4;
  dst[0x30] = ((3 * (int)(src[0x08])) + (int)(src[0x10]) + 2) / 4;
  dst[0x31] = ((9 * (int)(src[0x08])) + (3 * (int)(src[0x09])) + (3 * (int)(src[0x10])) + (int)(src[0x11]) + 8) / 16;
  dst[0x32] = ((9 * (int)(src[0x09])) + (3 * (int)(src[0x08])) + (3 * (int)(src[0x11])) + (int)(src[0x10]) + 8) / 16;
  dst[0x33] = ((9 * (int)(src[0x09])) + (3 * (int)(src[0x0A])) + (3 * (int)(src[0x11])) + (int)(src[0x12]) + 8) / 16;
  dst[0x34] = ((9 * (int)(src[0x0A])) + (3 * (int)(src[0x09])) + (3 * (int)(src[0x12])) + (int)(src[0x11]) + 8) / 16;
  dst[0x35] = ((9 * (int)(src[0x0A])) + (3 * (int)(src[0x0B])) + (3 * (int)(src[0x12])) + (int)(src[0x13]) + 8) / 16;
  dst[0x36] = ((9 * (int)(src[0x0B])) + (3 * (int)(src[0x0A])) + (3 * (int)(src[0x13])) + (int)(src[0x12]) + 8) / 16;
  dst[0x37] = ((9 * (int)(src[0x0B])) + (3 * (int)(src[0x0C])) + (3 * (int)(src[0x13])) + (int)(src[0x14]) + 8) / 16;
  dst[0x38] = ((9 * (int)(src[0x0C])) + (3 * (int)(src[0x0B])) + (3 * (int)(src[0x14])) + (int)(src[0x13]) + 8) / 16;
  dst[0x39] = ((9 * (int)(src[0x0C])) + (3 * (int)(src[0x0D])) + (3 * (int)(src[0x14])) + (int)(src[0x15]) + 8) / 16;
  dst[0x3A] = ((9 * (int)(src[0x0D])) + (3 * (int)(src[0x0C])) + (3 * (int)(src[0x15])) + (int)(src[0x14]) + 8) / 16;
  dst[0x3B] = ((9 * (int)(src[0x0D])) + (3 * (int)(src[0x0E])) + (3 * (int)(src[0x15])) + (int)(src[0x16]) + 8) / 16;
  dst[0x3C] = ((9 * (int)(src[0x0E])) + (3 * (int)(src[0x0D])) + (3 * (int)(src[0x16])) + (int)(src[0x15]) + 8) / 16;
  dst[0x3D] = ((9 * (int)(src[0x0E])) + (3 * (int)(src[0x0F])) + (3 * (int)(src[0x16])) + (int)(src[0x17]) + 8) / 16;
  dst[0x3E] = ((9 * (int)(src[0x0F])) + (3 * (int)(src[0x0E])) + (3 * (int)(src[0x17])) + (int)(src[0x16]) + 8) / 16;
  dst[0x3F] = ((3 * (int)(src[0x0F])) + (int)(src[0x17]) + 2) / 4;
  dst[0x40] = ((3 * (int)(src[0x10])) + (int)(src[0x08]) + 2) / 4;
  dst[0x41] = ((9 * (int)(src[0x10])) + (3 * (int)(src[0x11])) + (3 * (int)(src[0x08])) + (int)(src[0x09]) + 8) / 16;
  dst[0x42] = ((9 * (int)(src[0x11])) + (3 * (int)(src[0x10])) + (3 * (int)(src[0x09])) + (int)(src[0x08]) + 8) / 16;
  dst[0x43] = ((9 * (int)(src[0x11])) + (3 * (int)(src[0x12])) + (3 * (int)(src[0x09])) + (int)(src[0x0A]) + 8) / 16;
  dst[0x44] = ((9 * (int)(src[0x12])) + (3 * (int)(src[0x11])) + (3 * (int)(src[0x0A])) + (int)(src[0x09]) + 8) / 16;
  dst[0x45] = ((9 * (int)(src[0x12])) + (3 * (int)(src[0x13])) + (3 * (int)(src[0x0A])) + (int)(src[0x0B]) + 8) / 16;
  dst[0x46] = ((9 * (int)(src[0x13])) + (3 * (int)(src[0x12])) + (3 * (int)(src[0x0B])) + (int)(src[0x0A]) + 8) / 16;
  dst[0x47] = ((9 * (int)(src[0x13])) + (3 * (int)(src[0x14])) + (3 * (int)(src[0x0B])) + (int)(src[0x0C]) + 8) / 16;
  dst[0x48] = ((9 * (int)(src[0x14])) + (3 * (int)(src[0x13])) + (3 * (int)(src[0x0C])) + (int)(src[0x0B]) + 8) / 16;
  dst[0x49] = ((9 * (int)(src[0x14])) + (3 * (int)(src[0x15])) + (3 * (int)(src[0x0C])) + (int)(src[0x0D]) + 8) / 16;
  dst[0x4A] = ((9 * (int)(src[0x15])) + (3 * (int)(src[0x14])) + (3 * (int)(src[0x0D])) + (int)(src[0x0C]) + 8) / 16;
  dst[0x4B] = ((9 * (int)(src[0x15])) + (3 * (int)(src[0x16])) + (3 * (int)(src[0x0D])) + (int)(src[0x0E]) + 8) / 16;
  dst[0x4C] = ((9 * (int)(src[0x16])) + (3 * (int)(src[0x15])) + (3 * (int)(src[0x0E])) + (int)(src[0x0D]) + 8) / 16;
  dst[0x4D] = ((9 * (int)(src[0x16])) + (3 * (int)(src[0x17])) + (3 * (int)(src[0x0E])) + (int)(src[0x0F]) + 8) / 16;
  dst[0x4E] = ((9 * (int)(src[0x17])) + (3 * (int)(src[0x16])) + (3 * (int)(src[0x0F])) + (int)(src[0x0E]) + 8) / 16;
  dst[0x4F] = ((3 * (int)(src[0x17])) + (int)(src[0x0F]) + 2) / 4;
  dst[0x50] = ((3 * (int)(src[0x10])) + (int)(src[0x18]) + 2) / 4;
  dst[0x51] = ((9 * (int)(src[0x10])) + (3 * (int)(src[0x11])) + (3 * (int)(src[0x18])) + (int)(src[0x19]) + 8) / 16;
  dst[0x52] = ((9 * (int)(src[0x11])) + (3 * (int)(src[0x10])) + (3 * (int)(src[0x19])) + (int)(src[0x18]) + 8) / 16;
  dst[0x53] = ((9 * (int)(src[0x11])) + (3 * (int)(src[0x12])) + (3 * (int)(src[0x19])) + (int)(src[0x1A]) + 8) / 16;
  dst[0x54] = ((9 * (int)(src[0x12])) + (3 * (int)(src[0x11])) + (3 * (int)(src[0x1A])) + (int)(src[0x19]) + 8) / 16;
  dst[0x55] = ((9 * (int)(src[0x12])) + (3 * (int)(src[0x13])) + (3 * (int)(src[0x1A])) + (int)(src[0x1B]) + 8) / 16;
  dst[0x56] = ((9 * (int)(src[0x13])) + (3 * (int)(src[0x12])) + (3 * (int)(src[0x1B])) + (int)(src[0x1A]) + 8) / 16;
  dst[0x57] = ((9 * (int)(src[0x13])) + (3 * (int)(src[0x14])) + (3 * (int)(src[0x1B])) + (int)(src[0x1C]) + 8) / 16;
  dst[0x58] = ((9 * (int)(src[0x14])) + (3 * (int)(src[0x13])) + (3 * (int)(src[0x1C])) + (int)(src[0x1B]) + 8) / 16;
  dst[0x59] = ((9 * (int)(src[0x14])) + (3 * (int)(src[0x15])) + (3 * (int)(src[0x1C])) + (int)(src[0x1D]) + 8) / 16;
  dst[0x5A] = ((9 * (int)(src[0x15])) + (3 * (int)(src[0x14])) + (3 * (int)(src[0x1D])) + (int)(src[0x1C]) + 8) / 16;
  dst[0x5B] = ((9 * (int)(src[0x15])) + (3 * (int)(src[0x16])) + (3 * (int)(src[0x1D])) + (int)(src[0x1E]) + 8) / 16;
  dst[0x5C] = ((9 * (int)(src[0x16])) + (3 * (int)(src[0x15])) + (3 * (int)(src[0x1E])) + (int)(src[0x1D]) + 8) / 16;
  dst[0x5D] = ((9 * (int)(src[0x16])) + (3 * (int)(src[0x17])) + (3 * (int)(src[0x1E])) + (int)(src[0x1F]) + 8) / 16;
  dst[0x5E] = ((9 * (int)(src[0x17])) + (3 * (int)(src[0x16])) + (3 * (int)(src[0x1F])) + (int)(src[0x1E]) + 8) / 16;
  dst[0x5F] = ((3 * (int)(src[0x17])) + (int)(src[0x1F]) + 2) / 4;
  dst[0x60] = ((3 * (int)(src[0x18])) + (int)(src[0x10]) + 2) / 4;
  dst[0x61] = ((9 * (int)(src[0x18])) + (3 * (int)(src[0x19])) + (3 * (int)(src[0x10])) + (int)(src[0x11]) + 8) / 16;
  dst[0x62] = ((9 * (int)(src[0x19])) + (3 * (int)(src[0x18])) + (3 * (int)(src[0x11])) + (int)(src[0x10]) + 8) / 16;
  dst[0x63] = ((9 * (int)(src[0x19])) + (3 * (int)(src[0x1A])) + (3 * (int)(src[0x11])) + (int)(src[0x12]) + 8) / 16;
  dst[0x64] = ((9 * (int)(src[0x1A])) + (3 * (int)(src[0x19])) + (3 * (int)(src[0x12])) + (int)(src[0x11]) + 8) / 16;
  dst[0x65] = ((9 * (int)(src[0x1A])) + (3 * (int)(src[0x1B])) + (3 * (int)(src[0x12])) + (int)(src[0x13]) + 8) / 16;
  dst[0x66] = ((9 * (int)(src[0x1B])) + (3 * (int)(src[0x1A])) + (3 * (int)(src[0x13])) + (int)(src[0x12]) + 8) / 16;
  dst[0x67] = ((9 * (int)(src[0x1B])) + (3 * (int)(src[0x1C])) + (3 * (int)(src[0x13])) + (int)(src[0x14]) + 8) / 16;
  dst[0x68] = ((9 * (int)(src[0x1C])) + (3 * (int)(src[0x1B])) + (3 * (int)(src[0x14])) + (int)(src[0x13]) + 8) / 16;
  dst[0x69] = ((9 * (int)(src[0x1C])) + (3 * (int)(src[0x1D])) + (3 * (int)(src[0x14])) + (int)(src[0x15]) + 8) / 16;
  dst[0x6A] = ((9 * (int)(src[0x1D])) + (3 * (int)(src[0x1C])) + (3 * (int)(src[0x15])) + (int)(src[0x14]) + 8) / 16;
  dst[0x6B] = ((9 * (int)(src[0x1D])) + (3 * (int)(src[0x1E])) + (3 * (int)(src[0x15])) + (int)(src[0x16]) + 8) / 16;
  dst[0x6C] = ((9 * (int)(src[0x1E])) + (3 * (int)(src[0x1D])) + (3 * (int)(src[0x16])) + (int)(src[0x15]) + 8) / 16;
  dst[0x6D] = ((9 * (int)(src[0x1E])) + (3 * (int)(src[0x1F])) + (3 * (int)(src[0x16])) + (int)(src[0x17]) + 8) / 16;
  dst[0x6E] = ((9 * (int)(src[0x1F])) + (3 * (int)(src[0x1E])) + (3 * (int)(src[0x17])) + (int)(src[0x16]) + 8) / 16;
  dst[0x6F] = ((3 * (int)(src[0x1F])) + (int)(src[0x17]) + 2) / 4;
  dst[0x70] = ((3 * (int)(src[0x18])) + (int)(src[0x20]) + 2) / 4;
  dst[0x71] = ((9 * (int)(src[0x18])) + (3 * (int)(src[0x19])) + (3 * (int)(src[0x20])) + (int)(src[0x21]) + 8) / 16;
  dst[0x72] = ((9 * (int)(src[0x19])) + (3 * (int)(src[0x18])) + (3 * (int)(src[0x21])) + (int)(src[0x20]) + 8) / 16;
  dst[0x73] = ((9 * (int)(src[0x19])) + (3 * (int)(src[0x1A])) + (3 * (int)(src[0x21])) + (int)(src[0x22]) + 8) / 16;
  dst[0x74] = ((9 * (int)(src[0x1A])) + (3 * (int)(src[0x19])) + (3 * (int)(src[0x22])) + (int)(src[0x21]) + 8) / 16;
  dst[0x75] = ((9 * (int)(src[0x1A])) + (3 * (int)(src[0x1B])) + (3 * (int)(src[0x22])) + (int)(src[0x23]) + 8) / 16;
  dst[0x76] = ((9 * (int)(src[0x1B])) + (3 * (int)(src[0x1A])) + (3 * (int)(src[0x23])) + (int)(src[0x22]) + 8) / 16;
  dst[0x77] = ((9 * (int)(src[0x1B])) + (3 * (int)(src[0x1C])) + (3 * (int)(src[0x23])) + (int)(src[0x24]) + 8) / 16;
  dst[0x78] = ((9 * (int)(src[0x1C])) + (3 * (int)(src[0x1B])) + (3 * (int)(src[0x24])) + (int)(src[0x23]) + 8) / 16;
  dst[0x79] = ((9 * (int)(src[0x1C])) + (3 * (int)(src[0x1D])) + (3 * (int)(src[0x24])) + (int)(src[0x25]) + 8) / 16;
  dst[0x7A] = ((9 * (int)(src[0x1D])) + (3 * (int)(src[0x1C])) + (3 * (int)(src[0x25])) + (int)(src[0x24]) + 8) / 16;
  dst[0x7B] = ((9 * (int)(src[0x1D])) + (3 * (int)(src[0x1E])) + (3 * (int)(src[0x25])) + (int)(src[0x26]) + 8) / 16;
  dst[0x7C] = ((9 * (int)(src[0x1E])) + (3 * (int)(src[0x1D])) + (3 * (int)(src[0x26])) + (int)(src[0x25]) + 8) / 16;
  dst[0x7D] = ((9 * (int)(src[0x1E])) + (3 * (int)(src[0x1F])) + (3 * (int)(src[0x26])) + (int)(src[0x27]) + 8) / 16;
  dst[0x7E] = ((9 * (int)(src[0x1F])) + (3 * (int)(src[0x1E])) + (3 * (int)(src[0x27])) + (int)(src[0x26]) + 8) / 16;
  dst[0x7F] = ((3 * (int)(src[0x1F])) + (int)(src[0x27]) + 2) / 4;
  dst[0x80] = ((3 * (int)(src[0x20])) + (int)(src[0x18]) + 2) / 4;
  dst[0x81] = ((9 * (int)(src[0x20])) + (3 * (int)(src[0x21])) + (3 * (int)(src[0x18])) + (int)(src[0x19]) + 8) / 16;
  dst[0x82] = ((9 * (int)(src[0x21])) + (3 * (int)(src[0x20])) + (3 * (int)(src[0x19])) + (int)(src[0x18]) + 8) / 16;
  dst[0x83] = ((9 * (int)(src[0x21])) + (3 * (int)(src[0x22])) + (3 * (int)(src[0x19])) + (int)(src[0x1A]) + 8) / 16;
  dst[0x84] = ((9 * (int)(src[0x22])) + (3 * (int)(src[0x21])) + (3 * (int)(src[0x1A])) + (int)(src[0x19]) + 8) / 16;
  dst[0x85] = ((9 * (int)(src[0x22])) + (3 * (int)(src[0x23])) + (3 * (int)(src[0x1A])) + (int)(src[0x1B]) + 8) / 16;
  dst[0x86] = ((9 * (int)(src[0x23])) + (3 * (int)(src[0x22])) + (3 * (int)(src[0x1B])) + (int)(src[0x1A]) + 8) / 16;
  dst[0x87] = ((9 * (int)(src[0x23])) + (3 * (int)(src[0x24])) + (3 * (int)(src[0x1B])) + (int)(src[0x1C]) + 8) / 16;
  dst[0x88] = ((9 * (int)(src[0x24])) + (3 * (int)(src[0x23])) + (3 * (int)(src[0x1C])) + (int)(src[0x1B]) + 8) / 16;
  dst[0x89] = ((9 * (int)(src[0x24])) + (3 * (int)(src[0x25])) + (3 * (int)(src[0x1C])) + (int)(src[0x1D]) + 8) / 16;
  dst[0x8A] = ((9 * (int)(src[0x25])) + (3 * (int)(src[0x24])) + (3 * (int)(src[0x1D])) + (int)(src[0x1C]) + 8) / 16;
  dst[0x8B] = ((9 * (int)(src[0x25])) + (3 * (int)(src[0x26])) + (3 * (int)(src[0x1D])) + (int)(src[0x1E]) + 8) / 16;
  dst[0x8C] = ((9 * (int)(src[0x26])) + (3 * (int)(src[0x25])) + (3 * (int)(src[0x1E])) + (int)(src[0x1D]) + 8) / 16;
  dst[0x8D] = ((9 * (int)(src[0x26])) + (3 * (int)(src[0x27])) + (3 * (int)(src[0x1E])) + (int)(src[0x1F]) + 8) / 16;
  dst[0x8E] = ((9 * (int)(src[0x27])) + (3 * (int)(src[0x26])) + (3 * (int)(src[0x1F])) + (int)(src[0x1E]) + 8) / 16;
  dst[0x8F] = ((3 * (int)(src[0x27])) + (int)(src[0x1F]) + 2) / 4;
  dst[0x90] = ((3 * (int)(src[0x20])) + (int)(src[0x28]) + 2) / 4;
  dst[0x91] = ((9 * (int)(src[0x20])) + (3 * (int)(src[0x21])) + (3 * (int)(src[0x28])) + (int)(src[0x29]) + 8) / 16;
  dst[0x92] = ((9 * (int)(src[0x21])) + (3 * (int)(src[0x20])) + (3 * (int)(src[0x29])) + (int)(src[0x28]) + 8) / 16;
  dst[0x93] = ((9 * (int)(src[0x21])) + (3 * (int)(src[0x22])) + (3 * (int)(src[0x29])) + (int)(src[0x2A]) + 8) / 16;
  dst[0x94] = ((9 * (int)(src[0x22])) + (3 * (int)(src[0x21])) + (3 * (int)(src[0x2A])) + (int)(src[0x29]) + 8) / 16;
  dst[0x95] = ((9 * (int)(src[0x22])) + (3 * (int)(src[0x23])) + (3 * (int)(src[0x2A])) + (int)(src[0x2B]) + 8) / 16;
  dst[0x96] = ((9 * (int)(src[0x23])) + (3 * (int)(src[0x22])) + (3 * (int)(src[0x2B])) + (int)(src[0x2A]) + 8) / 16;
  dst[0x97] = ((9 * (int)(src[0x23])) + (3 * (int)(src[0x24])) + (3 * (int)(src[0x2B])) + (int)(src[0x2C]) + 8) / 16;
  dst[0x98] = ((9 * (int)(src[0x24])) + (3 * (int)(src[0x23])) + (3 * (int)(src[0x2C])) + (int)(src[0x2B]) + 8) / 16;
  dst[0x99] = ((9 * (int)(src[0x24])) + (3 * (int)(src[0x25])) + (3 * (int)(src[0x2C])) + (int)(src[0x2D]) + 8) / 16;
  dst[0x9A] = ((9 * (int)(src[0x25])) + (3 * (int)(src[0x24])) + (3 * (int)(src[0x2D])) + (int)(src[0x2C]) + 8) / 16;
  dst[0x9B] = ((9 * (int)(src[0x25])) + (3 * (int)(src[0x26])) + (3 * (int)(src[0x2D])) + (int)(src[0x2E]) + 8) / 16;
  dst[0x9C] = ((9 * (int)(src[0x26])) + (3 * (int)(src[0x25])) + (3 * (int)(src[0x2E])) + (int)(src[0x2D]) + 8) / 16;
  dst[0x9D] = ((9 * (int)(src[0x26])) + (3 * (int)(src[0x27])) + (3 * (int)(src[0x2E])) + (int)(src[0x2F]) + 8) / 16;
  dst[0x9E] = ((9 * (int)(src[0x27])) + (3 * (int)(src[0x26])) + (3 * (int)(src[0x2F])) + (int)(src[0x2E]) + 8) / 16;
  dst[0x9F] = ((3 * (int)(src[0x27])) + (int)(src[0x2F]) + 2) / 4;
  dst[0xA0] = ((3 * (int)(src[0x28])) + (int)(src[0x20]) + 2) / 4;
  dst[0xA1] = ((9 * (int)(src[0x28])) + (3 * (int)(src[0x29])) + (3 * (int)(src[0x20])) + (int)(src[0x21]) + 8) / 16;
  dst[0xA2] = ((9 * (int)(src[0x29])) + (3 * (int)(src[0x28])) + (3 * (int)(src[0x21])) + (int)(src[0x20]) + 8) / 16;
  dst[0xA3] = ((9 * (int)(src[0x29])) + (3 * (int)(src[0x2A])) + (3 * (int)(src[0x21])) + (int)(src[0x22]) + 8) / 16;
  dst[0xA4] = ((9 * (int)(src[0x2A])) + (3 * (int)(src[0x29])) + (3 * (int)(src[0x22])) + (int)(src[0x21]) + 8) / 16;
  dst[0xA5] = ((9 * (int)(src[0x2A])) + (3 * (int)(src[0x2B])) + (3 * (int)(src[0x22])) + (int)(src[0x23]) + 8) / 16;
  dst[0xA6] = ((9 * (int)(src[0x2B])) + (3 * (int)(src[0x2A])) + (3 * (int)(src[0x23])) + (int)(src[0x22]) + 8) / 16;
  dst[0xA7] = ((9 * (int)(src[0x2B])) + (3 * (int)(src[0x2C])) + (3 * (int)(src[0x23])) + (int)(src[0x24]) + 8) / 16;
  dst[0xA8] = ((9 * (int)(src[0x2C])) + (3 * (int)(src[0x2B])) + (3 * (int)(src[0x24])) + (int)(src[0x23]) + 8) / 16;
  dst[0xA9] = ((9 * (int)(src[0x2C])) + (3 * (int)(src[0x2D])) + (3 * (int)(src[0x24])) + (int)(src[0x25]) + 8) / 16;
  dst[0xAA] = ((9 * (int)(src[0x2D])) + (3 * (int)(src[0x2C])) + (3 * (int)(src[0x25])) + (int)(src[0x24]) + 8) / 16;
  dst[0xAB] = ((9 * (int)(src[0x2D])) + (3 * (int)(src[0x2E])) + (3 * (int)(src[0x25])) + (int)(src[0x26]) + 8) / 16;
  dst[0xAC] = ((9 * (int)(src[0x2E])) + (3 * (int)(src[0x2D])) + (3 * (int)(src[0x26])) + (int)(src[0x25]) + 8) / 16;
  dst[0xAD] = ((9 * (int)(src[0x2E])) + (3 * (int)(src[0x2F])) + (3 * (int)(src[0x26])) + (int)(src[0x27]) + 8) / 16;
  dst[0xAE] = ((9 * (int)(src[0x2F])) + (3 * (int)(src[0x2E])) + (3 * (int)(src[0x27])) + (int)(src[0x26]) + 8) / 16;
  dst[0xAF] = ((3 * (int)(src[0x2F])) + (int)(src[0x27]) + 2) / 4;
  dst[0xB0] = ((3 * (int)(src[0x28])) + (int)(src[0x30]) + 2) / 4;
  dst[0xB1] = ((9 * (int)(src[0x28])) + (3 * (int)(src[0x29])) + (3 * (int)(src[0x30])) + (int)(src[0x31]) + 8) / 16;
  dst[0xB2] = ((9 * (int)(src[0x29])) + (3 * (int)(src[0x28])) + (3 * (int)(src[0x31])) + (int)(src[0x30]) + 8) / 16;
  dst[0xB3] = ((9 * (int)(src[0x29])) + (3 * (int)(src[0x2A])) + (3 * (int)(src[0x31])) + (int)(src[0x32]) + 8) / 16;
  dst[0xB4] = ((9 * (int)(src[0x2A])) + (3 * (int)(src[0x29])) + (3 * (int)(src[0x32])) + (int)(src[0x31]) + 8) / 16;
  dst[0xB5] = ((9 * (int)(src[0x2A])) + (3 * (int)(src[0x2B])) + (3 * (int)(src[0x32])) + (int)(src[0x33]) + 8) / 16;
  dst[0xB6] = ((9 * (int)(src[0x2B])) + (3 * (int)(src[0x2A])) + (3 * (int)(src[0x33])) + (int)(src[0x32]) + 8) / 16;
  dst[0xB7] = ((9 * (int)(src[0x2B])) + (3 * (int)(src[0x2C])) + (3 * (int)(src[0x33])) + (int)(src[0x34]) + 8) / 16;
  dst[0xB8] = ((9 * (int)(src[0x2C])) + (3 * (int)(src[0x2B])) + (3 * (int)(src[0x34])) + (int)(src[0x33]) + 8) / 16;
  dst[0xB9] = ((9 * (int)(src[0x2C])) + (3 * (int)(src[0x2D])) + (3 * (int)(src[0x34])) + (int)(src[0x35]) + 8) / 16;
  dst[0xBA] = ((9 * (int)(src[0x2D])) + (3 * (int)(src[0x2C])) + (3 * (int)(src[0x35])) + (int)(src[0x34]) + 8) / 16;
  dst[0xBB] = ((9 * (int)(src[0x2D])) + (3 * (int)(src[0x2E])) + (3 * (int)(src[0x35])) + (int)(src[0x36]) + 8) / 16;
  dst[0xBC] = ((9 * (int)(src[0x2E])) + (3 * (int)(src[0x2D])) + (3 * (int)(src[0x36])) + (int)(src[0x35]) + 8) / 16;
  dst[0xBD] = ((9 * (int)(src[0x2E])) + (3 * (int)(src[0x2F])) + (3 * (int)(src[0x36])) + (int)(src[0x37]) + 8) / 16;
  dst[0xBE] = ((9 * (int)(src[0x2F])) + (3 * (int)(src[0x2E])) + (3 * (int)(src[0x37])) + (int)(src[0x36]) + 8) / 16;
  dst[0xBF] = ((3 * (int)(src[0x2F])) + (int)(src[0x37]) + 2) / 4;
  dst[0xC0] = ((3 * (int)(src[0x30])) + (int)(src[0x28]) + 2) / 4;
  dst[0xC1] = ((9 * (int)(src[0x30])) + (3 * (int)(src[0x31])) + (3 * (int)(src[0x28])) + (int)(src[0x29]) + 8) / 16;
  dst[0xC2] = ((9 * (int)(src[0x31])) + (3 * (int)(src[0x30])) + (3 * (int)(src[0x29])) + (int)(src[0x28]) + 8) / 16;
  dst[0xC3] = ((9 * (int)(src[0x31])) + (3 * (int)(src[0x32])) + (3 * (int)(src[0x29])) + (int)(src[0x2A]) + 8) / 16;
  dst[0xC4] = ((9 * (int)(src[0x32])) + (3 * (int)(src[0x31])) + (3 * (int)(src[0x2A])) + (int)(src[0x29]) + 8) / 16;
  dst[0xC5] = ((9 * (int)(src[0x32])) + (3 * (int)(src[0x33])) + (3 * (int)(src[0x2A])) + (int)(src[0x2B]) + 8) / 16;
  dst[0xC6] = ((9 * (int)(src[0x33])) + (3 * (int)(src[0x32])) + (3 * (int)(src[0x2B])) + (int)(src[0x2A]) + 8) / 16;
  dst[0xC7] = ((9 * (int)(src[0x33])) + (3 * (int)(src[0x34])) + (3 * (int)(src[0x2B])) + (int)(src[0x2C]) + 8) / 16;
  dst[0xC8] = ((9 * (int)(src[0x34])) + (3 * (int)(src[0x33])) + (3 * (int)(src[0x2C])) + (int)(src[0x2B]) + 8) / 16;
  dst[0xC9] = ((9 * (int)(src[0x34])) + (3 * (int)(src[0x35])) + (3 * (int)(src[0x2C])) + (int)(src[0x2D]) + 8) / 16;
  dst[0xCA] = ((9 * (int)(src[0x35])) + (3 * (int)(src[0x34])) + (3 * (int)(src[0x2D])) + (int)(src[0x2C]) + 8) / 16;
  dst[0xCB] = ((9 * (int)(src[0x35])) + (3 * (int)(src[0x36])) + (3 * (int)(src[0x2D])) + (int)(src[0x2E]) + 8) / 16;
  dst[0xCC] = ((9 * (int)(src[0x36])) + (3 * (int)(src[0x35])) + (3 * (int)(src[0x2E])) + (int)(src[0x2D]) + 8) / 16;
  dst[0xCD] = ((9 * (int)(src[0x36])) + (3 * (int)(src[0x37])) + (3 * (int)(src[0x2E])) + (int)(src[0x2F]) + 8) / 16;
  dst[0xCE] = ((9 * (int)(src[0x37])) + (3 * (int)(src[0x36])) + (3 * (int)(src[0x2F])) + (int)(src[0x2E]) + 8) / 16;
  dst[0xCF] = ((3 * (int)(src[0x37])) + (int)(src[0x2F]) + 2) / 4;
  dst[0xD0] = ((3 * (int)(src[0x30])) + (int)(src[0x38]) + 2) / 4;
  dst[0xD1] = ((9 * (int)(src[0x30])) + (3 * (int)(src[0x31])) + (3 * (int)(src[0x38])) + (int)(src[0x39]) + 8) / 16;
  dst[0xD2] = ((9 * (int)(src[0x31])) + (3 * (int)(src[0x30])) + (3 * (int)(src[0x39])) + (int)(src[0x38]) + 8) / 16;
  dst[0xD3] = ((9 * (int)(src[0x31])) + (3 * (int)(src[0x32])) + (3 * (int)(src[0x39])) + (int)(src[0x3A]) + 8) / 16;
  dst[0xD4] = ((9 * (int)(src[0x32])) + (3 * (int)(src[0x31])) + (3 * (int)(src[0x3A])) + (int)(src[0x39]) + 8) / 16;
  dst[0xD5] = ((9 * (int)(src[0x32])) + (3 * (int)(src[0x33])) + (3 * (int)(src[0x3A])) + (int)(src[0x3B]) + 8) / 16;
  dst[0xD6] = ((9 * (int)(src[0x33])) + (3 * (int)(src[0x32])) + (3 * (int)(src[0x3B])) + (int)(src[0x3A]) + 8) / 16;
  dst[0xD7] = ((9 * (int)(src[0x33])) + (3 * (int)(src[0x34])) + (3 * (int)(src[0x3B])) + (int)(src[0x3C]) + 8) / 16;
  dst[0xD8] = ((9 * (int)(src[0x34])) + (3 * (int)(src[0x33])) + (3 * (int)(src[0x3C])) + (int)(src[0x3B]) + 8) / 16;
  dst[0xD9] = ((9 * (int)(src[0x34])) + (3 * (int)(src[0x35])) + (3 * (int)(src[0x3C])) + (int)(src[0x3D]) + 8) / 16;
  dst[0xDA] = ((9 * (int)(src[0x35])) + (3 * (int)(src[0x34])) + (3 * (int)(src[0x3D])) + (int)(src[0x3C]) + 8) / 16;
  dst[0xDB] = ((9 * (int)(src[0x35])) + (3 * (int)(src[0x36])) + (3 * (int)(src[0x3D])) + (int)(src[0x3E]) + 8) / 16;
  dst[0xDC] = ((9 * (int)(src[0x36])) + (3 * (int)(src[0x35])) + (3 * (int)(src[0x3E])) + (int)(src[0x3D]) + 8) / 16;
  dst[0xDD] = ((9 * (int)(src[0x36])) + (3 * (int)(src[0x37])) + (3 * (int)(src[0x3E])) + (int)(src[0x3F]) + 8) / 16;
  dst[0xDE] = ((9 * (int)(src[0x37])) + (3 * (int)(src[0x36])) + (3 * (int)(src[0x3F])) + (int)(src[0x3E]) + 8) / 16;
  dst[0xDF] = ((3 * (int)(src[0x37])) + (int)(src[0x3F]) + 2) / 4;
  dst[0xE0] = ((3 * (int)(src[0x38])) + (int)(src[0x30]) + 2) / 4;
  dst[0xE1] = ((9 * (int)(src[0x38])) + (3 * (int)(src[0x39])) + (3 * (int)(src[0x30])) + (int)(src[0x31]) + 8) / 16;
  dst[0xE2] = ((9 * (int)(src[0x39])) + (3 * (int)(src[0x38])) + (3 * (int)(src[0x31])) + (int)(src[0x30]) + 8) / 16;
  dst[0xE3] = ((9 * (int)(src[0x39])) + (3 * (int)(src[0x3A])) + (3 * (int)(src[0x31])) + (int)(src[0x32]) + 8) / 16;
  dst[0xE4] = ((9 * (int)(src[0x3A])) + (3 * (int)(src[0x39])) + (3 * (int)(src[0x32])) + (int)(src[0x31]) + 8) / 16;
  dst[0xE5] = ((9 * (int)(src[0x3A])) + (3 * (int)(src[0x3B])) + (3 * (int)(src[0x32])) + (int)(src[0x33]) + 8) / 16;
  dst[0xE6] = ((9 * (int)(src[0x3B])) + (3 * (int)(src[0x3A])) + (3 * (int)(src[0x33])) + (int)(src[0x32]) + 8) / 16;
  dst[0xE7] = ((9 * (int)(src[0x3B])) + (3 * (int)(src[0x3C])) + (3 * (int)(src[0x33])) + (int)(src[0x34]) + 8) / 16;
  dst[0xE8] = ((9 * (int)(src[0x3C])) + (3 * (int)(src[0x3B])) + (3 * (int)(src[0x34])) + (int)(src[0x33]) + 8) / 16;
  dst[0xE9] = ((9 * (int)(src[0x3C])) + (3 * (int)(src[0x3D])) + (3 * (int)(src[0x34])) + (int)(src[0x35]) + 8) / 16;
  dst[0xEA] = ((9 * (int)(src[0x3D])) + (3 * (int)(src[0x3C])) + (3 * (int)(src[0x35])) + (int)(src[0x34]) + 8) / 16;
  dst[0xEB] = ((9 * (int)(src[0x3D])) + (3 * (int)(src[0x3E])) + (3 * (int)(src[0x35])) + (int)(src[0x36]) + 8) / 16;
  dst[0xEC] = ((9 * (int)(src[0x3E])) + (3 * (int)(src[0x3D])) + (3 * (int)(src[0x36])) + (int)(src[0x35]) + 8) / 16;
  dst[0xED] = ((9 * (int)(src[0x3E])) + (3 * (int)(src[0x3F])) + (3 * (int)(src[0x36])) + (int)(src[0x37]) + 8) / 16;
  dst[0xEE] = ((9 * (int)(src[0x3F])) + (3 * (int)(src[0x3E])) + (3 * (int)(src[0x37])) + (int)(src[0x36]) + 8) / 16;
  dst[0xEF] = ((3 * (int)(src[0x3F])) + (int)(src[0x37]) + 2) / 4;
  dst[0xF0] = src[0x38];
  dst[0xF1] = ((3 * (int)(src[0x38])) + (int)(src[0x39]) + 2) / 4;
  dst[0xF2] = ((3 * (int)(src[0x39])) + (int)(src[0x38]) + 2) / 4;
  dst[0xF3] = ((3 * (int)(src[0x39])) + (int)(src[0x3A]) + 2) / 4;
  dst[0xF4] = ((3 * (int)(src[0x3A])) + (int)(src[0x39]) + 2) / 4;
  dst[0xF5] = ((3 * (int)(src[0x3A])) + (int)(src[0x3B]) + 2) / 4;
  dst[0xF6] = ((3 * (int)(src[0x3B])) + (int)(src[0x3A]) + 2) / 4;
  dst[0xF7] = ((3 * (int)(src[0x3B])) + (int)(src[0x3C]) + 2) / 4;
  dst[0xF8] = ((3 * (int)(src[0x3C])) + (int)(src[0x3B]) + 2) / 4;
  dst[0xF9] = ((3 * (int)(src[0x3C])) + (int)(src[0x3D]) + 2) / 4;
  dst[0xFA] = ((3 * (int)(src[0x3D])) + (int)(src[0x3C]) + 2) / 4;
  dst[0xFB] = ((3 * (int)(src[0x3D])) + (int)(src[0x3E]) + 2) / 4;
  dst[0xFC] = ((3 * (int)(src[0x3E])) + (int)(src[0x3D]) + 2) / 4;
  dst[0xFD] = ((3 * (int)(src[0x3E])) + (int)(src[0x3F]) + 2) / 4;
  dst[0xFE] = ((3 * (int)(src[0x3F])) + (int)(src[0x3E]) + 2) / 4;
  dst[0xFF] = src[0x3F];

  // clang-format on
}

static void  //
basic_handsum_decode__scale_horizontal(
    basic_handsum_decode__pixel_buffer* pixbuf) {
  uint8_t dst[1024];
  uint32_t w = pixbuf->width;
  const uint8_t* src = pixbuf->bgra_pixels;

  for (int y = 0; y < 16; y++) {
    int dstx = 0;
    uint32_t acc0 = 0;
    uint32_t acc1 = 0;
    uint32_t acc2 = 0;
    uint32_t acc3 = 0;
    uint32_t remainder = 16;
    for (int srcx = 0; srcx < 16; srcx++) {
      int si = (64 * y) + (4 * srcx);
      uint32_t s0 = src[si + 0];
      uint32_t s1 = src[si + 1];
      uint32_t s2 = src[si + 2];
      uint32_t s3 = src[si + 3];

      if (remainder > w) {
        remainder -= w;
        acc0 += w * s0;
        acc1 += w * s1;
        acc2 += w * s2;
        acc3 += w * s3;

      } else {
        acc0 += remainder * s0;
        acc1 += remainder * s1;
        acc2 += remainder * s2;
        acc3 += remainder * s3;

        int di = (4 * w * y) + (4 * dstx);
        dst[di + 0] = (acc0 + 8) / 16;
        dst[di + 1] = (acc1 + 8) / 16;
        dst[di + 2] = (acc2 + 8) / 16;
        dst[di + 3] = (acc3 + 8) / 16;
        dstx++;

        uint32_t partial = w - remainder;

        acc0 = partial * s0;
        acc1 = partial * s1;
        acc2 = partial * s2;
        acc3 = partial * s3;

        remainder = 16 - partial;
      }
    }
  }

  for (int y = 0; y < 16; y++) {
    memcpy(&pixbuf->bgra_pixels[4 * w * y], &dst[4 * w * y], 4 * w);
  }
}

static void  //
basic_handsum_decode__scale_vertical(
    basic_handsum_decode__pixel_buffer* pixbuf) {
  uint8_t dst[1024];
  uint32_t h = pixbuf->height;
  const uint8_t* src = pixbuf->bgra_pixels;

  for (int x = 0; x < 16; x++) {
    int dsty = 0;
    uint32_t acc0 = 0;
    uint32_t acc1 = 0;
    uint32_t acc2 = 0;
    uint32_t acc3 = 0;
    uint32_t remainder = 16;
    for (int srcy = 0; srcy < 16; srcy++) {
      int si = (64 * srcy) + (4 * x);
      uint32_t s0 = src[si + 0];
      uint32_t s1 = src[si + 1];
      uint32_t s2 = src[si + 2];
      uint32_t s3 = src[si + 3];

      if (remainder > h) {
        remainder -= h;
        acc0 += h * s0;
        acc1 += h * s1;
        acc2 += h * s2;
        acc3 += h * s3;

      } else {
        acc0 += remainder * s0;
        acc1 += remainder * s1;
        acc2 += remainder * s2;
        acc3 += remainder * s3;

        int di = (64 * dsty) + (4 * x);
        dst[di + 0] = (acc0 + 8) / 16;
        dst[di + 1] = (acc1 + 8) / 16;
        dst[di + 2] = (acc2 + 8) / 16;
        dst[di + 3] = (acc3 + 8) / 16;
        dsty++;

        uint32_t partial = h - remainder;

        acc0 = partial * s0;
        acc1 = partial * s1;
        acc2 = partial * s2;
        acc3 = partial * s3;

        remainder = 16 - partial;
      }
    }
  }

  memcpy(&pixbuf->bgra_pixels[0], &dst[0], 64 * h);
}

static void  //
basic_handsum_decode__convert_ycbcr_bgra(
    basic_handsum_decode__pixel_buffer* dst,
    const uint8_t* restrict yy,
    const uint8_t* restrict cb,
    const uint8_t* restrict cr) {
  for (int i = 0; i < 256; i++) {
    int32_t yy1 = (int32_t)(yy[i]) * 0x10101;
    int32_t cb1 = (int32_t)(cb[i]) - 128;
    int32_t cr1 = (int32_t)(cr[i]) - 128;

    int32_t r = yy1 + (91881 * cr1);
    if (((uint32_t)(r)&0xFF000000u) == 0) {
      r >>= 16;
    } else {
      r = -1 ^ (r >> 31);
    }

    int32_t g = yy1 - (22554 * cb1) - (46802 * cr1);
    if (((uint32_t)(g)&0xFF000000u) == 0) {
      g >>= 16;
    } else {
      g = -1 ^ (g >> 31);
    }

    int32_t b = yy1 + (116130 * cb1);
    if (((uint32_t)(b)&0xFF000000u) == 0) {
      b >>= 16;
    } else {
      b = -1 ^ (b >> 31);
    }

    dst->bgra_pixels[(4 * i) + 0] = b;
    dst->bgra_pixels[(4 * i) + 1] = g;
    dst->bgra_pixels[(4 * i) + 2] = r;
    dst->bgra_pixels[(4 * i) + 3] = 0xFF;
  }
}

BASIC_HANDSUM_DECODE__MAYBE_STATIC int  //
basic_handsum_decode__decode(basic_handsum_decode__pixel_buffer* dst,
                             const uint8_t* src_ptr,
                             const size_t src_len) {
  if (dst == NULL) {
    return BASIC_HANDSUM_DECODE__RESULT__DST_IS_NULL;
  }
  dst->width = 0;
  dst->height = 0;

  if (src_len < 147) {
    return BASIC_HANDSUM_DECODE__RESULT__SRC_IS_TOO_SHORT;
  } else if ((src_ptr[0] != 0xFE) ||           //
             (src_ptr[1] != 0xD7) ||           //
             ((src_ptr[2] & 0xE0) != 0x60) ||  //
             ((src_ptr[2] & 0x1F) == 0x1F)) {
    return BASIC_HANDSUM_DECODE__RESULT__SRC_IS_NOT_HANDSUM_C3Q4;
  }

  if ((src_ptr[2] & 0x10) == 0x00) {  // Landscape.
    dst->width = 16;
    dst->height = 1 + (src_ptr[2] & 0x0F);
  } else {  // Portrait.
    dst->width = 1 + (src_ptr[2] & 0x0F);
    dst->height = 16;
  }

  uint8_t luma_quad_block[256];
  uint8_t tmp[64];
  uint8_t cb_quad_block[256];
  uint8_t cr_quad_block[256];
  size_t bit_offset = 24;

  bit_offset = basic_handsum_decode__decode_block_q4(  //
      &luma_quad_block[0x00], 16, src_ptr, bit_offset);
  bit_offset = basic_handsum_decode__decode_block_q4(  //
      &luma_quad_block[0x08], 16, src_ptr, bit_offset);
  bit_offset = basic_handsum_decode__decode_block_q4(  //
      &luma_quad_block[0x80], 16, src_ptr, bit_offset);
  bit_offset = basic_handsum_decode__decode_block_q4(  //
      &luma_quad_block[0x88], 16, src_ptr, bit_offset);

  basic_handsum_decode__smooth_block_seams_16x16(&luma_quad_block[0]);

  bit_offset = basic_handsum_decode__decode_block_q4(  //
      &tmp[0], 8, src_ptr, bit_offset);

  basic_handsum_decode__scale_and_bias_chroma_down(&tmp[0]);
  basic_handsum_decode__upsample_from(&cb_quad_block[0], &tmp[0]);

  bit_offset = basic_handsum_decode__decode_block_q4(  //
      &tmp[0], 8, src_ptr, bit_offset);

  basic_handsum_decode__scale_and_bias_chroma_down(&tmp[0]);
  basic_handsum_decode__upsample_from(&cr_quad_block[0], &tmp[0]);

  basic_handsum_decode__convert_ycbcr_bgra(
      dst, &luma_quad_block[0], &cb_quad_block[0], &cr_quad_block[0]);

  if (dst->width < 16) {
    basic_handsum_decode__scale_horizontal(dst);
  } else if (dst->height < 16) {
    basic_handsum_decode__scale_vertical(dst);
  }

  return BASIC_HANDSUM_DECODE__RESULT__OK;
}

#endif  // BASIC_HANDSUM_DECODE_IMPLEMENTATION
#endif  // BASIC_HANDSUM_DECODE_INCLUDE_GUARD
