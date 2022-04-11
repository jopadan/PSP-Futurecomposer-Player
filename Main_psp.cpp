#include <ctype.h>
#include <iomanip.h>
#include <fstream.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspaudio.h>
#include <pspaudiolib.h>


#include "FC.h"
#include "MyTypes.h"
#include "LamePaula.h"

#define printf  pspDebugScreenPrintf

PSP_MODULE_INFO ("FCPLAY", 0, 1, 1);

/* Exit callback */
int
exitCallback (int arg1, int arg2, void *common)
{
	sceKernelExitGame ();
	return 0;
}

/* Callback thread */
int
callbackThread (SceSize args, void *argp)
{
	int cbid;

	cbid = sceKernelCreateCallback ("Exit Callback", exitCallback, NULL);
	sceKernelRegisterExitCallback (cbid);
	sceKernelSleepThreadCB ();

	return 0;
}

/* Sets up the callback thread and returns its thread id */
int
setupCallbacks (void)
{
	int thid = 0;

	thid = sceKernelCreateThread ("update_thread", callbackThread, 0x11,
				      0xFA0, THREAD_ATTR_USER, 0);
	if (thid >= 0) {
		sceKernelStartThread (thid, 0, 0);
	}
	return thid;
}


int done = 0;
int channel;

int
audio_thread (SceSize args, void *argp)
{
	fprintf (stderr, "audio_thread: start\n");
	ubyte* sample_buffer;
    sample_buffer = (ubyte *) malloc (1024 * 4);
    while (!done) {
		mixerFillBuffer(sample_buffer, 1024 * 4);
    	sceAudioOutputPannedBlocking(channel, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, sample_buffer); 
    }
    free(sample_buffer);
	return 0;
}


char infile[] = "host0:/songs/dextrous-synthtronic.fc4";


int main(int argc, char *argv[])
{
    char *file_buffer;
    int  file_len;
    SceCtrlData pad;
    pspDebugScreenInit ();
    setupCallbacks (); 
    sceCtrlSetSamplingCycle (0);
    sceCtrlSetSamplingMode (PSP_CTRL_MODE_ANALOG);

    printf ("--------------------------------------------------\n");
    printf ("FC Play by Optixx 2006\n");
    printf ("--------------------------------------------------\n");


    SceUID fd;
    SceIoStat stat;
    sceIoGetstat(infile,&stat);
    file_len = stat.st_size;
    printf("Open File '%s' (%i bytes) \n",infile,file_len);
    file_buffer = (char*)malloc(file_len);
    fprintf(stderr,"Song buffer=%p\n",file_buffer);
    if ((fd = sceIoOpen (infile, PSP_O_RDONLY, 0777))) {
            file_len = 
            sceIoRead (fd, file_buffer,file_len);
            sceIoClose (fd);
    } else {
        fprintf(stderr,"Cannot open '%s'\n",infile);
        return -1;
    }
    printf("Song '%s' loaded\n",infile);

    
    channel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL,1024,PSP_AUDIO_FORMAT_STEREO); 
    printf("Got audio channel %i \n",channel);
    if (!FC_init(file_buffer, file_len, 0, 0)) {
	    fprintf( stderr, "File format not recognized.\n");
	    return -1;
    }
    printf("FC init done\n");
 
    printf("Mixer init\n");
    mixerInit(44100, 16, 2, 0);
    mixerSetReplayingSpeed();

	SceUID thid;
	thid = sceKernelCreateThread ("audio_thread", audio_thread, 0x18,
				      0x1000, PSP_THREAD_ATTR_USER, NULL);
	if (thid < 0) {
		fprintf (stderr, "Error, could not create thread\n");
		sceKernelSleepThread ();
	}
	sceKernelStartThread (thid, 0, NULL);
    printf("Enter Main loop\n");
	while (!done) {
		sceCtrlReadBufferPositive (&pad, 1);
		if (pad.Buttons & PSP_CTRL_CROSS) {
			done=1;
		}
		sceKernelDelayThread (1000);
	}
	fprintf (stderr, "done..\n");
	return 0;
}
