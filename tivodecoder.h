/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */

#ifndef TIVO_DECODER_H_
#define TIVO_DECODER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "tdconfig.h"
#include <stddef.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "turing_stream.h"


#if SIZEOF_OFF_T == 8
# define OFF_T_TYPE off_t
# define OFF_T_FORMAT  "llu"
#elif defined(WIN32)
# define OFF_T_TYPE __int64
# define OFF_T_FORMAT  "llu"
#else
# warning "Not compiling for large file (>2G) support!"
# define OFF_T_TYPE off_t
# define OFF_T_FORMAT  "lu"
#endif

typedef int (*write_func_t) (void * mem, int size, void * fh);

#define PICTURE_START_CODE      0x00
#define SLICE_START_CODE_MIN    0x01
#define SLICE_START_CODE_MAX    0xAF
#define USER_DATA_START_CODE    0xB2
#define SEQUENCE_HEADER_CODE    0xB3
#define SEQUENCE_ERROR_CODE     0xB4
#define EXTENSION_START_CODE    0xB5
#define SEQUENCE_END_CODE       0xB7
#define GROUP_START_CODE        0xB8
#define SYSTEM_START_CODE_MIN   0xB9
#define SYSTEM_START_CODE_MAX   0xFF
#define ISO_END_CODE            0xB9
#define PACK_START_CODE         0xBA
#define SYSTEM_START_CODE       0xBB
#define VIDEO_ELEMENTARY_STREAM 0xe0

#define TRANSPORT_STREAM        0x47

#define TS_FRAME_SIZE 			188

#define TS_PES_HDR_BUFSIZE		1024






//============================
// TS Specific data structures
//============================

typedef enum
{
	TS_PID_TYPE_RESERVED = 1,
	TS_PID_TYPE_NULL_PACKET,
	TS_PID_TYPE_PROGRAM_ASSOCIATION_TABLE,
	TS_PID_TYPE_PROGRAM_MAP_TABLE,
	TS_PID_TYPE_CONDITIONAL_ACCESS_TABLE,
	TS_PID_TYPE_NETWORK_INFORMATION_TABLE,
	TS_PID_TYPE_SERVICE_DESCRIPTION_TABLE,
	TS_PID_TYPE_EVENT_INFORMATION_TABLE,
	TS_PID_TYPE_RUNNING_STATUS_TABLE,
	TS_PID_TYPE_TIME_DATE_TABLE,
	TS_PID_TYPE_AUDIO_VIDEO_PRIVATE_DATA,
	TS_PID_TYPE_NONE
} ts_packet_pid_types;


typedef enum
{
	TS_STREAM_TYPE_AUDIO = 1,
	TS_STREAM_TYPE_VIDEO,
	TS_STREAM_TYPE_PRIVATE_DATA,
	TS_STREAM_TYPE_OTHER,
	TS_STREAM_TYPE_NONE
} ts_stream_types;


typedef struct
{
    // the 16-bit value match for the packet pids
    unsigned char code_match_lo;      // low end of the range of matches
    unsigned char code_match_hi;      // high end of the range of matches

    // what kind of TS packet is it?
    ts_stream_types ts_stream_type;
}
ts_pmt_stream_type_info;


#define TS_STREAM_ELEMENT_MAX 10

typedef struct
{
    // the 16-bit value match for the packet pids
    unsigned short code_match_lo;      // low end of the range of matches
    unsigned short code_match_hi;      // high end of the range of matches

    // what kind of TS packet is it?
    ts_packet_pid_types ts_packet;
}
ts_packet_tag_info;

typedef struct _TS_header
{
	unsigned int 	sync_byte:8;
	unsigned int 	transport_error_indicator:1;
	unsigned int 	payload_unit_start_indicator:1;
	unsigned int	transport_priority:1;
	unsigned int	pid:13;
	unsigned int	transport_scrambling_control:2;
	unsigned int	adaptation_field_exists:1;
	unsigned int	payload_data_exists:1;
	unsigned int	continuity_counter:4;
}
TS_Header;

typedef struct _TS_adaptation_field
{
	unsigned short	adaptation_field_length:8;
	unsigned short	discontinuity_indicator:1;
	unsigned short	random_access_indicator:1;
	unsigned short	elementary_stream_priority_indicator:1;
	unsigned short	pcr_flag:1;
	unsigned short	opcr_flag:1;
	unsigned short	splicing_point_flag:1;
	unsigned short	transport_private_data_flag:1;
	unsigned short	adaptation_field_extension_flag:1;
}
TS_Adaptation_Field;

typedef struct _TS_PAT_data
{
	unsigned char	version_number;
	unsigned char	current_next_indicator;
	unsigned char	section_number;
	unsigned char	last_section_number;
	unsigned short	program_map_pid;
} TS_PAT_data;

typedef struct _TS_Turing_Stuff
{
	int						block_no;
	int						crypted;
	unsigned char			unknown_field[4];
	unsigned char			key[16];
} TS_Turing_Stuff;

typedef struct _TS_PES_Packet
{
	unsigned char			marker_bits:2;
	unsigned char			scrambling_control:2;
	unsigned char			priority:1;
	unsigned char			data_alignment_indicator:1;
	unsigned char			copyright:1;
	unsigned char			original_or_copy:1;
	unsigned char			PTS_DTS_indicator:2;
	unsigned char			ESCR_flag:1;
	unsigned char			ES_rate_flag:1;
	unsigned char			DSM_trick_mode_flag:1;
	unsigned char			additional_copy_info_flag:1;
	unsigned char			CRC_flag:1;
	unsigned char			extension_flag:1;
	unsigned int			PES_header_length;
} PES_packet;

typedef struct _TS_Prog_Elements
{
	unsigned char			stream_type_id;
	unsigned int			stream_pid;
	unsigned char			stream_id;
	ts_stream_types			stream_type;
	TS_Turing_Stuff			turing_stuff;
	unsigned int			pkt_length;
	PES_packet				PES_pkt;	
	unsigned char			PES_hdr[TS_FRAME_SIZE];
	unsigned int			PES_hdr_len;
	
} TS_Stream_Element;

typedef struct _TS_Stream
{
	unsigned char			* pPacket;
	unsigned int			payload_offset;
	unsigned int			initial_offset;
	unsigned int			packet_counter;
	TS_Header				ts_header;
	TS_Adaptation_Field		ts_adapt;
	TS_PAT_data				ts_pat;
	ts_packet_pid_types		ts_packet_type;
	TS_Stream_Element		ts_stream_elem[TS_STREAM_ELEMENT_MAX];
} TS_Stream;


/*
 * called for each frame
 */
int process_ps_frame(unsigned char code, turing_state * turing, OFF_T_TYPE packet_start, void * packet_stream, read_func_t read_handler, void * ofh, write_func_t write_handler);
int process_ts_frame(turing_state * turing, OFF_T_TYPE packet_start, void * packet_stream, read_func_t read_handler, void * ofh, write_func_t write_handler);

#ifdef __cplusplus
}
#endif
#endif
