#define _DEFAULT_SOURCE
#include "common.h"


static void setup_signal_handler(void);
static void sigint_handler(int signum);
static void parse_args(int argc,char *argv[], char **ip_address, char **port_str);
_Noreturn static void usage(const char *program_name, int exit_code, const char *message);
static int create_socket(int domain, int type, int protocol);
static void bind_socket(int sock_fd, struct sockaddr_storage *addr, in_port_t port);
static int receive_packet(int sock_fd, packet_t *packet, struct sockaddr_storage *client_addr, socklen_t *client_addr_len);
static int handle_packet(packet_t *packet, int *sequence_counter);
static void send_ack(int sock_fd, int sequence_num, packet_t *ack_packet, struct sockaddr_storage *cliet_addr, socklen_t *client_addr_len);
static void close_socket(int sock_fd);


static volatile sig_atomic_t exit_flag = 0;

int main(int argc, char *argv[]) {

    packet_t                packet;
    packet_t                ack_packet;
    char                   *ip_address;
    char                   *port_str;
    struct sockaddr_storage addr;
    socklen_t               addr_len;
    in_port_t               port;
    int                     sock_fd;
    struct sockaddr_storage client_addr;
    socklen_t               client_addr_len;
    int                     sequence_counter;

    ip_address = NULL;
    port_str = NULL;
    sequence_counter = -1;
    client_addr_len = sizeof(client_addr);

    setup_signal_handler();
    parse_args(argc, argv, &ip_address, &port_str);

    printf("ip address: %s\n", ip_address);
    printf("port_str: %s\n", port_str);

    convert_address(ip_address, &addr, &addr_len);

    parse_port(port_str, &port);

    printf("Port: %d\n", port);

    sock_fd = create_socket(addr.ss_family, SOCK_DGRAM, 0);

    printf("Server address family: %d\n", addr.ss_family);

    bind_socket(sock_fd, &addr, port);

    while(!exit_flag) {
        
        if(receive_packet(sock_fd, &packet, &client_addr, &client_addr_len)){

            printf("Client address family: %d\n", client_addr.ss_family);
            printf("Client address length: %u\n", client_addr_len);
            
            if(handle_packet(&packet, &sequence_counter)) {
                send_ack(sock_fd, sequence_counter, &ack_packet, &client_addr, &client_addr_len);
            }
        }

    }

    close_socket(sock_fd);

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

static void parse_args(int argc,char *argv[], char **ip_address, char **port_str) {
    int opt;
    int option_index = 0;
    int ip_set = 0;
    int port_set = 0;

    static struct option long_options[] = {
        {"listen-ip", required_argument, 0, 1},
        {"listen-port", required_argument, 0, 2},
        {"help", required_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    opterr = 0;

    while((opt = getopt_long(argc, argv, "h", long_options, &option_index)) != -1) {
        switch(opt){
            case 1:
                if(ip_set){
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --listen-ip");
                }
                *ip_address = optarg;
                ip_set = 1;
                break;
            case 2:
                if(port_set){
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --listen-port");
                }
                *port_str = optarg;
                port_set = 1;
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

    if (!*ip_address || !*port_str) {
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
    fputs("  --listen-ip <ip>         IP address to bind to\n", stderr);
    fputs("  --listen-port <port>     UDP port to listen on\n", stderr);
    fputs("  -h, --help               Display this help message\n", stderr);
    exit(exit_code);
}

static int create_socket(int domain, int type, int protocol){

    int sockfd = socket(domain, type, protocol);

    if(sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

static void bind_socket(int sock_fd, struct sockaddr_storage *addr, in_port_t port) {

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

static int receive_packet(int sock_fd, packet_t *packet, struct sockaddr_storage *client_addr, socklen_t *client_addr_len) {

    printf("Listening for connections...\n");
    
    ssize_t bytes_received = recvfrom(sock_fd, packet, sizeof(*packet), 0, (struct sockaddr *)client_addr, client_addr_len);

    if(bytes_received < 0) {
        perror("Error with recvfrom");
        close_socket(sock_fd);
        exit(EXIT_FAILURE);
    }

    if((size_t)bytes_received != sizeof(*packet)) {
        fprintf(stderr, "Received incomplete or malformed packet (%zd bytes, expected %zu)\n", bytes_received, sizeof(*packet));
        return 0;
    }

    return 1;
}

static int handle_packet(packet_t *packet, int *sequence_counter) {
    
    if(packet->sequence < *sequence_counter) {
        return 0;
    } else if (packet->sequence == *sequence_counter) {
        return 1;
    } else {
        printf("Packet %d: %s\n", packet->sequence, packet->payload);
        (*sequence_counter)++;
        return 1;
    }
}

static void send_ack(int sock_fd, int sequence_num, packet_t *ack_packet, struct sockaddr_storage *client_addr, socklen_t *client_addr_len) {
    ack_packet->sequence = sequence_num;
    strncpy(ack_packet->payload, "Acknowledged", LINE_LEN);
    ack_packet->payload[LINE_LEN - 1] = '\0';

    ssize_t bytes_sent = sendto(sock_fd, ack_packet, sizeof(*ack_packet), 0, (struct sockaddr *)client_addr, *client_addr_len);

    if(bytes_sent == -1) {
        perror("Error sending packet to client");
        exit(EXIT_FAILURE);
    }
}

static void close_socket(int sock_fd) {

    printf("Closing client socket\n");
            
    if(close(sock_fd) == -1) {
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}
