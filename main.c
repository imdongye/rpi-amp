/*
    201821089 임동예
	
	3개의 소캣으로 악기의 인풋을 받는 원격 앰프 모듈

	Note: 
		악기 연결되고 자신의 아이디를 먼저 보내서 식별한다. [CLNT]0
		클라이언트 포트 특정 어떻게하는지 모르겠음.
		종료할때는 [CLNT]KILL

	Todo:
		1. UDP 통신

	gcc -o server main.c -lpthread -ldl -lm -lSDL2 && ./server 12345
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
#define MAX_CLNT 3
static int serv_connecting_sockfd;
static int clnt_sockfds[MAX_CLNT];
static pthread_t clnt_tids[MAX_CLNT];
static pthread_t tid_test;

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
	char msg[SIZE_MSG];
	int note;
	int fontIdx = 1;
	int clntId = -1;
	int prevNote = -1;

    while(1) {
        int nr_msg = read(clnt_sockfd, msg, SIZE_MSG);
        if(nr_msg<=0) {
            printf("[] 소켓에서 읽었는데 에러 또는 클라이언트 종료됨\n");
            break;
        } 
		else if(strcmp("[CLNT]KILL\n", msg)==0)  {
			printf("[] 클라이언트의 쉘 종료 메시지 수신 후 종료\n");
			break;
        } 
		else if( nr_msg==4 && msg[0]=='i' && msg[0]=='i' && msg[1]=='d' )  {
			clntId = atoi(msg+2);
			printf("[] 아이디 입력됨 %d\n", clntId);
			switch(clntId) {
			case 0: fontIdx = 0; break;
			case 1: fontIdx = 0; break;
			case 2: fontIdx = 0; break;
			}
			// Todo: 클라id에 따라서 음역대도 설정
			continue;
        }
		note = atoi(msg);
		printf("font:%d, note:%d\n", fontIdx, note);

		// 이전에 재생한 노트재생을 종료한다. 종료 안해도되긴한다.
		if(prevNote>0)
			tsf_note_off(g_TinySoundFont, fontIdx, prevNote);
		tsf_note_on(g_TinySoundFont, fontIdx, note, 1.f);
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
			printf("[] 악기종료 %d\n", i);

		}
	}

	deinit();
	return 0;
}
