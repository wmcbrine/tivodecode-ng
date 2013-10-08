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

#include "tivo_decoder_mpeg_parser.hxx"


//
//UINT8 buffer[] = { 0x12, 0x23, 0x34, 0x45, 0x56, 0x00, 0x00, 0x01, 0xFF };
//UINT16 bufLen  = 9;
//
//main()
//{
//    TiVoDecoder_MPEG2_Parser parser(buffer, bufLen);
//
//    UINT32 nextbits = 0;
//
//    nextbits = parser.nextbits(8);
//    fprintf(stderr, "nextbits = 0x%08x\n", nextbits);
//
//    parser.advanceBits(3);
//
//    nextbits = parser.nextbits(8);
//    fprintf(stderr, "nextbits = 0x%08x\n", nextbits);
//
//    nextbits = parser.nextbits(32);
//    fprintf(stderr, "nextbits = 0x%08x\n", nextbits);
//
//    nextbits = parser.nextbits(36);
//    fprintf(stderr, "nextbits = 0x%08x\n", nextbits);
//
//    parser.next_start_code();
//
//    nextbits = parser.nextbits(32);
//    fprintf(stderr, "nextbits = 0x%08x\n", nextbits);
//}


TiVoDecoder_MPEG2_Parser::TiVoDecoder_MPEG2_Parser()
{
    _bit_ptr = 0;
    _end_of_file = FALSE;
    _buffer_length = 0;
    _pBuffer = NULL;

  /* start codes */ picture_start_code      = 0x100; slice_start_code        =
    0x101; user_data_start_code    = 0x1b2; sequence_header_code    = 0x1b3;
    sequence_error_code     = 0x1b4; extension_start_code    = 0x1b5;
    sequence_end_code       = 0x1b7; group_start_code        = 0x1b8;

    /* extension start codes */
    sequence_extension_id                   = 0x1;
    sequence_display_extension_id           = 0x2;
    quant_matrix_extension_id               = 0x3;
    copyright_extension_id                  = 0x4;
    sequence_scalable_extension_id          = 0x5;
    picture_display_extension_id            = 0x7;
    picture_coding_extension_id             = 0x8;
    picture_spatial_scalable_extension_id   = 0x9;
    picture_temporal_scalable_extension_id  = 0xa;

    /* scalable modes */
    data_partitioning       = 0x0;
    spatial_scalability     = 0x1;
    snr_scalability         = 0x2;
    temporal_scalability    = 0x3;

    progressive_sequence    = 0x00;
    repeat_first_field      = 0x00;
    top_field_first         = 0x00;
    vertical_size           = 0x00;
    scalable_mode           = 0x00;
    picture_structure       = 0x00;
}

TiVoDecoder_MPEG2_Parser::TiVoDecoder_MPEG2_Parser(UINT8 * pBuffer, UINT16 bufLen)
{
    _pBuffer = pBuffer;
    _buffer_length = bufLen;
    _bit_ptr = 0;
    _end_of_file = FALSE;

    /* start codes */
    picture_start_code      = 0x100;
    slice_start_code        = 0x101;
    user_data_start_code    = 0x1b2;
    sequence_header_code    = 0x1b3;
    sequence_error_code     = 0x1b4;
    extension_start_code    = 0x1b5;
    sequence_end_code       = 0x1b7;
    group_start_code        = 0x1b8;

    /* extension start codes */
    sequence_extension_id                   = 0x1;
    sequence_display_extension_id           = 0x2;
    quant_matrix_extension_id               = 0x3;
    copyright_extension_id                  = 0x4;
    sequence_scalable_extension_id          = 0x5;
    picture_display_extension_id            = 0x7;
    picture_coding_extension_id             = 0x8;
    picture_spatial_scalable_extension_id   = 0x9;
    picture_temporal_scalable_extension_id  = 0xa;

    /* scalable modes */
    data_partitioning       = 0x0;
    spatial_scalability     = 0x1;
    snr_scalability         = 0x2;
    temporal_scalability    = 0x3;

    progressive_sequence    = 0x00;
    repeat_first_field      = 0x00;
    top_field_first         = 0x00;
    vertical_size           = 0x00;
    scalable_mode           = 0x00;
    picture_structure       = 0x00;
}

