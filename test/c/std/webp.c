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
This test program is typically run indirectly, by the "wuffs test" or "wuffs
bench" commands. These commands take an optional "-mimic" flag to check that
Wuffs' output mimics (i.e. exactly matches) other libraries' output, such as
giflib for GIF, libpng for PNG, etc.

To manually run this test:

for CC in clang gcc; do
  $CC -std=c99 -Wall -Werror webp.c && ./a.out
  rm -f a.out
done

Each edition should print "PASS", amongst other information, and exit(0).

Add the "wuffs mimic cflags" (everything after the colon below) to the C
compiler flags (after the .c file) to run the mimic tests.

To manually run the benchmarks, replace "-Wall -Werror" with "-O3" and replace
the first "./a.out" with "./a.out -bench". Combine these changes with the
"wuffs mimic cflags" to run the mimic benchmarks.
*/

// ¿ wuffs mimic cflags: -DWUFFS_MIMIC -lwebp

// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// To use that single file as a "foo.c"-like implementation, instead of a
// "foo.h"-like header, #define WUFFS_IMPLEMENTATION before #include'ing or
// compiling it.
#define WUFFS_IMPLEMENTATION

// Defining the WUFFS_CONFIG__MODULE* macros are optional, but it lets users of
// release/c/etc.c choose which parts of Wuffs to build. That file contains the
// entire Wuffs standard library, implementing a variety of codecs and file
// formats. Without this macro definition, an optimizing compiler or linker may
// very well discard Wuffs code for unused codecs, but listing the Wuffs
// modules we use makes that process explicit. Preprocessing means that such
// code simply isn't compiled.
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__VP8
#define WUFFS_CONFIG__MODULE__WEBP

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../release/c/wuffs-unsupported-snapshot.c"
#include "../testlib/testlib.c"
#ifdef WUFFS_MIMIC
#include "../mimiclib/webp.c"
#endif

static wuffs_webp__decoder g_webp_decoder;

// ---------------- WebP Tests

const char*  //
wuffs_webp_decode(uint64_t* n_bytes_out,
                  wuffs_base__io_buffer* dst,
                  uint32_t wuffs_initialize_flags,
                  wuffs_base__pixel_format pixfmt,
                  uint32_t* quirks_ptr,
                  size_t quirks_len,
                  wuffs_base__io_buffer* src) {
  wuffs_webp__decoder* dec = &g_webp_decoder;
  CHECK_STATUS("initialize",
               wuffs_webp__decoder__initialize(dec, sizeof *dec, WUFFS_VERSION,
                                               wuffs_initialize_flags));
  return do_run__wuffs_base__image_decoder(
      wuffs_webp__decoder__upcast_as__wuffs_base__image_decoder(dec),
      n_bytes_out, dst, pixfmt, quirks_ptr, quirks_len, src);
}

// --------

const char*  //
test_wuffs_webp_decode_interface_lossless() {
  CHECK_FOCUS(__func__);
  wuffs_webp__decoder* dec = &g_webp_decoder;
  CHECK_STATUS("initialize",
               wuffs_webp__decoder__initialize(
                   dec, sizeof *dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));
  return do_test__wuffs_base__image_decoder(
      wuffs_webp__decoder__upcast_as__wuffs_base__image_decoder(dec),
      "test/data/bricks-color.lossless.webp", 0, SIZE_MAX, 160, 120,
      0xFF022460);
}

const char*  //
test_wuffs_webp_decode_interface_lossy() {
  CHECK_FOCUS(__func__);
  wuffs_webp__decoder* dec = &g_webp_decoder;
  CHECK_STATUS("initialize",
               wuffs_webp__decoder__initialize(
                   dec, sizeof *dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));
  return do_test__wuffs_base__image_decoder(
      wuffs_webp__decoder__upcast_as__wuffs_base__image_decoder(dec),
      "test/data/bricks-color.lossy.webp", 0, SIZE_MAX, 160, 120, 0xFF032665);
}

