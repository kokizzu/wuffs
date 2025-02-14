// Copyright 2025 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

package lowleveljpeg

import (
	"bytes"
	"image"
	"image/jpeg"
	"io"
	"testing"
)

var (
	// pjw8x8 is test/data/pjw-thumbnail.png scaled down to 16×16 and then 8×8.
	pjw8x8 = BlockU8{
		0xFF, 0xFF, 0xAF, 0x40, 0x50, 0xBF, 0xFF, 0xFF,
		0xFF, 0xEF, 0xEF, 0xFF, 0x40, 0x00, 0x50, 0xFF,
		0xFF, 0x60, 0xDF, 0xFF, 0xB0, 0x00, 0x00, 0xCF,
		0xA0, 0x00, 0x90, 0x70, 0x10, 0x00, 0x00, 0xEF,
		0xEF, 0x8F, 0xCF, 0xB0, 0x50, 0x20, 0x60, 0xFF,
		0xFF, 0xB0, 0xCF, 0x90, 0x20, 0x20, 0x80, 0xFF,
		0xFF, 0xFF, 0xCF, 0xA0, 0x30, 0x50, 0xEF, 0xFF,
		0xFF, 0xFF, 0xB0, 0x20, 0x00, 0x70, 0xFF, 0xFF,
	}

	// pjw16x16 is test/data/pjw-thumbnail.png scaled down to 16×16.
	pjw16x16 = QuadBlockU8{
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x80, 0x80, 0x80, 0xBF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0xBF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0xFF, 0xFF, 0xFF, 0x40, 0x00, 0x00, 0x00, 0x40, 0xBF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xBF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x40, 0xFF, 0xFF,
		0xFF, 0xFF, 0x80, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x40, 0x00, 0x00, 0x00, 0x00, 0x80, 0xFF,
		0xFF, 0xFF, 0x00, 0x80, 0xBF, 0xBF, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00, 0xBF, 0xFF,
		0xFF, 0x40, 0x00, 0x00, 0xBF, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBF, 0xFF,
		0xFF, 0x40, 0x00, 0x00, 0xBF, 0x80, 0x80, 0xBF, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
		0xFF, 0xBF, 0x00, 0xBF, 0xFF, 0xBF, 0xBF, 0xFF, 0x40, 0x80, 0x80, 0x00, 0x00, 0x80, 0xFF, 0xFF,
		0xFF, 0xFF, 0xBF, 0xBF, 0xBF, 0xBF, 0x80, 0x80, 0x40, 0x40, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0x40, 0x00, 0x40, 0x40, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xBF, 0x80, 0xBF, 0x80, 0x80, 0x80, 0x00, 0x40, 0x40, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0xBF, 0x80, 0x00, 0x40, 0x40, 0x00, 0x40, 0xBF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0xFF, 0xFF, 0x40, 0x00, 0x40, 0xBF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x80, 0x40, 0x00, 0x00, 0x00, 0x00, 0xBF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x40, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	}
)

func diffU8(a uint8, b uint8) uint8 {
	if a < b {
		return b - a
	}
	return a - b
}

func diffU16(a uint16, b uint16) uint16 {
	if a < b {
		return b - a
	}
	return a - b
}

func checkPixelApproxEqual(tt *testing.T, dst image.RGBA64Image, src image.RGBA64Image, x int, y int) {
	const tolerance = 0x300 // RGBA64At returns values in the range [0x0000, 0xFFFF].
	dpix := dst.RGBA64At(x, y)
	spix := src.RGBA64At(x, y)
	if (diffU16(dpix.R, spix.R) > tolerance) ||
		(diffU16(dpix.G, spix.G) > tolerance) ||
		(diffU16(dpix.B, spix.B) > tolerance) ||
		(diffU16(dpix.A, spix.A) > tolerance) {
		tt.Errorf("At (%d, %d): dpix = %04X, spix = %04X", x, y, dpix, spix)
	}
}

