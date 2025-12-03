#include "common.h"
#include "log.h"

static void parse_args(int argc,char *argv[], char **ip_address, char **port_str);
_Noreturn static void usage(const char *program_name, int exit_code, const char *message);
static int receive_packet(int sock_fd, packet_t *packet, struct sockaddr_storage *client_addr, socklen_t *client_addr_len);
static int handle_packet(packet_t *packet, int *sequence_counter);
static void send_ack(int sock_fd, int sequence_num, packet_t *ack_packet, struct sockaddr_storage *cliet_addr, socklen_t *client_addr_len);

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

    convert_address(ip_address, &addr, &addr_len);

    parse_port(port_str, &port);

    sock_fd = create_socket(addr.ss_family, SOCK_DGRAM, 0);

    bind_socket(sock_fd, &addr, port);

    while(!exit_flag) {
        
        if(receive_packet(sock_fd, &packet, &client_addr, &client_addr_len)){

            log_packet(LOG_SERVER, "Received", packet.sequence, packet.payload, 0);


            if(handle_packet(&packet, &sequence_counter)) {
                send_ack(sock_fd, sequence_counter, &ack_packet, &client_addr, &client_addr_len);
            }
        }

    }

    close_socket(sock_fd);
    log_close();
    exit(EXIT_SUCCESS);

}

static void parse_args(int argc,char *argv[], char **ip_address, char **port_str) {
    int opt;
    int option_index = 0;
    int ip_set = 0;
    int port_set = 0;
    int log_set = 0;

    static struct option long_options[] = {
        {"listen-ip", required_argument, 0, 1},
        {"listen-port", required_argument, 0, 2},
        {"log", no_argument, 0, 'l'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    opterr = 0;

    while((opt = getopt_long(argc, argv, "hl", long_options, &option_index)) != -1) {
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
            case 'l':
                if(log_set) {
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --log/-l");
                }
                log_init("server_log.txt");
                log_set = 1;
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
    fputs("  -l, --log                Enables logging\n", stderr);
    fputs("  -h, --help               Display this help message\n", stderr);
    exit(exit_code);
}

static int receive_packet(int sock_fd, packet_t *packet, struct sockaddr_storage *client_addr, socklen_t *client_addr_len) {
    
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
        log_packet(LOG_SERVER, "Ignored", packet->sequence, packet->payload, 0);
        return 0;
    } else if (packet->sequence == *sequence_counter) {
        return 1;
    } else {
        log_event(LOG_SERVER, "Message: %s from Packet %d", packet->payload, packet->sequence);
        (*sequence_counter) = packet->sequence;
        return 1;
    }
}

static void send_ack(int sock_fd, int sequence_num, packet_t *ack_packet, struct sockaddr_storage *client_addr, socklen_t *client_addr_len) {
    ack_packet->sequence = sequence_num;
    strncpy(ack_packet->payload, "Acknowledged", LINE_LEN);
    ack_packet->payload[LINE_LEN - 1] = '\0';

    ssize_t bytes_sent = sendto(sock_fd, ack_packet, sizeof(*ack_packet), 0, (struct sockaddr *)client_addr, *client_addr_len);
    log_packet(LOG_SERVER, "Sent", ack_packet->sequence, ack_packet->payload, 1);


    if(bytes_sent == -1) {
        perror("Error sending packet to client");
        exit(EXIT_FAILURE);
    }
}