/*
    201821089 임동예
	
	서버에 신호만 보내는 테스트용 클라이언트
	
	gcc -o client test_clnt.c -lpthread -ldl -lm && ./client 127.0.0.1 5000
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include "copied_types.h"

#define SIZE_MSG 256
#define SIZE_REQ 256

static int serv_sockfd;

static void deinit() {
    payload_t payload = {99, 0, 0};
    int rst_sock = write(serv_sockfd, &payload, sizeof(payload));
    if( rst_sock<=0 ) {
        printf("[] 소캣에 쓰기했는데 에러 또는 서버종료됨\n");
        exit(1);
    }
	close(serv_sockfd);
	printf("[] id>10으로 킬 신호 전달하고 클라종료\n");
	exit(0);
}

static void signalHandel(int sig) {
    if(sig==SIGINT || sig==SIGTERM) {
        printf("[] 강제종료되면서 쓰레드 리소스 해제\n");
		deinit();
    }
}

int main( int argc, char* argv[] )
{
    if( argc!=3 ) {
        fprintf(stderr, "wrong params\n");
        exit(1);
    } 
    unsigned short serv_port=atoi(argv[2]);
    in_addr_t serv_ip = inet_addr(argv[1]);
    printf("[] %d번 포트 접수 완료\n", serv_port);

    // 서버 연결
    {
        if(serv_ip==INADDR_NONE) {
            fprintf(stderr, "wrong ip addr format\n");
            exit(1);
        }

        // socket
        serv_sockfd = socket(PF_INET, SOCK_STREAM, 0);
        if( serv_sockfd < 0 ) {
            fprintf(stderr, "socket() failed\n");
            exit(1);
        }
        // connect
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(serv_port);
        serv_addr.sin_addr.s_addr = serv_ip;
        int rst_connect = connect(serv_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        if( rst_connect < 0 ) {
            fprintf(stderr, "connect() failed\n");
            exit(1);
        }

		// 강제종료 시그널
		signal(SIGTERM, signalHandel);
    	signal(SIGINT, signalHandel);

        printf("[] 서버 연결 완료\n");
    }

    // 서버 신호 발신 코드
    char req[SIZE_REQ];
    int font = 1;
    int note = 50;
    int vol = 100;
    while(1) {
        payload_t payload;
        {
            note += 1;
            vol -= 10;
            payload.id = font;
            payload.note = note;
            payload.volume = vol;
        }
            
        //scanf("%d %d %d", &payload.id, &payload.note, &payload.volume);
        int rst_sock = write(serv_sockfd, &payload, sizeof(payload));
        if( rst_sock<=0 ) {
            printf("[] 소캣에 쓰기했는데 에러 또는 서버종료됨\n");
            break;
        }
        printf("send\n");
        sleep(1);
    }

    deinit();
    return 0;
}