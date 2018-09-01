#ifndef SERVIDOR_H_
#define SERVIDOR_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include "../commons/log.h"

	uint32_t build_server(uint32_t puerto, t_log *archivoLog);
	uint32_t connect_server(char *dest_ip, uint32_t port, t_log *archivoLog);
	uint32_t receive_int(uint32_t socket, uint32_t *buf);
	uint32_t receive_string(uint32_t socket, char **buf);
	uint32_t send_string(uint32_t socket, char *buf);
	uint32_t send_int(uint32_t socket, uint32_t buf);
#endif
