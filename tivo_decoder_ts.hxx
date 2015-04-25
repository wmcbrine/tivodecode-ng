#ifndef TIVO_DECODER_TS_HXX_
#define TIVO_DECODER_TS_HXX_

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

#include <deque>
#include <map>
using namespace std;

#include "tivo_types.hxx"
#include "tivo_decoder_base.hxx"

extern std::map<UINT32, BOOL> pktDumpMap;
extern std::map<UINT32, BOOL>::iterator pktDumpMap_iter;

#define TS_FRAME_SIZE  188

#define PICTURE_START_CODE      0x100
#define SLICE_START_CODE_MIN    0x101
#define SLICE_START_CODE_MAX    0x1AF
#define USER_DATA_START_CODE    0x1B2
#define SEQUENCE_HEADER_CODE    0x1B3
#define SEQUENCE_ERROR_CODE     0x1B4
#define EXTENSION_START_CODE    0x1B5
#define SEQUENCE_END_CODE       0x1B7
#define GROUP_START_CODE        0x1B8
#define SYSTEM_START_CODE_MIN   0x1B9
#define SYSTEM_START_CODE_MAX   0x1FF
#define ISO_END_CODE            0x1B9
#define PACK_START_CODE         0x1BA
#define SYSTEM_START_CODE       0x1BB
#define VIDEO_ELEMENTARY_STREAM 0x1E0

#define ANCILLARY_DATA_CODE     0x1F9

//#define PICTURE_START_CODE      0x00
//#define SLICE_START_CODE_MIN    0x01
//#define SLICE_START_CODE_MAX    0xAF
//#define USER_DATA_START_CODE    0xB2
//#define SEQUENCE_HEADER_CODE    0xB3
//#define SEQUENCE_ERROR_CODE     0xB4
//#define EXTENSION_START_CODE    0xB5
//#define SEQUENCE_END_CODE       0xB7
//#define GROUP_START_CODE        0xB8
//#define SYSTEM_START_CODE_MIN   0xB9
//#define SYSTEM_START_CODE_MAX   0xFF
//#define ISO_END_CODE            0xB9
//#define PACK_START_CODE         0xBA
//#define SYSTEM_START_CODE       0xBB
//#define VIDEO_ELEMENTARY_STREAM 0xE0

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


typedef struct _TS_Turing_Stuff
{
    int                     block_no;
    int                     crypted;
    unsigned char           unknown_field[4];
    unsigned char           key[16];
}__attribute__((packed))  TS_Turing_Stuff;


typedef struct _TS_header
{
    unsigned int    sync_byte:8;
    unsigned int    transport_error_indicator:1;
    unsigned int    payload_unit_start_indicator:1;
    unsigned int    transport_priority:1;
    unsigned int    pid:13;
    unsigned int    transport_scrambling_control:2;
    unsigned int    adaptation_field_exists:1;
    unsigned int    payload_data_exists:1;
    unsigned int    continuity_counter:4;
} __attribute__((packed)) TS_Header;

typedef struct _TS_adaptation_field
{
    unsigned short  adaptation_field_length:8;
    unsigned short  discontinuity_indicator:1;
    unsigned short  random_access_indicator:1;
    unsigned short  elementary_stream_priority_indicator:1;
    unsigned short  pcr_flag:1;
    unsigned short  opcr_flag:1;
    unsigned short  splicing_point_flag:1;
    unsigned short  transport_private_data_flag:1;
    unsigned short  adaptation_field_extension_flag:1;
} __attribute__((packed)) TS_Adaptation_Field;

typedef struct _TS_PAT_data
{
    unsigned char   version_number;
    unsigned char   current_next_indicator;
    unsigned char   section_number;
    unsigned char   last_section_number;
    unsigned short  program_map_pid;
} __attribute__((packed)) TS_PAT_data;

typedef struct _TS_PES_Packet
{
    unsigned char           marker_bits:2;
    unsigned char           scrambling_control:2;
    unsigned char           priority:1;
    unsigned char           data_alignment_indicator:1;
    unsigned char           copyright:1;
    unsigned char           original_or_copy:1;
    unsigned char           PTS_DTS_indicator:2;
    unsigned char           ESCR_flag:1;
    unsigned char           ES_rate_flag:1;
    unsigned char           DSM_trick_mode_flag:1;
    unsigned char           additional_copy_info_flag:1;
    unsigned char           CRC_flag:1;
    unsigned char           extension_flag:1;
    unsigned int            PES_header_length;
} __attribute__((packed)) PES_packet;


typedef struct _TS_Prog_Elements
{
    UINT8                   streamId;
    UINT16                  pesPktLength;
    PES_packet              PES_hdr;
} __attribute__((packed)) TS_Stream_Element;


typedef struct
{
    // the 16-bit value match for the packet pids
    unsigned char code_match_lo;      // low end of the range of matches
    unsigned char code_match_hi;      // high end of the range of matches

    // what kind of TS packet is it?
    ts_stream_types ts_stream_type;
}
ts_pmt_stream_type_info;


