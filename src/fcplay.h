#pragma once

#ifdef __PSP__
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspaudio.h>
#include <pspaudiolib.h>
#else
#include <alsa/asoundlib.h>
#endif

#include "FC.h"

#ifdef __PSP__
#define printf  pspDebugScreenPrintf
PSP_MODULE_INFO ("FCPLAY", 0, 1, 1);
#endif

using namespace std;
using namespace std::filesystem;
 
namespace sys
{
	bool done = false;

void print_usage(const char* program_file)
{
	cerr << "Usage: " << program_file << " [OPTION]... [FILE]..." << std::endl;
	cerr << "Play Future Composer Tracker .fc4/.fc13 audio FILEs (the current directory by default)." << std::endl << std::endl;

	cerr << "Mandatory arguments to long options are mandatory for short options too." << std::endl;
	cerr << "  -r, --recursive             recursively traverse directories" << std::endl;
	cerr << "  -s OFFSET, --start=OFFSET   step start offset" << std::endl;
	cerr << "  -e OFFSET, --end=OFFSET     step end offset" << std::endl;
	cerr << std::endl;
}

namespace config
{
	bool recursive = false;
	bool ttyout    = true;
	constexpr static const uint32_t FC_silent_data_len = 8 + 1;	// see FC.cpp
	namespace step
	{
		int start = 0;
		int end   = 0;
	};
	namespace audio
	{
		constexpr static const unsigned int DEFAULT_PCM_FREQ    = 44100;
		constexpr static const unsigned int DEFAULT_BITS        = 16;
		constexpr static const unsigned int DEFAULT_CHANNELS    = 2;
		constexpr static const unsigned int DEFAULT_BUFFER_SIZE = 4096;
		int pcm_freq    = DEFAULT_PCM_FREQ;
		int bits        = DEFAULT_BITS;
		int channels    = DEFAULT_CHANNELS;
		int buffer_size = DEFAULT_BUFFER_SIZE;
	};

	void args(int argc, char* argv[])
	{
#ifdef __PSP__
		argc = 3;
		delete[] (argv);
		argv = new char*[3];
		argv[0] = (char*)"fcplay";
		argv[1] = (char*)"-r";
		argv[2] = (char*)"host0:/songs/";
#endif
		if(argc < 1)
		{
			print_usage(argv[0]);
			exit(EXIT_FAILURE);
		}

		int opt;
		int option_index;

		static struct option long_options[] =
		{
			{ "recursive",       no_argument,    0, 'r' },
			{ "start"    , required_argument,    0, 's' },
			{ "end"      , required_argument,    0, 'e' },
			{ "file"     , required_argument,    0,  0  }, 
			{ NULL       ,                 0, NULL,  0  }
		};
		while((opt = getopt_long(argc, argv, "rs:e:", long_options, &option_index)) != -1)
		{
			switch(opt)
			{
				case 'r':
					recursive = true;
					break;
				case 's':
					if(optarg)
						step::start = atoi(optarg);
					break;
				case 'e':
					if(optarg)
						step::end   = atoi(optarg);
					break;
				default:
					print_usage(argv[0]);
					exit(EXIT_FAILURE);
					break;	
			}
		}

		if(optind == argc) { sys::print_usage(argv[0]); exit(EXIT_FAILURE); }
	}
};
namespace input
{
#ifdef __PSP__
	using gamepad = SceCtrlData;
	gamepad pad;
#endif
	struct old
	{
		void (*sighup)  (int);
		void (*sigint)  (int);
		void (*sigquit) (int);
		void (*sigterm) (int);
	};

	void handler(int signum)
	{
		switch (signum)
		{
			case SIGHUP:  done = true; break;
			case SIGINT:  done = true; break;
			case SIGQUIT: done = true; break;
			case SIGTERM: done = true; break;
			default:                   break;
		}
	}
	bool init()
	{
#ifdef __PSP__
		sceCtrlSetSamplingCycle (0);
		sceCtrlSetSamplingMode (PSP_CTRL_MODE_ANALOG);
#endif
		return ((signal(SIGHUP , &sys::input::handler) == SIG_ERR)
		     || (signal(SIGINT , &sys::input::handler) == SIG_ERR)
		     || (signal(SIGQUIT, &sys::input::handler) == SIG_ERR)
	             || (signal(SIGTERM, &sys::input::handler) == SIG_ERR)) ? false : true;
	}
	bool pressed (int buttons)
	{
#ifdef __PSP__
		return pad.Buttons & buttons;
#else
		return false;
#endif
	}
	bool update()
	{
#ifdef __PSP__
		sceCtrlReadBufferPositive (&pad, 1);
		return !pressed(PSP_CTRL_CROSS);
#else
		return false;
#endif
	}
};
namespace video
{
	bool init()
	{
#ifdef __PSP__
		pspDebugScreenInit ();
#endif
		return true;
	}
};

namespace audio
{
#ifdef __PSP__
	using handle = int;
	sys::audio::handle playback_handle;
#else
	using handle = snd_pcm_t;
	using params = snd_pcm_hw_params_t;
	sys::audio::params *hw_params;
	sys::audio::handle *playback_handle;
#endif
	std::vector<uint8_t> sample_buf;