void TiVoDecoder_MPEG2_Parser::setBuffer(UINT8 * pBuffer, UINT16 bufLen )
{
    _pBuffer = pBuffer;
    _buffer_length = bufLen;
    _bit_ptr = 0;
    _end_of_file = FALSE;
}

BOOL TiVoDecoder_MPEG2_Parser::byteAligned()
{
    UINT32 align_mode = _bit_ptr % 8;
    return(align_mode == 0);
}

void TiVoDecoder_MPEG2_Parser::advanceBits(UINT32 n)
{
    _bit_ptr += n;

    if(_bit_ptr >= (_buffer_length*8))
        _end_of_file = TRUE;
}

INT32 TiVoDecoder_MPEG2_Parser::nextbits(UINT32 n)
{
    UINT32 bit_ptr_copy = _bit_ptr;
    UINT8  alignDelta   = bit_ptr_copy % 8;
    UINT8  byte         = 0;
    INT32  n_copy       = n;
    INT32  nread        = 0;
    INT32  value        = 0;

    if( alignDelta )
    {
        int i = 0;
        int j = 0;

        bit_ptr_copy -= alignDelta;
        nread         = readByte(bit_ptr_copy, byte);

        for(i=0; i<alignDelta; i++)
        {
            byte &= ~(1<<(7-i));
            bit_ptr_copy++;
        }

        for(j=alignDelta; (j<8) && n_copy; j++, n_copy--)
            bit_ptr_copy++;

        if(0 == n_copy)
            for( ; j<8; j++)
                byte = byte >> 1;

        value = byte;
    }

    while ( n_copy > 0 )
    {
        nread = readByte(bit_ptr_copy, byte);
        if(nread == 0)
        {
            _end_of_file = TRUE;
            fprintf(stderr,"EOF\n");
            return -1;
        }

        bit_ptr_copy += 8;
        n_copy       -= 8;
        value = (value << 8) | byte;
    }

    if (n_copy < 0)
    {
//        value = value >> (8+n_copy);
        value = value >> (0-n_copy);
    }

    return value;
}

INT32 TiVoDecoder_MPEG2_Parser::readByte(UINT32 bit_pos, UINT8 & byte)
{
    byte = 0;

    if(bit_pos > _buffer_length*8)
    {
        _end_of_file = TRUE;
        return(-1);
    }

    byte = _pBuffer[bit_pos/8];
    return(1);
}

// =========================================================================

void TiVoDecoder_MPEG2_Parser::next_start_code()
{
    BOOL aligned = byteAligned();
    while(FALSE == aligned)
    {
        advanceBits(1);
        aligned = byteAligned();
    }

//    while((FALSE==_end_of_file) && (0x00==nextbits(8)) && (0x000001 != nextbits(24)))
//    {
//        advanceBits(8);
//    }

    while(FALSE==_end_of_file)
    {
        if(0x000001 == nextbits(24))
            break;
        else if(0x000000 == nextbits(24))
            advanceBits(8);
        else
            break;
    }

    return;
}

// =========================================================================

void TiVoDecoder_MPEG2_Parser::video_sequence()
{
    sequence_header();
    if (nextbits(32) == extension_start_code)
    {
        sequence_extension();
        do
        {
            extension_and_user_data(0);
            do
            {
                if (nextbits(32) == group_start_code)
                {
                    group_of_pictures_header();
                    extension_and_user_data(1);
                }
                picture_header();
                extension_and_user_data(2);
                picture_data();
            } while ((nextbits(32) == picture_start_code) || (nextbits(32) == group_start_code));

            if (nextbits(32) != sequence_end_code)
            {
                sequence_header();
                sequence_extension();
            }
        } while (nextbits(32) != sequence_end_code);
    }
    else
    {
        /* ISO/IEC 11172-2 */
    }

//  sequence_end_code:32;
    advanceBits(32);

    return;
}

