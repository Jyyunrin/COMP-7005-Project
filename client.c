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
#include <signal.h>
#include <getopt.h>


static void setup_signal_handler(void);
static void sigint_handler(int signum);
static void parse_args(int argc,char *argv[], char **ip_address, char **port_str, char **timeout_str, char **max_retries_str);
_Noreturn static void usage(const char *program_name, int exit_code, const char *message);
static void convert_address(const char *ip_address, struct sockaddr_storage *addr, socklen_t *addr_len);
static void parse_port(char *port_str, in_port_t *port);
static void parse_parameters(char *timeout_str, char *max_retries_str, int *timeout, int *max_retries);
static int fill_packet(packet_t *packet, int seq);
static int create_socket(int domain, int type, int protocol);
static void get_address_to_server(struct sockaddr_storage *addr, in_port_t port);
static void send_packet(int sock_fd, packet_t *packet, struct sockaddr *addr, socklen_t addr_len);
static int receive_acknowledgement(int sock_fd, packet_t *ack_packet, struct sockaddr *addr, socklen_t *addr_len, int *current_sequence);
static void close_socket(int sock_fd);

static volatile sig_atomic_t exit_flag = 0;

int main(int argc, char *argv[]) {

    packet_t               packet;
    packet_t               ack_packet;
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
    char                    message[LINE_LEN];
    struct timeval          socket_timevalue;
    int                     succesfully_received;


    ip_address = NULL;
    port_str = NULL;
    timeout_str = NULL;
    max_retries_str = NULL;
    sequence_counter = 0;
    succesfully_received = 0;

    setup_signal_handler();
    parse_args(argc, argv, &ip_address, &port_str, &timeout_str, &max_retries_str);

    printf("ip address: %s\n", ip_address);
    printf("port_str: %s\n", port_str);
    printf("timeout_str: %s\n", timeout_str);
    printf("max_retries_str: %s\n", max_retries_str);

    convert_address(ip_address, &addr, &addr_len);

    parse_port(port_str, &port);

    printf("Port: %d\n", port);

    parse_parameters(timeout_str, max_retries_str, &timeout, &max_retries);

    printf("Timeout: %d\n", timeout);
    printf("Max_Retries: %d\n", max_retries);

    sock_fd = create_socket(addr.ss_family, SOCK_DGRAM, 0);
    get_address_to_server(&addr, port);

    socket_timevalue.tv_sec = timeout;
    socket_timevalue.tv_usec = 0;

    if(setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &socket_timevalue, sizeof(socket_timevalue)) < 0){
        perror("Setting socket receive timeout failed");
        exit(EXIT_FAILURE);
    }

    while (!exit_flag) {
        succesfully_received = 0;

        if(!fill_packet(&packet, sequence_counter)) {
            exit_flag = 1;
            continue;
        };

        printf("Sock: %d\n", sock_fd);
        printf("Packet Sequence: %d\n", packet.sequence);
        printf("Packet Payload: %s\n", packet.payload);

        for (int attempt = 0; attempt <= max_retries; attempt++) {

            printf("Attempting attempt %d\n", attempt);

            send_packet(sock_fd, &packet, (struct sockaddr *)&addr, addr_len);

            if(receive_acknowledgement(sock_fd, &ack_packet, (struct sockaddr *)&addr, &addr_len, &sequence_counter)){
                succesfully_received = 1;
                break;
            } else {
                continue;
            }
        }

        if(!succesfully_received) {
            fprintf(stderr, "Failed to receive ACK for packet %d after %d attempts\n", packet.sequence, max_retries + 1);
            sequence_counter++;
        }
    }

    close_socket(sock_fd);
    return EXIT_SUCCESS;
}

static void setup_signal_handler(void) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if(sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Signal handler setup failed");
        exit(EXIT_FAILURE);
    }
}

static void sigint_handler(int signum) {

    exit_flag = 1;
}

