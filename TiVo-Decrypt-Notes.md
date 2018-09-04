TiVo TS header structure
========================

The TS file maintains the same overall file layout as the PS files.
There is a small header, followed by three "chunks" of info at the
beginning of the TS .TiVo file.


The header
----------

    struct {
        char           filetype[4];
        /* all fields are in network byte order */
        unsigned short dummy_0004;
        unsigned short flags;
        unsigned short dummy_0008;
        unsigned char  mpeg_offset[4]; /* unsigned int */
        unsigned short chunks;
    } raw_tivo_header;


Points of interest
------------------

- "filetype" is always "TiVo".

- dummy_0004 is unknown, at the moment.

- flags & 0x80 is always zero

- flags & 0x40 == 0x40 seems to mean NZ/AUS file.
- flags & 0x40 == 0x00 seems to mean US file.

- flags & 0x20 == 0x20 seems to mean TS file.
- flags & 0x20 == 0x00 seems to mean PS file.

- flags & 0x10 == 0x10 seems to mean HD file.
- flags & 0x10 == 0x00 seems to mean SD file.

- flags & 0x0F == 0x0D seems to mean Series3 unit.
- flags & 0x05 == 0x05 seems to mean DVD capable unit.
- flags & 0x05 == 0x01 seems to mean Series2 unit.

- dummy_0008 is unknown, at the moment.

- "mpeg_offset" is the offset at which the real MPEG data starts
  (typically around 14336).

- "chunks" denotes the number of TiVo-specific chunks of data that
  follow (typically 3).


The chunks
----------

    #define SIZEOF_STREAM_CHUNK 12
    typedef struct tivo_stream_chunk_s {
        unsigned int   chunk_size;    /* Size of chunk */
        unsigned int   data_size;     /* Length of the payload */
        unsigned short id;            /* Chunk ID */
        unsigned short type;          /* Subtype */
        unsigned char  data[1];       /* Variable length data */
    } tivo_stream_chunk;

- Chunk size denotes the size of the current chunk.
- Data size is the size of the current chunk, less the chunk header.
- ID is the sequential chunk number of the current chunk (0-based).

type==0 is a clear-text copyright notice.

    <?xml version="1.0" encoding="utf-8"?>
    <license xmlns="http://www.tivo.com/developer/xml/license-1.0">
    <notice>
    blah blah blah
    </notice>
    <created>
    01/12/2005 11:22:01
    </created>
    <fingerprint>
    ... hex string that is 224 hex digits long ....
    </fingerprint>
    <salt>
    ... hex string that is 32 hex digits long...
    </salt>
    </license>

type==1 is an encrypted copy of the TiVo XML metadata for this file.

The encryption scheme used here is the metadata variant (see "Crytpo"
below) of the Turing stream cipher.

Typically, there are two chunks of this type, and the two are nearly
identical. The only variations I can see are that the second one
includes the "uniqueId" field, which is the unique seriesId (i.e.
SH01234567) used by TiVo to perform grouping in the Now Playing List.


Crypto
======

The encryption method utilized by TiVo is the open source Turing stream
cipher, by Qualcomm.

https://opensource.qualcomm.com/index.php?turing

It follows the usual crypto usage pattern :

- initialize crypto session (to zero)
- "salt" the crypto session with predefined values (MAK, etc)
- pass blocks of the stream data through the cipher to encrypt/decrypt it

The metadata chunks use an initial MD5 pass to generate its Turing salt
value.

The transport stream data packets use an initial SHA-1 pass to generate
its Turing salt value.


Acquiring TiVo TS crypto info
=============================

- We must first acquire the MPEG TS "Program Association Table" (PAT).
  This packet's only useful data element is the "program_MAP_pid" field,
  which instructs us as the to PID of the "Program Map Table" (PMT)
  packet.  The PAT is always located at PID 0.

- Then, we must acquire the MPEG TS "Program Map Table" (PMT).  The PMT
  is a table of the elementary stream pids within the MPEG file, and
  their types.  The PMT may be located at PID identified in the PAT, or
  (more likely) may be encapsulated in an audio/video packet.

  Typically, a TiVo file will contain three streams.

  - 0x21 = video
  - 0x22 = audio
  - 0x23 = private data : this is where TiVo puts the crypto goodies

