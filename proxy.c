
#include "common.h"
#include "log.h"
#include <time.h>
#include <sys/time.h>


typedef struct delayed_packet {
    packet_t packet;
    struct timeval send_time;
    struct delayed_packet *next;
    
} delayed_packet_t;

static void init_random();
static void parse_args(int argc,char *argv[], char **listen_ip_str, char **listen_port_str, char **target_ip_str, char **target_port_str,
                    char **client_drop_str, char **server_drop_str, char **client_delay_str, char **server_delay_str, char **client_delay_min_time_str,
                    char **client_delay_max_time_str, char **server_delay_min_time_str, char **server_delay_max_time_str);
_Noreturn static void usage(const char *program_name, int exit_code, const char *message);
static int parse_int_param(const char *str, const char *name);
static int determine_noise(const int drop_chance, const int delay_chance);
static int determine_delay(const int min_time, const int max_time);
static void add_to_delay_queue(delayed_packet_t **queue, delayed_packet_t *new_node);
static void process_delay_queue(int sock_fd, delayed_packet_t **queue, struct sockaddr *dest_addr, socklen_t addr_len);

int main(int argc, char *argv[]) {
    
    char                   *listen_ip_str;
    char                   *listen_port_str;
    char                   *target_ip_str;
    char                   *target_port_str;
    char                   *client_drop_str;
    char                   *server_drop_str;
    char                   *client_delay_str;
    char                   *server_delay_str;
    char                   *client_delay_min_time_str;
    char                   *client_delay_max_time_str;
    char                   *server_delay_min_time_str;
    char                   *server_delay_max_time_str;
    struct sockaddr_storage listen_ip;
    struct sockaddr_storage target_ip;
    socklen_t               listen_ip_len;
    socklen_t               target_ip_len;
    in_port_t               listen_port;
    in_port_t               target_port;
    int                     client_drop;
    int                     server_drop;
    int                     client_delay;
    int                     server_delay;
    int                     client_delay_min;
    int                     client_delay_max;
    int                     server_delay_min;
    int                     server_delay_max;
    int                     client_sock_fd;
    int                     server_sock_fd;
    struct timeval          socket_timevalue;
    struct sockaddr_storage client_addr;
    socklen_t               client_addr_len;
    delayed_packet_t        *client_to_server_queue;
    delayed_packet_t        *server_to_client_queue;

    listen_ip_str = NULL;
    listen_port_str = NULL;
    target_ip_str = NULL;
    target_port_str = NULL;
    client_drop_str = NULL;
    server_drop_str = NULL;
    client_delay_str = NULL;
    server_delay_str = NULL;
    client_delay_min_time_str = NULL;
    client_delay_max_time_str = NULL;
    server_delay_min_time_str = NULL;
    server_delay_max_time_str = NULL;
    socket_timevalue.tv_sec = 2;
    socket_timevalue.tv_usec = PROXY_TIMEOUT_TIME;
    client_addr_len = sizeof(client_addr);
    client_to_server_queue = NULL;
    server_to_client_queue = NULL;

    init_random();
    setup_signal_handler();
    parse_args(argc, argv, &listen_ip_str, &listen_port_str, &target_ip_str, &target_port_str, &client_drop_str, &server_drop_str, &client_delay_str,
            &server_delay_str, &client_delay_min_time_str, &client_delay_max_time_str, &server_delay_min_time_str, &server_delay_max_time_str);

    printf("listen_ip: %s\n", listen_ip_str);
    printf("listen_port: %s\n", listen_port_str);
    printf("target_ip: %s\n", target_ip_str);
    printf("target_port: %s\n", target_port_str);
    printf("client_drop: %s\n", client_drop_str);
    printf("server_drop: %s\n", server_drop_str);
    printf("client_delay: %s\n", client_delay_str);
    printf("server_delay: %s\n", server_delay_str);
    printf("client_delay_min_time: %s\n", client_delay_min_time_str);
    printf("client_delay_max_time: %s\n", client_delay_max_time_str);
    printf("server_delay_min_time: %s\n", server_delay_min_time_str);
    printf("server_delay_max_time: %s\n", server_delay_max_time_str);

    convert_address(listen_ip_str, &listen_ip, &listen_ip_len);
    convert_address(target_ip_str, &target_ip, &target_ip_len);

    parse_port(listen_port_str, &listen_port);
    parse_port(target_port_str, &target_port);

    printf("Listen Port: %d\n", listen_port);
    printf("Target Port: %d\n", target_port);

    client_drop         = parse_int_param(client_drop_str, "client-drop");
    server_drop         = parse_int_param(server_drop_str, "server-drop");
    client_delay        = parse_int_param(client_delay_str, "client-delay");
    server_delay        = parse_int_param(server_delay_str, "server-delay");
    client_delay_min    = parse_int_param(client_delay_min_time_str, "client-delay-min");
    client_delay_max    = parse_int_param(client_delay_max_time_str, "client-delay-max");
    server_delay_min    = parse_int_param(server_delay_min_time_str, "server-delay-min");
    server_delay_max    = parse_int_param(server_delay_max_time_str, "server-delay-max");

    if(client_delay_min > client_delay_max || server_delay_min > server_delay_max) {
        fprintf(stderr, "Delay min time cannot be greater than delay max time");
        exit(EXIT_FAILURE);
    }

    client_sock_fd = create_socket(listen_ip.ss_family, SOCK_DGRAM, 0);
    server_sock_fd = create_socket(target_ip.ss_family, SOCK_DGRAM, 0);

    printf("Client Socket: %d\n", client_sock_fd);
    printf("Server Socket: %d\n", server_sock_fd);

    // Bind listening socket to listening ip:port
    bind_socket(client_sock_fd, &listen_ip, listen_port);

    // Get server ip address
    get_address_to_server(&target_ip, target_port);

    if (setsockopt(client_sock_fd, SOL_SOCKET, SO_RCVTIMEO, &socket_timevalue, sizeof(socket_timevalue)) < 0) {
        perror("setsockopt client_sock_fd SO_RCVTIMEO");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_sock_fd, SOL_SOCKET, SO_RCVTIMEO, &socket_timevalue, sizeof(socket_timevalue)) < 0) {
        perror("setsockopt server_sock_fd SO_RCVTIMEO");
        exit(EXIT_FAILURE);
    }

    while (!exit_flag) {

        packet_t packet;
        packet_t packet_server;
        ssize_t n = recvfrom(client_sock_fd, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &client_addr_len);

        if (n > 0) {
            int noise = determine_noise(client_drop, client_delay);

            if(!noise) {
                printf("Sending packet %d to server\n", packet.sequence);
                send_packet(server_sock_fd, &packet, (struct sockaddr *)&target_ip, listen_ip_len);
            } else if (noise == 2) {
                printf("Delaying packet %d to server\n", packet.sequence);
                struct timeval now;
                struct timeval send_time;
                gettimeofday(&now, NULL);

                int delay_time = determine_delay(client_delay_min, client_delay_max);

                send_time.tv_sec = now.tv_sec + (delay_time/1000);
                send_time.tv_usec = now.tv_usec + (delay_time % 1000) * 1000;

                delayed_packet_t *delayed_packet = malloc(sizeof(delayed_packet_t));
                if (!delayed_packet) {
                    perror("malloc failed");
                    exit(EXIT_FAILURE);
                }

                delayed_packet->packet = packet;
                delayed_packet->send_time = send_time;
                delayed_packet->next = NULL;

                add_to_delay_queue(&client_to_server_queue, delayed_packet);

            } else {
                printf("Dropping packet %d to server\n", packet.sequence);
            }

        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("recvfrom client");
            break;
        }

        n = recvfrom(server_sock_fd, &packet_server, sizeof(packet_server), 0, (struct sockaddr *)&target_ip, &target_ip_len);

        if (n > 0) {
            int noise = determine_noise(server_drop, server_delay);

            if(!noise) {
                printf("Sending packet %d to client\n", packet_server.sequence);
                send_packet(client_sock_fd, &packet_server, (struct sockaddr *)&client_addr, client_addr_len);
            } else if (noise == 2) {
                printf("Delaying packet %d to client\n", packet.sequence);
                struct timeval now;
                struct timeval send_time;
                gettimeofday(&now, NULL);

                int delay_time = determine_delay(server_delay, server_delay_max);

                send_time.tv_sec = now.tv_sec + (delay_time/1000);
                send_time.tv_usec = now.tv_usec + (delay_time % 1000) * 1000;

                delayed_packet_t *delayed_packet = malloc(sizeof(delayed_packet_t));
                if (!delayed_packet) {
                    perror("malloc failed");
                    exit(EXIT_FAILURE);
                }

                delayed_packet->packet = packet;
                delayed_packet->send_time = send_time;
                delayed_packet->next = NULL;

                add_to_delay_queue(&server_to_client_queue, delayed_packet);
            } else {
                printf("Dropping packet %d to client\n", packet.sequence);
            }
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("recvfrom server");
            break;
        }


        process_delay_queue(server_sock_fd, &client_to_server_queue, (struct sockaddr *)&target_ip, target_ip_len);
        process_delay_queue(client_sock_fd, &server_to_client_queue, (struct sockaddr *)&client_addr, client_addr_len);

    }


    close_socket(client_sock_fd);
    close_socket(server_sock_fd);

}

