/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING file for license terms
 */

#include <iostream>

#include "tivo_decoder_mpeg_parser.hxx"

TiVoDecoder_MPEG2_Parser::TiVoDecoder_MPEG2_Parser()
{
    _bit_ptr = 0;
    _end_of_file = false;
    _buffer_length = 0;
    _pBuffer = NULL;
    hdr_len = 0;

    /* start codes */
    slice_start_code        = 0x101;

    /* scalable modes */
    data_partitioning       = 0x0;

    progressive_sequence    = 0x00;
    repeat_first_field      = 0x00;
    top_field_first         = 0x00;
    vertical_size           = 0x00;
    scalable_mode           = 0x00;
    picture_structure       = 0x00;
}

TiVoDecoder_MPEG2_Parser::TiVoDecoder_MPEG2_Parser(uint8_t *pBuffer,
                                                   uint16_t bufLen)
{
    _pBuffer = pBuffer;
    _buffer_length = bufLen;
    _bit_ptr = 0;
    _end_of_file = false;
    hdr_len = 0;

    /* start codes */
    slice_start_code     = 0x101;

    /* scalable modes */
    data_partitioning    = 0x0;

    progressive_sequence = 0x00;
    repeat_first_field   = 0x00;
    top_field_first      = 0x00;
    vertical_size        = 0x00;
    scalable_mode        = 0x00;
    picture_structure    = 0x00;
}

void TiVoDecoder_MPEG2_Parser::setBuffer(uint8_t *pBuffer, uint16_t bufLen)
{
    _pBuffer = pBuffer;
    _buffer_length = bufLen;
    _bit_ptr = 0;
    _end_of_file = false;
}

bool TiVoDecoder_MPEG2_Parser::byteAligned()
{
    uint32_t align_mode = _bit_ptr % 8;
    return !align_mode;
}

void TiVoDecoder_MPEG2_Parser::advanceBits(uint32_t n)
{
    _bit_ptr += n;
    hdr_len  += n;

    if (_bit_ptr >= (_buffer_length * 8))
        _end_of_file = true;
}

uint32_t TiVoDecoder_MPEG2_Parser::nextbits(uint32_t n)
{
    uint32_t bit_ptr_copy = _bit_ptr;
    uint8_t  alignDelta   = bit_ptr_copy % 8;
    uint8_t  byte         = 0;
    int32_t  n_copy       = n;
    int32_t  nread        = 0;
    uint32_t value        = 0;

    if (alignDelta)
    {
        int i = 0;
        int j = 0;

        bit_ptr_copy -= alignDelta;
        nread         = readByte(bit_ptr_copy, byte);

        for (i = 0; i < alignDelta; i++)
        {
            byte &= ~(1 << (7 - i));
            bit_ptr_copy++;
        }

        for (j = alignDelta; (j < 8) && n_copy; j++, n_copy--)
            bit_ptr_copy++;

        if (0 == n_copy)
            for ( ; j < 8; j++)
                byte = byte >> 1;

        value = byte;
    }

    while (n_copy > 0)
    {
        nread = readByte(bit_ptr_copy, byte);
        if (nread == 0)
        {
            _end_of_file = true;
            std::cerr << "EOF\n";
            return -1;
        }

        bit_ptr_copy += 8;
        n_copy       -= 8;
        value = (value << 8) | byte;
    }

    if (n_copy < 0)
        value = value >> (0 - n_copy);

    return value;
}

int32_t TiVoDecoder_MPEG2_Parser::readByte(uint32_t bit_pos, uint8_t &byte)
{
    byte = 0;

    if (bit_pos > _buffer_length * 8)
    {
        _end_of_file = true;
        return -1;
    }

    byte = _pBuffer[bit_pos / 8];
    return 1;
}

// =========================================================================

void TiVoDecoder_MPEG2_Parser::next_start_code()
{
    bool aligned = byteAligned();
    while (false == aligned)
    {
        advanceBits(1);
        aligned = byteAligned();
    }

    while (false == _end_of_file)
    {
        if (0x000001 == nextbits(24))
            break;
        else if (0x000000 == nextbits(24))
            advanceBits(8);
        else
            break;
    }
}

// =========================================================================

void TiVoDecoder_MPEG2_Parser::sequence_end(uint16_t &len)
{
//  sequence_header_code:32;
    advanceBits(32);
    len = hdr_len;
}

void TiVoDecoder_MPEG2_Parser::ancillary_data(uint16_t &len)
{
//  sequence_header_code:32;
    advanceBits(32);
    len = hdr_len;
}

