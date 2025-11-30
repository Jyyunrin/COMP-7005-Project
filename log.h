#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef enum {
    LOG_CLIENT,
    LOG_PROXY,
    LOG_SERVER
} log_source_t;

void log_init(const char *filename);
void log_close();
void log_packet(log_source_t src, const char *action, int sequence, const char *message);
void log_event(log_source_t src, const char *text, ...);

#endif
