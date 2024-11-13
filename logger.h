#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>

void log_timestamp(const char *message);
void log_command(const char *command, uint32_t hash, const char *name, uint32_t salary);

#endif // LOGGER_H
