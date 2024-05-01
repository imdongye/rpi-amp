## rpi-amp

4개의 클라이언트는 서로다른 센서를 가지고 주파수와 진폭값을 서버에 보낸다.

서버는 로컬네트워크에서 bsd socket api로 수신한 값으로 사운드폰트를 출력한다.



---

![created by luftaquila](readme/overview.png)

![](https://img.youtube.com/vi/MmDjA5BigVI/0.jpg)

[유튜브 링크](https://youtu.be/MmDjA5BigVI?si=FCbgIehlzDGsObau)

[전체 프로젝트 링크](https://git.ajou.ac.kr/luftaquila/trinity)

---

windows에서 minisdl_audio.c가 컴파일돼서 정상작동하지만

rpi에서 런타임에 DSP_GetDeviceBuf의 mixbuf에서 segmentation fault가 발생함.

apt install lib-sdl2-dev로 설치된 sdl를 동적링킹으로 사용하면 동작한다.


Note: 
rpi에서 example1.c를 실행할때 
Assertion 'm' failed at pulse/mainloop.c:919, function pa_mainloop_iterate(). Aborting.
pulse/mainloop.c:919,[1] + Done                       "/usr/bin/gdb" --interpreter=mi --tty=${DbgTerm} 0<"/tmp/Microsoft-MIEngine-In-feawpsix.wbv" 1>"/tmp/Microsoft-MIEngine-Out-cpa5qpvx.0cd"
위와 같은 에러 발생

병준님 로컬네트워크 주소 192.168.33.70

```

sudo apt install cmake libasound2-dev libsdl2-dev pulseaudio
( sox libsox-fmt-mp3 freeglut3-dev )

cmake -Bbuild .

cmake --build ./build

./build/amp_exe

// 또는

gcc -o server main.c -lpthread -ldl -lm -lSDL2 && ./server 5000

gcc -o client test_clnt.c -lpthread -ldl -lm && ./client 127.0.0.1 5000

```

