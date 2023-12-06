#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1

#define SIZE_VAL_PATH 256
#define SIZE_VAL 64
#define SIZE_DIR_PATH 256
#define SIZE_BUFFER 3

static int GPIOExport(int pin) {
    char buffer[SIZE_BUFFER];
    ssize_t bytes_written;
    int fd = open("/sys/class/gpio/export", O_WRONLY); // 덮어쓰기
    if(fd<0) {
        fprintf(stderr, "[lim] Failed to open export for writing\n");
        return -1;
    }
    bytes_written = snprintf(buffer, SIZE_BUFFER, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return 0;
}

// 입출력 방향 설정
static int GPIODirection(int pin, int dir) {
    static const char s_directions_str[] = "in\0out";
    char path[SIZE_DIR_PATH];
    int fd;

    snprintf(path, SIZE_DIR_PATH, "/sys/class/gpio/gpio%d/direction", pin);
    fd = open(path, O_WRONLY); // 덮어쓰기
    if(fd<0) {
        fprintf(stderr, "[lim] Failed to open gpio direction for writeing\n");
        return -1;
    }
    if(write(fd, &s_directions_str[(IN==dir)?0:3], (IN==dir)?2:3) < 0) {
        fprintf(stderr, "[lim] Failed to set direction\n");
        return -1;
    }
    close(fd);
    return 0;
}

static int GPIORead(int pin) {
    char path[SIZE_VAL_PATH];
    char value_str[3];
    int fd;

    snprintf(path, SIZE_VAL_PATH, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_RDONLY);
    if(fd<0) {
        fprintf(stderr, "[lim] Failed to open gpio for reading\n");
        return -1;
    }
    if(read(fd, value_str, 3)<0) {
        fprintf(stderr, "[lim] Failed to read value\n");
        return -1;
    }
    close(fd);
    return atoi(value_str);
}

static int GPIOWrite(int pin, int value) {
    static const char s_value_str[] = "01";
    char path[SIZE_VAL_PATH];
    int fd;
    snprintf(path, SIZE_VAL_PATH, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_WRONLY); // 덮어쓰기
    if(fd<0) {
        fprintf(stderr, "[lim] Failed to open gpio direction for writeing\n");
        return -1;
    }
    if( write(fd, &s_value_str[(LOW==value)?0:1], 1)!=1 ) {
        fprintf(stderr, "[lim] Failed to write value\n");
        return -1;
    }
    close(fd);
    return 0;
}

//제어권 반납
static int GPIOUnexport(int pin) {
    char buffer[SIZE_BUFFER];
    ssize_t bytes_written;
    int fd = open("/sys/class/gpio/unexport", O_WRONLY); // 덮어쓰기
    if(fd<0) {
        fprintf(stderr, "Failed to open unexport for writing\n");
        return -1;
    }
    bytes_written = snprintf(buffer, SIZE_BUFFER, "%d", O_WRONLY);
    write(fd, buffer, bytes_written);
    close(fd);
    return 0;
}

/************************************ PWM ***********************************/

static int PWMExport(int pwmnum)
{
    char buffer[SIZE_BUFFER];
    int fd, byte;

    fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
    if( fd < 0 ) {
        fprintf(stderr, "Failed to open export for export!\n");
        return -1;
    }

    byte = snprintf(buffer, SIZE_BUFFER, "%d", pwmnum); // 문자열 변환했을때 길이
    // echo 1 > /sys/class/pwm/pwmchip0/export
    write(fd, buffer, byte);
    close(fd);

    sleep(1); // 왜 슬립?

    return 0;
}

static int PWMWritePeriod(int pwmnum, int value)
{
    char path[SIZE_VAL_PATH];
    char val_str[SIZE_VAL];
    int fd, byte;

    // TODO: Enter the period path.
    snprintf(path, SIZE_VAL_PATH, "/sys/class/pwm/pwmchip0/pwm%d/period", pwmnum);
    fd = open(path, O_WRONLY);
    if( fd < 0 ) {
        fprintf(stderr, "Failed to open in period!\n");
        return -1;
    }
    byte = snprintf(val_str, SIZE_VAL, "%d", value);

    if( write(fd, val_str, byte)<0 ) {
        fprintf(stderr, "Failed to write value in period!\n");
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}

static int PWMWriteDutyCycle(int pwmnum, int value)
{
    char path[SIZE_VAL_PATH];
    char val_str[SIZE_VAL];
    int fd, byte;

    // TODO: Enter the duty_cycle path.
    snprintf(path, SIZE_VAL_PATH, "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", pwmnum);
    fd = open(path, O_WRONLY);
    if( fd<0 ) {
        fprintf(stderr, "Failed to open in duty cycle!\n");
        return (-1);
    }
    byte = snprintf(val_str, SIZE_VAL, "%d", value);

    if( write(fd, val_str, byte)<0 ) {
        fprintf(stderr, "Failed to write value in duty cycle!\n");
        close(fd);
        return -1;
    }
    close(fd);

    return (0);
}

static int PWMEnable(int pwmnum)
{
    static const char enable_str[] = "1";
    char path[SIZE_DIR_PATH];
    int fd;

    // TODO: Enter the enable path.
    snprintf(path, SIZE_DIR_PATH, "/sys/class/pwm/pwmchip0/pwm%d/enable", pwmnum);
    fd = open(path, O_WRONLY);
    if( fd < 0 ) {
        fprintf(stderr, "Failed to open in enable!\n");
        return -1;
    }
    if( write(fd, enable_str, strlen(enable_str))<0 ) {
        fprintf(stderr, "Failed to enable pwm!\n");
        close(fd);
        return -1;
    }
    close(fd);

    return (0);
}


static void ErrorHandling(char* message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}