void TiVoDecoder_MPEG2_Parser::sequence_header(uint16_t &len)
{
//  sequence_header_code:32;
//  horizontal_size:12;
    advanceBits(44);

//  vertical_size:12;
    vertical_size = nextbits(12);
    advanceBits(12);

//  aspect_ratio_information:4;
//  frame_rate_code:4;
//  bit_rate_value:18;
//  marker_bit:1 = 1;
//  vbv_buffer_size_value:10;
//  constrained_parameters_flag:1;
    advanceBits(38);

//  load_intra_quantiser_matrix:1;
    uint32_t load_intra_quantiser_matrix = nextbits(1);
    advanceBits(1);

    if (load_intra_quantiser_matrix == 1)
    {
//      intra_quantiser_matrix:8[64];
        advanceBits(8 * 64);
    }

//  load_non_intra_quantiser_matrix:1;
    uint32_t load_non_intra_quantiser_matrix = nextbits(1);
    advanceBits(1);

    if (load_non_intra_quantiser_matrix == 1)
    {
//      non_intra_quantiser_matrix:8[64];
        advanceBits(8 * 64);
    }

    next_start_code();
    len = hdr_len;
}

void TiVoDecoder_MPEG2_Parser::user_data(uint16_t &len)
{
//  user_data_start_code:32;
    advanceBits(32);

    while (nextbits(24) != 0x000001)
    {
//      user_data++:8;
        advanceBits(8);
    }

    next_start_code();
    len = hdr_len;
}

void TiVoDecoder_MPEG2_Parser::extension_header(uint16_t &len)
{
//  extension_start_code:32;
    advanceBits(32);

    uint8_t type = nextbits(4);

    if (1 == type)
        sequence_extension(len);
    else if (2 == type)
        sequence_display_extension(len);
    else if (8 == type)
        picture_coding_extension(len);
    len = hdr_len;
}

void TiVoDecoder_MPEG2_Parser::sequence_extension(uint16_t &len)
{
//  extension_start_code:32;
//  advanceBits(32);

//  sequence_extension_id:4;
//  profile_and_level_indication:8;
    advanceBits(12);

//  progressive_sequence:1;
    progressive_sequence = nextbits(1);
    advanceBits(1);

//  chroma_format:2;
//  horizontal_size_extension:2;
//  vertical_size_extension:2;
//  bit_rate_extension:12;
//  marker_bit:1 = 1;
//  vbv_buffer_size_extension:8;
//  low_delay:1;
//  frame_rate_extension_n:2;
//  frame_rate_extension_d:5;
    advanceBits(35);

    next_start_code();
    len = hdr_len;
}

void TiVoDecoder_MPEG2_Parser::sequence_display_extension(uint16_t &len)
{
//  extension_start_code:32;
//  advanceBits(32);

//  sequence_display_extension_id:4;
//  video_format:3;
    advanceBits(7);

//  colour_description:1;
    uint8_t colour_description = nextbits(1);
    advanceBits(1);

    if (colour_description == 1)
    {
//      colour_primaries:8;
//      transfer_characteristics:8;
//      matrix_coefficients:8;
        advanceBits(24);
    }

//  display_horizontal_size:14;
//  marker_bit:1 = 1;
//  display_vertical_size:14;
//  marker_bit:3 = 1;
    advanceBits(30);

    next_start_code();
    len = hdr_len;
}

void TiVoDecoder_MPEG2_Parser::picture_coding_extension(uint16_t &len)
{
//  extension_start_code:32;
//  advanceBits(32);

//  picture_coding_extension_id:4;
//  f_code:4[2][2];
//  intra_dc_precision:2;
    advanceBits(22);

//  picture_structure:2;
    picture_structure = nextbits(2);
    advanceBits(2);

//  top_field_first:1;
    top_field_first = nextbits(1);
    advanceBits(1);

//  frame_pred_frame_dct:1;
//  concealment_motion_vectors:1;
//  q_scale_type:1;
//  intra_vlc_format:1;
//  alternate_scan:1;
    advanceBits(5);

//  repeat_first_field:1;
    repeat_first_field = nextbits(1);
    advanceBits(1);

//  chroma_420_type:1;
//  progressive_frame:1;
    advanceBits(2);

//  composite_display_flag:1;
    uint8_t composite_display_flag = nextbits(1);
    advanceBits(1);

    if (composite_display_flag == 1)
    {
//      v_axis:1;
//      field_sequence:3;
//      sub_carrier:1;
//      burst_amplitude:7;
//      sub_carrier_phase:8;
        advanceBits(20);
    }

    next_start_code();
    len = hdr_len;
}

