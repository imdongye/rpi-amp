/*
    201821089 임동예
	
	서버에 신호만 보내는 테스트용 클라이언트
	
	gcc -o client test_clnt.c -lpthread -ldl -lm
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "examples/minisdl_audio.h"
#define TSF_IMPLEMENTATION
#include "tsf.h"


#define SIZE_MSG 256
#define SIZE_REQ 256


// Holds the global instance pointer
static tsf* g_TinySoundFont;

// A Mutex so we don't call note_on/note_off while rendering audio samples
static SDL_mutex* g_Mutex;

// 스트림이 오디오 버퍼
static void mixAudio(void* data, Uint8 *stream, int len)
{
	// Render the audio samples in float format
	int sampleCount = (len / (2 * sizeof(float))); //2 output channels
	tsf_render_float(g_TinySoundFont, (float*)stream, sampleCount, 0);
}



static void init() {
	SDL_AudioSpec OutputAudioSpec;
	OutputAudioSpec.freq = 44100; // 1초에 재생되는 샘플 수
	OutputAudioSpec.format = AUDIO_F32;
	OutputAudioSpec.channels = 2;
	OutputAudioSpec.samples = 4096; // 버퍼사이즈 4096개의 샘플이 저장되면 신호를 전달한다.
	OutputAudioSpec.callback = mixAudio;

	if( SDL_AudioInit(TSF_NULL) < 0 ) {
		fprintf(stderr, "Could not initialize audio hardware or driver\n");
		exit(1);

	}

	g_TinySoundFont = tsf_load_filename("florestan-subset.sf2");
	if( !g_TinySoundFont ) {
		fprintf(stderr, "Could not load SoundFont\n");
		exit(1);
	}
	// Set the SoundFont rendering output mode
	tsf_set_output(g_TinySoundFont, TSF_STEREO_INTERLEAVED, OutputAudioSpec.freq, 0);


	if( SDL_OpenAudio(&OutputAudioSpec, TSF_NULL) < 0 ) {
		fprintf(stderr, "Could not open the audio hardware or the desired audio output format\n");
		exit(1);
	}
	SDL_PauseAudio(0);

	g_Mutex = SDL_CreateMutex();
}

static void deinit() {
	// 리소스 해제
	tsf_close(g_TinySoundFont);
	SDL_DestroyMutex(g_Mutex);
	SDL_CloseAudio();
}

pthread_t tid_test;
void threadInput() {
	char req[SIZE_REQ];
	int fontIdx = 1;
	int prevNote = -1;
    while(1) {
        int nr_req = read(STDIN_FILENO, req, SIZE_REQ);
        if(nr_req<=0) {
            printf("[] read stdin error\n");
            break;
        }
        int note = atoi(req);
		printf("%d\n", note);
		if(prevNote>0)
			tsf_note_off(g_TinySoundFont, fontIdx, prevNote);
		tsf_note_on(g_TinySoundFont, fontIdx, note, 1.f);
		prevNote = note;
    }
    printf("[] input 쓰레드 탈출\n");
}

int main(int argc, char *argv[])
{
	int i;
	int notes[7] = { 48, 50, 52, 53, 55, 57, 59 };
	init();

	// for( i = 0; i < tsf_get_presetcount(g_TinySoundFont); i++ )
	// {
	// 	//Get exclusive mutex lock, end the previous note and play a new note
	// 	printf("Play note %d with preset #%d '%s'\n", notes[i % 7], i, tsf_get_presetname(g_TinySoundFont, i));
	// 	tsf_note_off(g_TinySoundFont, i - 1, notes[(i - 1) % 7]);
	// 	tsf_note_on(g_TinySoundFont, i, notes[i % 7], 1.0f);
	// 	SDL_Delay(1000);
	// }
	printf("[] 입출력 쓰레드 등록, 시작\n");
    int rst_thd;
    rst_thd = pthread_create(&tid_test, NULL, threadInput, NULL);
    if( rst_thd!=0 ) {
        fprintf(stderr, "error pthread_create with code : %d\n", rst_thd);
        exit(1);
    }
    int status;
	rst_thd = pthread_join(tid_test, (void**)&status);
    if( rst_thd!=0 ) {
        fprintf(stderr, "error pthread_join with code : %d\n", rst_thd);
        exit(1);
    }

	deinit();
	return 0;
}
