// History: This once was a simple mixer (it still is ;) that was
// used in a private MOD/TFMX player and was configured at run-time
// to mix up to 32 individual audio channels. In this particular
// version 16-bit and mono are moved back in. Most of that old code
// has not been touched though because it has done its job well even
// if it looks quite ugly. So, please bear with me if you find things
// in here that are not used by the FC player.

#include "LamePaula.h"

uint16_t MIXER_patterns;
uint16_t MIXER_samples;
uint16_t MIXER_voices;
uint8_t MIXER_speed;
uint8_t MIXER_bpm;
uint8_t MIXER_count;
uint16_t MIXER_minPeriod;
uint16_t MIXER_maxPeriod;

struct channel logChannel[32];

void (*mixerPlayRout)();
const char* mixerFormatName = 0;

void* mixerFill8bitMono( void*, uint32_t );
void* mixerFill8bitStereo( void*, uint32_t );
void* mixerFill16bitMono( void*, uint32_t );
void* mixerFill16bitStereo( void*, uint32_t );

void* (*mixerFillRout)( void*, uint32_t ) = &mixerFill8bitMono;

void mixerSetReplayingSpeed();

static const uint32_t AMIGA_CLOCK_PAL = 3546895;
static const uint32_t AMIGA_CLOCK_NTSC = 3579546;
static const uint32_t AMIGA_CLOCK = AMIGA_CLOCK_PAL;

static int8_t mix8[256];
static int16_t mix16[256];

static uint8_t zero8bit;   // ``zero''-sample
static uint16_t zero16bit;  // either signed or unsigned

static uint32_t pcmFreq;
static uint8_t bufferScale;

static uint32_t samplesAdd;
static uint32_t samplesPnt;
static uint16_t samples, samplesOrg;

static uint32_t toFill = 0;

static uint8_t emptySample;


void channel::takeNextBuf()
{
    if (!isOn)
    {
        // If channel is off, take sample START parameters.
        start = paula.start;
        length = paula.length;
        length <<= 1;
        if (length == 0)  // Paula would play $FFFF words (!)
            length = 1;
        end = start+length;
    }
    
    repeatStart = paula.start;
    repeatLength = paula.length;
    repeatLength <<= 1;
    if (repeatLength == 0)  // Paula would play $FFFF words (!)
        repeatLength = 1;
    repeatEnd = repeatStart+repeatLength;
}

void channel::on()
{
    takeNextBuf();

    isOn = true;
}

void channel::updatePerVol()
{
    if (paula.period != curPeriod)
    {
        period = paula.period;  // !!!
        curPeriod = paula.period;
        if (curPeriod != 0)
        {
            stepSpeed = (AMIGA_CLOCK/pcmFreq) / curPeriod;
            stepSpeedPnt = (((AMIGA_CLOCK/pcmFreq)%curPeriod)*65536) / curPeriod;
        }
        else
        {
            stepSpeed = stepSpeedPnt = 0;
        }
    }
    
    volume = paula.volume;
    if (volume > 64)
    {
        volume = 64;
    }
}

