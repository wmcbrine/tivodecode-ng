tivodecode-ng 0.4
-----------------

  * Have to flush stdout with MSVC.

  * Some tidying up; more C++-ish. See git log for details.

tivodecode-ng 0.3
-----------------

  * Builds on Windows with MSVC; no manual copying of tdconfig.h.

  * Fix for recovery from decrypt error; fix for bad MAK returning
    success. By Hugh Mackworth.

tivodecode-ng 0.2
-----------------

  * Builds on Windows with MinGW; builds without configure.

  * Removed much cruft; fully C++.

tivodecode-ng 0.1
-----------------

  * Improved transport stream support.

  * Renamed project to distinguish from classic tivodecode.

  * Revived tdcat.

  * Minor cleanups.

tivodecode 0.4.1 - 0.4.4
------------------------

  * Tweaks to improve transport stream support; other minor changes.

tivodecode 0.4
--------------

  * Mostly ported to C++. tdcat not working.

tivodecode 0.3pre6
------------------

  * stdin and stdout parameters "-" was not functioning (tdcat.c &
    tivodecode.c)

tivodecode 0.3pre5
------------------

  * add support for tivo files with unencrypted metadata chunks
    (tdcat.c & tivodecode.c)

tivodecode 0.3pre4
------------------

  * same as pre3 with updated config files

tivodecode 0.3pre3
------------------

  * reverted back to functioning '-D' command line argument (tivodecode.c)

  * applied windows stdin/stdout patches for functioning pipes in windows
    (tivodecode.c, tdcat.c, cli_common.h)

tivodecode 0.3pre1
------------------

  * add support for TS files.

  * known bug :
    Packet N of PID XX starts a new series of PES headers, which spill
    over into packet N+1 of PID XX, which has an encrypted payload.
    The problem is how to cleanly make that "continuation" from packet
    N to N+1, such that the offset into Packet N+1 can be determined
    where the decrypt needs to start.

tivodecode 0.2pre4
------------------

  * ?

tivodecode 0.2pre3
------------------

  * add an option to not process video - handy if all you want is to
    dump the metadata

  * fix error in logic rework to get prepare_frame to take stream_id and
    block_id of 0, caused improper decryption of mpeg data

  * add string.h include to md5 to make it work on solaris (seems
    memmove comes from there), remove sys/param.h since I don't know why
    it was there and don't know its portability

  * add support to decrypt metadata and dump out to files.  Needs some
    work to make it easier to use

  * add md5 routines from postgresql's pgcrypto contrib module, slightly
    hacked so that it does not require a 64-bit integer type, which should
    not be an issue for my uses.  md5 is needed to generate the decryption
    keys for the metadata

  * fix turing_stream to work with stream_id of 0
    (needed for work on metadata decrypting)

  * refactor tivo file parsing to expose access to the rest of the data
    in the headers and other data chunks

tivodecode 0.2pre2
------------------

  * fix merge error - some includes were in the wrong place, resulting
    in broken builds due to missing defines from tdconfig.h

tivodecode 0.2pre1
------------------

  * port upstream changes from postgresql's getopt* files, which remove
    the advertising clause from the license on the file

  * add headers to Makefile.am for installation, clean up headers that
    will be installed so that one just includes tivodecoder.h, clean up
    tivodecoder.c so that it does not call exit() since it is called
    from a lib and the caller may not want to exit, instead return -2

  * copy changes to getopt_long.h from t2sami: don't declare extern vars
    for getopt if using the system's getopt - rely on it taking care of
    that.  Caused issue for t2sami on cygwin, tivodecode worked fine,
    but may as well fix it here too

  * openbsd compile fixes - ntohl requires sys/types.h

  * redo define for ftello/fseeko to be more robust, add warning if
    falling back to ftell/fseek for questionable large file support

  * clean up types to prevent warnings on windows.  fix up large
    file support for windows

  * build a library capable of being called by something other than
    the command line tivodecode program

  * rewrite handling of encrypted packets to not have a hardcoded set of
    stream_ids, but to keep track of an arbitrary number of states in a
    circular linked list. There should never be more than 2 or 3 of
    them, AFAIK, so this should be acceptable performance-wise.

tivodecode 0.1.4
----------------

  * add support for storing MAK in a config file, ~/.tivodecode_mak, or
    if HOME (or USERPROFILE on Windows) is not set, /.tivodecode_mak
    (C:/.tivodecode_mak on Windows)

  * tweak largefile support to be more correct for seek and tell, per
    report in forum.  This should not affect anyone, as in order for pipe
    support to work I could not seek anyway.

tivodecode 0.1.3
----------------

  * implement support for encrypted packet type 0xbd, apparently used
    for AC3 audio for 'Humax DVD-burning TiVos'

tivodecode 0.1.2
----------------

  * fix bug introduced recently while fixing signedness warnings on
    gcc-4.1.  Thanks to tateu for the fix.

tivodecode 0.1.1
----------------

  * change include file for htonl after compile farm failure on Mac OSX

tivodecode 0.1.0
----------------

  * apply mak verification patch from Kees Cook
    <kees@outflux.net>, fix signedness warnings in gcc 4.1, add
    permission statement from Kees Cook to file derived from mpegcat
