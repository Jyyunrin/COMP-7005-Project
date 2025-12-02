#include "log.h"
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>

static FILE *log_file = NULL;

static const char *source_to_string(log_source_t src) {
    switch (src) {
        case LOG_CLIENT: return "CLIENT";
        case LOG_PROXY:  return "PROXY";
        case LOG_SERVER: return "SERVER";
        default:         return "UNKNOWN";
    }
}

void log_init(const char *filename) {
    log_file = fopen(filename, "w");
    if (!log_file) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
}

void log_close() {
    if (log_file) fclose(log_file);
}

void log_packet(log_source_t src, const char *action, int sequence, const char *message, int new_line) {
    if (!log_file) return;

    time_t now = time(NULL);

    fprintf(
        log_file,
        "%ld %s %s Packet %d\n",
        now,
        source_to_string(src),
        action,
        sequence
    );

    if (new_line){
        fprintf(log_file, "\n");
    }

    fflush(log_file);

    fprintf(
        stderr,
        "%ld %s %s Packet %d\n",
        now,
        source_to_string(src),
        action,
        sequence
    );
    if (new_line){
        fprintf(stderr, "\n");
    }
    fflush(stderr);
}


void log_event(log_source_t src, const char *text, ...) {
    if (!log_file) return;

    time_t now = time(NULL);
    fprintf(log_file, "%ld %s ", now, source_to_string(src));

    va_list args;
    va_start(args, text);
    vfprintf(log_file, text, args);
    va_end(args);

    fprintf(log_file, "\n");
    fflush(log_file);

    fprintf(stderr, "%ld %s ", now, source_to_string(src));
    va_start(args, text);  // restart args for stderr
    vfprintf(stderr, text, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
}