void mixerInit(uint32_t freq, int bits, int channels, uint16_t zero)
{
    pcmFreq = freq;
    bufferScale = 0;
    
    if (bits == 8)
    {
        zero8bit = zero;
        if (channels == 1)
        {
            mixerFillRout = &mixerFill8bitMono;
        }
        else  // if (channels == 2)
        {
            mixerFillRout = &mixerFill8bitStereo;
            ++bufferScale;
        }
    }
    else  // if (bits == 16)
    {
        zero16bit = zero;
        ++bufferScale;
        if (channels == 1)
        {
            mixerFillRout = &mixerFill16bitMono;
        }
        else  // if (channels == 2)
        {
            mixerFillRout = &mixerFill16bitStereo;
            ++bufferScale;
        }
    }
    
	uint16_t ui;
    long si;
	uint16_t voicesPerChannel = MIXER_voices/channels;

    // Input samples: 80,81,82,...,FE,FF,00,01,02,...,7E,7F
    // Array: 00/x, 01/x, 02/x,...,7E/x,7F/x,80/x,81/x,82/x,...,FE/x/,FF/x 
    ui = 0;
    si = 0;
    while (si++ < 128)
		mix8[ui++] = (int8_t)(si/voicesPerChannel);
    
    si = -128;
    while (si++ < 0)
		mix8[ui++] = (int8_t)(si/voicesPerChannel);
    
    // Input samples: 80,81,82,...,FE,FF,00,01,02,...,7E,7F
    // Array: 0/x, 100/x, 200/x, ..., FF00/x
    ui = 0;
    si = 0;
    while (si < 128*256)
    {
		mix16[ui++] = (int16_t)(si/voicesPerChannel);
        si += 256;
    }
    si = -128*256;
    while (si < 0)
    {
		mix16[ui++] = (int16_t)(si/voicesPerChannel);
        si += 256;
    }

    for ( int i = 0; i < 32; i++ )
    {
        logChannel[i].start = &emptySample;
        logChannel[i].end = &emptySample+1;
        logChannel[i].repeatStart = &emptySample;
        logChannel[i].repeatEnd = &emptySample+1;
        logChannel[i].length = 1;
        logChannel[i].curPeriod = 0;
        logChannel[i].stepSpeed = 0;
        logChannel[i].stepSpeedPnt = 0;
        logChannel[i].stepSpeedAddPnt = 0;
        logChannel[i].volume = 0;
        logChannel[i].off();
    }
    
    mixerSetReplayingSpeed();
}

void mixerFillBuffer( void* buf, size_t len )
{
    // Both, 16-bit and stereo samples take more memory.
    // Hence fewer samples fit into the buffer.
    len >>= bufferScale;
  
    while ( len > 0 )
    {
        if ( toFill > len )
        {
            buf = (*mixerFillRout)(buf, len);
            toFill -= len;
            len = 0;
        }
        else if ( toFill > 0 )
        {
            buf = (*mixerFillRout)(buf, toFill);
            len -= toFill;
            toFill = 0;   
        }
	
        if ( toFill == 0 )
        {
            (*mixerPlayRout)();

            const uint32_t temp = ( samplesAdd += samplesPnt );
            samplesAdd = temp & 0xFFFF;
            toFill = samples + ( temp > 65535 );
	  
            for ( int i = 0; i < MIXER_voices; i++ )
            {
                if ( logChannel[i].period != logChannel[i].curPeriod )
                {
                    logChannel[i].curPeriod = logChannel[i].period;
                    if (logChannel[i].curPeriod != 0)
                    {
                        logChannel[i].stepSpeed = (AMIGA_CLOCK/pcmFreq) / logChannel[i].period;
                        logChannel[i].stepSpeedPnt = (((AMIGA_CLOCK/pcmFreq) % logChannel[i].period ) * 65536 ) / logChannel[i].period;
                    }
                    else
                    {
                        logChannel[i].stepSpeed = logChannel[i].stepSpeedPnt = 0;
                    }
                }
            }
        }
        
    } // while len
}

void mixerSetReplayingSpeed()
{
    samples = ( samplesOrg = pcmFreq / 50 );
    samplesPnt = (( pcmFreq % 50 ) * 65536 ) / 50;  
    samplesAdd = 0;
}

void mixerSetBpm( uint16_t bpm )
{
    uint16_t callsPerSecond = (bpm * 2) / 5;
    samples = ( samplesOrg = pcmFreq / callsPerSecond );
    samplesPnt = (( pcmFreq % callsPerSecond ) * 65536 ) / callsPerSecond;  
    samplesAdd = 0; 
}