void TiVoDecoder_MPEG2_Parser::sequence_end()
{
//  sequence_header_code:32;
    advanceBits(32);
}

void TiVoDecoder_MPEG2_Parser::sequence_header()
{
//  sequence_header_code:32;
    advanceBits(32);

//  horizontal_size:12;
    advanceBits(12);

//  vertical_size:12;
    vertical_size = nextbits(12);
    advanceBits(12);

//  aspect_ratio_information:4;
    advanceBits(4);

//  frame_rate_code:4;
    advanceBits(4);

//  bit_rate_value:18;
    advanceBits(18);

//  marker_bit:1 = 1;
    advanceBits(1);

//  vbv_buffer_size_value:10;
    advanceBits(10);

//  constrained_parameters_flag:1;
    advanceBits(1);

//  load_intra_quantiser_matrix:1;
    UINT32 load_intra_quantiser_matrix = nextbits(1);
    advanceBits(1);

    if (load_intra_quantiser_matrix == 1)
    {
//      intra_quantiser_matrix:8[64];
        advanceBits(8*64);
    }

//  load_non_intra_quantiser_matrix:1;
    UINT32 load_non_intra_quantiser_matrix = nextbits(1);
    advanceBits(1);

    if (load_non_intra_quantiser_matrix == 1)
    {
//      non_intra_quantiser_matrix:8[64];
        advanceBits(8*64);
    }

    next_start_code();
}

void TiVoDecoder_MPEG2_Parser::extension_and_user_data(INT32 i)
{
    while ((nextbits(32) == extension_start_code) || (nextbits(32) == user_data_start_code))
    {
        if ((i != 1) && (nextbits(32) == extension_start_code))
        {
            extension_data(i);
        }
    }

    if (nextbits(32) == user_data_start_code)
    {
        user_data();
    }
}

void TiVoDecoder_MPEG2_Parser::extension_data(INT32 i)
{
    while (nextbits(32) == extension_start_code)
    {
//      extension_start_code:32;
        UINT32 extension_start_code = nextbits(32);
        advanceBits(32);

        if (i==0)
        {
            if (nextbits(4) == sequence_display_extension_id)
            {
                sequence_display_extension();
            }
            else
            {
                sequence_scalable_extension();
            }
        }

        if (i==2)
        {
            if (nextbits(4) == quant_matrix_extension_id)
            {
                quant_matrix_extension();
            }
            else if (nextbits(4) == copyright_extension_id)
            {
                copyright_extension();
            }
            else if (nextbits(4) == picture_display_extension_id)
            {
                picture_display_extension();
            }
            else if (nextbits(4) == picture_spatial_scalable_extension_id)
            {
                picture_spatial_scalable_extension();
            }
            else
            {
                picture_temporal_scalable_extension();
            }
        }
    }
}

void TiVoDecoder_MPEG2_Parser::user_data()
{
//  user_data_start_code:32;
    UINT32 user_data_start_code = nextbits(32);
    advanceBits(32);

    while (nextbits(24) != 0x000001)
    {
//      user_data++:8;
        UINT8 user_data = nextbits(8);
        advanceBits(8);
    }

    next_start_code();
}


void TiVoDecoder_MPEG2_Parser::extension_header()
{
//  extension_start_code:32;
    advanceBits(32);

    UINT8 type = nextbits(4);

    if( 1 == type )
        sequence_extension();
    else if (2 == type )
        sequence_display_extension();
    else if (8 == type )
        picture_coding_extension();
}

