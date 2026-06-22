# WebP

WebP is a still image format, described in [RFC
6386](https://www.ietf.org/rfc/rfc6386.txt).

The ["Making Software - Image
Compression"](https://www.makingsoftware.com/chapters/image-compression) book
chapter also has a good overview of a number of image formats, including WebP.

There are two WebP variants: lossy (based on VP8) and lossless. Wuffs' VP8
implementation is in a separate package, as the WebM moving image format also
uses VP8. There's no Wuffs WebM package yet but there might be in the future.
