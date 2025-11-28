
#define _DEFAULT_SOURCE
#include "common.h"

static void setup_signal_handler(void);
static void sigint_handler(int signum);
static void parse_args(int argc,char *argv[], char **listen_ip_str, char **listen_port_str, char **target_ip_str, char **target_port_str,
                    char **client_drop_str, char **server_drop_str, char **client_delay_str, char **server_delay_str, char **client_delay_min_time_str,
                    char **client_delay_max_time_str, char **server_delay_min_time_str, char **server_delay_max_time_str);
_Noreturn static void usage(const char *program_name, int exit_code, const char *message);

static volatile sig_atomic_t exit_flag = 0;

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
        {"help", required_argument, 0, 'h'},
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
    fputs("  -h, --help                       Display this help message\n", stderr);
    exit(exit_code);
}