const char*  //
test_wuffs_webp_decode_many_small_reads() {
  CHECK_FOCUS(__func__);
  wuffs_webp__decoder* dec = &g_webp_decoder;
  CHECK_STATUS("initialize",
               wuffs_webp__decoder__initialize(
                   dec, sizeof *dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

  wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
      .data = g_src_slice_u8,
  });
  CHECK_STRING(read_file(&src, "test/data/mona-lisa.21x32.q0.lossy.webp"));

  wuffs_base__image_config ic = ((wuffs_base__image_config){});
  CHECK_STATUS("decode_image_config",
               wuffs_webp__decoder__decode_image_config(dec, &ic, &src));

  const uint32_t w = 21;
  if (wuffs_base__pixel_config__width(&ic.pixcfg) != 21) {
    RETURN_FAIL("width: have %" PRIu32 ", want 21",
                wuffs_base__pixel_config__width(&ic.pixcfg));
  }
  const uint32_t h = 32;
  if (wuffs_base__pixel_config__height(&ic.pixcfg) != 32) {
    RETURN_FAIL("height: have %" PRIu32 ", want 32",
                wuffs_base__pixel_config__height(&ic.pixcfg));
  }
  if (wuffs_base__pixel_config__pixel_format(&ic.pixcfg).repr !=
      WUFFS_BASE__PIXEL_FORMAT__BGRX) {
    RETURN_FAIL("pixel_format: have %" PRIu32
                ", want WUFFS_BASE__PIXEL_FORMAT__BGRX",
                wuffs_base__pixel_config__pixel_format(&ic.pixcfg).repr);
  }

  // 30 is 12 for the RIFF container header, 8 for the RIFF chunk header and
  // 10 for the VP8 header.
  if (wuffs_base__image_config__first_frame_io_position(&ic) != 30) {
    RETURN_FAIL("first_frame_io_position: have %" PRIu64 ", want 30",
                wuffs_base__image_config__first_frame_io_position(&ic));
  }

  wuffs_base__pixel_buffer pb = ((wuffs_base__pixel_buffer){});
  CHECK_STATUS("set_from_slice", wuffs_base__pixel_buffer__set_from_slice(
                                     &pb, &ic.pixcfg, g_pixel_slice_u8));
  wuffs_base__pixel_buffer__set_color_u32_fill_rect(
      &pb, wuffs_base__make_rect_ie_u32(0, 0, w, h), 0xFF234567);

  uint64_t m = wuffs_webp__decoder__workbuf_len(dec).min_incl;
  wuffs_base__slice_u8 workbuf = g_work_slice_u8;
  if (workbuf.len > m) {
    workbuf.len = m;
  }

  const uint64_t rlimit = 10;
  int num_iters = 0;
  while (true) {
    num_iters++;
    wuffs_base__io_buffer limited_src = make_limited_reader(src, rlimit);
    size_t old_ri = src.meta.ri;

    wuffs_base__status status = wuffs_webp__decoder__decode_frame(
        dec, &pb, &limited_src, WUFFS_BASE__PIXEL_BLEND__SRC, workbuf, NULL);
    src.meta.ri += limited_src.meta.ri;

    if (wuffs_base__status__is_ok(&status)) {
      break;
    }
    if (status.repr != wuffs_base__suspension__short_read) {
      RETURN_FAIL("decode_frame: have \"%s\", want \"%s\"", status.repr,
                  wuffs_base__suspension__short_read);
    }

    if (src.meta.ri < old_ri) {
      RETURN_FAIL("read index src.meta.ri went backwards");
    } else if (src.meta.ri == old_ri) {
      RETURN_FAIL("no progress was made");
    }
  }

  if (num_iters <= 1) {
    RETURN_FAIL("num_iters: have %d, want > 1", num_iters);
  }

  wuffs_base__color_u32_argb_premul last_pixel =
      wuffs_base__pixel_buffer__color_u32_at(&pb, w - 1, h - 1);
  if (last_pixel != 0xFF210A1B) {
    RETURN_FAIL("last_pixel: have 0x%" PRIX32 ", want 0xFF210A1B", last_pixel);
  }

  return NULL;
}

// ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

