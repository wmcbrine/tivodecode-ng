/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING file for license terms
 */

#ifndef TIVO_PARSE_HXX_
#define TIVO_PARSE_HXX_

#include <iostream>
#include <string>

#include "happyfile.hxx"
#include "Turing.hxx"
#include "turing_stream.hxx"

extern int  o_verbose;
extern bool o_pkt_dump;

inline bool IS_VERBOSE()   { return o_pkt_dump || (o_verbose >= 1); }
inline bool IS_VVERBOSE()  { return o_pkt_dump || (o_verbose >= 2); }
inline bool IS_VVVERBOSE() { return o_pkt_dump || (o_verbose >= 3); }

inline void VERBOSE(const char *s)   { if (IS_VERBOSE())   std::cerr << s; }
inline void VVERBOSE(const char *s)  { if (IS_VVERBOSE())  std::cerr << s; }
inline void VVVERBOSE(const char *s) { if (IS_VVVERBOSE()) std::cerr << s; }

/*
 * Initial header formats lifted from ezrec's posting:
 * http://www.dealdatabase.com/forum/showthread.php?t=41132
 */

typedef enum
{
    TIVO_FORMAT_NONE = 0,
    TIVO_FORMAT_PS,
    TIVO_FORMAT_TS,
    TIVO_FORMAT_MAX
}
TiVoFormatType;

typedef enum
{
    TIVO_CHUNK_PLAINTEXT_XML = 0,
    TIVO_CHUNK_ENCRYPTED_XML = 1
}
TiVoChunkType;

class TiVoStreamHeader
{
    public:
        uint16_t flags;
        uint32_t mpeg_offset;    /* 0-based offset of MPEG stream */
        uint16_t chunks;         /* Number of metadata chunks */

        TiVoFormatType getFormatType();
        bool           read(HappyFile *file);
        void           dump();

        TiVoStreamHeader();
};

class TiVoStreamChunk
{
    public:
        uint32_t chunkSize;  /* Size of chunk */
        uint32_t dataSize;   /* Length of the payload */
        uint16_t id;         /* Chunk ID */
        uint16_t type;       /* Subtype */
        uint8_t  *pData;     /* Variable length data */

        bool     read(HappyFile *file);
        bool     write(HappyFile *file);
        void     dump();

        void     setupTuringKey(TuringState &turing, const std::string &mak);
        void     setupMetadataKey(TuringState &turing, const std::string &mak);
        void     decryptMetadata(TuringState &turing, uint16_t offsetVal);

        TiVoStreamChunk();
        ~TiVoStreamChunk();
};

#endif /* TIVO_PARSE_HXX_ */
