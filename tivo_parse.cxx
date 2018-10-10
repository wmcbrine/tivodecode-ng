/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING file for license terms
 */

#include <cstring>
#include <iostream>
#include <string>

#include "bits.hxx"
#include "hexlib.hxx"
#include "tivo_parse.hxx"

int o_verbose;
bool o_pkt_dump;

TiVoStreamHeader::TiVoStreamHeader()
{
    flags       = 0;
    mpeg_offset = 0;
    chunks      = 0;
}

bool TiVoStreamHeader::read(HappyFile &file)
{
    uint8_t buffer[16];

    if (file.read(buffer, 16) != 16)
    {
        std::perror("read header");
        return false;
    }

    if (std::strncmp((char *)buffer, "TiVo", 4))
    {
        std::perror("Not a TiVo file!");
        return false;
    }

    flags       = GET16(buffer + 6);
    mpeg_offset = GET32(buffer + 10);
    chunks      = GET16(buffer + 14);

    return true;
}

void TiVoStreamHeader::dump()
{
    if (IS_VERBOSE())
    {
        const char *unit;

        switch (flags & 0x0F)
        {
            case 0x0D : unit = "Series3";     break;
            case 0x05 : unit = "DVD-capable"; break;
            case 0x01 : unit = "Series2";     break;
            default   : unit = "Unknown";
        }

        std::cerr << "TiVo Header : \n"
                  << " flags : " << flags
                  << "\n     origin : "
                  << ((flags & 0x40) ? "AUS/NZ" : "US")
                  << "\n     format : "
                  << ((flags & 0x20) ? "Transport Stream" : "Program Stream")
                  << "\n     source : "
                  << ((flags & 0x10) ? "HDTV" : "SDTV")
                  << "\n  TiVo unit : " << unit
                  << "\nmpeg_offset : " << mpeg_offset
                  << "\nchunks      : " << chunks << "\n\n";
    }
}

TiVoFormatType TiVoStreamHeader::getFormatType()
{
    return (flags & 0x20) ? TIVO_FORMAT_TS : TIVO_FORMAT_PS;
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

bool TiVoStreamChunk::read(HappyFile &file)
{
    uint8_t buffer[12];

    if (file.read(buffer, 12) != 12)
    {
        std::perror("read chunk");
        return false;
    }

    chunkSize   = GET32(buffer);
    dataSize    = GET32(buffer + 4);
    id          = GET16(buffer + 8);
    type        = GET16(buffer + 10);

    uint16_t readSize = chunkSize - 12;
    pData = new uint8_t[readSize];

    if (file.read(pData, readSize) != readSize)
    {
        std::perror("read chunk data");
        return false;
    }

    return true;
}

bool TiVoStreamChunk::write(HappyFile &file)
{
    if (file.write(pData, dataSize) != dataSize)
        return false;

    return true;
}

void TiVoStreamChunk::dump()
{
    if (IS_VERBOSE())
    {
        std::cerr << "TiVo Chunk  : " << id
                  << "\nchunkSize   : " << chunkSize
                  << "\ndataSize    : " << dataSize
                  << "\ntype        : " << type << "\n\n";

        hexbulk(pData, dataSize);

        std::cerr << "\n\n";
    }
}

void TiVoStreamChunk::setupTuringKey(TuringState &turing,
                                     const std::string &mak)
{
    turing.setup_key(pData, dataSize, mak);
}

void TiVoStreamChunk::setupMetadataKey(TuringState &turing,
                                       const std::string &mak)
{
    turing.setup_metadata_key(pData, dataSize, mak);
}

void TiVoStreamChunk::decryptMetadata(TuringState &turing, uint16_t offsetVal)
{
    turing.prepare_frame(0, 0);
    turing.active->skip_data(offsetVal);
    turing.active->decrypt_buffer(pData, dataSize);
}
