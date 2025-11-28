#include "common.h"

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