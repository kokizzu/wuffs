// Copyright 2024 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

// ----------------

/*
stb-imagedumper decodes images, printing them (as ANSI color codes) to stdout.

It demonstrates using the STB Image API (e.g. stbi_load and stbi_image_free)
with either the actual STB Image implementation or Wuffs' reimplementation. By
default, it uses Wuffs' reimplementation. Define the USE_THE_REAL_STBI macro to
use the actual STB Image implementation instead: the stb_image.h file from
https://github.com/nothings/stb

The STB Image API is simple, covering the basic "decode this image data (as a
file, as bytes, etc) as RGB pixels" need. Wuffs also has its own API. Two, in
fact: a higher-level C++ one and a lower-level C one.

Wuffs' C++ API is also a one-shot "decode an image, synchronously" API but also
gives the caller access to metadata (e.g. EXIF orientation, color profiles),
more control over destination pixel formats (e.g. RGB versus BGR byte order,
premultiplied versus non-premultiplied alpha) and over memory allocators.

Wuffs' C API provides all of that (since the C++ API is implemented using the C
API) but also supports multi-shot streaming (asynchronous) I/O and animated
(not just still, single-frame) images.

See example/convert-to-nia and example/imageviewer in the Wuffs repository for
examples of using its C and C++ APIs.

Coming back to this program, if -demo is passed, it decodes four versions of
the same image of the Mona Lisa, in Thumbhash, PKM/ETC2, JPEG and PNG formats.
Only the last one is lossless.

As of September 2024, the actual STB Image implementation only decodes two out
of four: JPEG and PNG. Wuffs' reimplementation decodes all four demo images.

To run:

$CC stb-imagedumper.c && ./a.out *.{jpeg,png}; rm -f a.out

for a C compiler $CC, such as clang or gcc.
*/

#include <stdio.h>

#if defined(USE_THE_REAL_STBI)

#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STB_IMAGE_IMPLEMENTATION
#include "/path/to/your/copy/of/github.com/nothings/stb/stb_image.h"

#else  // defined(USE_THE_REAL_STBI)

// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// To use that single file as a "foo.c"-like implementation, instead of a
// "foo.h"-like header, #define WUFFS_IMPLEMENTATION before #include'ing or
// compiling it.
#define WUFFS_IMPLEMENTATION

// Defining the WUFFS_CONFIG__STATIC_FUNCTIONS macro is optional, but when
// combined with WUFFS_IMPLEMENTATION, it demonstrates making all of Wuffs'
// functions have static storage.
//
// This can help the compiler ignore or discard unused code, which can produce
// faster compiles and smaller binaries. Other motivations are discussed in the
// "ALLOW STATIC IMPLEMENTATION" section of
// https://raw.githubusercontent.com/nothings/stb/master/docs/stb_howto.txt
#define WUFFS_CONFIG__STATIC_FUNCTIONS

// Defining the WUFFS_CONFIG__MODULE* macros are optional, but it lets users of
// release/c/etc.c choose which parts of Wuffs to build. That file contains the
// entire Wuffs standard library, implementing a variety of codecs and file
// formats. Without this macro definition, an optimizing compiler or linker may
// very well discard Wuffs code for unused codecs, but listing the Wuffs
// modules we use makes that process explicit. Preprocessing means that such
// code simply isn't compiled.
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__ADLER32
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__BMP
#define WUFFS_CONFIG__MODULE__CRC32
#define WUFFS_CONFIG__MODULE__DEFLATE
#define WUFFS_CONFIG__MODULE__ETC2
#define WUFFS_CONFIG__MODULE__GIF
#define WUFFS_CONFIG__MODULE__JPEG
#define WUFFS_CONFIG__MODULE__NETPBM
#define WUFFS_CONFIG__MODULE__NIE
#define WUFFS_CONFIG__MODULE__PNG
#define WUFFS_CONFIG__MODULE__QOI
#define WUFFS_CONFIG__MODULE__TARGA
#define WUFFS_CONFIG__MODULE__THUMBHASH
#define WUFFS_CONFIG__MODULE__VP8
#define WUFFS_CONFIG__MODULE__WBMP
#define WUFFS_CONFIG__MODULE__WEBP
#define WUFFS_CONFIG__MODULE__ZLIB

// Defining this enables Wuffs' reimplementation of the STB library's API.
#define WUFFS_CONFIG__ENABLE_DROP_IN_REPLACEMENT__STB

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../release/c/wuffs-unsupported-snapshot.c"

#endif  // defined(USE_THE_REAL_STBI)

// ----------------

const uint8_t g_src_ptr__mona_lisa_21_32_th[] = {
    0xc3, 0xbe, 0xfe, 0xd1, 0x18, 0x0a, 0x1d, 0x02, 0x78, 0x95, 0x7f, 0x88,
    0x87, 0x88, 0x87, 0x88, 0x77, 0x87, 0x47, 0x49, 0x7f, 0xa6, 0x02, 0x17,
};
const size_t g_src_len__mona_lisa_21_32_th = 24;