void TiVoDecoder_MPEG2_Parser::sequence_extension()
{
//  extension_start_code:32;
//  advanceBits(32);

//  sequence_extension_id:4;
    advanceBits(4);
//  profile_and_level_indication:8;
    advanceBits(8);
//  progressive_sequence:1;
    progressive_sequence = nextbits(1);
    advanceBits(1);
//  chroma_format:2;
    advanceBits(2);
//  horizontal_size_extension:2;
    advanceBits(2);
//  vertical_size_extension:2;
    advanceBits(2);
//  bit_rate_extension:12;
    advanceBits(12);
//  marker_bit:1 = 1;
    advanceBits(1);
//  vbv_buffer_size_extension:8;
    advanceBits(8);
//  low_delay:1;
    advanceBits(1);
//  frame_rate_extension_n:2;
    advanceBits(2);
//  frame_rate_extension_d:5;
    advanceBits(5);

    next_start_code();
}

void TiVoDecoder_MPEG2_Parser::sequence_display_extension()
{
//  extension_start_code:32;
//  advanceBits(32);

//  sequence_display_extension_id:4;
    advanceBits(4);
//  video_format:3;
    advanceBits(3);
//  colour_description:1;
    UINT8 colour_description = nextbits(1);
    advanceBits(1);

    if (colour_description == 1)
    {
//      colour_primaries:8;
        advanceBits(8);
//      transfer_characteristics:8;
        advanceBits(8);
//      matrix_coefficients:8;
        advanceBits(8);
    }

//  display_horizontal_size:14;
    advanceBits(14);
//  marker_bit:1 = 1;
    advanceBits(1);
//  display_vertical_size:14;
    advanceBits(14);
//  marker_bit:3 = 1;
    advanceBits(3);
    
    next_start_code();
}

void TiVoDecoder_MPEG2_Parser::picture_coding_extension()
{
//  extension_start_code:32;
//  advanceBits(32);

//  picture_coding_extension_id:4;
    advanceBits(4);
//  f_code:4[2][2];
    advanceBits(4*2*2);
//  intra_dc_precision:2;
    advanceBits(2);
//  picture_structure:2;
    picture_structure = nextbits(2);
    advanceBits(2);
//  top_field_first:1;
    top_field_first = nextbits(1);
    advanceBits(1);
//  frame_pred_frame_dct:1;
    advanceBits(1);
//  concealment_motion_vectors:1;
    advanceBits(1);
//  q_scale_type:1;
    advanceBits(1);
//  intra_vlc_format:1;
    advanceBits(1);
//  alternate_scan:1;
    advanceBits(1);
//  repeat_first_field:1;
    repeat_first_field = nextbits(1);
    advanceBits(1);
//  chroma_420_type:1;
    advanceBits(1);
//  progressive_frame:1;
    advanceBits(1);
//  composite_display_flag:1;
    UINT8 composite_display_flag = nextbits(1);
    advanceBits(1);

    if (composite_display_flag == 1)
    {
//      v_axis:1;
        advanceBits(1);
//      field_sequence:3;
        advanceBits(3);
//      sub_carrier:1;
        advanceBits(1);
//      burst_amplitude:7;
        advanceBits(7);
//      sub_carrier_phase:8;
        advanceBits(8);
    }

    next_start_code();
}

void TiVoDecoder_MPEG2_Parser::sequence_scalable_extension()
{
//  sequence_scalable_extension_id:4;
    advanceBits(4);
//  scalable_mode:2;
    scalable_mode = nextbits(2);
    advanceBits(2);
//  layer_id:4;
    advanceBits(4);

    if (scalable_mode == spatial_scalability)
    {
//      lower_layer_prediction_horizontal_size:14;
        advanceBits(14);
//      marker_bit:1;
        advanceBits(1);
//      lower_layer_prediction_vertical_size:14;
        advanceBits(14);
//      horizontal_subsampling_factor_m:5;
        advanceBits(5);
//      horizontal_subsampling_factor_n:5;
        advanceBits(5);
//      vertical_subsampling_factor_m:5;
        advanceBits(5);
//      vertical_subsampling_factor_n:5;
        advanceBits(5);
    }

    if (scalable_mode == temporal_scalability)
    {
//      picture_mux_enable:1;
        UINT8 picture_mux_enable = nextbits(1);
        advanceBits(1);

        if (picture_mux_enable == 1)
        {
//          mux_to_progressive_sequence:1;
            advanceBits(1);
        }

//      picture_mux_order:3;
        advanceBits(3);
//      picture_mux_factor:3;
        advanceBits(3);
    }

    next_start_code();
}

