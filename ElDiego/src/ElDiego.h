/*
 * ElDiego.h
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */

#ifndef SRC_ELDIEGO_H_
#define SRC_ELDIEGO_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "funciones/funciones.h"
#include "commons/config.h"
#include "servidor/servidor.h"

// constantes
char *PATH_LOG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Logs/logDiego.txt";
char *PATH_CONFIG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/ElDiego/config.txt";

// estructuras
typedef struct {
	uint32_t PUERTO;
	char *IP_SAFA;
	uint32_t PUERTO_SAFA;
	char *IP_MDJ;
	uint32_t PUERTO_MDJ;
	char *IP_FM9;
	uint32_t PUERTO_FM9;
	uint32_t TRANSFER_SIZE;
} config_t;

// variables
t_log *log_diego;
config_t config;
uint32_t safa;
uint32_t mdj;
uint32_t fm9;
pthread_t thread_servidor;

// funciones
config_t load_config();
void server();
void command_handler(uint32_t command);

#endif /* SRC_ELDIEGO_H_ */
