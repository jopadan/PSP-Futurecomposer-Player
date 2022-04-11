#include <stdio.h>
#include <fstream>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include "FC.h"
#include "MyTypes.h"


#include <alsa/asoundlib.h>

using namespace std;

const udword extraFileBufLen = 8 + 1;	// see FC.cpp

bool exitFlag;

void (*oldSigHupHandler) (int);
void (*oldSigIntHandler) (int);
void (*oldSigQuitHandler) (int);
void (*oldSigTermHandler) (int);
void mysighandler(int signum)
{
	switch (signum) {
	case SIGHUP:
		{
			exitFlag = true;
			break;
		}
	case SIGINT:
		{
			exitFlag = true;
			break;
		}
	case SIGQUIT:
		{
			exitFlag = true;
			break;
		}
	case SIGTERM:
		{
			exitFlag = true;
			break;
		}
	default:
		break;
	}
}

int main(int argc, char *argv[])
{
	int inFileArgN = 0;
	int startStep = 0;
	int endStep = 0;
	int i;
	int err;
	snd_pcm_t *playback_handle;
	snd_pcm_hw_params_t *hw_params;

	for (int a = 1; a < argc; a++) {
		if (argv[a][0] == '-') {
			if (strncasecmp(&argv[a][0], "--start=", 8) == 0) {
				startStep = atoi(argv[a] + 8);
			} else if (strncasecmp(&argv[a][0], "--end=", 6) == 0) {
				endStep = atoi(argv[a] + 6);
			}
		} else {
			if (inFileArgN == 0) {
				inFileArgN = a;
			} else {
				cerr << argv[0] << ": Missings argument(s)." << endl;
			}
		}
	}

	if (inFileArgN == 0) {
		cerr << "Missing file name argument." << endl;
		exit(-1);
	}

	ubyte *buffer = 0;
	streampos fileLen = 0;
	ifstream myIn(argv[inFileArgN], ios::in | ios::binary | ios::ate);
	myIn.seekg(0, ios::end);
	fileLen = myIn.tellg();
	buffer = (ubyte *) malloc((int) fileLen + extraFileBufLen);
	myIn.seekg(0, ios::beg);
	cout << "Loading" << flush;
	streampos restFileLen = fileLen;
	while (restFileLen > INT_MAX) {
		myIn.read((char *) buffer + (fileLen - restFileLen), INT_MAX);
		restFileLen -= INT_MAX;
		cout << "." << flush;
	}
	if (restFileLen > 0) {
		myIn.read((char *) buffer + (fileLen - restFileLen), restFileLen);
		cout << "." << flush;
	}
	cout << endl << flush;

	if (myIn.bad()) {
		cerr << argv[0] << ": ERROR: Cannot read file: " << argv[inFileArgN] << endl;
		return (-1);
	}
	myIn.close();
	udword pcmFreq = 44100;
	ubyte *sampleBuffer = new ubyte[pcmFreq];

	if ((err = snd_pcm_open(&playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf(stderr, "cannot open audio device %s (%s)\n", argv[1], snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_any(playback_handle, hw_params)) < 0) {
		fprintf(stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf(stderr, "cannot set access type (%s)\n", snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_set_format(playback_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf(stderr, "cannot set sample format (%s)\n", snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, (unsigned int *) &pcmFreq, 0)) < 0) {
		fprintf(stderr, "cannot set sample rate (%s)\n", snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_set_channels(playback_handle, hw_params, 2)) < 0) {
		fprintf(stderr, "cannot set channel count (%s)\n", snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params(playback_handle, hw_params)) < 0) {
		fprintf(stderr, "cannot set parameters (%s)\n", snd_strerror(err));
		exit(1);
	}

	snd_pcm_hw_params_free(hw_params);

	if ((err = snd_pcm_prepare(playback_handle)) < 0) {
		fprintf(stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror(err));
		exit(1);
	}

	exitFlag = false;
	if ((signal(SIGHUP, &mysighandler) == SIG_ERR)
	    || (signal(SIGINT, &mysighandler) == SIG_ERR)
	    || (signal(SIGQUIT, &mysighandler) == SIG_ERR) || (signal(SIGTERM, &mysighandler) == SIG_ERR)) {
		cerr << argv[0] << ": ERROR: Cannot set signal handlers." << endl;
		exit(-1);
	}

	if (FC_init(buffer, fileLen, startStep, endStep)) {
		cout << "Module format: " << mixerFormatName << endl;
		extern void mixerInit(udword freq, int bits, int channels, uword zero);
		mixerInit(pcmFreq, 16, 2, 0);
		extern void mixerSetReplayingSpeed();
		mixerSetReplayingSpeed();
		while (true) {
			extern void mixerFillBuffer(void *, udword);
			mixerFillBuffer(sampleBuffer, 1024);
			if ((err = snd_pcm_writei(playback_handle, sampleBuffer, 256)) != 256) {
				fprintf(stderr, "write to audio interface failed (%s)\n", snd_strerror(err));
				exit(1);
			}
			if (exitFlag)
				break;
		}
	} else {
		cout << "File format not recognized." << endl;
	}
	snd_pcm_close(playback_handle);
	delete[]sampleBuffer;
	return (0);
}