const char*  //
do_test_mimic_webp_decode(const char* filename) {
  wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
      .data = g_src_slice_u8,
  });
  CHECK_STRING(read_file(&src, filename));

  src.meta.ri = 0;
  wuffs_base__io_buffer have = ((wuffs_base__io_buffer){
      .data = g_have_slice_u8,
  });
  CHECK_STRING(wuffs_webp_decode(
      NULL, &have, WUFFS_INITIALIZE__DEFAULT_OPTIONS,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, &src));

  src.meta.ri = 0;
  wuffs_base__io_buffer want = ((wuffs_base__io_buffer){
      .data = g_want_slice_u8,
  });
  CHECK_STRING(mimic_webp_decode(
      NULL, &want, WUFFS_INITIALIZE__DEFAULT_OPTIONS,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, &src));

  return check_io_buffers_equal("", &have, &want);
}

const char*  //
test_mimic_webp_lossless_decode_image_19k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/bricks-gray.lossless.webp");
}

const char*  //
test_mimic_webp_lossless_decode_image_40k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/hat.lossless.webp");
}

const char*  //
test_mimic_webp_lossless_decode_image_77k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/bricks-dither.lossless.webp");
}

const char*  //
test_mimic_webp_lossless_decode_image_552k_32bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode(
      "test/data/hibiscus.primitive.lossless.webp");
}

const char*  //
test_mimic_webp_lossless_decode_image_4002k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/harvesters.lossless.webp");
}

const char*  //
test_mimic_webp_lossy_decode_image_19k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/bricks-gray.lossy.webp");
}

const char*  //
test_mimic_webp_lossy_decode_image_40k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/hat.lossy.webp");
}

const char*  //
test_mimic_webp_lossy_decode_image_77k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/bricks-color.lossy.webp");
}

const char*  //
test_mimic_webp_lossy_decode_image_552k_32bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/hibiscus.primitive.lossy.webp");
}

const char*  //
test_mimic_webp_lossy_decode_image_4002k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/harvesters.lossy.webp");
}

#endif  // WUFFS_MIMIC

// ---------------- WebP Benches

const char*  //
bench_wuffs_webp_lossless_decode_image_19k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__Y), NULL, 0,
      "test/data/bricks-gray.lossless.webp", 0, SIZE_MAX, 50);
}

const char*  //
bench_wuffs_webp_lossless_decode_image_40k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/hat.lossless.webp", 0, SIZE_MAX, 30);
}

const char*  //
bench_wuffs_webp_lossless_decode_image_77k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/bricks-dither.lossless.webp", 0, SIZE_MAX, 50);
}

const char*  //
bench_wuffs_webp_lossless_decode_image_552k_32bpp() {
  uint32_t q = WUFFS_BASE__QUIRK_IGNORE_CHECKSUM;
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      &q, 1, "test/data/hibiscus.primitive.lossless.webp", 0, SIZE_MAX, 4);
}

const char*  //
bench_wuffs_webp_lossless_decode_image_4002k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/harvesters.lossless.webp", 0, SIZE_MAX, 1);
}

const char*  //
bench_wuffs_webp_lossy_decode_image_19k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__Y), NULL, 0,
      "test/data/bricks-gray.lossy.webp", 0, SIZE_MAX, 50);
}

const char*  //
bench_wuffs_webp_lossy_decode_image_40k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/hat.lossy.webp", 0, SIZE_MAX, 30);
}

const char*  //
bench_wuffs_webp_lossy_decode_image_77k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/bricks-color.lossy.webp", 0, SIZE_MAX, 50);
}

const char*  //
bench_wuffs_webp_lossy_decode_image_552k_32bpp() {
  uint32_t q = WUFFS_BASE__QUIRK_IGNORE_CHECKSUM;
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      &q, 1, "test/data/hibiscus.primitive.lossy.webp", 0, SIZE_MAX, 4);
}

const char*  //
bench_wuffs_webp_lossy_decode_image_4002k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/harvesters.lossy.webp", 0, SIZE_MAX, 1);
}

// ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

const char*  //
bench_mimic_webp_lossless_decode_image_19k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__Y), NULL, 0,
      "test/data/bricks-gray.lossless.webp", 0, SIZE_MAX, 50);
}

const char*  //
bench_mimic_webp_lossless_decode_image_40k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/hat.lossless.webp", 0, SIZE_MAX, 30);
}

const char*  //
bench_mimic_webp_lossless_decode_image_77k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/bricks-dither.lossless.webp", 0, SIZE_MAX, 50);
}