void* mixerFill8bitMono( void* buf, uint32_t numberOfSamples )
{
    uint8_t* buffer8bit = (uint8_t*)buf;
    for ( int i = 0; i < MIXER_voices; i++ )
    {
        buffer8bit = (uint8_t*)buf;
      
        for ( uint32_t n = numberOfSamples; n > 0; n-- )
        {
            if ( i == 0 )
            {
                *buffer8bit = zero8bit;
            }
            logChannel[i].stepSpeedAddPnt += logChannel[i].stepSpeedPnt;
            logChannel[i].start += ( logChannel[i].stepSpeed + ( logChannel[i].stepSpeedAddPnt > 65535 ) );
            logChannel[i].stepSpeedAddPnt &= 65535;
            if ( logChannel[i].start < logChannel[i].end )
            {
                *buffer8bit += ( logChannel[i].volume * mix8[*logChannel[i].start] ) >> 6;
            }
            else
            {
                if ( logChannel[i].looping )
                {
                    logChannel[i].start = logChannel[i].repeatStart;
                    logChannel[i].end = logChannel[i].repeatEnd;
                    if ( logChannel[i].start < logChannel[i].end )
                    {
                        *buffer8bit += ( logChannel[i].volume * mix8[*logChannel[i].start] ) >> 6;
                    }
                }
            }
            buffer8bit++;
        }
    }
    return(buffer8bit);
}

void* mixerFill8bitStereo( void* buf, uint32_t numberOfSamples )
{
    uint8_t* buffer8bit = (uint8_t*)buf;
    for ( int i = 1; i < MIXER_voices; i+=2 )
    {
        buffer8bit = ((uint8_t*)buf)+1;
      
        for ( uint32_t n = numberOfSamples; n > 0; n-- )
        {
            if ( i == 1 )
            {
                *buffer8bit = zero8bit;
            }
            logChannel[i].stepSpeedAddPnt += logChannel[i].stepSpeedPnt;
            logChannel[i].start += ( logChannel[i].stepSpeed + ( logChannel[i].stepSpeedAddPnt > 65535 ) );
            logChannel[i].stepSpeedAddPnt &= 65535;
            if ( logChannel[i].start < logChannel[i].end )
            {
                *buffer8bit += ( logChannel[i].volume * mix8[*logChannel[i].start] ) >> 6;
            }
            else
            {
                if ( logChannel[i].looping )
                {
                    logChannel[i].start = logChannel[i].repeatStart;
                    logChannel[i].end = logChannel[i].repeatEnd;
                    if ( logChannel[i].start < logChannel[i].end )
                    {
                        *buffer8bit += ( logChannel[i].volume * mix8[*logChannel[i].start] ) >> 6;
                    }
                }
            }
            buffer8bit += 2;
        }
    }
    for ( int i = 0; i < MIXER_voices; i+=2 )
    {
        buffer8bit = (uint8_t*)buf;
      
        for ( uint32_t n = numberOfSamples; n > 0; n-- )
        {
            if ( i == 0 )
            {
                *buffer8bit = zero8bit;
            }
            logChannel[i].stepSpeedAddPnt += logChannel[i].stepSpeedPnt;
            logChannel[i].start += ( logChannel[i].stepSpeed + ( logChannel[i].stepSpeedAddPnt > 65535 ) );
            logChannel[i].stepSpeedAddPnt &= 65535;
            if ( logChannel[i].start < logChannel[i].end )
            {
                *buffer8bit += ( logChannel[i].volume * mix8[*logChannel[i].start] ) >> 6;
            }
            else
            {
                if ( logChannel[i].looping )
                {
                    logChannel[i].start = logChannel[i].repeatStart;
                    logChannel[i].end = logChannel[i].repeatEnd;
                    if ( logChannel[i].start < logChannel[i].end )
                    {
                        *buffer8bit += ( logChannel[i].volume * mix8[*logChannel[i].start] ) >> 6;
                    }
                }
            }
            buffer8bit += 2;
        }
    }
    return(buffer8bit);
}

