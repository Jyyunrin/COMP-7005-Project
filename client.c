#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "common.h"
#include <stdint.h>
#include <errno.h>
#include <inttypes.h> 


static void parse_args(int argc,char *argv[], char **ip_address, char **port_str, char **timeout_str, char **max_retries_str);
_Noreturn static void usage(const char *program_name, int exit_code, const char *message);
static void convert_address(const char *ip_address, struct sockaddr_storage *addr, socklen_t *addr_len);
static void parse_port(char *port_str, in_port_t *port);
static void parse_parameters(char *timeout_str, char *max_retries_str, int *timeout, int *max_retries);
static void fill_packet(packet_t *packet, int seq);
static int create_socket(int domain, int type, int protocol);
static void get_address_to_server(struct sockaddr_storage *addr, in_port_t port);


int main(int argc, char *argv[]) {

    packet_t               packet;
    char                   *ip_address;
    char                   *port_str;
    char                   *timeout_str;
    char                   *max_retries_str;
    struct sockaddr_storage addr;
    socklen_t               addr_len;
    in_port_t               port;
    int                     timeout;
    int                     max_retries;
    int                     sock_fd;
    int                     sequence_counter;
    ssize_t                 bytes_sent;

    ip_address = NULL;
    port_str = NULL;
    timeout_str = NULL;
    max_retries_str = NULL;
    sequence_counter = 0;

    parse_args(argc, argv, &ip_address, &port_str, &timeout_str, &max_retries_str);

    printf("ip address: %s\n", ip_address);
    printf("port_str: %s\n", port_str);
    printf("timeout_str: %s\n", timeout_str);
    printf("max_retries_str: %s\n", max_retries_str);

    convert_address(ip_address, &addr, addr_len);

    parse_port(port_str, &port);

    printf("Port: %d\n", port);

    parse_parameters(timeout_str, max_retries_str, &timeout, &max_retries);

    printf("Timeout: %d\n", timeout);
    printf("Max_Retries: %d\n", max_retries);

    sock_fd = create_socket(addr.ss_family, SOCK_DGRAM, 0);
    fill_packet(&packet, sequence_counter);

    printf("Sock: %d\n", sock_fd);
    printf("Packet Sequence: %d\n", packet.sequence);
    printf("Packet Payload: %s\n", packet.payload);

    bytes_sent = sendto(sock_fd, &packet, sizeof(packet), 0, (struct sockaddr *)&addr, addr_len);
    
}

static void parse_args(int argc,char *argv[], char **ip_address, char **port_str, char **timeout_str, char **max_retries_str) {
    int opt;

    opterr = 0;

    while((opt = getopt(argc, argv, "h")) != -1) {
        switch(opt){
            case 'h':
                usage(argv[0], EXIT_SUCCESS, NULL);
                break;
            case '?': {
                char message[UNKNOWN_OPTION_MESSAGE_LEN];
                snprintf(message, sizeof(message), "Unknown option '-%c'", optopt);
                usage(argv[0], EXIT_FAILURE, message);
                break;
            }
            default:
                usage(argv[0], EXIT_FAILURE, NULL);
        }
    }

    int required_args = 4;

    if(argc - optind < required_args) {
        usage(argv[0], EXIT_FAILURE, "Too few arguments.");
    } else if (argc - optind > required_args) {
        usage(argv[0], EXIT_FAILURE, "Too many arguments");
    }

    *ip_address = argv[optind];
    *port_str = argv[optind + 1];
    *timeout_str = argv[optind + 2];
    *max_retries_str = argv[optind + 3];
}

_Noreturn static void usage(const char *program_name, int exit_code, const char* message){
    if(message) {
        fprintf(stderr, "%s\n", message);
    }

    fprintf(stderr, "Usage: %s [-h] <target-ip> <target-port> <timeout> <max-retries>\n", program_name);
    fputs("Options:\n", stderr);
    fputs("  -h  Display this help message\n", stderr);
    exit(exit_code);
}