void TiVoDecoder_MPEG2_Parser::group_of_pictures_header()
{
//  group_start_code:32;
    advanceBits(32);
//  time_code:25;
    advanceBits(25);
//  closed_gop:1;
    advanceBits(1);
//  broken_link:1;
    advanceBits(1);

    next_start_code();
}

void TiVoDecoder_MPEG2_Parser::picture_header()
{
//  picture_start_code:32;
    advanceBits(32);
//  temporal_reference:10;
    advanceBits(10);
//  picture_coding_type:3;
    UINT8 picture_coding_type = nextbits(3);
    advanceBits(3);
//  vbv_delay:16;
    advanceBits(16);

    if ((picture_coding_type == 2) || (picture_coding_type == 3))
    {
//      full_pel_forward_vector:1;
        advanceBits(1);
//      forward_f_code:3;
        advanceBits(3);
    }

    if (picture_coding_type == 3)
    {
//      full_pel_backward_vector:1;
        advanceBits(1);
//      backward_f_code:3;
        advanceBits(3);
    }

    while (nextbits(1) == 1)
    {
//      extra_bit_picture:1 = 1;
        advanceBits(1);
//      extra_information_picture++:8;
        advanceBits(8);
    }

//  extra_bit_picture:1 = 0;
    advanceBits(1);

//    next_start_code(); // ???
}

void TiVoDecoder_MPEG2_Parser::pes_header()
{
//  picture_start_code:24;
    advanceBits(24);

    BOOL extensionPresent = FALSE;
    UINT8 streamId        = nextbits(8);

    advanceBits(8);

    if( streamId == 0xBD )
        extensionPresent = TRUE;
    else if( streamId == 0xBE )
        extensionPresent = FALSE;
    else if( streamId == 0xBF )
        extensionPresent = FALSE;
    else if( ( streamId >= 0xC0 ) && ( streamId <= 0xDF ) )
        extensionPresent = TRUE;
    else if( ( streamId >= 0xE0 ) && ( streamId <= 0xEF ) )
        extensionPresent = TRUE;

    UINT16 pesHdrLen = nextbits(16);
    advanceBits(16);

    if(TRUE==extensionPresent)
    {
        pes_header_extension();
    }
    
    next_start_code();
}

