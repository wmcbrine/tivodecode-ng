#ifndef TIVO_PARSE_HXX_
#define TIVO_PARSE_HXX_

#include "happyfile.hxx"
#include "Turing.hxx"
#include "turing_stream.hxx"

extern uint16_t portable_ntohs(uint8_t *pVal);
extern uint32_t portable_ntohl(uint8_t *pVal);

#define static_strlen(str) (sizeof(str) - 1)

extern int  o_verbose;
extern bool o_pkt_dump;

#define IS_VERBOSE     ( (o_pkt_dump) || (o_verbose >= 1) )
#define IS_VVERBOSE    ( (o_pkt_dump) || (o_verbose >= 2) )
#define IS_VVVERBOSE   ( (o_pkt_dump) || (o_verbose >= 3) )

#define VERBOSE(...)   if (IS_VERBOSE)   { std::fprintf(stderr, __VA_ARGS__); }
#define VVERBOSE(...)  if (IS_VVERBOSE)  { std::fprintf(stderr, __VA_ARGS__); }
#define VVVERBOSE(...) if (IS_VVVERBOSE) { std::fprintf(stderr, __VA_ARGS__); }

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
        uint16_t dummy_0006;
        uint32_t mpeg_offset;    /* 0-based offset of MPEG stream */
        uint16_t chunks;         /* Number of metadata chunks */

        TiVoFormatType getFormatType();
        bool           read(HappyFile *file);
        void           dump();
        uint16_t       size() { return sizeof(TiVoStreamHeader); };

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
        uint16_t size() { return sizeof(TiVoStreamChunk) - sizeof(uint8_t*); };

        void     setupTuringKey(TuringState *pTuring, uint8_t *pMAK);
        void     setupMetadataKey(TuringState *pTuring, uint8_t *pMAK);
        void     decryptMetadata(TuringState *pTuring, uint16_t offsetVal);

        TiVoStreamChunk();
        ~TiVoStreamChunk();
};

#endif /* TIVO_PARSE_HXX_ */
