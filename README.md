tivodecode-ng version 0.4
=========================

Copyright 2006-2018, Jeremy Drake et al.
See COPYING for license terms.

tivodecode-ng ("ng" for "next generation") is a portable command-line
tool for decrypting .TiVo video files into program streams or transport
streams, depending on the source. It also will decrypt the metadata
(title, description, etc.). It's based on tivodecode by Jeremy Drake,
with enhancements by several contributors. tivodecode-ng adds support
for transport streams, although that's a work in progress. (Note that
support for program streams is as complete as ever, and this version is
faster than classic tivodecode.)

You can build tivodecode-ng with autoconf:

    ./configure
    make
    make install

or without, if the default makefile is compatible with your system:

    make

or with Microsoft Visual C++:

    nmake -f Makefile.msv

You can specify the MAK either on the command line (which takes
precedence), or in a config file called "~/.tivodecode_mak". On Windows,
the file should be in your %USERPROFILE% directory, or in c:\ if there
is no %USERPROFILE% set.


Usage
-----

tivodecode is the main executable, while tdcat is also provided, for an
easier way to handle the metadata.


    tivodecode [--help] [--verbose|-v] [--no-verify|-n] {--mak|-m} mak
               [{--out|-o} outfile] <tivofile>

    --mak, -m          media access key (required)
    --out, -o          output file (default stdout)
    --verbose, -v      verbose (add more v's for more verbosity)
    --no-verify, -n    do not verify MAK while decoding
    --dump-metadata,-D dump metadata from TiVo file to xml files (development)
    --no-video, -x     don't decode video, exit after metadata
    --version, -V      print the version information and exit
    --help, -h         print this help and exit


    tdcat [--help] {--mak|-m} mak [--chunk-1|-1] [--chunk-2|-2]
          [{--out|-o} outfile] <tivofile>

    -m, --mak         media access key (required)
    -o, --out,        output file (default stdout)
    -V, --version,    print the version information and exit
    -h, --help,       print this help and exit

    -1, --chunk-1     output chunk 1 (default if unspecified)
    -2, --chunk-2     output chunk 2


The file name specified for the tivo file may be -, which means stdin.
