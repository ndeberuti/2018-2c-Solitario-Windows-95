/*
 * MDJ.h
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */

#ifndef SRC_MDJ_H_
#define SRC_MDJ_H_

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
char *PATH_LOG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Logs/logMDJ.txt";
char *PATH_CONFIG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/MDJ/config.txt";

// estructuras
typedef struct {
	uint32_t PUERTO;
	char *PUNTO_MONTAJE;
	uint32_t RETARDO;
} config_t;

// variables
t_log *log_mdj;
config_t config;
pthread_t thread_servidor;

// funciones
config_t load_config();
void server();
void command_handler(uint32_t command);

#endif /* SRC_MDJ_H_ */