- Not all packets in the audio and video streams are encrypted.  This is
  verified by comparing encrypted TS files and decrypted MPEG files as
  generated by DirectShowDump.  As such, we only need to worry about the
  packets that are encrypted (which is denoted by the packet's TS header
  transport_scrambling_control field being set to a non-zero value,
  usually 0x3).

- Packets in the "private data" stream contain the necessary info to
  decrypt the audio and video streams within the TiVo MPEG TS file.
  They appear periodically within the packet stream, and the Turing key
  value changes with each new private data packet.


TiVo Private Data format
------------------------

    typedef struct TiVo_Private_Data_s
    {
        unsigned int                filetype;
        unsigned short              validator;
        unsigned short              unknown1;
        unsigned char               unknown2;
        unsigned char               stream_bytes;
        TiVo_Private_Data_Stream    data_streams[N];
    } TiVo_Private_Data;

- The filetype is always "TiVo".

- The validator is always 0x8103.

- The unknown1 field purpose is not known.

- The unknown2 field purpose is not known, but is typically 0x00 or 0x01.

- The stream_bytes field indicates how many bytes containing key
  information follow the TiVo private data header.  (20-bytes per key).

- The num_streams indicates how many TiVo_Private_Data_Stream elements
  follow.

        typedef struct TiVo_Private_Data_Stream_s
        {
            unsigned short      pid;
            unsigned char       stream_id;
            unsigned char       reserved;
            unsigned char       key[16];
        } TiVo_Private_Data_Stream;

  - The pid is the Packet ID for this stream (0x21, 0x22, etc).

  - The stream_id is the TS Stream ID (0xE0 video, 0xC0 audio, etc).

  - The reserved field is always 0x10.

  - The key is the 16-byte Turing key used to decrypt this particular
    stream.

- The Turing key has two additional values folded into it for later
  use in the decrypt process :

  - The first value is the "block_no" value, and indicates the
    Turing block number for this portion of the cipher process.  It
    increments over time within the stream.

  - The second value is the "crypted" value, which appears to be
    some sort of checksum value used by the stream cipher.  In PS
    files, this number appears to be a usable hexadecimal value, and
    as such is used in the decrypt process.  However, in the TS
    files, this number is always zero, and is not used in the
    decrypt process.

- When a TiVo private data packet is encountered, we must update our
  session's Turing values (key, block_no) to the newer values.  Each
  packet stream (i.e. audio, video) within the transport stream will
  have its own Turing session.  So, we must maintain these separate
  Turing sessions across packets.

- When an audio/video packet is encountered, we must first verify if
  it is indeed encrypted by checking the TS header's
  transport_scrambling_control field.  Any non-zero value (typically
  0x3) denotes an encrypted packet.

- We must take care to only decrypt payload data, and not packet
  stream control data.  As such, we must pass over other MPEG TS
  headers, such as the Adaptation field or PES headers.

- When we've finally located the beginning of the encrypted payload
  data, we must regenerate our Turing cipher for this particular
  session.  For this packet, we must :
  - locate this particular PID's Turing session
  - extract the "block_no" and "crypted" values from the Turing key
    using by calling do_header()
  - prepare the Turing frame for decrypt by calling prepare_frame()
    and pass it the PID and block_no.
  - decrypt the payload by calling decrypt_buffer(), passing in a
    pointer to the start of the encrypted payload, and the size of
    the payload to be decrypted.


TiVo PS decrypt methodology
===========================

- Identify in the stream where private data is located (inside the
  private data packets identified as PID 0x23).  This is the crypto
  key.

- Prepare for Turing decrypt operations by passing in this key,
  along with two values (block_no and crypted) into the do_header()
  function.  This does some bitfield math on these three elements,
  and results in new values being inserted into the block_no and
  crypted fields.  The do_header() function was reconstructed from
  the TiVo DLL by other individuals, and the understanding behind it
  is incomplete.  There is some stuff going on in there that is
  unknown.  Although, it's obviously doing the right stuff (at least
  for PS operations).

- We then pass the new block_no and stream_id values into the
  prepare_frame() function to prep the Turing structures for data
  decryption.

- We then pass the new crypted value (4 bytes worth) into the
  decrypt_buffer() function to get the Turing goodies ready for real
  data.

- We then pass the entire amount of decrypted data through the
  decrypt_buffer() function to decrypt the data.  Since this is a
  data stream, the size of this operation can be anywhere from 1000
  to 15000 bytes.

- Rinse and repeat.