const uint8_t g_src_ptr__mona_lisa_21_32_etc2_pkm[] = {
    0x50, 0x4b, 0x4d, 0x20, 0x32, 0x30, 0x00, 0x01, 0x00, 0x18, 0x00, 0x20,
    0x00, 0x15, 0x00, 0x20, 0xb2, 0x74, 0x15, 0x32, 0x74, 0x8c, 0xb3, 0x18,
    0xae, 0x70, 0x14, 0xaf, 0x70, 0x84, 0xf3, 0x58, 0xbb, 0xf2, 0x29, 0x92,
    0x88, 0x80, 0x99, 0x1b, 0x63, 0x72, 0x42, 0x0b, 0x51, 0x19, 0x33, 0x2a,
    0xac, 0x70, 0x15, 0x2f, 0x78, 0x9c, 0xf3, 0x18, 0xae, 0x72, 0x15, 0xaf,
    0x72, 0x9c, 0xf3, 0xd9, 0x97, 0x97, 0x65, 0x09, 0x99, 0x9f, 0xe6, 0x09,
    0x96, 0x94, 0x65, 0x4e, 0xe8, 0x08, 0xc3, 0x48, 0xb8, 0x96, 0x43, 0x70,
    0x71, 0x99, 0x12, 0x23, 0x37, 0x27, 0x25, 0x6c, 0x8c, 0xe7, 0x2d, 0x11,
    0x8a, 0x91, 0x60, 0x66, 0xf1, 0x0c, 0x0c, 0x08, 0x96, 0x9e, 0x60, 0x07,
    0x99, 0x99, 0x22, 0x22, 0x42, 0x42, 0x31, 0x22, 0xcc, 0x8e, 0xd9, 0x19,
    0x57, 0x54, 0x3e, 0x2a, 0xe2, 0x67, 0xd8, 0x80, 0xb7, 0x85, 0x42, 0x71,
    0xfa, 0xa8, 0xa1, 0x69, 0x24, 0x14, 0x23, 0x08, 0x0f, 0xf0, 0x1e, 0x0b,
    0x44, 0x06, 0xa2, 0xa2, 0xee, 0xef, 0x18, 0x96, 0x66, 0x6d, 0x56, 0x27,
    0x22, 0x22, 0x33, 0x33, 0x53, 0x48, 0x3e, 0x27, 0x00, 0xdf, 0xd6, 0x01,
    0x0c, 0x22, 0x76, 0x4b, 0x41, 0x5f, 0xc1, 0x46, 0x8d, 0x6b, 0x35, 0x85,
    0x11, 0x17, 0x2a, 0xe9, 0x99, 0x05, 0x42, 0x93, 0x88, 0x8f, 0xa7, 0x91,
    0x6f, 0x66, 0x46, 0x2f, 0x30, 0x9f, 0xc6, 0xdb, 0x63, 0x5a, 0x48, 0x07,
    0x66, 0x66, 0xcc, 0xcc, 0x42, 0x15, 0xa9, 0x9a, 0xc0, 0x4f, 0x35, 0x36,
    0xf3, 0x84, 0x42, 0x27, 0xcd, 0xfe, 0x22, 0x11, 0xb4, 0x93, 0x52, 0x85,
    0xe2, 0x2a, 0xe1, 0x56, 0x15, 0x63, 0x32, 0x27, 0x32, 0xee, 0xdd, 0x1c,
    0x25, 0x13, 0x23, 0x24, 0x0f, 0x0b, 0x3c, 0x04, 0x14, 0x64, 0x64, 0x43,
    0xee, 0xee, 0x44, 0x44, 0x48, 0x3f, 0x30, 0x27, 0x7e, 0xe8, 0xb6, 0xd0,
    0x33, 0x22, 0x28, 0x2f, 0x77, 0x44, 0x4c, 0x03, 0x36, 0x27, 0x28, 0x22,
    0xfc, 0xcc, 0x89, 0x90, 0x05, 0x12, 0x21, 0x23, 0xf0, 0x20, 0x11, 0x10,
    0x21, 0x19, 0x29, 0x02, 0x8c, 0xef, 0x38, 0xcb, 0x07, 0x34, 0x54, 0x43,
    0x77, 0x77, 0x77, 0x77, 0x36, 0x27, 0x30, 0x43, 0xee, 0xf8, 0xd4, 0xcc,
    0x5a, 0x37, 0x23, 0x6c, 0xc6, 0x2f, 0x93, 0x94, 0x96, 0x63, 0x32, 0x50,
    0xb1, 0x98, 0x13, 0xbb, 0xa9, 0x15, 0x10, 0x92, 0xb9, 0x99, 0x57, 0x33,
    0x27, 0x1f, 0x36, 0x03, 0x08, 0xdf, 0xd0, 0xab, 0x32, 0x28, 0x3f, 0x03,
    0xee, 0xee, 0x00, 0x00, 0x8e, 0x90, 0x0d, 0x0f, 0x0e, 0x40, 0x81, 0x09,
    0xb1, 0x15, 0x10, 0x92, 0xee, 0xef, 0xdc, 0xce, 0x34, 0x1e, 0x27, 0x23,
    0xfe, 0x20, 0x62, 0x1c, 0x8a, 0x88, 0x07, 0x87, 0x06, 0x30, 0x40, 0x86,
    0x0a, 0x01, 0x18, 0x02, 0xcc, 0xc4, 0x33, 0x10, 0x98, 0x9a, 0x0d, 0x9a,
    0x1a, 0x58, 0x60, 0x47,
};
const size_t g_src_len__mona_lisa_21_32_etc2_pkm = 400;

