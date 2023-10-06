#!/bin/bash -eu
# Copyright 2021 The Wuffs Authors.
#
# Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
# https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
# <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
# option. This file may not be copied, modified, or distributed
# except according to those terms.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT

# ----------------

# This script prints the CRC-32 checksum of the decoded pixels (in the NIA
# format, see doc/spec/nie-spec.md) of every image given as a command line
# argument: if a file then itself, if a directory then the files under it. It
# skips any non-image file found (non-image meaning not decodable by Wuffs'
# standard library).
#
# It is not perfect. It can have false positives and false negatives.
# Nonetheless, running it regularly (compiled against the in-development
# release/c/wuffs-unsupported-snapshot.c) can help detect regressions.
#
# "False positive" means that a non-image file can produce a non-zero checksum.
# Any file (containing binary data) starting with two '\x00' NUL bytes can
# sometimes be mistaken for a WBMP-formatted image file.
#
# "False negative" means that an image file that produces an all-zero checksum
# will be skipped.

if [ ! -e wuffs-root-directory.txt ]; then
  echo "$0 should be run from the Wuffs root directory."
  exit 1
elif [ ! -e gen/bin/example-convert-to-nia ]; then
  echo "Run \"./build-example.sh example/convert-to-nia\" first."
  exit 1
elif [ ! -e gen/bin/example-crc32 ]; then
  echo "Run \"./build-example.sh example/crc32\" first."
  exit 1
fi

sources=$@
if [ $# -eq 0 ]; then
  sources=test/data
fi

# ----

handle() {
  local c=$(gen/bin/example-convert-to-nia <$1 2>/dev/null | gen/bin/example-crc32)
  if [ "$c" != "00000000" ]; then
    echo $c $1
  fi
}

# ----

echo "# Generated by script/print-nia-checksums.sh"
for f in $sources; do
  if [ -f $f ]; then
    handle $f
  elif [ -d $f ]; then
    for g in `find $f -type f | LANG=C sort`; do
      handle $g
    done
  else
    echo "Could not open $f"
    exit 1
  fi
done