func testLowLevelJpegEncodeJpegDecode(tt *testing.T, src image.RGBA64Image, colorType ColorType) {
	quants := Array2QuantizationFactors{}
	quants.SetToStandardValues(MaximumQuality)
	opts := &EncoderOptions{
		QuantizationFactors: &quants,
	}

	bounds := src.Bounds()
	buf := &bytes.Buffer{}
	enc := Encoder{}
	if err := enc.Reset(buf, colorType, bounds.Dx(), bounds.Dy(), opts); err != nil {
		tt.Fatalf("enc.Reset: %v", err)
	}

	if colorType == ColorTypeGray {
		srcU8s := Array1BlockU8{}
		srcI16s := Array1BlockI16{}
		for y := bounds.Min.Y; y < bounds.Max.Y; y += 8 {
			for x := bounds.Min.X; x < bounds.Max.X; x += 8 {
				srcU8s.ExtractFrom(src, x, y)
				srcI16s.ForwardDCTFrom(&srcU8s)
				if err := enc.Add1(buf, &srcI16s); err != nil {
					tt.Fatalf("enc.Add1: %v", err)
				}
			}
		}

	} else if colorType == ColorTypeYCbCr444 {
		srcU8s := Array3BlockU8{}
		srcI16s := Array3BlockI16{}
		for y := bounds.Min.Y; y < bounds.Max.Y; y += 8 {
			for x := bounds.Min.X; x < bounds.Max.X; x += 8 {
				srcU8s.ExtractFrom(src, x, y)
				srcI16s.ForwardDCTFrom(&srcU8s)
				if err := enc.Add3(buf, &srcI16s); err != nil {
					tt.Fatalf("enc.Add3: %v", err)
				}
			}
		}

	} else {
		tt.Fatalf("unexpected colorType: %v", colorType)
	}

	dst, err := jpeg.Decode(buf)
	if err != nil {
		tt.Fatalf("jpeg.Decode: %v", err)
	} else if got := dst.Bounds(); got != bounds {
		tt.Fatalf("dst.Bounds: got %v, want %v", got, bounds)
	}

	for y := bounds.Min.Y; y < bounds.Max.Y; y++ {
		for x := bounds.Min.X; x < bounds.Max.X; x++ {
			checkPixelApproxEqual(tt, dst.(image.RGBA64Image), src, x, y)
		}
	}
}

func TestColorTypeGray(tt *testing.T) {
	src := image.NewGray(image.Rect(0, 0, 8, 8))
	copy(src.Pix, pjw8x8[:])
	testLowLevelJpegEncodeJpegDecode(tt, src, ColorTypeGray)
}

func TestColorTypeYCbCr444(tt *testing.T) {
	const width, height = 12, 15
	src := image.NewYCbCr(image.Rect(0, 0, width, height), image.YCbCrSubsampleRatio444)
	bounds := src.Bounds()
	for y := bounds.Min.Y; y < bounds.Max.Y; y++ {
		for x := bounds.Min.X; x < bounds.Max.X; x++ {
			i := (width * y) + x
			j := (16 * y) + x
			src.Y[i] = pjw16x16[j]
			src.Cb[i] = uint8(j)
			src.Cr[i] = 0xAA
		}
	}
	testLowLevelJpegEncodeJpegDecode(tt, src, ColorTypeYCbCr444)
}