typedef struct
{
    // the 16-bit value match for the packet pids
    unsigned short code_match_lo;      // low end of the range of matches
    unsigned short code_match_hi;      // high end of the range of matches

    // what kind of TS packet is it?
    ts_packet_pid_types ts_packet;
}
ts_packet_tag_info;



extern ts_packet_tag_info ts_packet_tags[];
extern ts_pmt_stream_type_info ts_pmt_stream_tags[];

class TiVoDecoderTS;
class TiVoDecoderTsStream;
class TiVoDecoderTsPacket;

typedef std::deque<UINT16>                              TsLengths;
typedef std::deque<UINT16>::iterator                    TsLengths_it;

typedef std::deque<TiVoDecoderTsPacket*>                TsPackets;
typedef std::deque<TiVoDecoderTsPacket*>::iterator      TsPackets_it;

typedef std::map<int, TiVoDecoderTsStream*>             TsStreams;
typedef std::map<int, TiVoDecoderTsStream*>::iterator   TsStreams_it;

typedef std::map<UINT32,BOOL>                           TsPktDump;
typedef std::map<UINT32,BOOL>::iterator                 TsPktDump_iter;
    
extern TsPktDump pktDumpMap;

/* All elements are in big-endian format and are packed */

class TiVoDecoderTS : public TiVoDecoder
{
    private:

        TsStreams       streams;
        UINT32          pktCounter;
        TS_PAT_data     patData;

    public:

        int handlePkt_PAT( TiVoDecoderTsPacket * pPkt );
        int handlePkt_PMT( TiVoDecoderTsPacket * pPkt );
        int handlePkt_TiVo( TiVoDecoderTsPacket * pPkt );
        int handlePkt_AudioVideo( TiVoDecoderTsPacket * pPkt );

        virtual BOOL process();
        TiVoDecoderTS(TuringState *pTuringState, HappyFile *pInfile,
                      FILE *pOutfile);
        ~TiVoDecoderTS();

};


class TiVoDecoderTsStream
{
    public:

        TiVoDecoderTS * pParent;
        TsPackets       packets;
        TsLengths       pesHdrLengths;

        UINT8           stream_type_id;
        UINT16          stream_pid;
        UINT8           stream_id;
        ts_stream_types stream_type;

        FILE            * pOutfile;

        TS_Turing_Stuff turing_stuff;
        
        UINT8           pesDecodeBuffer[TS_FRAME_SIZE * 10];
        
        void            setDecoder(TiVoDecoderTS * pDecoder);
        BOOL            addPkt(TiVoDecoderTsPacket * pPkt);
        BOOL            getPesHdrLength(UINT8 * pBuffer, UINT16 bufLen);
        BOOL            decrypt(UINT8 * pBuffer, UINT16 bufLen);

        TiVoDecoderTsStream(UINT16 pid);
        ~TiVoDecoderTsStream();
};


class TiVoDecoderTsPacket
{
    public:
        static int			globalBufferLen;
        static UINT8		globalBuffer[TS_FRAME_SIZE*3];

        TiVoDecoderTsStream * pParent;
        UINT32              packetId;

        BOOL                isValid;
        BOOL                isPmt;
        BOOL                isTiVo;

        UINT8               buffer[TS_FRAME_SIZE];
        UINT8               payloadOffset;
        UINT8               pesHdrOffset;
        TS_Header           tsHeader;
        TS_Adaptation_Field tsAdaptation;
        ts_packet_pid_types ts_packet_type;

        int           read(HappyFile *pInfile);
        BOOL          decode();
        void          dump();
        void          setStream(TiVoDecoderTsStream * pStream);

        inline void   setTiVoPkt(BOOL isTiVoPkt)
            { isTiVo = isTiVoPkt; }
        inline BOOL   isTiVoPkt()
            { return isTiVo; }
        inline void   setPmtPkt(BOOL isPmtPkt)
            { isPmt = isPmtPkt; }
        inline BOOL   isPmtPkt()
            { return isPmt; }
            
        inline BOOL   isTsPacket()
            { return (buffer[0] == 0x47) ? TRUE : FALSE;  }
        inline UINT16 getPID()
            { UINT16 val = portable_ntohs(&buffer[1]); return val & 0x1FFF; }
        inline BOOL   getPayloadStartIndicator()
            { UINT16 val = portable_ntohs(&buffer[1]); return (val & 0x4000) ?
              TRUE: FALSE; }
        inline BOOL   getScramblingControl()
            { return (buffer[3] &  0xC0) ? TRUE : FALSE; }
        inline void   clrScramblingControl()
            { buffer[3] &= ~(0xC0); }
        inline BOOL  getPayloadExists()
            { return (buffer[3] &  0x10) ? TRUE : FALSE; }
        inline BOOL  getAdaptHdrExists()
            { return (buffer[3] &  0x20) ? TRUE : FALSE; }

        TiVoDecoderTsPacket();
};



#endif /* TIVO_DECODER_TS_HXX_ */



/* vi:set ai ts=4 sw=4 expandtab: */

