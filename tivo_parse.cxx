/*
 * tivodecode-ng
 * Copyright 2006-2015, Jeremy Drake et al.
 * See COPYING file for license terms
 */

#include <cstdio>
#include <cstring>

#include "hexlib.hxx"
#include "tivo_parse.hxx"

int o_verbose;
bool o_pkt_dump;

uint32_t portable_ntohl(uint8_t *pVal)
{
    return (pVal[0] << 24) | (pVal[1] << 16) | (pVal[2]<<8) | pVal[3];
}

uint32_t portable_ntohl(uint32_t val)
{
    uint8_t *pVal = (uint8_t*) &val;
    return (pVal[0] << 24) | (pVal[1] << 16) | (pVal[2]<<8) | pVal[3];
}

uint16_t portable_ntohs(uint8_t *pVal)
{
    return (pVal[0] << 8) | pVal[1];
}

uint16_t portable_ntohs(uint16_t val)
{
    uint8_t *pVal = (uint8_t*) &val;
    return (pVal[0] << 8) | pVal[1];
}

// ===================================================================

TiVoStreamHeader::TiVoStreamHeader()
{
    std::memset(fileType, 0, 4);
    dummy_0004  = 0;
    dummy_0006  = 0;
    dummy_0008  = 0;
    mpeg_offset = 0;
    chunks      = 0;
}

bool TiVoStreamHeader::read(HappyFile *file)
{    
    if (file->read(this, size()) != size())
    {
        std::perror("read header");
        return false;
    }    
    
    dummy_0004  = portable_ntohs(dummy_0004);
    dummy_0006  = portable_ntohs(dummy_0006);
    dummy_0008  = portable_ntohs(dummy_0008);
    mpeg_offset = portable_ntohl(mpeg_offset);   
    chunks      = portable_ntohs(chunks);
    
    if (std::strncmp(fileType, "TiVo", 4))
    {
        std::perror("Not a TiVo file!");
        return false;    
    }

    return true;
}

void TiVoStreamHeader::dump(uint8_t dbgLevel)
{
    if (IS_VERBOSE)
    {
        const char *unit;

        switch (dummy_0006 & 0x0F)
        {
            case 0x0D : unit = "Series3";     break;
            case 0x05 : unit = "DVD-capable"; break;
            case 0x01 : unit = "Series2";     break;
            default   : unit = "Unknown";
        }

        std::fprintf(stderr, "TiVo Header : \n");
        std::fprintf(stderr, "fileType    : %02x:%02x:%02x:%02x (%c%c%c%c)\n",
                     fileType[0], fileType[1], fileType[2], fileType[3],
                     fileType[0], fileType[1], fileType[2], fileType[3]);

        std::fprintf(stderr, " dummy_0004 : 0x%04x\n", dummy_0004);
        std::fprintf(stderr, " dummy_0006 : 0x%04x\n", dummy_0006);
        std::fprintf(stderr, "     origin : %s\n",
                     (dummy_0006 & 0x40) ? "AUS/NZ" : "US" );
        std::fprintf(stderr, "     format : %s\n",
                     (dummy_0006 & 0x20) ? "Transport Stream" :
                     "Program Stream" );
        std::fprintf(stderr, "     source : %s\n",
                     (dummy_0006 & 0x10) ? "HDTV" : "SDTV");

        std::fprintf(stderr, "  TiVo unit : %s\n", unit);

        std::fprintf(stderr, "dummy_0008  : 0x%04x\n", dummy_0008);
        std::fprintf(stderr, "mpeg_offset : %d\n", mpeg_offset);
        std::fprintf(stderr, "chunks      : %d\n\n", chunks);
    }
}

TiVoFormatType TiVoStreamHeader::getFormatType()
{
    if (dummy_0006 & 0x20)
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
    if (pData)
        delete[] pData;
}

bool TiVoStreamChunk::read(HappyFile *file)
{    
    if (file->read(this, size()) != size())
    {
        std::perror("read chunk");
        return false;
    }    

    chunkSize   = portable_ntohl(chunkSize);
    dataSize    = portable_ntohl(dataSize);
    id          = portable_ntohs(id);
    type        = portable_ntohs(type);

    uint16_t readSize = chunkSize - size(); 
    pData = new uint8_t[readSize];
    if (NULL == pData)
    {
        std::perror("chunk data alloc");
        return false;
    }
    
    if (file->read(pData, readSize) != readSize)
    {
        std::perror("read chunk data");
        return false;
    }     
    
    return true;
}

bool TiVoStreamChunk::write(HappyFile *file)
{
    if (file->write(pData, dataSize) != dataSize)
        return false;
        
    return true; 
}

void TiVoStreamChunk::dump(uint8_t dbgLevel)
{
    if (IS_VERBOSE)
    {
        std::fprintf(stderr, "TiVo Chunk  : (0x%08x) %d\n", id, id);
        std::fprintf(stderr, "chunkSize   : (0x%08x) %d\n", chunkSize,
                     chunkSize);
        std::fprintf(stderr, "dataSize    : (0x%08x) %d\n", dataSize,
                     dataSize);
        std::fprintf(stderr, "type        : (0x%08x) %d\n", type, type);

        hexbulk(pData, dataSize);
        
        std::fprintf(stderr, "\n\n" );
    }
}

void TiVoStreamChunk::setupTuringKey(TuringState *pTuring, uint8_t *pMAK)
{
    if (NULL == pTuring || NULL == pMAK)
    {
        std::perror("bad param");
        return;
    }

    pTuring->setup_key(pData, dataSize, (char *)pMAK);
}

void TiVoStreamChunk::setupMetadataKey(TuringState *pTuring, uint8_t *pMAK)
{
    pTuring->setup_metadata_key(pData, dataSize, (char *)pMAK);

//    std::cerr << "METADATA TURING DUMP : INIT\n";
//    pTuring->dump();
}

void TiVoStreamChunk::decryptMetadata(TuringState *pTuring, uint16_t offsetVal)
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
