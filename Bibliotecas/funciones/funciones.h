#ifndef FUNCIONES_H_
#define FUNCIONES_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include "../commons/log.h"
#include "../commons/string.h"

t_log *init_log(char *file, char *program_name, bool is_active_console, t_log_level level);
bool str_eq(const char *s1, const char *s2);
void print_c(t_log *log_file, char *message_template, ...);
bool isDirectoryExists(const char *path);
bool isFileExists(const char *path);

#endif