static void convert_address(const char *ip_address, struct sockaddr_storage *addr, socklen_t *addr_len) {

    memset(addr, 0, sizeof(*addr));

    if(inet_pton(AF_INET, ip_address, &(((struct sockaddr_in *)addr)->sin_addr)) == 1) {
        addr->ss_family = AF_INET;
        *addr_len = sizeof(struct sockaddr_in);

    } else if(inet_pton(AF_INET6, ip_address, &(((struct sockaddr_in6 *)addr)->sin6_addr)) == 1) {
        addr->ss_family = AF_INET6;
        *addr_len = sizeof(struct sockaddr_in6);

    } else {
        fprintf(stderr, "%s is not an IPv4 or IPv6 address\n", ip_address);
        exit(EXIT_FAILURE);
    }
}

static void parse_port(char *port_str, in_port_t *port) {
    char *endptr;
    uintmax_t parsed_port;

    if (port_str == NULL || *port_str == '\0') {
        fprintf(stderr, "Port cannot be empty\n");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    parsed_port = strtoumax(port_str, &endptr, BASE_TEN);

    if(errno != 0) {
        perror("Error parsing port number\n");
        exit(EXIT_FAILURE);
    }

    if(*endptr != '\0') {
        fprintf(stderr, "Invalid, non-numeric characters in port argument\n");
        exit(EXIT_FAILURE);
    }

    if((parsed_port > UINT16_MAX)) {
        fprintf(stderr, "Port value is out of range\n");
        exit(EXIT_FAILURE);
    }

    *port = (in_port_t) parsed_port;

}

static void parse_parameters(char *timeout_str, char *max_retries_str, int *timeout, int *max_retries) {

    char *endptr;
    uintmax_t parsed_timeout;
    uintmax_t parsed_max_retries;

    if (timeout_str == NULL || *timeout_str == '\0') {
        fprintf(stderr, "Timeout cannot be empty\n");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    parsed_timeout = strtoumax(timeout_str, &endptr, BASE_TEN);

    if(errno == ERANGE || parsed_timeout < 0 || parsed_timeout > MAX_TIMEOUT) {
        fprintf(stderr, "Timeout out of range: %s\n", timeout_str);
        exit(EXIT_FAILURE);
    }

    if (*endptr != '\0') {
        fprintf(stderr, "Invalid character in timeout arg: %s\n", timeout_str);
        exit(EXIT_FAILURE);
    }

    *timeout = (int)parsed_timeout;

    errno = 0;
    parsed_max_retries = strtoumax(max_retries_str, &endptr, BASE_TEN);

    if(errno == ERANGE || parsed_max_retries < 0 || parsed_max_retries > MAX_RETRIES) {
        fprintf(stderr, "Max retries out of range: %s\n", max_retries_str);
        exit(EXIT_FAILURE);
    }

    if (*endptr != '\0') {
        fprintf(stderr, "Invalid character in max-retries arg: %s\n", max_retries_str);
        exit(EXIT_FAILURE);
    }

    *max_retries = (int) parsed_max_retries;
}

static int create_socket(int domain, int type, int protocol){

    int sockfd = socket(domain, type, protocol);

    if(sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

static void fill_packet(packet_t *packet, int seq) {
    packet->sequence = seq;
    strncpy(packet->payload, "Hello Server", LINE_LEN);
    packet->payload[LINE_LEN - 1] = '\0';
}

static void get_address_to_server(struct sockaddr_storage *addr, in_port_t port) {

    if(addr->ss_family == AF_INET) {

        struct sockaddr_in *ipv4_addr;
        ipv4_addr = (struct sockaddr_in *)addr;
        ipv4_addr->sin_family = AF_INET;
        ipv4_addr->sin_port = htons(port);

    } else if (addr->ss_family == AF_INET6) {
        
        struct sockaddr_in6 *ipv6_addr;
        ipv6_addr = (struct sockaddr_in6 *)addr;
        ipv6_addr->sin6_family = AF_INET6;
        ipv6_addr->sin6_port = htons(port);

    }
}