void TiVoDecoder_MPEG2_Parser::group_of_pictures_header(uint16_t &len)
{
//  group_start_code:32;
//  time_code:25;
//  closed_gop:1;
//  broken_link:1;
    advanceBits(59);

    next_start_code();
    len = hdr_len;
}

void TiVoDecoder_MPEG2_Parser::picture_header(uint16_t &len)
{
//  picture_start_code:32;
//  temporal_reference:10;
    advanceBits(42);

//  picture_coding_type:3;
    uint8_t picture_coding_type = nextbits(3);
    advanceBits(3);

//  vbv_delay:16;
    advanceBits(16);

    if ((picture_coding_type == 2) || (picture_coding_type == 3))
    {
//      full_pel_forward_vector:1;
//      forward_f_code:3;
        advanceBits(4);
    }

    if (picture_coding_type == 3)
    {
//      full_pel_backward_vector:1;
//      backward_f_code:3;
        advanceBits(4);
    }

    while (nextbits(1) == 1)
    {
//      extra_bit_picture:1 = 1;
//      extra_information_picture++:8;
        advanceBits(9);
    }

//  extra_bit_picture:1 = 0;
//    advanceBits(1); // ????

    len = hdr_len;

//    next_start_code(); // ???
}

void TiVoDecoder_MPEG2_Parser::pes_header(uint16_t &len)
{
//  picture_start_code:24;
    advanceBits(24);

    bool extensionPresent = false;
    uint8_t streamId      = nextbits(8);

    advanceBits(8);

    if (streamId == 0xBD)
        extensionPresent = true;
    else if (streamId == 0xBE)
        extensionPresent = false;
    else if (streamId == 0xBF)
        extensionPresent = false;
    else if ((streamId >= 0xC0) && (streamId <= 0xDF))
        extensionPresent = true;
    else if ((streamId >= 0xE0) && (streamId <= 0xEF))
        extensionPresent = true;

    advanceBits(16);

    if (true == extensionPresent)
        pes_header_extension(len);

    next_start_code();
    len = hdr_len;
}

