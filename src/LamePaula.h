#pragma once

#include "common.h"

extern uint16_t MIXER_patterns;
extern uint16_t MIXER_samples;
extern uint16_t MIXER_voices;
extern uint8_t MIXER_speed;
extern uint8_t MIXER_bpm;
extern uint8_t MIXER_count;
extern uint16_t MIXER_minPeriod;
extern uint16_t MIXER_maxPeriod;

extern const char* mixerFormatName;
extern void (*mixerPlayRout)();

extern void mixerSetBpm( uint16_t );

struct _paula
{
    // Paula
    const uint8_t* start;  // start address
    uint16_t length;        // length in 16-bit words
    uint16_t period;
    uint16_t volume;        // 0-64
};

class channel
{
 public:
    _paula paula;
    
    bool isOn;
    
    const uint8_t* start;
    const uint8_t* end;
    uint32_t length;
    
    const uint8_t* repeatStart;
    const uint8_t* repeatEnd;
    uint32_t repeatLength;
    
    uint8_t bitsPerSample;
    uint16_t volume;
    uint16_t period;
    uint32_t sampleFrequency;
    bool sign;
    bool looping;  // whether to loop sample buffer continously (PAULA emu)
    uint8_t panning;
  //
    uint16_t curPeriod;
    uint32_t stepSpeed;
    uint32_t stepSpeedPnt;
    uint32_t stepSpeedAddPnt;

    void off()
    {
        isOn = false;
    }

    channel()
    {
        off();
    }
    
    void on();
    void takeNextBuf();    // take parameters from paula.* (or just to repeat.*)
    void updatePerVol();   // period, volume
};

void mixerFillBuffer( void* buf, size_t len );
void mixerInit(uint32_t freq, int bits, int channels, uint16_t zero);
void mixerSetReplayingSpeed();