const uint8_t g_src_ptr__mona_lisa_21_32_q50_jpeg[] = {
    0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46, 0x00, 0x01,
    0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xff, 0xdb, 0x00, 0x43,
    0x00, 0x10, 0x0b, 0x0c, 0x0e, 0x0c, 0x0a, 0x10, 0x0e, 0x0d, 0x0e, 0x12,
    0x11, 0x10, 0x13, 0x18, 0x28, 0x1a, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23,
    0x25, 0x1d, 0x28, 0x3a, 0x33, 0x3d, 0x3c, 0x39, 0x33, 0x38, 0x37, 0x40,
    0x48, 0x5c, 0x4e, 0x40, 0x44, 0x57, 0x45, 0x37, 0x38, 0x50, 0x6d, 0x51,
    0x57, 0x5f, 0x62, 0x67, 0x68, 0x67, 0x3e, 0x4d, 0x71, 0x79, 0x70, 0x64,
    0x78, 0x5c, 0x65, 0x67, 0x63, 0xff, 0xdb, 0x00, 0x43, 0x01, 0x11, 0x12,
    0x12, 0x18, 0x15, 0x18, 0x2f, 0x1a, 0x1a, 0x2f, 0x63, 0x42, 0x38, 0x42,
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
    0x63, 0x63, 0xff, 0xc0, 0x00, 0x11, 0x08, 0x00, 0x20, 0x00, 0x15, 0x03,
    0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01, 0xff, 0xc4, 0x00,
    0x1f, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0xff, 0xc4, 0x00, 0xb5, 0x10, 0x00,
    0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00,
    0x00, 0x01, 0x7d, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21,
    0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81,
    0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24,
    0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25,
    0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a,
    0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56,
    0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86,
    0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
    0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3,
    0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6,
    0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
    0xda, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1,
    0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xff, 0xc4, 0x00,
    0x1f, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0xff, 0xc4, 0x00, 0xb5, 0x11, 0x00,
    0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00,
    0x01, 0x02, 0x77, 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31,
    0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08,
    0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0, 0x15,
    0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18,
    0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55,
    0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84,
    0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa,
    0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4,
    0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
    0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xff, 0xda, 0x00,
    0x0c, 0x03, 0x01, 0x00, 0x02, 0x11, 0x03, 0x11, 0x00, 0x3f, 0x00, 0x42,
    0x96, 0xfe, 0x53, 0x49, 0xbd, 0x48, 0x51, 0xb8, 0x80, 0x6a, 0xac, 0xee,
    0x2d, 0xaf, 0xe3, 0x82, 0x6f, 0x29, 0xa3, 0x90, 0x7c, 0xae, 0x99, 0xe0,
    0xfa, 0x1a, 0xce, 0x90, 0x30, 0x80, 0x03, 0xb9, 0x9d, 0xbe, 0x55, 0xdb,
    0xfd, 0xee, 0xd5, 0x6f, 0x5a, 0xb4, 0xbe, 0x4f, 0xb2, 0xdc, 0x5d, 0x10,
    0x58, 0x7f, 0x02, 0x64, 0xe3, 0xb9, 0xae, 0x5b, 0x25, 0x24, 0xae, 0x68,
    0xa1, 0xa3, 0xd0, 0xd3, 0xfb, 0x3c, 0x41, 0x41, 0xca, 0xe0, 0x8c, 0x83,
    0xeb, 0x45, 0x66, 0x47, 0xe6, 0xbc, 0x6a, 0xc8, 0xec, 0xea, 0x47, 0x1b,
    0x4e, 0x71, 0x45, 0x55, 0xbc, 0xc8, 0xb3, 0xec, 0x67, 0x4d, 0x33, 0x9f,
    0x2d, 0xa2, 0x0e, 0x7c, 0xbe, 0x84, 0x73, 0xcf, 0xaf, 0x15, 0x1d, 0xe6,
    0xa7, 0x3d, 0xd3, 0x22, 0xcf, 0x2b, 0xca, 0x8a, 0x30, 0x14, 0xb7, 0x15,
    0x1d, 0x85, 0xf1, 0xb5, 0x9c, 0xca, 0x4b, 0x9e, 0x0e, 0x00, 0x38, 0xe6,
    0x9b, 0x7b, 0x7e, 0x97, 0x12, 0x99, 0x3c, 0x84, 0xdc, 0x47, 0x27, 0x15,
    0xb7, 0x2d, 0xa5, 0x6b, 0x1a, 0x37, 0x75, 0x7b, 0x96, 0x2c, 0x66, 0xdb,
    0x11, 0x01, 0x88, 0xe7, 0xb1, 0xc5, 0x15, 0x46, 0x19, 0x80, 0x5e, 0x68,
    0xab, 0x71, 0x42, 0x4d, 0x9f, 0xff, 0xd9,
};
const size_t g_src_len__mona_lisa_21_32_q50_jpeg = 799;