static void parse_args(int argc,char *argv[], char **ip_address, char **port_str, char **timeout_str, char **max_retries_str) {
    int opt;
    int option_index = 0;
    int ip_set = 0;
    int port_set = 0;
    int timeout_set = 0;
    int retries_set = 0;

    static struct option long_options[] = {
        {"target-ip", required_argument, 0, 1},
        {"target-port", required_argument, 0, 2},
        {"timeout", required_argument, 0, 3},
        {"max-retries", required_argument, 0, 4},
        {"help", required_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    opterr = 0;

    while((opt = getopt_long(argc, argv, "h", long_options, &option_index)) != -1) {
        switch(opt){
            case 1:
                if(ip_set){
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --target-ip");
                }
                *ip_address = optarg;
                ip_set = 1;
                break;
            case 2:
                if(port_set){
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --target-port");
                }
                *port_str = optarg;
                port_set = 1;
                break;
            case 3:
                if(timeout_set){
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --timeout");
                }
                *timeout_str = optarg;
                timeout_set = 1;
                break;
            case 4:
                if(retries_set){
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --max-retries");
                }
                *max_retries_str = optarg;
                retries_set = 1;
                break;
            case 'h':
                usage(argv[0], EXIT_SUCCESS, NULL);
                break;
            case '?': {
                char message[UNKNOWN_OPTION_MESSAGE_LEN];
                snprintf(message, sizeof(message), "Unknown option");
                usage(argv[0], EXIT_FAILURE, message);
                break;
            }
            default:
                usage(argv[0], EXIT_FAILURE, NULL);
        }
    }

    if (!*ip_address || !*port_str || !*timeout_str || !*max_retries_str) {
        usage(argv[0], EXIT_FAILURE, "Missing required arguments.");
    }

    if (optind < argc) {
        usage(argv[0], EXIT_FAILURE, "Unexpected extra arguments.");
    }
}

_Noreturn static void usage(const char *program_name, int exit_code, const char* message){
    if(message) {
        fprintf(stderr, "%s\n", message);
    }

    fprintf(stderr, "Usage: %s [options]\n", program_name);
    fputs("Options:\n", stderr);
    fputs("  --target-ip <ip>         IP address to bind to\n", stderr);
    fputs("  --target-port <port>     UDP port to listen on\n", stderr);
    fputs("  --timeout <seconds>      Timeout for client messaging\n", stderr);
    fputs("  --max-retries <number>   Maximum resend attempts\n", stderr);
    fputs("  -h, --help               Display this help message\n", stderr);
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

static int fill_packet(packet_t *packet, int seq) {

    char message[LINE_LEN];

    printf("Please type a message to send to the server: ");
    fflush(stdout);

    if(fgets(message, sizeof(message), stdin) == NULL) {
        printf("\n");
        return 0;
    }

    message[strcspn(message, "\n")] = '\0';
    
    packet->sequence = seq;
    strncpy(packet->payload, message, LINE_LEN);
    packet->payload[LINE_LEN - 1] = '\0';

    return 1;
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

static void send_packet(int sock_fd, packet_t *packet, struct sockaddr *addr, socklen_t addr_len) {

    ssize_t bytes_sent = sendto(sock_fd, packet, sizeof(*packet), 0, addr, addr_len);

    if(bytes_sent == -1) {
        perror("Error sending packet to server");
        exit(EXIT_FAILURE);
    }
}

static int receive_acknowledgement(int sock_fd, packet_t *ack_packet, struct sockaddr *addr, socklen_t *addr_len, int *current_sequence) {

    ssize_t bytes_received = recvfrom(sock_fd, ack_packet, sizeof(*ack_packet), 0, addr, addr_len);

    if (bytes_received >= 0) {
        if(ack_packet->sequence == *current_sequence) {
            (*current_sequence)++;
            printf("%s received for packet %d\n", ack_packet->payload, ack_packet->sequence);
                return 1;
            } else {
                return 0;
            }
    } else {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        } else {
            perror("Error with Recvfrom");
            exit(EXIT_FAILURE);
        }
    }

}

static void close_socket(int sock_fd) {

    printf("Closing client socket\n");
            
    if(close(sock_fd) == -1) {
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}