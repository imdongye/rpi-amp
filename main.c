/*
    201821089 임동예
	
	3개의 소캣으로 악기의 인풋을 받는 원격 앰프 모듈

	Note: 
		악기 연결되고 자신의 아이디를 먼저 보내서 식별한다. [CLNT]0
		클라이언트 포트 특정 어떻게하는지 모르겠음.
		종료할때는 id 값이 10이상의 값을 보내줘야한다.

		소캣 메시지 첫번째 id, 두번째 note(30~110), 새번째 volume(0~100)
		주기는 10ms

	Todo:
		1. UDP 통신

	gcc -o server main.c -lpthread -ldl -lm -lSDL2 && ./server 5000
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

#include "copied_types.h"


#define MAX_VOLUME 100

#define MAX_CLNT 3
static int serv_connecting_sockfd;
static int clnt_sockfds[MAX_CLNT];
static pthread_t clnt_tids[MAX_CLNT];
static pthread_t tid_test;
// button, ultrasonic, waterlevel
static int fonts[MAX_CLNT] = {0,1,2};

// Holds the global instance pointer
static tsf* g_TinySoundFont;

// A Mutex so we don't call note_on/note_off while rendering audio samples
static SDL_mutex* g_Mutex;

// 오디오 콜백 스트림이 오디오 버퍼
static void mixAudio(void* data, Uint8 *stream, int len)
{
	// Render the audio samples in float format
	int sampleCount = (len / (2 * sizeof(float))); //2 output channels
	tsf_render_float(g_TinySoundFont, (float*)stream, sampleCount, 0);
}


static void deinit() {
	// 리소스 해제
	int i;
	for(i=0; i<MAX_CLNT; i++) {
        if(clnt_sockfds[i]!=-1) {
            close(clnt_sockfds[i]);
            clnt_sockfds[i] = -1;
        }
		// Todo:쓰레드 종료
    }
	close(serv_connecting_sockfd);

	tsf_close(g_TinySoundFont);
	SDL_DestroyMutex(g_Mutex);
	SDL_CloseAudio();
	
    printf("[] 리소스 해제 후 종료\n");
    exit(0);
}

static void* threadClnt(void* data) {
	int clnt_sockfd = *(int*)data;
	int prevNote = -1;

    while(1) {
		payload_t payload;
		int font, note;
		float volume;
        int nr_msg = read(clnt_sockfd, &payload, sizeof(payload));
        if(nr_msg<=0) {
            printf("[] 소켓에서 읽었는데 에러 또는 클라이언트 종료됨\n");
            break;
        }
		if( payload.id>10) {
            printf("[] 클라이언트의 킬 신호 수신\n");
			break;
		}
		font = fonts[payload.id];
		note = payload.note;
		volume = payload.volume/(float)MAX_VOLUME;
		volume = (volume>1.f)?1.f:volume;

		printf("font:%d, note:%d, vol:%f\n", font, note, volume);

		// 이전에 재생한 노트재생을 종료한다. 종료 안해도되긴한다.
		if(prevNote>0)
			tsf_note_off(g_TinySoundFont, font, prevNote);
		tsf_note_on(g_TinySoundFont, font, note, volume);
		prevNote = note;
    }
    printf("[] threadClnt 쓰레드 탈출\n");
}

static void signalHandel(int sig) {
    if(sig==SIGINT || sig==SIGTERM) {
        printf("[] 서버 종료\n");
        deinit();
    }
}

int main(int argc, char *argv[])
{
	unsigned short serv_port;

	if( argc!=2 ) {
        fprintf(stderr, "wrong params\n");
        exit(1);
    } 
	serv_port=atoi(argv[1]);

	SDL_AudioSpec OutputAudioSpec;
	OutputAudioSpec.freq = 44100; // 1초에 재생되는 샘플 수
	OutputAudioSpec.format = AUDIO_F32;
	OutputAudioSpec.channels = 2;
	OutputAudioSpec.samples = 4096; // 버퍼사이즈 4096개의 샘플이 저장되면 신호를 전달한다.
	OutputAudioSpec.callback = mixAudio;

	/* 오디오 */
	{
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

	/* 서버 */
	{
		memset(clnt_sockfds, -1, sizeof(clnt_sockfds));

		// socket
		serv_connecting_sockfd = socket(PF_INET, SOCK_STREAM, 0);
		if( serv_connecting_sockfd < 0 ) {
			fprintf(stderr, "socket() failed\n");
			exit(1);
		}

		// bind
		struct sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(serv_port);
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		int rst_bind = bind(serv_connecting_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
		if( rst_bind < 0 ) {
			fprintf(stderr, "bind() failed\n");
			exit(1);
		}
		printf("[] 바인드 완료\n");

		// 강제종료 시그널
		signal(SIGTERM, signalHandel);
    	signal(SIGINT, signalHandel);

		// listen
		printf("[] %d번 포트 오픈\n", serv_port);
		int rst_listen = listen(serv_connecting_sockfd, 5);
		if( rst_listen < 0 ) {
			fprintf(stderr, "bind() failed\n");
			exit(1);
		}

		// accept
		int i;
		for( i=0; i<MAX_CLNT; i++ ) {
			printf("[] 연결대기중\n");
			struct sockaddr_in clnt_addr;
			socklen_t clnt_addr_size = sizeof(clnt_addr);
			clnt_sockfds[i] = accept(serv_connecting_sockfd, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
			if( clnt_sockfds[i] < 0 ) {
				fprintf(stderr, "accept() failed\n");
				exit(1);
			}
			int rst_thd;
			printf("[] 연결완료 클라포트%d\n", clnt_addr.sin_port);
			// 악기 신호 수신 쓰레드 생성
			rst_thd = pthread_create(&clnt_tids[i], NULL, threadClnt, (void*)&clnt_sockfds[i]);
			if( rst_thd!=0 ) {
				fprintf(stderr, "error pthread_create with code : %d\n", i);
				exit(1);
			}
		}
	}

	// 종료
	{
		int rst_thd, status, i;
		for( i=0; i<MAX_CLNT; i++ ) {
			rst_thd = pthread_join(clnt_tids[i], (void**)&status);
			if( rst_thd!=0 ) {
				fprintf(stderr, "error pthread_join with code : %d\n", rst_thd);
				exit(1);
			}
			printf("[] %d악기종료 %d\n", i);
		}
	}

	deinit();
	return 0;
}