void TiVoDecoder_MPEG2_Parser::pes_header_extension()
{
    UINT8 pes_private_data_flag  = 0;
    UINT8 pack_header_field_flag = 0;
    UINT8 program_packet_sequence_counter_flag = 0;
    UINT8 p_std_buffer_flag = 0;
    UINT8 pes_extension_field_length = 0;
    UINT8 PTS_DTS_flags = 0;
    UINT8 ESCR_flag = 0;
    UINT8 ES_rate_flag = 0;
    UINT8 additional_copy_flag = 0;
    UINT8 PES_CRC_flag = 0;
    UINT8 pes_extension_flag2 = 0;
    UINT8 PES_extension_flag = 0;
    
//  marker_bit:2 = 0;
    advanceBits(2);
//  pes_scrambling_control:2 = 0;
    advanceBits(2);
//  pes_priority:1 = 0;
    advanceBits(1);
//  data_alignment_indicator:1 = 0;
    advanceBits(1);
//  copyright:1 = 0;
    advanceBits(1);
//  original_or_copy:1 = 0;
    advanceBits(1);
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

    if(PTS_DTS_flags == 2)
    {
//      marker_bit:4 = 0;
        advanceBits(4);
//      pts_32_30:3 = 0;
        advanceBits(3);
//      marker_bit:1 = 0;
        advanceBits(1);
//      pts_29_15:15 = 0;
        advanceBits(15);
//      marker_bit:1 = 0;
        advanceBits(1);
//      pts_14_0:15 = 0;
        advanceBits(15);
//      marker_bit:1 = 0;
        advanceBits(1);
    }
    else if(PTS_DTS_flags == 3)
    {
//      marker_bit:4 = 0;
        advanceBits(4);
//      pts_32_30:3 = 0;
        advanceBits(3);
//      marker_bit:1 = 0;
        advanceBits(1);
//      pts_29_15:15 = 0;
        advanceBits(15);
//      marker_bit:1 = 0;
        advanceBits(1);
//      pts_14_0:15 = 0;
        advanceBits(15);
//      marker_bit:1 = 0;
        advanceBits(1);
//      marker_bit:4 = 0;
        advanceBits(4);
//      dts_32_30:3 = 0;
        advanceBits(3);
//      marker_bit:1 = 0;
        advanceBits(1);
//      dts_29_15:15 = 0;
        advanceBits(15);
//      marker_bit:1 = 0;
        advanceBits(1);
//      dts_14_0:15 = 0;
        advanceBits(15);
//      marker_bit:1 = 0;
        advanceBits(1);
    }

    if(ESCR_flag)
    {
//      marker_bit:2 = 0;
        advanceBits(2);
//      escr_32_30:3 = 0;
        advanceBits(3);
//      marker_bit:1 = 0;
        advanceBits(1);
//      escr_29_15:15 = 0;
        advanceBits(15);
//      marker_bit:1 = 0;
        advanceBits(1);
//      escr_14_0:15 = 0;
        advanceBits(15);
//      marker_bit:1 = 0;
        advanceBits(1);
//      escr_ext:9 = 0;
        advanceBits(9);
//      marker_bit:1 = 0;
        advanceBits(1);
    }

    if(ES_rate_flag)
    {
//      marker_bit:1 = 0;
        advanceBits(1);
//      ES_rate:22 = 0;
        advanceBits(22);
//      marker_bit:1 = 0;
        advanceBits(1);
    }
    
    if(additional_copy_flag)
    {
//      marker_bit:1 = 0;
        advanceBits(1);
//      additional_copy_info:7 = 0;
        advanceBits(7);
    } 
    
    if(PES_CRC_flag)
    {
//      previous_pes_crc:16 = 0;
        advanceBits(16);
    }
    
    if(PES_extension_flag)
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
    
    if(pes_private_data_flag)
    {
//      pes_private_data:8[16] = 0;
        advanceBits(8*16);        
    }

    if(pack_header_field_flag)
    {
//      pack_field_length:8 = 0;
        advanceBits(8);        
    }

    if(program_packet_sequence_counter_flag)
    {
//      marker_bit:1 = 0;
        advanceBits(1);   
//      packet_sequence_counter:7 = 0;
        advanceBits(7);
//      marker_bit:1 = 0;
        advanceBits(1);
//      mpeg1_mpeg2_identifier:1 = 0;
        advanceBits(1);
//      original_stuffing_length:6 = 0;
        advanceBits(6);
    }
    
    if(p_std_buffer_flag)
    {
//      marker_bits:2 = 0;
        advanceBits(2);     
//      p_std_buffer_scale:1 = 0;
        advanceBits(1);
//      p_std_buffer_size:13 = 0;
        advanceBits(13);    
    }
    
    if(pes_extension_flag2)
    {
//      marker_bits:1 = 0;
        advanceBits(1);  
        pes_extension_field_length = nextbits(7);
        advanceBits(7);
//      marker_bits:8 = 0;
        advanceBits(8);
    }
    
    if(pes_extension_field_length)
    {
//      pes_extension_field:8[n] = 0;
        advanceBits(8*pes_extension_field_length);    
    }
    
    while(0xFF == nextbits(8))
    {
        advanceBits(8);    
    }
}