void* mixerFill16bitMono( void* buf, uint32_t numberOfSamples )
{
    int16_t* buffer16bit = (int16_t*)buf;
    for ( int i = 0; i < MIXER_voices; i++ )
    {
        buffer16bit = (int16_t*)buf;
	
        for ( uint32_t n = numberOfSamples; n > 0; n-- )
        {
            if ( i == 0 )
            {
                *buffer16bit = zero16bit;
            }
            logChannel[i].stepSpeedAddPnt += logChannel[i].stepSpeedPnt;
            logChannel[i].start += ( logChannel[i].stepSpeed + ( logChannel[i].stepSpeedAddPnt > 65535 ) );
            logChannel[i].stepSpeedAddPnt &= 65535;
            if ( logChannel[i].start < logChannel[i].end )
            {
                *buffer16bit += ( logChannel[i].volume * mix16[*logChannel[i].start] ) >> 6;
            }
            else
            {
                if ( logChannel[i].looping )
                {
                    logChannel[i].start = logChannel[i].repeatStart;
                    logChannel[i].end = logChannel[i].repeatEnd;
                    if ( logChannel[i].start < logChannel[i].end )
                    {
                        *buffer16bit += ( logChannel[i].volume * mix16[*logChannel[i].start] ) >> 6;
                    }
                }
            }
            buffer16bit++;
        }
    }
    return(buffer16bit);
}

void* mixerFill16bitStereo( void* buf, uint32_t numberOfSamples )
{
    int16_t* buffer16bit = (int16_t*)buf;
    for ( int i = 1; i < MIXER_voices; i+=2 )
    {
        buffer16bit = ((int16_t*)buf)+1;
	
        for ( uint32_t n = numberOfSamples; n > 0; n-- )
        {
            if ( i == 1 )
            {
                *buffer16bit = zero16bit;
            }
            logChannel[i].stepSpeedAddPnt += logChannel[i].stepSpeedPnt;
            logChannel[i].start += ( logChannel[i].stepSpeed + ( logChannel[i].stepSpeedAddPnt > 65535 ) );
            logChannel[i].stepSpeedAddPnt &= 65535;
            if ( logChannel[i].start < logChannel[i].end )
            {
                *buffer16bit += ( logChannel[i].volume * mix16[*logChannel[i].start] ) >> 6;
            }
            else
            {
                if ( logChannel[i].looping )
                {
                    logChannel[i].start = logChannel[i].repeatStart;
                    logChannel[i].end = logChannel[i].repeatEnd;
                    if ( logChannel[i].start < logChannel[i].end )
                    {
                        *buffer16bit += ( logChannel[i].volume * mix16[*logChannel[i].start] ) >> 6;
                    }
                }
            }
            buffer16bit += 2;
        }
    }
    for ( int i = 0; i < MIXER_voices; i+=2 )
    {
        buffer16bit = (int16_t*)buf;
	
        for ( uint32_t n = numberOfSamples; n > 0; n-- )
        {
            if ( i == 0 )
            {
                *buffer16bit = zero16bit;
            }
            logChannel[i].stepSpeedAddPnt += logChannel[i].stepSpeedPnt;
            logChannel[i].start += ( logChannel[i].stepSpeed + ( logChannel[i].stepSpeedAddPnt > 65535 ) );
            logChannel[i].stepSpeedAddPnt &= 65535;
            if ( logChannel[i].start < logChannel[i].end )
            {
                *buffer16bit += ( logChannel[i].volume * mix16[*logChannel[i].start] ) >> 6;
            }
            else
            {
                if ( logChannel[i].looping )
                {
                    logChannel[i].start = logChannel[i].repeatStart;
                    logChannel[i].end = logChannel[i].repeatEnd;
                    if ( logChannel[i].start < logChannel[i].end )
                    {
                        *buffer16bit += ( logChannel[i].volume * mix16[*logChannel[i].start] ) >> 6;
                    }
                }
            }
            buffer16bit += 2;
        }
    }
    return(buffer16bit);
}
