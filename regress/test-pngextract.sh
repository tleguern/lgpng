#!/bin/sh

cd "$(dirname "$0")"

echo "TAP version 13"
echo "1..2"

#
# skRf.dat is the data part of a `skRf` chunk, containing a small header and an
# embedded PNG file.
#

code=0

if [ "$(../pngextract -f skRf.dat | file -bi -)" = "image/png" ]; then
	echo "ok 1 - read file as argument"
else
	echo "not ok 1 - read file as argument"
	code=1
fi

if [ "$(cat skRf.dat | ../pngextract | file -bi -)" = "image/png" ]; then
	echo "ok 2 - read file from stdin"
else
	echo "not ok 2 - read file from stdin"
	code=1
fi

exit $code
