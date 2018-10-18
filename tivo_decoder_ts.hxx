/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING file for license terms
 */

#ifndef TIVO_DECODER_TS_HXX_
#define TIVO_DECODER_TS_HXX_

#include <deque>
#include <map>

#include "bits.hxx"
#include "tivo_decoder_base.hxx"

extern std::map<uint32_t, bool> pktDumpMap;

const int TS_FRAME_SIZE = 188;

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
    int     block_no;
    int     crypted;
    uint8_t unknown_field[4];
    uint8_t key[16];
} TS_Turing_Stuff;

typedef struct _TS_header
{
    unsigned int sync_byte;
    unsigned int transport_error_indicator;
    unsigned int payload_unit_start_indicator;
    unsigned int transport_priority;
    unsigned int pid;
    unsigned int transport_scrambling_control;
    unsigned int adaptation_field_exists;
    unsigned int payload_data_exists;
    unsigned int continuity_counter;
} TS_Header;

typedef struct _TS_adaptation_field
{
    uint16_t adaptation_field_length;
    uint16_t discontinuity_indicator;
    uint16_t random_access_indicator;
    uint16_t elementary_stream_priority_indicator;
    uint16_t pcr_flag;
    uint16_t opcr_flag;
    uint16_t splicing_point_flag;
    uint16_t transport_private_data_flag;
    uint16_t adaptation_field_extension_flag;
} TS_Adaptation_Field;

typedef struct _TS_PAT_data
{
    uint8_t  version_number;
    uint8_t  current_next_indicator;
    uint8_t  section_number;
    uint8_t  last_section_number;
    uint16_t program_map_pid;
} TS_PAT_data;

typedef struct _TS_PES_Packet
{
    uint8_t marker_bits;
    uint8_t scrambling_control;
    uint8_t priority;
    uint8_t data_alignment_indicator;
    uint8_t copyright;
    uint8_t original_or_copy;
    uint8_t PTS_DTS_indicator;
    uint8_t ESCR_flag;
    uint8_t ES_rate_flag;
    uint8_t DSM_trick_mode_flag;
    uint8_t additional_copy_info_flag;
    uint8_t CRC_flag;
    uint8_t extension_flag;
    unsigned int  PES_header_length;
} PES_packet;

typedef struct _TS_Prog_Elements
{
    uint8_t    streamId;
    uint16_t   pesPktLength;
    PES_packet PES_hdr;
} TS_Stream_Element;

typedef struct
{
    // the 16-bit value match for the packet pids
    uint8_t code_match_lo;    // low end of the range of matches
    uint8_t code_match_hi;    // high end of the range of matches

    // what kind of TS packet is it?
    ts_stream_types ts_stream_type;
}
ts_pmt_stream_type_info;

typedef struct
{
    // the 16-bit value match for the packet pids
    uint16_t code_match_lo;      // low end of the range of matches
    uint16_t code_match_hi;      // high end of the range of matches

    // what kind of TS packet is it?
    ts_packet_pid_types ts_packet;
}
ts_packet_tag_info;

extern ts_packet_tag_info ts_packet_tags[];
extern ts_pmt_stream_type_info ts_pmt_stream_tags[];

class TiVoDecoderTS;
class TiVoDecoderTsStream;
class TiVoDecoderTsPacket;

typedef std::deque<uint16_t>                          TsLengths;
typedef std::deque<uint16_t>::iterator                TsLengths_it;

typedef std::deque<TiVoDecoderTsPacket*>              TsPackets;
typedef std::deque<TiVoDecoderTsPacket*>::iterator    TsPackets_it;

typedef std::map<int, TiVoDecoderTsStream*>           TsStreams;
typedef std::map<int, TiVoDecoderTsStream*>::iterator TsStreams_it;

typedef std::map<uint32_t,bool>                       TsPktDump;
typedef std::map<uint32_t,bool>::iterator             TsPktDump_iter;

extern TsPktDump pktDumpMap;

class TiVoDecoderTS : public TiVoDecoder
{
    private:
        TsStreams   streams;
        uint32_t    pktCounter;
        TS_PAT_data patData;

    public:
        int handlePkt_PAT(TiVoDecoderTsPacket *pPkt);
        int handlePkt_PMT(TiVoDecoderTsPacket *pPkt);
        int handlePkt_TiVo(TiVoDecoderTsPacket *pPkt);

        virtual bool process();
        TiVoDecoderTS(TuringState &pTuringState, HappyFile &pInfile,
                      HappyFile &pOutfile);
        ~TiVoDecoderTS();
};

class TiVoDecoderTsStream
{
    private:
        HappyFile       &pOutfile;
        TiVoDecoderTS   *pParent;
        TsPackets       packets;
        TsLengths       pesHdrLengths;

        uint8_t         pesDecodeBuffer[TS_FRAME_SIZE * 10];

    public:
        uint8_t         stream_type_id;
        uint16_t        stream_pid;
        uint8_t         stream_id;
        ts_stream_types stream_type;
        TS_Turing_Stuff turing_stuff;

        bool            addPkt(TiVoDecoderTsPacket *pPkt);
        bool            getPesHdrLength(uint8_t *pBuffer, uint16_t bufLen);
        bool            decrypt(uint8_t *pBuffer, uint16_t bufLen);

        TiVoDecoderTsStream(HappyFile &pFileOut, TiVoDecoderTS *pDecoder,
                            uint16_t pid);
        ~TiVoDecoderTsStream();
};

class TiVoDecoderTsPacket
{
    private:
        static int          globalBufferLen;
        static uint8_t      globalBuffer[TS_FRAME_SIZE * 3];

        bool                isValid;
        bool                isPmt;
        bool                isTiVo;

        TS_Header           tsHeader;
        TS_Adaptation_Field tsAdaptation;

    public:
        TiVoDecoderTsStream *pParent;
        uint32_t            packetId;

        uint8_t             buffer[TS_FRAME_SIZE];
        uint8_t             payloadOffset;
        uint8_t             pesHdrOffset;
        ts_packet_pid_types ts_packet_type;

        int  read(HappyFile &pInfile);
        bool decode();
        void dump();
        void setStream(TiVoDecoderTsStream *pStream);

        inline void setTiVoPkt(bool isTiVoPkt)
            { isTiVo = isTiVoPkt; }
        inline bool isTiVoPkt()
            { return isTiVo; }
        inline void setPmtPkt(bool isPmtPkt)
            { isPmt = isPmtPkt; }
        inline bool isPmtPkt()
            { return isPmt; }

        inline bool   isTsPacket()
            { return (buffer[0] == 0x47) ? true : false; }
        inline uint16_t getPID()
            { return GET16(&buffer[1]) & 0x1FFF; }
        inline bool   getPayloadStartIndicator()
            { return (GET16(&buffer[1]) & 0x4000) ? true : false; }
        inline bool   getScramblingControl()
            { return (buffer[3] & 0xC0) ? true : false; }
        inline void   clrScramblingControl()
            { buffer[3] &= ~(0xC0); }
        inline bool   getPayloadExists()
            { return (buffer[3] & 0x10) ? true : false; }
        inline bool   getAdaptHdrExists()
            { return (buffer[3] & 0x20) ? true : false; }

        TiVoDecoderTsPacket();
};

#endif /* TIVO_DECODER_TS_HXX_ */