// TestWikipediaDCT confirms that we can reproduce the FDCT computation from
// https://en.wikipedia.org/wiki/JPEG#Discrete_cosine_transform
//
// We also then check the FDCT+IDCT round trip is approximately equal to the
// original BlockU8 values. It's not exactly equal due to rounding errors.
func TestWikipediaDCT(tt *testing.T) {
	srcU8 := BlockU8{
		52, 55, 61, 66, 70, 61, 64, 73,
		63, 59, 55, 90, 109, 85, 69, 72,
		62, 59, 68, 113, 144, 104, 66, 73,
		63, 58, 71, 122, 154, 106, 70, 69,
		67, 61, 68, 104, 126, 88, 68, 70,
		79, 65, 60, 70, 77, 68, 58, 75,
		85, 71, 64, 59, 55, 61, 65, 83,
		87, 79, 69, 68, 65, 76, 78, 94,
	}

	gotI16 := srcU8.ForwardDCT()

	wantI16 := BlockI16{
		-415, -30, -61, 27, 56, -20, -2, 0,
		4, -22, -61, 10, 13, -7, -9, 5,
		-47, 7, 77, -25, -29, 10, 5, -6,
		-49, 12, 34, -15, -10, 6, 2, 2,
		12, -7, -13, -4, -2, 2, -3, 3,
		-8, 3, 2, -6, -2, 1, 4, 2,
		-1, 0, 0, -2, -1, -3, 4, -1,
		0, 0, -1, -4, -1, 0, 1, 2,
	}

	if gotI16 != wantI16 {
		tt.Fatalf("gotI16:\n%v\nwantI16:\n%v", gotI16, wantI16)
	}

	gotU8 := gotI16.InverseDCT()

	const tolerance = 1 // BlockU8 values are in the range [0x00, 0xFF].
	for y := 0; y < 8; y++ {
		for x := 0; x < 8; x++ {
			got := gotU8[(8*y)+x]
			want := srcU8[(8*y)+x]
			if diff := diffU8(got, want); diff > tolerance {
				tt.Errorf("At (%d, %d): got 0x%02X, want 0x%02X", x, y, got, want)
			}
		}
	}
}

