/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING file for license terms
 */

#include <cstdio>
#include <cstring>

#include "hexlib.hxx"
#include "tivo_parse.hxx"

int o_verbose;
bool o_pkt_dump;

uint32_t get32(uint8_t *pVal)
{
    return (pVal[0] << 24) | (pVal[1] << 16) | (pVal[2] << 8) | pVal[3];
}

uint16_t get16(uint8_t *pVal)
{
    return (pVal[0] << 8) | pVal[1];
}

// ===================================================================

TiVoStreamHeader::TiVoStreamHeader()
{
    flags       = 0;
    mpeg_offset = 0;
    chunks      = 0;
}

bool TiVoStreamHeader::read(HappyFile *file)
{
    uint8_t buffer[16];

    if (file->read(buffer, 16) != 16)
    {
        std::perror("read header");
        return false;
    }

    if (std::strncmp((char *)buffer, "TiVo", 4))
    {
        std::perror("Not a TiVo file!");
        return false;
    }

    flags       = get16(buffer + 6);
    mpeg_offset = get32(buffer + 10);
    chunks      = get16(buffer + 14);

    return true;
}

void TiVoStreamHeader::dump()
{
    if (IS_VERBOSE)
    {
        const char *unit;

        switch (flags & 0x0F)
        {
            case 0x0D : unit = "Series3";     break;
            case 0x05 : unit = "DVD-capable"; break;
            case 0x01 : unit = "Series2";     break;
            default   : unit = "Unknown";
        }

        std::fprintf(stderr, "TiVo Header : \n");

        std::fprintf(stderr, " flags : 0x%04x\n", flags);
        std::fprintf(stderr, "     origin : %s\n",
                     (flags & 0x40) ? "AUS/NZ" : "US" );
        std::fprintf(stderr, "     format : %s\n",
                     (flags & 0x20) ? "Transport Stream" :
                     "Program Stream" );
        std::fprintf(stderr, "     source : %s\n",
                     (flags & 0x10) ? "HDTV" : "SDTV");

        std::fprintf(stderr, "  TiVo unit : %s\n", unit);

        std::fprintf(stderr, "mpeg_offset : %d\n", mpeg_offset);
        std::fprintf(stderr, "chunks      : %d\n\n", chunks);
    }
}

TiVoFormatType TiVoStreamHeader::getFormatType()
{
    if (flags & 0x20)
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
    uint8_t buffer[12];

    if (file->read(buffer, 12) != 12)
    {
        std::perror("read chunk");
        return false;
    }

    chunkSize   = get32(buffer);
    dataSize    = get32(buffer + 4);
    id          = get16(buffer + 8);
    type        = get16(buffer + 10);

    uint16_t readSize = chunkSize - 12;
    pData = new uint8_t[readSize];

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

void TiVoStreamChunk::dump()
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
