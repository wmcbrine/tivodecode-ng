/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING file for license terms
 */

#ifndef TIVO_DECODER_MPEG_PARSER_HXX_
#define TIVO_DECODER_MPEG_PARSER_HXX_

#include <cstdint>

class TiVoDecoder_MPEG2_Parser
{
    private:
        uint32_t _bit_ptr;
        bool     _end_of_file;
        uint32_t _buffer_length;
        uint8_t  *_pBuffer;
        uint16_t hdr_len;

        /* start codes */
        uint32_t slice_start_code;

        /* scalable modes */
        uint8_t  data_partitioning;

        /* private variables */
        uint8_t  progressive_sequence;
        uint8_t  repeat_first_field;
        uint8_t  top_field_first;
        uint16_t vertical_size;
        uint8_t  scalable_mode;
        uint8_t  picture_structure;

    public:
        TiVoDecoder_MPEG2_Parser();
        TiVoDecoder_MPEG2_Parser(uint8_t *pBuffer, uint16_t bufLen);
        void setBuffer(uint8_t *pBuffer, uint16_t bufLen);

        inline bool     isEndOfFile() { return _end_of_file; }
        inline uint16_t getReadPos()  { return _bit_ptr / 8; }
        inline void     clear()       { hdr_len = 0;         }

        bool  byteAligned();
        void  advanceBits(uint32_t n);
        uint32_t nextbits(uint32_t n);
        int32_t  readByte(uint32_t bit_pos, uint8_t &byte);

        void  next_start_code();
        void  sequence_end(uint16_t &len);
        void  sequence_header(uint16_t &len);
        void  user_data(uint16_t &len);
        void  sequence_extension(uint16_t &len);
        void  sequence_display_extension(uint16_t &len);
        void  group_of_pictures_header(uint16_t &len);
        void  picture_header(uint16_t &len);
        void  picture_coding_extension(uint16_t &len);
        void  picture_data(uint16_t &len);
        void  extension_header(uint16_t &len);
        void  pes_header(uint16_t &len);
        void  pes_header_extension(uint16_t &len);
        void  ancillary_data(uint16_t &len);

        void  slice(uint16_t &len);
        void  macroblock(uint16_t &len);
};

#endif /* TIVO_DECODER_MPEG_PARSER_HXX_ */