func TestQuantizationFactors(tt *testing.T) {
	testCases := []struct {
		quality int
		want    QuantizationFactors
	}{{

		quality: 1,
		want: QuantizationFactors{
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		},
	}, {

		quality: 25,
		want: QuantizationFactors{
			0x20, 0x16, 0x14, 0x20, 0x30, 0x50, 0x66, 0x7A,
			0x18, 0x18, 0x1C, 0x26, 0x34, 0x74, 0x78, 0x6E,
			0x1C, 0x1A, 0x20, 0x30, 0x50, 0x72, 0x8A, 0x70,
			0x1C, 0x22, 0x2C, 0x3A, 0x66, 0xAE, 0xA0, 0x7C,
			0x24, 0x2C, 0x4A, 0x70, 0x88, 0xDA, 0xCE, 0x9A,
			0x30, 0x46, 0x6E, 0x80, 0xA2, 0xD0, 0xE2, 0xB8,
			0x62, 0x80, 0x9C, 0xAE, 0xCE, 0xF2, 0xF0, 0xCA,
			0x90, 0xB8, 0xBE, 0xC4, 0xE0, 0xC8, 0xCE, 0xC6,
		},
	}, {

		quality: 50,
		want: QuantizationFactors{
			0x10, 0x0B, 0x0A, 0x10, 0x18, 0x28, 0x33, 0x3D,
			0x0C, 0x0C, 0x0E, 0x13, 0x1A, 0x3A, 0x3C, 0x37,
			0x0E, 0x0D, 0x10, 0x18, 0x28, 0x39, 0x45, 0x38,
			0x0E, 0x11, 0x16, 0x1D, 0x33, 0x57, 0x50, 0x3E,
			0x12, 0x16, 0x25, 0x38, 0x44, 0x6D, 0x67, 0x4D,
			0x18, 0x23, 0x37, 0x40, 0x51, 0x68, 0x71, 0x5C,
			0x31, 0x40, 0x4E, 0x57, 0x67, 0x79, 0x78, 0x65,
			0x48, 0x5C, 0x5F, 0x62, 0x70, 0x64, 0x67, 0x63,
		},
	}, {

		quality: 75,
		want: QuantizationFactors{
			0x08, 0x06, 0x05, 0x08, 0x0C, 0x14, 0x1A, 0x1F,
			0x06, 0x06, 0x07, 0x0A, 0x0D, 0x1D, 0x1E, 0x1C,
			0x07, 0x07, 0x08, 0x0C, 0x14, 0x1D, 0x23, 0x1C,
			0x07, 0x09, 0x0B, 0x0F, 0x1A, 0x2C, 0x28, 0x1F,
			0x09, 0x0B, 0x13, 0x1C, 0x22, 0x37, 0x34, 0x27,
			0x0C, 0x12, 0x1C, 0x20, 0x29, 0x34, 0x39, 0x2E,
			0x19, 0x20, 0x27, 0x2C, 0x34, 0x3D, 0x3C, 0x33,
			0x24, 0x2E, 0x30, 0x31, 0x38, 0x32, 0x34, 0x32,
		},
	}, {

		quality: 90,
		want: QuantizationFactors{
			0x03, 0x02, 0x02, 0x03, 0x05, 0x08, 0x0A, 0x0C,
			0x02, 0x02, 0x03, 0x04, 0x05, 0x0C, 0x0C, 0x0B,
			0x03, 0x03, 0x03, 0x05, 0x08, 0x0B, 0x0E, 0x0B,
			0x03, 0x03, 0x04, 0x06, 0x0A, 0x11, 0x10, 0x0C,
			0x04, 0x04, 0x07, 0x0B, 0x0E, 0x16, 0x15, 0x0F,
			0x05, 0x07, 0x0B, 0x0D, 0x10, 0x15, 0x17, 0x12,
			0x0A, 0x0D, 0x10, 0x11, 0x15, 0x18, 0x18, 0x14,
			0x0E, 0x12, 0x13, 0x14, 0x16, 0x14, 0x15, 0x14,
		},
	}, {

		quality: 95,
		want: QuantizationFactors{
			0x02, 0x01, 0x01, 0x02, 0x02, 0x04, 0x05, 0x06,
			0x01, 0x01, 0x01, 0x02, 0x03, 0x06, 0x06, 0x06,
			0x01, 0x01, 0x02, 0x02, 0x04, 0x06, 0x07, 0x06,
			0x01, 0x02, 0x02, 0x03, 0x05, 0x09, 0x08, 0x06,
			0x02, 0x02, 0x04, 0x06, 0x07, 0x0B, 0x0A, 0x08,
			0x02, 0x04, 0x06, 0x06, 0x08, 0x0A, 0x0B, 0x09,
			0x05, 0x06, 0x08, 0x09, 0x0A, 0x0C, 0x0C, 0x0A,
			0x07, 0x09, 0x0A, 0x0A, 0x0B, 0x0A, 0x0A, 0x0A,
		},
	}, {

		quality: 100,
		want: QuantizationFactors{
			0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
			0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
			0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
			0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
			0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
			0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
			0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
			0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		},
	}}

	for _, tc := range testCases {
		qf := QuantizationFactors{}
		qf.SetToStandardValues(QuantizationStandardValuesTypeLuma, tc.quality)
		if qf != tc.want {
			tt.Errorf("quality = %d\ngot:\n%v\nwant:\n%v", tc.quality, qf, tc.want)
		}
	}
}

func TestNoAllocation(tt *testing.T) {
	enc := Encoder{}
	srcI16s := Array1BlockI16{}
	got := testing.AllocsPerRun(100, func() {
		enc.Reset(io.Discard, ColorTypeGray, 8, 8, nil)
		enc.Add1(io.Discard, &srcI16s)
	})
	if got != 0 {
		tt.Fatalf("AllocsPerRun: got %v, want 0", got)
	}
}

func TestDownsampleFrom(tt *testing.T) {
	dst := BlockU8{}
	dst.DownsampleFrom(&pjw16x16)
	if dst != pjw8x8 {
		tt.Fatalf("got:\n%v\nwant:\n%v", dst, pjw8x8)
	}
}

