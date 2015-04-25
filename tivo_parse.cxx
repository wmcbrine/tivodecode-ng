/*
* tivodecode, (c) 2006, Jeremy Drake
* See COPYING file for license terms
*/
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

#include "hexlib.hxx"
#include "tivo_parse.hxx"

int o_verbose;
BOOL o_pkt_dump;

UINT32 portable_ntohl( UINT8 * pVal )
{
    return (pVal[0] << 24) | (pVal[1] << 16) | (pVal[2]<<8) | pVal[3];
}

UINT32 portable_ntohl( UINT32 val )
{
    UINT8 * pVal = (UINT8*) &val;
    return (pVal[0] << 24) | (pVal[1] << 16) | (pVal[2]<<8) | pVal[3];
}

UINT16 portable_ntohs( UINT8 * pVal )
{
    return (pVal[0] << 8) | pVal[1];
}

UINT16 portable_ntohs( UINT16 val )
{
    UINT8 * pVal = (UINT8*) &val;
    return (pVal[0] << 8) | pVal[1];
}

// ===================================================================

TiVoStreamHeader::TiVoStreamHeader()
{
    memset(fileType, 0, 4);
    dummy_0004  = 0;
    dummy_0006  = 0;
    dummy_0008  = 0;
    mpeg_offset = 0;
    chunks      = 0;
}

BOOL TiVoStreamHeader::read(HappyFile *file)
{    
    if (file->read(this, size()) != size())
    {
        perror("read header");
        return FALSE;
    }    
    
    dummy_0004  = portable_ntohs(dummy_0004);
    dummy_0006  = portable_ntohs(dummy_0006);
    dummy_0008  = portable_ntohs(dummy_0008);
    mpeg_offset = portable_ntohl(mpeg_offset);   
    chunks      = portable_ntohs(chunks);
    
    if( strncmp(fileType,"TiVo", 4 ) )
    {
        perror("Not a TiVo file!");
        return FALSE;    
    }

    return TRUE;
}

void TiVoStreamHeader::dump(UINT8 dbgLevel)
{
    VERBOSE( "TiVo Header : \n");
    VERBOSE( "fileType    : %02x:%02x:%02x:%02x (%c%c%c%c)\n", 
        fileType[0], fileType[1], fileType[2], fileType[3], 
        fileType[0], fileType[1], fileType[2], fileType[3] );
        
    VERBOSE( " dummy_0004 : 0x%04x\n", dummy_0004 );
    
    VERBOSE( " dummy_0006 : 0x%04x\n", dummy_0006 );
    VERBOSE( "     origin : %s\n", 
        (dummy_0006 & 0x40) ? "AUS/NZ" : "US" );
    VERBOSE( "     format : %s\n", 
        (dummy_0006 & 0x20) ? "Transport Stream" : "Program Stream" );  
    VERBOSE( "     source : %s\n", 
        (dummy_0006 & 0x10) ? "HDTV" : "SDTV" ); 
            
    char unit[10];            
    switch( dummy_0006 & 0x0F )
    {
        case 0x0D : sprintf(unit, "Series3");     break;
        case 0x05 : sprintf(unit, "DVD-capable"); break;
        case 0x01 : sprintf(unit, "Series2");     break;
        default   : sprintf(unit, "Unknown"); 
    }
            
    VERBOSE( "  TiVo unit : %s\n", unit); 
                        
    VERBOSE( "dummy_0008  : 0x%04x\n", dummy_0008 );
    VERBOSE( "mpeg_offset : %d\n", mpeg_offset );
    VERBOSE( "chunks      : %d\n\n", chunks );
}


TiVoFormatType TiVoStreamHeader::getFormatType()
{
    if( dummy_0006 & 0x20 )
    {
        return TIVO_FORMAT_TS;
    }
    else
    {
        return TIVO_FORMAT_PS;
    }
}

// ===================================================================

TiVoStreamChunk::TiVoStreamChunk()
{
    chunkSize = 0;
    dataSize  = 0;
    id        = 0;
    type      = 0;
    pData     = NULL;
}

TiVoStreamChunk::~TiVoStreamChunk()
{
    if(pData)
    {
        delete [] pData;
    }
}

BOOL TiVoStreamChunk::read(HappyFile *file)
{    
    if (file->read(this, size()) != size())
    {
        perror ("read chunk");
        return FALSE;
    }    
    
    chunkSize   = portable_ntohl(chunkSize);
    dataSize    = portable_ntohl(dataSize);
    id          = portable_ntohs(id);
    type        = portable_ntohs(type);
    
    UINT16 readSize = chunkSize - size(); 
    pData = new UINT8[readSize];
    if(NULL == pData)
    {
        perror("chunk data alloc");
        return FALSE;
    }
    
    if (file->read(pData, readSize) != readSize)
    {
        perror("read chunk data");
        return FALSE;
    }     
    
    return TRUE;
}

BOOL TiVoStreamChunk::write(FILE * file)
{
    if(fwrite(pData, 1, dataSize, file) != dataSize)
        return FALSE;
        
    return TRUE; 
}

void TiVoStreamChunk::dump(UINT8 dbgLevel)
{
    VERBOSE( "TiVo Chunk  : (0x%08x) %d\n", id, id);
    VERBOSE( "chunkSize   : (0x%08x) %d\n", chunkSize, chunkSize );
    VERBOSE( "dataSize    : (0x%08x) %d\n", dataSize,  dataSize );
    VERBOSE( "type        : (0x%08x) %d\n", type,      type );
    
    if( IS_VERBOSE )
        hexbulk(pData, dataSize);
        
    VERBOSE( "\n\n" );
}

void TiVoStreamChunk::setupTuringKey(TuringState *pTuring, UINT8 *pMAK)
{
    if (NULL == pTuring || NULL == pMAK)
    {
        perror("bad param");
        return;
    }

    pTuring->setup_key(pData, dataSize, (char *)pMAK);
}

void TiVoStreamChunk::setupMetadataKey(TuringState *pTuring, UINT8 *pMAK)
{
    pTuring->setup_metadata_key(pData, dataSize, (char *)pMAK);

//    std::cerr << "METADATA TURING DUMP : INIT\n";
//    pTuring->dump();
}

void TiVoStreamChunk::decryptMetadata(TuringState *pTuring, UINT16 offsetVal)
{
//    std::cerr << "METADATA TURING DECRYPT : INIT : offsetVal " << offsetVal << "\n";
//    pTuring->dump();

    pTuring->prepare_frame(0, 0);

//    std::cerr << "METADATA TURING DECRYPT : AFTER PREPARE\n";
//    pTuring->dump();

    pTuring->skip_data(offsetVal);

//    std::cerr << "METADATA TURING DECRYPT : AFTER SKIP\n";
//    pTuring->dump(pTuring);

    pTuring->decrypt_buffer(pData, dataSize);

//    std::cerr << "METADATA TURING DECRYPT : AFTER DECRYPT\n";
//    pTuring->dump();
}

/* vi:set ai ts=4 sw=4 expandtab: */