const uint8_t g_src_ptr__mona_lisa_21_32_png[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x20,
    0x08, 0x02, 0x00, 0x00, 0x00, 0x72, 0xc2, 0xa4, 0xdf, 0x00, 0x00, 0x06,
    0x07, 0x49, 0x44, 0x41, 0x54, 0x38, 0x11, 0x05, 0xc1, 0x49, 0x6f, 0x1c,
    0xc7, 0x15, 0x00, 0xe0, 0xf7, 0x5e, 0x55, 0x75, 0x4f, 0xcf, 0xc6, 0x19,
    0x92, 0xe2, 0x2a, 0x79, 0xd3, 0x62, 0x51, 0x4b, 0x64, 0x05, 0x50, 0x60,
    0x04, 0xc9, 0xd5, 0xc8, 0x21, 0x08, 0x92, 0x8b, 0x91, 0x6b, 0x72, 0x09,
    0xf2, 0x07, 0xfc, 0x7b, 0x72, 0xcf, 0x3d, 0x97, 0x9c, 0x02, 0x23, 0x80,
    0x01, 0x5b, 0xb6, 0xe1, 0x48, 0x72, 0x44, 0x59, 0x1c, 0x6e, 0xe2, 0x32,
    0xe4, 0xec, 0x3d, 0xdd, 0x5d, 0x5d, 0xf5, 0xde, 0xf3, 0xf7, 0xe1, 0x5f,
    0xbf, 0xf8, 0xa3, 0xb0, 0x17, 0x40, 0x00, 0x46, 0xa2, 0x10, 0x02, 0x21,
    0x00, 0x9a, 0x06, 0x29, 0xa3, 0x51, 0xb4, 0x51, 0x18, 0xd5, 0x8b, 0x88,
    0x04, 0x8e, 0x40, 0x8a, 0x46, 0xc9, 0x91, 0x31, 0x84, 0xce, 0x88, 0xd8,
    0x1b, 0x2d, 0x65, 0x21, 0x16, 0x01, 0xb4, 0x28, 0x50, 0x13, 0x01, 0xaa,
    0xb5, 0x64, 0x38, 0xa8, 0x73, 0xe8, 0x5c, 0xac, 0x09, 0x94, 0xc8, 0x19,
    0x08, 0x8c, 0x28, 0xa0, 0xa0, 0x36, 0x33, 0x04, 0xc6, 0xa0, 0xd4, 0x62,
    0x93, 0x24, 0x51, 0xb0, 0x48, 0xa0, 0xe0, 0x48, 0x23, 0xb0, 0x63, 0x15,
    0x4b, 0x60, 0x4c, 0xc6, 0x62, 0x35, 0x30, 0x35, 0x9c, 0xb8, 0x86, 0xa2,
    0xd5, 0xba, 0x04, 0x11, 0x40, 0x8b, 0x2e, 0x23, 0x14, 0x82, 0xa0, 0x36,
    0xb5, 0x69, 0x23, 0x23, 0x10, 0x03, 0x41, 0x15, 0x10, 0x53, 0x6b, 0x33,
    0xe1, 0xd2, 0xd8, 0x74, 0xbc, 0x7f, 0x74, 0xf5, 0x6a, 0x70, 0x32, 0x44,
    0xaf, 0x74, 0xeb, 0xc1, 0xd6, 0x83, 0xdf, 0xdc, 0xab, 0x84, 0x54, 0x8c,
    0x49, 0xb3, 0x94, 0x82, 0x35, 0xc4, 0x62, 0x04, 0x9d, 0xf9, 0xec, 0xb3,
    0x5f, 0x34, 0x5b, 0x59, 0x9a, 0x38, 0x4b, 0xd4, 0x6c, 0x24, 0x96, 0xb0,
    0xd9, 0x6e, 0xe7, 0x97, 0xcb, 0x7e, 0x7e, 0xfe, 0xeb, 0x5f, 0x6d, 0x43,
    0xf4, 0x2f, 0x5e, 0x2f, 0xbf, 0xfd, 0xfa, 0xaa, 0xd7, 0x6f, 0xdc, 0x7e,
    0xfc, 0x3e, 0x28, 0x35, 0xb3, 0xb4, 0xd9, 0x30, 0xa8, 0x6a, 0x8c, 0x71,
    0xd6, 0xda, 0x76, 0x27, 0x71, 0xce, 0x24, 0x49, 0x33, 0xb1, 0xa0, 0xa1,
    0x0e, 0xac, 0xac, 0x6e, 0xf6, 0xe6, 0xf0, 0xf3, 0xcf, 0x1f, 0xf6, 0xd7,
    0x3b, 0x6b, 0x6b, 0x66, 0x34, 0xf6, 0xd7, 0x73, 0xfc, 0xcf, 0xbf, 0x0f,
    0xee, 0x3e, 0x7d, 0xaf, 0xdb, 0x6d, 0x38, 0x47, 0x8e, 0xb2, 0xd4, 0xa5,
    0x12, 0x4a, 0x4a, 0x32, 0xb3, 0xf7, 0xe8, 0xfd, 0x32, 0x2f, 0x8c, 0x32,
    0x87, 0x4a, 0xa0, 0xb1, 0xb1, 0xb9, 0x76, 0x35, 0x38, 0x3d, 0xfb, 0xe1,
    0xf8, 0xc1, 0xc7, 0xbd, 0xf1, 0x28, 0x3f, 0x3f, 0x2f, 0x98, 0x75, 0xff,
    0xa8, 0x1e, 0xcd, 0x95, 0xd2, 0x1a, 0x33, 0xeb, 0x05, 0x6e, 0xb4, 0xa9,
    0x9d, 0x61, 0xe9, 0xe3, 0x68, 0x01, 0xf6, 0xf5, 0xcb, 0xd1, 0xce, 0x07,
    0x2b, 0xf3, 0x69, 0xbe, 0x58, 0x94, 0x9d, 0x5e, 0x9b, 0xe8, 0xa3, 0xa3,
    0xc1, 0x3c, 0x7a, 0x3a, 0x1e, 0x8c, 0x22, 0x87, 0xe9, 0x2c, 0xa4, 0xce,
    0xb6, 0x1a, 0xce, 0x80, 0x0e, 0x87, 0x01, 0xdf, 0x95, 0xe6, 0xdc, 0xef,
    0x76, 0x37, 0x7b, 0xad, 0xd4, 0xfb, 0x98, 0x97, 0xce, 0x66, 0x16, 0xc7,
    0x93, 0x3a, 0x31, 0x75, 0xa8, 0x65, 0x34, 0x99, 0x5c, 0x9e, 0x7c, 0x33,
    0x3d, 0x5c, 0x3c, 0x5e, 0xd3, 0xc7, 0x9f, 0xec, 0x1e, 0x1c, 0x4c, 0x9e,
    0x3e, 0x69, 0xff, 0xf7, 0xab, 0xa3, 0x7c, 0x19, 0x8c, 0x33, 0x12, 0x38,
    0x51, 0xf1, 0x62, 0x06, 0x43, 0x5f, 0x2c, 0xc3, 0x2c, 0xaf, 0x2b, 0x0f,
    0x76, 0x3c, 0x2a, 0x9d, 0x8f, 0xad, 0x26, 0x20, 0x11, 0x2a, 0x2d, 0x72,
    0x3f, 0x9b, 0x87, 0xb7, 0x8b, 0x78, 0x3a, 0xb8, 0x7a, 0xfd, 0xe3, 0xa4,
    0xf6, 0xfe, 0xc7, 0x37, 0x0b, 0x56, 0x21, 0xe3, 0x16, 0x53, 0x7f, 0x79,
    0xb6, 0xa0, 0x24, 0x7d, 0x51, 0xd6, 0x6f, 0x9b, 0xce, 0xcf, 0x7d, 0x62,
    0x52, 0xaa, 0x4b, 0x6f, 0x85, 0x31, 0x88, 0x9f, 0x95, 0x75, 0x15, 0x6d,
    0x6a, 0xd7, 0xd7, 0xd3, 0xb3, 0xcb, 0xfa, 0xdb, 0xff, 0x4d, 0xef, 0xdf,
    0xeb, 0x1e, 0x1d, 0xcf, 0xce, 0x2e, 0xbc, 0x2a, 0xa2, 0x48, 0x42, 0xd8,
    0x69, 0x52, 0x6a, 0xd5, 0xd7, 0x5a, 0x2e, 0x82, 0x32, 0x29, 0x83, 0xd9,
    0xd9, 0xd8, 0x08, 0xec, 0xad, 0x4d, 0xc5, 0x38, 0x9b, 0xa5, 0xab, 0x9b,
    0x7d, 0x27, 0x32, 0x39, 0x9e, 0xbc, 0x38, 0xac, 0x22, 0xcb, 0x0f, 0xaf,
    0x66, 0xc3, 0x19, 0xb3, 0x4a, 0x5e, 0x2b, 0x26, 0xa6, 0xb7, 0xd3, 0xb1,
    0x96, 0xb8, 0x8c, 0xbe, 0x8a, 0x49, 0x33, 0xb3, 0x48, 0xd6, 0x64, 0x4d,
    0xdb, 0x36, 0x49, 0x3b, 0xcb, 0x1a, 0x40, 0xb6, 0x91, 0x3a, 0x57, 0x1a,
    0xb4, 0x8d, 0xe4, 0x7a, 0x59, 0xcf, 0x4a, 0x79, 0xfa, 0x49, 0x3f, 0x04,
    0x7f, 0x72, 0x9c, 0x7f, 0xf9, 0x7d, 0xcd, 0x55, 0x58, 0x8c, 0xe6, 0x2b,
    0xab, 0xcd, 0x76, 0xcb, 0xde, 0xec, 0x42, 0x5e, 0x56, 0x00, 0xa9, 0xdd,
    0xed, 0x58, 0xbf, 0xd2, 0x5d, 0xb5, 0x53, 0x3b, 0x1b, 0x5d, 0x4c, 0xe9,
    0x5d, 0x21, 0xbe, 0x92, 0xb2, 0xd6, 0xdf, 0xfd, 0xb6, 0xf7, 0xa7, 0x3f,
    0x7c, 0x98, 0xa5, 0x48, 0x0e, 0x9f, 0x7f, 0x35, 0xb8, 0xbe, 0x1c, 0x9f,
    0xe4, 0x30, 0xda, 0x9f, 0xdd, 0x78, 0x86, 0x1f, 0xdf, 0xec, 0x64, 0xc8,
    0xbe, 0x32, 0xbe, 0x62, 0x7b, 0x43, 0xf2, 0xea, 0x7a, 0xbc, 0xb1, 0x6d,
    0x4c, 0xdf, 0xce, 0xe7, 0x15, 0x93, 0x5e, 0x8f, 0xe1, 0xe1, 0x8e, 0xfc,
    0xfd, 0x2f, 0x7b, 0xab, 0x1b, 0xeb, 0x65, 0x51, 0x20, 0x41, 0x2b, 0x01,
    0x90, 0x68, 0x10, 0xb7, 0x9b, 0x36, 0x8e, 0xca, 0xb5, 0x87, 0x5d, 0x01,
    0x77, 0xcb, 0x25, 0x17, 0x17, 0x62, 0x9e, 0xdd, 0xd9, 0x46, 0x60, 0x61,
    0xd3, 0xef, 0x63, 0x3b, 0x89, 0xfb, 0x87, 0x04, 0x91, 0xbf, 0xf8, 0xdb,
    0xad, 0xbb, 0x8f, 0xde, 0x63, 0x4e, 0x4c, 0xd2, 0x50, 0x88, 0x83, 0xd7,
    0xa7, 0xf3, 0xa5, 0x3c, 0x79, 0xd4, 0xce, 0x1a, 0xe6, 0xbb, 0x97, 0xe1,
    0xf2, 0xb4, 0x6c, 0x52, 0xd2, 0xea, 0xb5, 0xb6, 0x37, 0xad, 0xd5, 0xda,
    0x6c, 0xad, 0x10, 0x23, 0x69, 0xc9, 0xe7, 0x57, 0xe9, 0xe9, 0x28, 0x3e,
    0xd8, 0xa5, 0x68, 0xd2, 0xe1, 0x75, 0xbe, 0xbe, 0xd1, 0x6a, 0x76, 0x56,
    0xae, 0xcf, 0x97, 0xe3, 0x12, 0xa7, 0x9c, 0xbe, 0xfa, 0x2e, 0x1c, 0x0c,
    0x63, 0x19, 0x5d, 0x71, 0x69, 0x5b, 0x21, 0xaf, 0x47, 0xb2, 0x71, 0x7b,
    0xd5, 0x9e, 0x8d, 0xe1, 0xc3, 0x16, 0x74, 0xfb, 0x7c, 0x51, 0xe2, 0xd7,
    0x6f, 0x92, 0x5e, 0x8f, 0xb2, 0x2e, 0xfd, 0xe3, 0x9f, 0x27, 0xb7, 0xdf,
    0x1b, 0xed, 0x6e, 0x1f, 0x65, 0xad, 0xf4, 0xe0, 0xdd, 0xf4, 0x5f, 0x5f,
    0x2e, 0x67, 0x85, 0x5d, 0xeb, 0x1b, 0xe7, 0x34, 0x88, 0xaa, 0xa1, 0x05,
    0xa5, 0xb6, 0xe4, 0xb3, 0xd7, 0xde, 0x5e, 0x7a, 0x7b, 0x3c, 0x32, 0x1f,
    0x28, 0x8f, 0xe6, 0xca, 0x64, 0x1d, 0xe2, 0xd5, 0x54, 0x00, 0x8d, 0x19,
    0xeb, 0xc1, 0xd9, 0xb8, 0x60, 0x7d, 0x7b, 0x1e, 0x47, 0x73, 0xda, 0x58,
    0xd1, 0x76, 0x2a, 0x77, 0xee, 0xd0, 0xe1, 0xb9, 0xbc, 0x5b, 0xf0, 0xa4,
    0xe2, 0x1c, 0xdc, 0x52, 0x8c, 0xbd, 0xb3, 0x6b, 0x6e, 0xde, 0xee, 0xae,
    0x74, 0xe0, 0xea, 0x9b, 0x52, 0x44, 0x03, 0x53, 0x5e, 0x19, 0x83, 0x72,
    0x74, 0x16, 0x3a, 0x4d, 0xaa, 0x18, 0xf2, 0x82, 0x53, 0x87, 0x85, 0xd7,
    0xb6, 0xd5, 0xd1, 0x54, 0x06, 0x57, 0xe8, 0x32, 0x43, 0xce, 0xe6, 0xe4,
    0x3a, 0x4d, 0xb6, 0x11, 0xec, 0xfe, 0x69, 0x55, 0x56, 0xfc, 0xd3, 0x85,
    0x6c, 0xf7, 0x60, 0x5e, 0x40, 0xe1, 0xd1, 0x10, 0x2a, 0x60, 0xea, 0x74,
    0x96, 0xab, 0x08, 0x96, 0x51, 0xd6, 0xba, 0x34, 0xb8, 0x94, 0xeb, 0x89,
    0x18, 0x24, 0x05, 0xac, 0x58, 0x4e, 0x66, 0xb2, 0x03, 0xc1, 0x86, 0x10,
    0x4f, 0xa6, 0x3c, 0xc9, 0x35, 0x35, 0xe0, 0x90, 0x1b, 0xe8, 0xf3, 0xa0,
    0x8c, 0x94, 0xa5, 0xea, 0x6b, 0x98, 0x2e, 0x04, 0x11, 0x91, 0x70, 0x5e,
    0x70, 0x59, 0xe8, 0x9a, 0xc3, 0x5a, 0x58, 0x39, 0xb2, 0xb8, 0x2a, 0x00,
    0x0b, 0xd9, 0x58, 0x87, 0xe9, 0x42, 0x33, 0x88, 0x1d, 0xa8, 0xeb, 0x4a,
    0xa6, 0x0b, 0x98, 0xcc, 0x85, 0x8c, 0x36, 0x52, 0xf2, 0x15, 0x0a, 0x43,
    0x50, 0x55, 0xc5, 0xdc, 0x63, 0x8a, 0x6a, 0x55, 0x94, 0xa0, 0x08, 0x5c,
    0x87, 0x98, 0x18, 0x63, 0x0d, 0xda, 0x45, 0x45, 0x8b, 0x4a, 0xb7, 0xfb,
    0x72, 0x6f, 0x1b, 0x4a, 0x8f, 0x5b, 0xeb, 0x76, 0xeb, 0x56, 0x77, 0x38,
    0x2c, 0x9e, 0xff, 0xdf, 0x0f, 0xa7, 0x62, 0x13, 0x52, 0x23, 0x75, 0x10,
    0x44, 0x32, 0x84, 0x8c, 0x5a, 0x47, 0x09, 0x06, 0x92, 0x08, 0x45, 0x25,
    0x27, 0x73, 0x35, 0x3b, 0x6b, 0xb7, 0x96, 0x5e, 0x9d, 0x91, 0xcc, 0xc0,
    0x64, 0xae, 0x9f, 0xfe, 0xb2, 0xfb, 0xe7, 0xdf, 0x6f, 0x3c, 0xb9, 0x9b,
    0x3d, 0xbb, 0xdf, 0x68, 0x3b, 0x38, 0xbe, 0xe6, 0xeb, 0x05, 0x14, 0x35,
    0x20, 0x62, 0x60, 0x70, 0x68, 0x04, 0x89, 0x92, 0x46, 0xea, 0xac, 0x25,
    0x72, 0xce, 0x9a, 0xdd, 0xde, 0xd6, 0x24, 0xf7, 0xe3, 0x5c, 0xe6, 0x39,
    0x2f, 0x02, 0x0e, 0x47, 0x61, 0x7e, 0x5d, 0x34, 0x13, 0x59, 0x69, 0xd3,
    0xfd, 0x9b, 0xb4, 0xb7, 0x85, 0x75, 0x55, 0xde, 0x5e, 0x0f, 0xf7, 0x56,
    0xfc, 0xde, 0x5a, 0xf4, 0xb5, 0x8c, 0x2b, 0x5a, 0xed, 0xb7, 0x99, 0x95,
    0x01, 0x53, 0x14, 0xd3, 0x6b, 0x6d, 0xe6, 0x55, 0x14, 0xd0, 0x85, 0x97,
    0xa2, 0x96, 0x4f, 0xf7, 0xd2, 0xbd, 0xbb, 0xd9, 0xf0, 0xac, 0x88, 0x3e,
    0x8e, 0xe7, 0x7c, 0x79, 0x5e, 0x6e, 0x18, 0xee, 0x18, 0xdf, 0xa6, 0xa8,
    0x41, 0xad, 0xe8, 0x55, 0x09, 0x93, 0xa2, 0x06, 0x44, 0x50, 0x5d, 0x4f,
    0xc5, 0x8e, 0x8b, 0x88, 0xc0, 0x99, 0x51, 0x2f, 0x7a, 0xa3, 0x09, 0x1f,
    0xf5, 0x65, 0x25, 0xd3, 0xa3, 0x49, 0xdc, 0x1f, 0x94, 0x9b, 0xab, 0x26,
    0x9f, 0xc7, 0xa3, 0xf3, 0x38, 0x18, 0x01, 0x1a, 0x1b, 0x19, 0x0a, 0x0f,
    0x55, 0x88, 0x35, 0xa8, 0x33, 0x98, 0x99, 0x34, 0x25, 0xb0, 0xc8, 0x8c,
    0xa0, 0x55, 0x10, 0x54, 0x2d, 0x6a, 0x7a, 0x79, 0x50, 0x5f, 0x4d, 0xea,
    0xe7, 0x6f, 0x42, 0x14, 0xfd, 0xe9, 0x2c, 0x2e, 0x4b, 0x5e, 0x78, 0xbd,
    0xcc, 0xb1, 0x12, 0x04, 0x04, 0x15, 0xf1, 0x2c, 0x89, 0xa1, 0x44, 0x99,
    0x62, 0x18, 0x2f, 0xd1, 0x12, 0x6a, 0x64, 0x11, 0x95, 0xc4, 0xc0, 0xa2,
    0x8a, 0xdf, 0x0f, 0x20, 0x39, 0x94, 0xa9, 0xd7, 0x66, 0xd3, 0x84, 0x52,
    0xa7, 0xb9, 0x44, 0x01, 0x2f, 0xe0, 0x59, 0x22, 0x00, 0x01, 0x08, 0xa8,
    0x57, 0xed, 0x82, 0x54, 0x21, 0x90, 0x31, 0x36, 0x44, 0xae, 0x63, 0x34,
    0x04, 0x15, 0x6b, 0x93, 0x74, 0x54, 0x40, 0x1d, 0xc5, 0x22, 0x94, 0x75,
    0x5c, 0x44, 0x28, 0x45, 0x83, 0xaa, 0x45, 0x15, 0x55, 0x51, 0x00, 0x50,
    0x05, 0x60, 0x0e, 0x55, 0xcd, 0x0a, 0x84, 0x8a, 0x96, 0x05, 0x51, 0x91,
    0x05, 0x53, 0x03, 0x68, 0xa0, 0x0c, 0x2a, 0x0a, 0x0d, 0x02, 0x05, 0x42,
    0x54, 0x4b, 0x8a, 0xa0, 0x09, 0x68, 0x00, 0xf5, 0x0c, 0x04, 0x02, 0x00,
    0x51, 0xa1, 0x62, 0x35, 0x08, 0x15, 0xc3, 0xcf, 0x54, 0x8f, 0x0d, 0x07,
    0x3c, 0xd1, 0x0f, 0x66, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44,
    0xae, 0x42, 0x60, 0x82,
};
const size_t g_src_len__mona_lisa_21_32_png = 1600;