const char*  //
bench_mimic_webp_lossless_decode_image_552k_32bpp() {
  uint32_t q = WUFFS_BASE__QUIRK_IGNORE_CHECKSUM;
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      &q, 1, "test/data/hibiscus.primitive.lossless.webp", 0, SIZE_MAX, 4);
}

const char*  //
bench_mimic_webp_lossless_decode_image_4002k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/harvesters.lossless.webp", 0, SIZE_MAX, 1);
}

const char*  //
bench_mimic_webp_lossy_decode_image_19k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__Y), NULL, 0,
      "test/data/bricks-gray.lossy.webp", 0, SIZE_MAX, 50);
}

const char*  //
bench_mimic_webp_lossy_decode_image_40k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/hat.lossy.webp", 0, SIZE_MAX, 30);
}

const char*  //
bench_mimic_webp_lossy_decode_image_77k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/bricks-color.lossy.webp", 0, SIZE_MAX, 50);
}

const char*  //
bench_mimic_webp_lossy_decode_image_552k_32bpp() {
  uint32_t q = WUFFS_BASE__QUIRK_IGNORE_CHECKSUM;
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      &q, 1, "test/data/hibiscus.primitive.lossy.webp", 0, SIZE_MAX, 4);
}

const char*  //
bench_mimic_webp_lossy_decode_image_4002k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/harvesters.lossy.webp", 0, SIZE_MAX, 1);
}

#endif  // WUFFS_MIMIC

// ---------------- Manifest

proc g_tests[] = {

    test_wuffs_webp_decode_interface_lossless,
    test_wuffs_webp_decode_interface_lossy,
    test_wuffs_webp_decode_many_small_reads,

#ifdef WUFFS_MIMIC

    test_mimic_webp_lossless_decode_image_19k_8bpp,
    test_mimic_webp_lossless_decode_image_40k_24bpp,
    test_mimic_webp_lossless_decode_image_77k_8bpp,
    test_mimic_webp_lossless_decode_image_552k_32bpp,
    test_mimic_webp_lossless_decode_image_4002k_24bpp,
    test_mimic_webp_lossy_decode_image_19k_8bpp,
    test_mimic_webp_lossy_decode_image_40k_24bpp,
    test_mimic_webp_lossy_decode_image_77k_24bpp,
    // test_mimic_webp_lossy_decode_image_552k_32bpp,
    test_mimic_webp_lossy_decode_image_4002k_24bpp,

#endif  // WUFFS_MIMIC

    NULL,
};

proc g_benches[] = {

    bench_wuffs_webp_lossless_decode_image_19k_8bpp,
    bench_wuffs_webp_lossless_decode_image_40k_24bpp,
    bench_wuffs_webp_lossless_decode_image_77k_8bpp,
    bench_wuffs_webp_lossless_decode_image_552k_32bpp,
    bench_wuffs_webp_lossless_decode_image_4002k_24bpp,
    bench_wuffs_webp_lossy_decode_image_19k_8bpp,
    bench_wuffs_webp_lossy_decode_image_40k_24bpp,
    bench_wuffs_webp_lossy_decode_image_77k_24bpp,
    bench_wuffs_webp_lossy_decode_image_552k_32bpp,
    bench_wuffs_webp_lossy_decode_image_4002k_24bpp,

#ifdef WUFFS_MIMIC

    bench_mimic_webp_lossless_decode_image_19k_8bpp,
    bench_mimic_webp_lossless_decode_image_40k_24bpp,
    bench_mimic_webp_lossless_decode_image_77k_8bpp,
    bench_mimic_webp_lossless_decode_image_552k_32bpp,
    bench_mimic_webp_lossless_decode_image_4002k_24bpp,
    bench_mimic_webp_lossy_decode_image_19k_8bpp,
    bench_mimic_webp_lossy_decode_image_40k_24bpp,
    bench_mimic_webp_lossy_decode_image_77k_24bpp,
    bench_mimic_webp_lossy_decode_image_552k_32bpp,
    bench_mimic_webp_lossy_decode_image_4002k_24bpp,

#endif  // WUFFS_MIMIC

    NULL,
};

int  //
main(int argc, char** argv) {
  g_proc_package_name = "std/webp";
  return test_main(argc, argv, g_tests, g_benches);
}