	bool init()
	{
#ifdef __PSP__
		playback_handle = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL,1024,PSP_AUDIO_FORMAT_STEREO); 
		sample_buf.resize(sys::config::audio::buffer_size);
		return playback_handle != 0;
#else
		bool ret = !(snd_pcm_open(&playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0 ||
		snd_pcm_hw_params_malloc(&hw_params) < 0 ||
		snd_pcm_hw_params_any(playback_handle, hw_params) < 0 ||
		snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0 ||
		snd_pcm_hw_params_set_format(playback_handle, hw_params, SND_PCM_FORMAT_S16_LE) < 0 ||
		snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, (unsigned int *) &sys::config::audio::pcm_freq, 0) < 0 ||
		snd_pcm_hw_params_set_channels(playback_handle, hw_params, 2) < 0 ||
		snd_pcm_hw_params(playback_handle, hw_params) < 0 ||
		snd_pcm_prepare(playback_handle) < 0);
		
		snd_pcm_hw_params_free(hw_params);
		sample_buf.resize(sys::config::audio::pcm_freq);
		return ret;
#endif
	}
	bool halt()
	{
#ifdef __PSP__
		return true;
#else
		return snd_pcm_close(playback_handle) < 0;
#endif
	}
};

};

namespace player
{
	std::vector<path> playlist;

	bool is_playable(path src)
	{
		char fcBuf[5] = "";
		FILE* fp = fopen(src.c_str(), "rb");
		if(fread(fcBuf, 5, 1, fp) == 1)
		fclose(fp);
		return (fcBuf[0] == 0x53 && fcBuf[1] == 0x4D && fcBuf[2] == 0x4F && fcBuf[3] == 0x44 && fcBuf[4] == 0x00) ||
		       (fcBuf[0] == 0x46 && fcBuf[1] == 0x43 && fcBuf[2] == 0x31 && fcBuf[3] == 0x34 && fcBuf[4] == 0x00);
	}
	bool queue(path src)
	{
		src = !src.is_absolute() ? proximate(src) : src;
		if(exists(src))
		{
			if(!is_directory(src) && is_playable(src))
			{
				playlist.push_back(src);
				return true;
			}
			else if(is_directory(src))
			{
				directory_iterator dir_iter(src);
				while(dir_iter != end(dir_iter))
				{
					const directory_entry& dir_entry = *dir_iter++;
					if(!is_directory(dir_entry) || sys::config::recursive)
						queue(dir_entry.path());
				}
			}
		}
		return false;
	}
	bool play(size_t i = 0)
	{
		int err;
		std::vector<uint8_t> buf;
		size_t len = file_size(playlist[i]);

		if(!exists(playlist[i]) || len == 0) return false;

		buf.resize(len + sys::config::FC_silent_data_len);

		FILE* fp = fopen(playlist[i].c_str(), "rb");
		if(!fp) return false;

		err = fread(buf.data(), len, 1, fp);
		fclose(fp); if(err != 1) return false;

		if (!FC_init(buf.data(), len, sys::config::step::start, sys::config::step::end))
		{
			cerr << "File format not recognized." << endl;
			return false;
		}

		printf ("Open File '%s' (%zu bytes) \n", playlist[i].c_str(),len);
		printf ("Module format: %s\n", mixerFormatName);

		mixerInit(sys::audio::sample_buf.size(), sys::config::audio::bits, sys::config::audio::channels, 0);
		mixerSetReplayingSpeed();

		sys::done = false;
		while (!FC_songEnd && !sys::done)
		{
			mixerFillBuffer(sys::audio::sample_buf.data(), sys::config::audio::buffer_size >> 2);
#ifdef __PSP__
    			sceAudioOutputPannedBlocking(sys::audio::playback_handle, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, sys::audio::sample_buf.data());
#else
			if((err = snd_pcm_writei(sys::audio::playback_handle, sys::audio::sample_buf.data(), sys::config::audio::buffer_size >> 4)) != sys::config::audio::buffer_size >> 4)
			{
				fprintf(stderr, "write to audio interface failed (%s)\n", snd_strerror(err));
				exit(1);
			}
#endif
		}

		cout << endl;
		return true;
	}
};

