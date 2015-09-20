#ifndef __TIVO_DECODER_MPEG_PARSER_HXX__
#define __TIVO_DECODER_MPEG_PARSER_HXX__

#ifdef HAVE_CONFIG_H
#include "tdconfig.h"
#endif

#include <stdint.h>

class TiVoDecoder_MPEG2_Parser
{
    private:
        uint32_t _bit_ptr;
        bool   _end_of_file;
        uint32_t _buffer_length;
        uint8_t  *_pBuffer;
        uint16_t hdr_len;

        /* start codes */
        uint32_t picture_start_code;
        uint32_t slice_start_code;
        uint32_t user_data_start_code;
        uint32_t sequence_header_code;
        uint32_t sequence_error_code;
        uint32_t extension_start_code;
        uint32_t sequence_end_code;
        uint32_t group_start_code;

        /* extension start codes */
        uint8_t sequence_extension_id;
        uint8_t sequence_display_extension_id;
        uint8_t picture_coding_extension_id;

        /* scalable modes */
        uint8_t data_partitioning;
        
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

        inline bool   isEndOfFile() { return _end_of_file; }
        inline uint16_t getReadPos()  { return _bit_ptr / 8; }
        inline void   clear()       { hdr_len = 0;         }

        bool  byteAligned();
        void  advanceBits(uint32_t n);
        int32_t nextbits(uint32_t n);
        int32_t readByte(uint32_t bit_pos, uint8_t &byte);

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

#endif /* __TIVO_DECODER_MPEG_PARSER_HXX__ */
