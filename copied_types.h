typedef struct {
    int id;
    int note;
    int volume;
} payload_t;

typedef enum {
    ID_BUTTONS = 0,
    ID_ULTRASONIC,
    ID_WATERLEVEL,
    ID_ACCEL
} socket_id_t;