// ----------------

struct {
  int remaining_argc;
  char** remaining_argv;

  bool demo;
} g_flags = {0};

static const char* g_usage =
    "Usage: stb-imagedumper *.{jpeg,png}\n"
    "\n"
    "Flags:\n"
    "    -demo (dump the built-in demonstration images)\n";

const char*  //
parse_flags(int argc, char** argv) {
  int c = (argc > 0) ? 1 : 0;  // Skip argv[0], the program name.
  for (; c < argc; c++) {
    char* arg = argv[c];
    if (*arg++ != '-') {
      break;
    }

    // A double-dash "--foo" is equivalent to a single-dash "-foo". As special
    // cases, a bare "-" is not a flag (some programs may interpret it as
    // stdin) and a bare "--" means to stop parsing flags.
    if (*arg == '\x00') {
      break;
    } else if (*arg == '-') {
      arg++;
      if (*arg == '\x00') {
        c++;
        break;
      }
    }

    if (!strcmp(arg, "demo")) {
      g_flags.demo = true;
      continue;
    }

    return g_usage;
  }

  g_flags.remaining_argc = argc - c;
  g_flags.remaining_argv = argv + c;
  return NULL;
}

// ----------------

#define MAX_INCL_DIMENSION 10000

// BYTES_PER_COLOR_PIXEL is long enough to contain "\x1B[38;2;255;255;255mABC"
// plus a trailing NUL byte and a few bytes of slack. It starts with a true
// color terminal escape code. ABC is three bytes for the UTF-8 representation
// "\xE2\x96\x88" of "█", U+2588 FULL BLOCK.
#define BYTES_PER_COLOR_PIXEL 32