void TiVoDecoder_MPEG2_Parser::pes_header_extension(uint16_t &len)
{
    uint8_t pes_private_data_flag = 0;
    uint8_t pack_header_field_flag = 0;
    uint8_t program_packet_sequence_counter_flag = 0;
    uint8_t p_std_buffer_flag = 0;
    uint8_t pes_extension_field_length = 0;
    uint8_t PTS_DTS_flags = 0;
    uint8_t ESCR_flag = 0;
    uint8_t ES_rate_flag = 0;
    uint8_t additional_copy_flag = 0;
    uint8_t PES_CRC_flag = 0;
    uint8_t pes_extension_flag2 = 0;
    uint8_t PES_extension_flag = 0;

//  marker_bit:2 = 0;
//  pes_scrambling_control:2 = 0;
//  pes_priority:1 = 0;
//  data_alignment_indicator:1 = 0;
//  copyright:1 = 0;
//  original_or_copy:1 = 0;
    advanceBits(8);

//  PTS_DTS_flags:2 = 0;
    PTS_DTS_flags = nextbits(2);
    advanceBits(2);

//  ESCR_flag:1 = 0;
    ESCR_flag = nextbits(1);
    advanceBits(1);

//  ES_rate_flag:1 = 0;
    ES_rate_flag = nextbits(1);
    advanceBits(1);

//  DSM_trick_mode_flag:1 = 0;
    advanceBits(1);

//  additional_copy_info_flag:1 = 0;
    additional_copy_flag = nextbits(1);
    advanceBits(1);

//  PES_CRC_flag:1 = 0;
    PES_CRC_flag = nextbits(1);
    advanceBits(1);

//  PES_extension_flag:1 = 0;
    PES_extension_flag = nextbits(1);
    advanceBits(1);

//  PES_header_data_length:8 = 0;
    advanceBits(8);

    if (PTS_DTS_flags == 2)
    {
//      marker_bit:4 = 0;
//      pts_32_30:3 = 0;
//      marker_bit:1 = 0;
//      pts_29_15:15 = 0;
//      marker_bit:1 = 0;
//      pts_14_0:15 = 0;
//      marker_bit:1 = 0;
        advanceBits(40);
    }
    else if (PTS_DTS_flags == 3)
    {
//      marker_bit:4 = 0;
//      pts_32_30:3 = 0;
//      marker_bit:1 = 0;
//      pts_29_15:15 = 0;
//      marker_bit:1 = 0;
//      pts_14_0:15 = 0;
//      marker_bit:1 = 0;
//      marker_bit:4 = 0;
//      dts_32_30:3 = 0;
//      marker_bit:1 = 0;
//      dts_29_15:15 = 0;
//      marker_bit:1 = 0;
//      dts_14_0:15 = 0;
//      marker_bit:1 = 0;
        advanceBits(80);
    }

    if (ESCR_flag)
    {
//      marker_bit:2 = 0;
//      escr_32_30:3 = 0;
//      marker_bit:1 = 0;
//      escr_29_15:15 = 0;
//      marker_bit:1 = 0;
//      escr_14_0:15 = 0;
//      marker_bit:1 = 0;
//      escr_ext:9 = 0;
//      marker_bit:1 = 0;
        advanceBits(48);
    }

    if (ES_rate_flag)
    {
//      marker_bit:1 = 0;
//      ES_rate:22 = 0;
//      marker_bit:1 = 0;
        advanceBits(24);
    }

    if (additional_copy_flag)
    {
//      marker_bit:1 = 0;
//      additional_copy_info:7 = 0;
        advanceBits(8);
    }

    if (PES_CRC_flag)
    {
//      previous_pes_crc:16 = 0;
        advanceBits(16);
    }

    if (PES_extension_flag)
    {
//      pes_private_data_flag:1 = 0;
        pes_private_data_flag = nextbits(1);
        advanceBits(1);

//      pack_header_field_flag:3 = 0;
        pack_header_field_flag = nextbits(1);
        advanceBits(1);

//      program_packet_sequence_counter_flag:3 = 0;
        program_packet_sequence_counter_flag = nextbits(1);
        advanceBits(1);

//      p_std_buffer_flag:3 = 0;
        p_std_buffer_flag = nextbits(1);
        advanceBits(1);

//      marker_bit:3 = 0;
        advanceBits(3);

//      pes_extension_flag2:3 = 0;
        pes_extension_flag2 = nextbits(1);
        advanceBits(1);
    }

    if (pes_private_data_flag)
    {
//      pes_private_data:8[16] = 0;
        advanceBits(8 * 16);
    }

    if (pack_header_field_flag)
    {
//      pack_field_length:8 = 0;
        advanceBits(8);
    }

    if (program_packet_sequence_counter_flag)
    {
//      marker_bit:1 = 0;
//      packet_sequence_counter:7 = 0;
//      marker_bit:1 = 0;
//      mpeg1_mpeg2_identifier:1 = 0;
//      original_stuffing_length:6 = 0;
        advanceBits(16);
    }

    if (p_std_buffer_flag)
    {
//      marker_bits:2 = 0;
//      p_std_buffer_scale:1 = 0;
//      p_std_buffer_size:13 = 0;
        advanceBits(16);
    }

    if (pes_extension_flag2)
    {
//      marker_bits:1 = 0;
        advanceBits(1);

        pes_extension_field_length = nextbits(7);
        advanceBits(7);

//      marker_bits:8 = 0;
        advanceBits(8);
    }

    if (pes_extension_field_length)
    {
//      pes_extension_field:8[n] = 0;
        advanceBits(8 * pes_extension_field_length);
    }

    while (0xFF == nextbits(8))
        advanceBits(8);

    len = hdr_len;
}

void TiVoDecoder_MPEG2_Parser::picture_data(uint16_t &len)
{
    do
    {
        slice(len);
    }
    while (nextbits(32) == slice_start_code);

    next_start_code();
    len = hdr_len;
}

void TiVoDecoder_MPEG2_Parser::slice(uint16_t &len)
{
//  slice_start_code:32;
    advanceBits(32);

    if (vertical_size > 2800)
    {
//      slice_vertical_position_extension:3;
        advanceBits(3);
    }

    if (scalable_mode == data_partitioning)
    {
//      priority_breakpoint:7;
        advanceBits(7);
    }

//  quantiser_scale_code:5;
    advanceBits(5);

    if (nextbits(1) == 1)
    {
//      intra_slice_flag:1;
//      intra_slice:1;
//      reserved:7;
        advanceBits(9);

        while (nextbits(1) == 1)
        {
//          extra_bit_slice:1 = 1;
//          extra_information_slice++:8;
            advanceBits(9);
        }
    }

//  extra_bit_slice:1 = 0;
    advanceBits(1);

    do
    {
        macroblock(len);
    }
    while (nextbits(24) != 0x000001);

    next_start_code();
    len = hdr_len;
}

void TiVoDecoder_MPEG2_Parser::macroblock(uint16_t &len)
{
    len = hdr_len;
}
