#pragma once

#include "FC.h"

static const uint16_t SMOD_SONGTAB_OFFSET = 0x0064;      // 100

static const uint16_t FC14_SMPHEADERS_OFFSET = 0x0028;   // 40
static const uint16_t FC14_WAVEHEADERS_OFFSET = 0x0064;  // 100
static const uint16_t FC14_SONGTAB_OFFSET = 0x00b4;      // 180

static const uint16_t TRACKTAB_ENTRY_LENGTH = 0x000d;    // 3*4+1
static const uint16_t PATTERN_LENGTH = 0x0040;           // 32*2
static const uint8_t PATTERN_BREAK = 0x49;

static const uint8_t SEQ_END = 0xE1;

static const uint8_t SNDMOD_LOOP = 0xE0;
static const uint8_t SNDMOD_END = SEQ_END;
static const uint8_t SNDMOD_SETWAVE = 0xE2;
static const uint8_t SNDMOD_CHANGEWAVE = 0xE4;
static const uint8_t SNDMOD_NEWVIB = 0xE3;
static const uint8_t SNDMOD_SUSTAIN = 0xE8;
static const uint8_t SNDMOD_NEWSEQ = 0xE7;
static const uint8_t SNDMOD_SETPACKWAVE = 0xE9;
static const uint8_t SNDMOD_PITCHBEND = 0xEA;

static const uint8_t ENVELOPE_LOOP = 0xE0;
static const uint8_t ENVELOPE_END = SEQ_END;
static const uint8_t ENVELOPE_SUSTAIN = 0xE8;
static const uint8_t ENVELOPE_SLIDE = 0xEA;

struct _FC_admin
{
    uint16_t dmaFlags;  // which audio channels to turn on (AMIGA related)
    uint8_t count;     // speed count
    uint8_t speed;     // speed
    uint8_t RScount;
    bool isEnabled;  // player on => true, else false
    
    struct _moduleOffsets
    {
        uint32_t trackTable;
        uint32_t patterns;
        uint32_t sndModSeqs;
        uint32_t volModSeqs;
        uint32_t FC_silent;
    } offsets;

    int usedPatterns;
    int usedSndModSeqs;
    int usedVolModSeqs;
} 
FC_admin;

struct FC_SOUNDinfo_internal
{
    const uint8_t* start;
    uint16_t len, repOffs, repLen;
    // rest was place-holder (6 bytes)
};

struct _FC_SOUNDinfo
{
    // 10 samples/sample-packs
    // 80 waveforms
    FC_SOUNDinfo_internal snd[10+80];
}
FC_SOUNDinfo;

struct _FC_CHdata
{
    channel* ch;  // paula and mixer interface
    
    uint16_t dmaMask;
    
    uint32_t trackStart;     // track/step pattern table
    uint32_t trackEnd;
    uint16_t trackPos;

    uint32_t pattStart;
    uint16_t pattPos;
    
    int8_t transpose;       // TR
    int8_t soundTranspose;  // ST
    int8_t seqTranspose;    // from sndModSeq
    
    uint8_t noteValue;
    
    int8_t pitchBendSpeed;
    uint8_t pitchBendTime, pitchBendDelayFlag;
    
    uint8_t portaInfo, portDelayFlag;
    int16_t portaOffs;
    
    uint32_t volSeq;
    uint16_t volSeqPos;
    
    uint8_t volSlideSpeed, volSlideTime, volSustainTime;
    
    uint8_t envelopeSpeed, envelopeCount;
    
    uint32_t sndSeq;
    uint16_t sndSeqPos;
    
    uint8_t sndModSustainTime;
    
    uint8_t vibFlag, vibDelay, vibSpeed,
         vibAmpl, vibCurOffs, volSlideDelayFlag;
    
    int8_t volume;
    uint16_t period;
    
    const uint8_t* pSampleStart;
    uint16_t repeatOffset;
    uint16_t repeatLength;
    uint16_t repeatDelay;
};