static void  //
handle(const char* filename, const uint8_t* src_ptr, const size_t src_len) {
  int w = 0;
  int h = 0;
  int channels_in_file = 0;
  int bytes_per_pixel = STBI_rgb;
  unsigned char* pixels = NULL;

  if (src_len > 0) {
    pixels = stbi_load_from_memory(src_ptr, src_len,  //
                                   &w, &h, &channels_in_file, bytes_per_pixel);
  } else {
    pixels = stbi_load(filename,  //
                       &w, &h, &channels_in_file, bytes_per_pixel);
  }

  if (!pixels) {
    printf("%s\n", stbi_failure_reason());
    return;
  } else if ((w < 0) || (MAX_INCL_DIMENSION < w) ||  //
             (h < 0) || (MAX_INCL_DIMENSION < h)) {
    printf("main: image is too large\n");
    return;
  }

  unsigned char* src = pixels;
  static char buffer[MAX_INCL_DIMENSION * BYTES_PER_COLOR_PIXEL];
  for (int y = 0; y < h; y++) {
    char* dst = buffer;
    for (int x = 0; x < w; x++) {
      // "\xE2\x96\x88" is U+2588 FULL BLOCK. Before that is a true color
      // terminal escape code.
      dst += sprintf(dst, "\x1B[38;2;%d;%d;%dm\xE2\x96\x88",  //
                     (uint8_t)src[0], (uint8_t)src[1], (uint8_t)src[2]);
      src += bytes_per_pixel;
    }
    *dst++ = '\n';
    fwrite(buffer, sizeof(char), dst - buffer, stdout);
  }
  stbi_image_free(pixels);

  const char* reset_color_str = "\x1B[0m";
  const size_t reset_color_len = 4;
  fwrite(reset_color_str, sizeof(char), reset_color_len, stdout);
  fflush(stdout);
}