void TiVoDecoder_MPEG2_Parser::quant_matrix_extension()
{
//  quant_matrix_extension_id:4;
    advanceBits(4);
//  load_intra_quantiser_matrix:1;
    UINT8 load_intra_quantiser_matrix = nextbits(1);
    advanceBits(1);

    if (load_intra_quantiser_matrix == 1)
    {
//      intra_quantiser_matrix:8[64];
        advanceBits(8*64);
    }

//  load_non_intra_quantiser_matrix:1;
    UINT8 load_non_intra_quantiser_matrix = nextbits(1);
    advanceBits(1);

    if (load_non_intra_quantiser_matrix == 1)
    {
//      non_intra_quantiser_matrix:8[64];
        advanceBits(8*64);
    }

//  load_chroma_intra_quantiser_matrix:1;
    UINT8 load_chroma_intra_quantiser_matrix = nextbits(1);
    advanceBits(1);

    if (load_chroma_intra_quantiser_matrix == 1)
    {
//      chroma_intra_quantiser_matrix:8[64];
        advanceBits(8*64);
    }

//    load_chroma_non_intra_quantiser_matrix:1;
    UINT8 load_chroma_non_intra_quantiser_matrix = nextbits(1);
    advanceBits(1);

    if (load_chroma_non_intra_quantiser_matrix == 1)
    {
//      chroma_non_intra_quantiser_matrix:8[64];
        advanceBits(8*64);
    }

  next_start_code();
}

void TiVoDecoder_MPEG2_Parser::picture_display_extension()
{
    UINT8 number_of_frame_centre_offsets = 0;

//  picture_display_extension_id:4;
    advanceBits(4);

    if (progressive_sequence == 1)
    {
        if (repeat_first_field == 1)
        {
            if (top_field_first == 1)
            {
                number_of_frame_centre_offsets = 3;
            }
            else
            {
                number_of_frame_centre_offsets = 2;
            }
        }
        else
        {
            number_of_frame_centre_offsets = 1;
        }
    }
    else
    {
        UINT8 field_structure = 0; // ????
        if (picture_structure == field_structure)
        {
            number_of_frame_centre_offsets = 1;
        }
        else
        {
            if (repeat_first_field == 1)
            {
                number_of_frame_centre_offsets = 3;
            }
            else
            {
                number_of_frame_centre_offsets = 2;
            }
        }
    }

    for (UINT8 i=0;i<number_of_frame_centre_offsets;i++)
    {
//      frame_centre_horizontal_offset[i]:16;
        advanceBits(16);
//      marker_bit:1 = 1;
        advanceBits(1);
//      frame_centre_vertical_offset[i]:16;
        advanceBits(16);
//      marker_bit:1 = 1;
        advanceBits(1);
    }

    next_start_code();
}

void TiVoDecoder_MPEG2_Parser::picture_spatial_scalable_extension()
{
//  picture_spatial_scalable_extension_id:4;
    advanceBits(4);
//  lower_layer_temporal_reference:10;
    advanceBits(10);
//  marker_bit:1 = 1;
    advanceBits(1);
//  lower_layer_horizontal_offset:15;
    advanceBits(15);
//  marker_bit:1 = 1;
    advanceBits(1);
//  lower_layer_vertical_offset:15;
    advanceBits(15);
//  spatial_temporal_weight_code_table_index:2;
    advanceBits(2);
//  lower_layer_progressive_frame:1;
    advanceBits(1);
//  lower_layer_deinterlaced_field_select:1;
    advanceBits(1);

    next_start_code();
}

void TiVoDecoder_MPEG2_Parser::picture_temporal_scalable_extension()
{
//  picture_temporal_scalable_extension_id:4;
    advanceBits(4);
//  reference_select_code:2;
    advanceBits(2);
//  forward_temporal_reference:10;
    advanceBits(10);
//  marker_bit:1 = 1;
    advanceBits(1);
//  backward_temporal_reference:10;
    advanceBits(10);

    next_start_code();
}

void TiVoDecoder_MPEG2_Parser::copyright_extension()
{
//  copyright_extension_id:4;
    advanceBits(4);
//  copyright_flag:1;
    advanceBits(1);
//  copyright_identifier:8;
    advanceBits(8);
//  original_or_copy:1;
    advanceBits(1);
//  reserved:7;
    advanceBits(7);
//  copyright_number_1:20;
    advanceBits(20);
//  marker_bit:1 = 1;
    advanceBits(1);
//  copyright_number_2:22;
    advanceBits(22);
//  marker_bit:1 = 1;
    advanceBits(1);
//  copyright_number_3:22;
    advanceBits(22);

    next_start_code();
}

