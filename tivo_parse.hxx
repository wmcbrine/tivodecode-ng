#ifndef TIVO_PARSE_HXX_
#define TIVO_PARSE_HXX_

//#include <stddef.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

#ifdef HAVE_CONFIG_H
#include "tdconfig.h"
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "tivo_types.hxx"
#include "turing_stream.h"

/* genericized read function so that different underlying implementations can
 * be swapped out for more of a library setup
 */
typedef INT32 (*read_func_t)  (void * mem, int size, void * fh);
typedef INT32 (*write_func_t) (void * mem, int size, void * fh);

extern BOOL   isBigEndian();
extern UINT16 portable_ntohs( UINT8 * pVal );
extern UINT32 portable_ntohl( UINT8 * pVal );

extern int hread_wrapper(void * mem, int size, void * fh);
extern int fwrite_wrapper(void * mem, int size, void * fh);

#define static_strlen(str) (sizeof(str) - 1)

extern int o_verbose;

#define VERBOSE(...)        if ( o_verbose >= 1 ) { printf(__VA_ARGS__); }
#define VVERBOSE(...)       if ( o_verbose >= 2 ) { printf(__VA_ARGS__); }
#define VVVERBOSE(...)      if ( o_verbose >= 3 ) { printf(__VA_ARGS__); }
    
#define IS_VERBOSE          ( o_verbose >= 1 )
#define IS_VVERBOSE         ( o_verbose >= 2 )
#define IS_VVVERBOSE        ( o_verbose >= 3 )
    
    
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

/* All elements are in big-endian format and are packed */

class TiVoStreamHeader 
{
    public:

        CHAR        fileType[4];    /* the string 'TiVo' */
        UINT16      dummy_0004;
        UINT16      dummy_0006;
        UINT16      dummy_0008;
        UINT32      mpeg_offset;    /* 0-based offset of MPEG stream */
        UINT16      chunks;         /* Number of metadata chunks */
        
        TiVoFormatType  getFormatType();
        BOOL            read(void * file, read_func_t read_handler);
        void            dump(UINT8 dbgLevel=0);
        UINT16          size() { return(sizeof(TiVoStreamHeader)); };
    
        TiVoStreamHeader();
        
} __attribute__((packed)) ;


class TiVoStreamChunk 
{
    public:
        
        UINT32      chunkSize;  /* Size of chunk */
        UINT32      dataSize;   /* Length of the payload */
        UINT16      id;         /* Chunk ID */
        UINT16      type;       /* Subtype */
        UINT8       * pData;    /* Variable length data */
        
        BOOL   read(void * file,  read_func_t read_handler);       
        BOOL   write(void * file, write_func_t write_handler); 
        void   dump(UINT8 dbgLevel=0);
        UINT16 size() { return(sizeof(TiVoStreamChunk)-sizeof(UINT8*)); };
        
        void   setupTuringKey(turing_state * pTuring, UINT8 * pMAK);
        void   setupMetadataKey(turing_state * pTuring, UINT8 * pMAK);
        void   decryptMetadata(turing_state * pTuring, UINT16 offsetVal);
        
        TiVoStreamChunk();
        ~TiVoStreamChunk();
        
} __attribute__((packed));


#endif /* TIVO_PARSE_HXX_ */