int  //
main(int argc, char** argv) {
  const char* usage = parse_flags(argc, argv);
  if (usage) {
    fputs(usage, stderr);
    return 1;
  }

  struct {
    const char* filename;
    const uint8_t* src_ptr;
    const size_t src_len;
  } demos[] = {
      {
          "«demo»/mona-lisa.21x32.th",
          g_src_ptr__mona_lisa_21_32_th,
          g_src_len__mona_lisa_21_32_th,
      },
      {
          "«demo»/mona-lisa.21x32.etc2.pkm",
          g_src_ptr__mona_lisa_21_32_etc2_pkm,
          g_src_len__mona_lisa_21_32_etc2_pkm,
      },
      {
          "«demo»/mona-lisa.21x32.q50.jpeg",
          g_src_ptr__mona_lisa_21_32_q50_jpeg,
          g_src_len__mona_lisa_21_32_q50_jpeg,
      },
      {
          "«demo»/mona-lisa.21x32.png",
          g_src_ptr__mona_lisa_21_32_png,
          g_src_len__mona_lisa_21_32_png,
      },
      {
          NULL,
          NULL,
          0,
      },
  };

  if (g_flags.demo) {
    for (size_t c = 0; demos[c].filename != NULL; c++) {
      printf("\n%s (%zu bytes)\n", demos[c].filename, demos[c].src_len);
      handle(demos[c].filename, demos[c].src_ptr, demos[c].src_len);
      printf("\n");
    }
  }

  for (int c = 0; c < g_flags.remaining_argc; c++) {
    const char* filename = g_flags.remaining_argv[c];
    printf("\n%s\n", filename);
    handle(filename, NULL, 0);
    printf("\n");
  }
  return 0;
}
