# pnginfo

The pnginfo utility explores the content of PNG files.

## Contents

1. [Install](#install)
2. [Instructions](#instruction)
3. [License](#license)

## Install

#### Requires

* C compiler ;

### Build

    $ ./configure
    $ make
    # make install

## Instructions

It is possible to list the chunks in a given PNG file or to request the content of a specific chunk.

Examples:

```
$ pnginfo -l -f lena.png 
IHDR
sRGB
IDAT
IEND
$ pnginfo -c IHDR -f lena.png
IHDR: width: 512
IHDR: height: 512
IHDR: bitdepth: 8
IHDR: colourtype: truecolour
IHDR: compression: deflate
IHDR: filter: adaptive
IHDR: interlace method: standard
$ pnginfo -c sRGB -f lena.png
sRGB: rendering intent: perceptual
```

Only the following chunks are implemented right now:

* IHDR ;
* PLTE ;
* IEND ;
* cHRM ;
* gAMA ;
* sBIT ;
* sRGB ;
* tEXt ;
* bKGD ;
* hIST ;
* pHYs ;
* sPLT ;
* tIME.

Chunks that are not part of the 2003 PNG specification, either because they are custom made or part of an extension, are still listed but can't be queried.

The `-s` option can be handy if garbage is placed at the begining of a file. `pnginfo -s` will try to skip said garbage until it finds an acceptable PNG signature.

## License

All the code is licensed under the ISC License.
It's free, not GPLed !