void TiVoDecoder_MPEG2_Parser::picture_data()
{
    do
    {
        slice();
    }
    while (nextbits(32) == slice_start_code);

    next_start_code();
}

void TiVoDecoder_MPEG2_Parser::slice()
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
        advanceBits(1);
//      intra_slice:1;
        advanceBits(1);
//      reserved:7;
        advanceBits(7);

        while (nextbits(1) == 1)
        {
//          extra_bit_slice:1 = 1;
            advanceBits(1);
//          extra_information_slice++:8;
            advanceBits(8);
        }
    }

//  extra_bit_slice:1 = 0;
    advanceBits(1);

    do
    {
        macroblock();
    }
    while (nextbits(24) != 0x000001);

    next_start_code();
}

void TiVoDecoder_MPEG2_Parser::macroblock()
{
    return;
}

/*
void TiVoDecoder_MPEG2_Parser::macroblock()
{
    while (nextbits(11) == '0000 0001 000')
    {
        macroblock_escape:11;
    }

    macroblock_address_increment:mba_inc;
    macroblock_modes();

    if (macroblock_quant)
    {
        quantiser_scale_code:5;
    }

    if ((macroblock_type && macroblock_motion_forward) ||
    ((macroblock_type && macroblock_intra) && (concealment_motion_vectors == 1)))
    {
        motion_vectors(0);
    }

    if (macroblock_type && macroblock_motion_backward)
    {
        motion_vectors(1);
    }

    if ((macroblock_type && macroblock_intra) && (concealment_motion_vectors == 1))
    {
        marker_bit:1 = 1;
    }

    if (macroblock_pattern == 1)
    {
        coded_block_pattern();
    }

    for (i=0;i<block_count;i++)
    {
        block(i);
    }
}

macroblock_modes() {
  macroblock_type:mb_type;
  if ((spatial_temporal_weight_code_flag == 1) && (spatial_temporal_weight_codee_table_index != '00')) {
    spatial_temporal_weight_code:2;
  }
  if ((macroblock_type && macroblock_motion_forward) || (macroblock_type && macroblock_motion_backward) {
    if (picture_structure == frame_picture) {
      if (frame_pred_frame_dct == 0) {
        frame_motion_type:2
      }
    } else {
      field_motion_type:2;
    }
  }
  if ((picture_structure == frame_picture) && (frame_pred_frame_dct == 0) &&
      ((macroblock_type && macroblock_intra) || (macroblock_type || macroblock_pattern))) {
    dct_type:1;
  }
}

motion_vectors(s) {
  if (motion_vector_count == 1) {
    if ((mv_format == field) && (dmv != 1)) {
      motion_vertical_field_select[0][s]:1;
    }
    motion_vector(0,s);
  } else {
    motion_vertical_field_select[0][s];
    motion_vector(0,s);
    motion_vertical_field_select[1][s];
    motion_vector(1,s);
  }
}

motion_vector(r,s) {
  motion_code[r][s][0]:motion_code_vlc;
  if ((f_code[s][0] != 1) && (motion_code[r][s][0] != 0)) {
    motion_residual[r][s][0]:motion_residual_vlc;
  }
  if (dmv == 1) {
    dmvector[0]:dmvector_vlc;
  }
  if ((f_code[s][1] != 1) && (motion_code[r][s][1] != 0)) {
    motion_residual[r][s][1]:motion_residual_vlc;
  }
  if (dmv == 1) {
    dmvector[1]:dmvector_vlc;
  }
}

coded_block_pattern() {
  coded_block_pattern_420:coded_block_pattern_420_vlc;
  if (chroma_format == chroma_422) {
    coded_block_pattern_1:2;
  }
  if (chroma_format == chroma_444) {
    coded_block_pattern_2:6;
  }
}
*/
