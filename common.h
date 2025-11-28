#ifndef COMMON_H
#define COMMON_H
#define LINE_LEN 1024
#define UNKNOWN_OPTION_MESSAGE_LEN 24
#define BASE_TEN 10
#define MAX_TIMEOUT 100
#define MAX_RETRIES 100

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include "common.h"
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h> 
#include <signal.h>
#include <getopt.h>

typedef struct packet {
    int sequence;
    char payload[LINE_LEN];
} packet_t;

void convert_address(const char *ip_address, struct sockaddr_storage *addr, socklen_t *addr_len);
void parse_port(char *port_str, in_port_t *port);

#endif