func TestUpsampleFrom(tt *testing.T) {
	want := QuadBlockU8{
		0xFF, 0xFF, 0xFF, 0xEB, 0xC3, 0x93, 0x5C, 0x44, 0x4C, 0x6C, 0xA3, 0xCF, 0xEF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFE, 0xFC, 0xEC, 0xCE, 0xAB, 0x84, 0x67, 0x55, 0x5D, 0x7E, 0xA0, 0xC2, 0xDE, 0xF4, 0xFF,
		0xFF, 0xFC, 0xF6, 0xEE, 0xE4, 0xDB, 0xD3, 0xAC, 0x67, 0x3F, 0x35, 0x43, 0x69, 0x9D, 0xDE, 0xFF,
		0xFF, 0xF2, 0xD8, 0xD3, 0xE3, 0xF0, 0xFA, 0xD6, 0x85, 0x45, 0x17, 0x0F, 0x2D, 0x6A, 0xC5, 0xF3,
		0xFF, 0xE0, 0xA3, 0x9C, 0xCB, 0xEA, 0xF8, 0xE4, 0xAF, 0x6F, 0x25, 0x05, 0x0F, 0x46, 0xA9, 0xDB,
		0xE7, 0xBF, 0x70, 0x69, 0xAA, 0xCF, 0xD7, 0xC6, 0x9D, 0x66, 0x22, 0x00, 0x00, 0x36, 0xA1, 0xD7,
		0xB8, 0x90, 0x40, 0x3B, 0x81, 0xA0, 0x98, 0x7D, 0x4F, 0x2A, 0x0E, 0x00, 0x00, 0x3A, 0xAD, 0xE7,
		0xB4, 0x90, 0x48, 0x43, 0x81, 0x98, 0x88, 0x68, 0x38, 0x1A, 0x0E, 0x0C, 0x14, 0x4F, 0xBC, 0xF3,
		0xDB, 0xBF, 0x87, 0x80, 0xAA, 0xB7, 0xA8, 0x88, 0x58, 0x36, 0x22, 0x24, 0x3C, 0x75, 0xCE, 0xFB,
		0xF3, 0xDC, 0xAE, 0xA5, 0xC1, 0xC5, 0xB2, 0x8F, 0x5D, 0x3B, 0x29, 0x32, 0x56, 0x8E, 0xD9, 0xFF,
		0xFB, 0xE6, 0xBD, 0xB2, 0xC5, 0xC1, 0xA6, 0x7D, 0x47, 0x29, 0x23, 0x36, 0x62, 0x9A, 0xDD, 0xFF,
		0xFF, 0xF0, 0xD3, 0xC7, 0xCC, 0xC0, 0xA3, 0x78, 0x40, 0x26, 0x2A, 0x48, 0x80, 0xB5, 0xE6, 0xFF,
		0xFF, 0xFA, 0xF0, 0xE4, 0xD6, 0xC2, 0xA9, 0x80, 0x48, 0x32, 0x3E, 0x68, 0xAF, 0xDE, 0xF4, 0xFF,
		0xFF, 0xFF, 0xFF, 0xF1, 0xD5, 0xB5, 0x92, 0x69, 0x3B, 0x31, 0x4B, 0x7F, 0xCC, 0xF6, 0xFC, 0xFF,
		0xFF, 0xFF, 0xFF, 0xED, 0xCA, 0x9A, 0x5E, 0x33, 0x19, 0x23, 0x51, 0x8D, 0xD6, 0xFC, 0xFE, 0xFF,
		0xFF, 0xFF, 0xFF, 0xEB, 0xC4, 0x8C, 0x44, 0x18, 0x08, 0x1C, 0x54, 0x94, 0xDB, 0xFF, 0xFF, 0xFF,
	}

	dst := QuadBlockU8{}
	dst.UpsampleFrom(&pjw8x8)
	if dst != want {
		tt.Fatalf("got:\n%v\nwant:\n%v", dst, want)
	}
}
