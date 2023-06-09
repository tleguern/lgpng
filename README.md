# lgpng

The lgpng library allows to explore the PNG file format.
It is not (for now) intended to actually decode the pixels to display an image.

Some utilities using lgpng are provided in this repository.

## Contents

1. [Install](#install)
2. [pnginfo](#pnginfo)
3. [License](#license)

## Install

#### Requires

* C compiler ;
* zlib.

### Build

    $ ./configure
    $ make
    # make install

Alternatively install in your `$HOME/bin`:

    $ ./configure PREFIX=~/
    $ make
    $ make install

### Tests

A few regression tests are available when invoking the command:

    $ make regress

## pnginfo

It is possible to list the chunks in a given PNG file or to request the details of a specific chunk.

Examples:

```
$ pnginfo -f lena.png -l
IHDR
sRGB
IDAT
IEND
$ pnginfo -f lena.png -c IHDR
IHDR: width: 512
IHDR: height: 512
IHDR: bitdepth: 8
IHDR: colourtype: truecolour
IHDR: compression: deflate
IHDR: filter: adaptive
IHDR: interlace method: standard
$ cat lena.ong | pnginfo -c sRGB
sRGB: rendering intent: perceptual
```

Here is a list of supported chunks:

* PNG specification (Third edition):
  * IHDR
  * PLTE
  * IDAT
  * IDEN
  * tRNS
  * cHRM
  * gAMA
  * iCCP
  * sBIT
  * sRGB
  * cICP
  * tEXt
  * zTXt
  * bKGD
  * hIST
  * pHYs
  * sPLT
  * eXIf
  * tIME
  * acTL
  * fcTL
  * fdAT
* Extensions to the PNG specification (Version 1.6.0):
  * oFFs
  * gIFg
  * gIFx
  * sTER
* ImageMagick private chunks:
  * vpAg
  * caNv
  * orNt
* Skitch/Evernote private chunks:
  * skMf
  * skRf
* Microsoft private chunks:
  * msOG
* Worms Armageddon private chunks:
  * waLV
* GLDPNG private chunks:
  * tpNG

Unknown chunks can still be listed and queried but only very basic informations will be displayed.

The `-s` option can be handy if garbage is placed at the beginning of a file. `pnginfo -s` will try to skip said garbage until it finds an acceptable PNG signature.

Example:

```sh
$ curl https://example.org/file.png | pnginfo -s -l
```
## License

All the code is licensed under the ISC License.
It's free, not GPLed !
