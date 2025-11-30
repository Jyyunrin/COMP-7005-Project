#include "common.h"

static void parse_args(int argc,char *argv[], char **ip_address, char **port_str, char **timeout_str, char **max_retries_str);
_Noreturn static void usage(const char *program_name, int exit_code, const char *message);
static void parse_parameters(char *timeout_str, char *max_retries_str, int *timeout, int *max_retries);
static int fill_packet(packet_t *packet, int seq);
static int receive_acknowledgement(int sock_fd, packet_t *ack_packet, struct sockaddr *addr, socklen_t *addr_len, int *current_sequence);

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

static int receive_acknowledgement(int sock_fd, packet_t *ack_packet, struct sockaddr *addr, socklen_t *addr_len, int *current_sequence) {

    ssize_t bytes_received = recvfrom(sock_fd, ack_packet, sizeof(*ack_packet), 0, addr, addr_len);

    if (bytes_received >= 0) {
        if(ack_packet->sequence == *current_sequence) {

            (*current_sequence)++;
            printf("%s received for packet %d\n", ack_packet->payload, ack_packet->sequence);
            return 1;

        } else {
            
            printf("Old sequence: %d ack received from server, dropping!", ack_packet->sequence);
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