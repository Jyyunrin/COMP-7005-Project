#include "common.h"

volatile sig_atomic_t exit_flag = 0;

void setup_signal_handler(void) {
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

void convert_address(const char *ip_address, struct sockaddr_storage *addr, socklen_t *addr_len) {

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

void parse_port(char *port_str, in_port_t *port) {
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

int create_socket(int domain, int type, int protocol){

    int sockfd = socket(domain, type, protocol);

    if(sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

void bind_socket(int sock_fd, struct sockaddr_storage *addr, in_port_t port) {

    char addr_str[INET6_ADDRSTRLEN];
    in_port_t net_port;
    socklen_t addr_len;
    void *vaddr;
    net_port = htons(port);

    if(addr->ss_family == AF_INET) {
        struct sockaddr_in *ipv4_addr;

        ipv4_addr = (struct sockaddr_in *) addr;
        ipv4_addr->sin_port = net_port;
        addr_len = sizeof(struct sockaddr_in);
        vaddr = (void *)&(((struct sockaddr_in *)addr)->sin_addr);
    } else if(addr->ss_family == AF_INET6) {
        struct sockaddr_in6 *ipv6_addr;
        
        ipv6_addr = (struct sockaddr_in6 *) addr;
        ipv6_addr->sin6_port = net_port;
        addr_len = sizeof(struct sockaddr_in6);
        vaddr = (void *)&(((struct sockaddr_in6 *)addr)->sin6_addr);
    } else {
        fprintf(stderr, "Not a valid address family: %d\n", addr->ss_family);
        exit(EXIT_FAILURE);
    }

    if(inet_ntop(addr->ss_family, vaddr, addr_str, sizeof(addr_str)) == NULL) {
        perror("inet_ntop failed");
        exit(EXIT_FAILURE);
    }

    printf("Binding to: %s:%u\n", addr_str, port);
    
    if (bind(sock_fd, (struct sockaddr *)addr, addr_len) == -1){
        const char *error_msg;
        error_msg = strerror(errno);
        fprintf(stderr, "Connection Error: %d: %s\n", errno, error_msg);
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    printf("Bound to socket %s:%u\n", addr_str, port);
}

void close_socket(int sock_fd) {

    printf("Closing client socket\n");
            
    if(close(sock_fd) == -1) {
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}