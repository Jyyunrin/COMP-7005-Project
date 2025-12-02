#ifndef COMMON_H
#define COMMON_H
#define _DEFAULT_SOURCE
#define LINE_LEN 1024
#define UNKNOWN_OPTION_MESSAGE_LEN 24
#define BASE_TEN 10
#define MAX_TIMEOUT 100
#define MAX_RETRIES 100
#define MIN_INT_PARSE 0
#define MAX_INT_PARSE 100000
#define PROXY_TIMEOUT_S 2
#define PROXY_TIMEOUT_US 100000

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
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

extern volatile sig_atomic_t exit_flag;
void setup_signal_handler(void);
static void sigint_handler(int signum);
void convert_address(const char *ip_address, struct sockaddr_storage *addr, socklen_t *addr_len);
void parse_port(char *port_str, in_port_t *port);
int create_socket(int domain, int type, int protocol);
void bind_socket(int sock_fd, struct sockaddr_storage *addr, in_port_t port);
void get_address_to_server(struct sockaddr_storage *addr, in_port_t port);
void send_packet(int sock_fd, packet_t *packet, struct sockaddr *addr, socklen_t addr_len);
void close_socket(int sock_fd);



#endif