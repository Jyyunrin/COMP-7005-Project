#ifndef COMMON_H
#define COMMON_H
#define LINE_LEN 1024
#define UNKNOWN_OPTION_MESSAGE_LEN 24
#define BASE_TEN 10
#define MAX_TIMEOUT 100
#define MAX_RETRIES 100


typedef struct packet {
    int sequence;
    char payload[LINE_LEN];
} packet_t;

#endif