static void init_random() {
    srand((unsigned int)time(NULL));
}


static void parse_args(int argc,char *argv[], char **listen_ip_str, char **listen_port_str, char **target_ip_str, char **target_port_str,
                    char **client_drop_str, char **server_drop_str, char **client_delay_str, char **server_delay_str, char **client_delay_min_time_str,
                    char **client_delay_max_time_str, char **server_delay_min_time_str, char **server_delay_max_time_str){

    int opt;
    int option_index = 0;
    int listen_ip_set = 0;
    int listen_port_set = 0;
    int target_ip_set = 0;
    int target_port_set = 0;
    int client_drop_set = 0;
    int server_drop_set = 0;
    int client_delay_set = 0;
    int server_delay_set = 0;
    int client_delay_min_set = 0;
    int client_delay_max_set = 0;
    int server_delay_min_set = 0;
    int server_delay_max_set = 0;
    int log_set = 0;

    static struct option long_options[] = {
        {"listen-ip", required_argument, 0, 1},
        {"listen-port", required_argument, 0, 2},
        {"target-ip", required_argument, 0, 3},
        {"target-port", required_argument, 0, 4},
        {"client-drop", required_argument, 0, 5},
        {"server-drop", required_argument, 0, 6},
        {"client-delay", required_argument, 0, 7},
        {"server-delay", required_argument, 0, 8},
        {"client-delay-time-min", required_argument, 0, 9},
        {"client-delay-time-max", required_argument, 0, 10},
        {"server-delay-time-min", required_argument, 0, 11},
        {"server-delay-time-max", required_argument, 0, 12},
        {"log", no_argument, 0, 'l'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    opterr = 0;

    while((opt = getopt_long(argc, argv, "h", long_options, &option_index)) != -1) {
        switch(opt){
            case 1:
                if(listen_ip_set){
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --target-ip");
                }
                *listen_ip_str = optarg;
                listen_ip_set = 1;
                break;

            case 2:
                if (listen_port_set)
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --listen-port");
                *listen_port_str = optarg;
                listen_port_set = 1;
                break;

            case 3:
                if (target_ip_set)
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --target-ip");
                *target_ip_str = optarg;
                target_ip_set = 1;
                break;

            case 4:
                if (target_port_set)
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --target-port");
                *target_port_str = optarg;
                target_port_set = 1;
                break;

            case 5:
                if (client_drop_set)
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --client-drop");
                *client_drop_str = optarg;
                client_drop_set = 1;
                break;

            case 6:
                if (server_drop_set)
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --server-drop");
                *server_drop_str = optarg;
                server_drop_set = 1;
                break;

            case 7:
                if (client_delay_set)
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --client-delay");
                *client_delay_str = optarg;
                client_delay_set = 1;
                break;

            case 8:
                if (server_delay_set)
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --server-delay");
                *server_delay_str = optarg;
                server_delay_set = 1;
                break;

            case 9:
                if (client_delay_min_set)
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --client-delay-time-min");
                *client_delay_min_time_str = optarg;
                client_delay_min_set = 1;
                break;

            case 10:
                if (client_delay_max_set)
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --client-delay-time-max");
                *client_delay_max_time_str = optarg;
                client_delay_max_set = 1;
                break;

            case 11:
                if (server_delay_min_set)
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --server-delay-time-min");
                *server_delay_min_time_str = optarg;
                server_delay_min_set = 1;
                break;

            case 12:
                if (server_delay_max_set)
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --server-delay-time-max");
                *server_delay_max_time_str = optarg;
                server_delay_max_set = 1;
                break;
            case 'l':
                if(log_set) {
                    usage(argv[0], EXIT_FAILURE, "Duplicate option: --log/-l");
                }
                log_init("proxy_log.txt");
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

    if (!listen_ip_set || !listen_port_set ||
        !target_ip_set || !target_port_set ||
        !client_drop_set || !server_drop_set ||
        !client_delay_set || !server_delay_set ||
        !client_delay_min_set || !client_delay_max_set ||
        !server_delay_min_set || !server_delay_max_set)
    {
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
    fputs("  --listen-ip <ip>                 IP address to bind for client packets\n", stderr);
    fputs("  --listen-port <port>             Port to listen on for client packets\n", stderr);

    fputs("  --target-ip <ip>                 Server IP address to forward packets to\n", stderr);
    fputs("  --target-port <port>             Server port number\n", stderr);

    fputs("  --client-drop <percent>          Drop chance (%) for packets from client\n", stderr);
    fputs("  --server-drop <percent>          Drop chance (%) for packets from server\n", stderr);

    fputs("  --client-delay <percent>         Delay chance (%) for packets from client\n", stderr);
    fputs("  --server-delay <percent>         Delay chance (%) for packets from server\n", stderr);

    fputs("  --client-delay-time-min <ms>     Minimum delay time (ms) for client packets\n", stderr);
    fputs("  --client-delay-time-max <ms>     Maximum delay time (ms) for client packets\n", stderr);

    fputs("  --server-delay-time-min <ms>     Minimum delay time (ms) for server packets\n", stderr);
    fputs("  --server-delay-time-max <ms>     Maximum delay time (ms) for server packets\n", stderr);

    fputs("  -l, --log                        Enables logging\n", stderr);
    fputs("  -h, --help                       Display this help message\n", stderr);
    exit(exit_code);
}

static int parse_int_param(const char *str, const char *name) {
    char *endptr;
    long value;

    if(!str) {
        fprintf(stderr, "Missing value for %s\n", name);
        exit(EXIT_FAILURE);
    }

    errno = 0;
    value = strtol(str, &endptr, 10);

    if(errno != 0) {
        perror("strtol");
        exit(EXIT_FAILURE);
    }

    if(*endptr != '\0') {
        fprintf(stderr, "Invalid non-numeric characters in %s: %s\n", name, str);
    }

    if(value < MIN_INT_PARSE || value > MAX_INT_PARSE) {
        fprintf(stderr, "%s value is out of range", name);
        exit(EXIT_FAILURE);
    }

    return (int)value;

}

static int determine_noise(const int drop_chance, const int delay_chance) {
    int percent = rand() % 100 + 1;

    // Drop
    if(percent <= drop_chance) {
        return 1;
    }

    percent = rand() % 100 + 1;

    // Delay
    if (percent <= delay_chance) {
        return 2;
    }

    // Send normally
    return 0;
}

static int determine_delay(const int min_time, const int max_time) {
    return min_time + rand() % (max_time - min_time + 1);
}

static void add_to_delay_queue(delayed_packet_t **queue, delayed_packet_t *new_node) {

    if(*queue == NULL) {
        *queue = new_node;
    } else {
        delayed_packet_t *current = *queue;
        while(current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
}

static void process_delay_queue(int sock_fd, delayed_packet_t **queue, struct sockaddr *dest_addr, socklen_t addr_len) {
    struct timeval now;
    gettimeofday(&now, NULL);

    while (*queue) {
        delayed_packet_t *delayed_packet = *queue;

        if (timercmp(&now, &delayed_packet->send_time, >=)) {
            printf("Sending delayed packet %d\n", delayed_packet->packet.sequence);
            send_packet(sock_fd, &delayed_packet->packet, dest_addr, addr_len);
            *queue = delayed_packet->next;
            free(delayed_packet);
        } else {
            break;
        }
    }
}