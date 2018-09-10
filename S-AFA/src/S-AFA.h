/*
 * S-AFA.h
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */

#ifndef SRC_S_AFA_H_
#define SRC_S_AFA_H_

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
#include <readline/readline.h>
#include <readline/history.h>
#include "funciones/funciones.h"
#include "commons/config.h"
#include "commons/collections/list.h"
#include "servidor/servidor.h"
#include "enums.h"

// constantes
char *PATH_LOG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Logs/logSAFA.txt";
char *PATH_CONFIG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/S-AFA/config.txt";

#define MAX_PARAMS 1

// estructuras
typedef struct {
	uint32_t PUERTO;
	char *ALGORITMO;
	uint32_t QUANTUM;
	uint32_t MULTIPROGRAMACION;
	uint32_t RETARDO_PLANIF;
} config_t;

typedef struct {
	char *comando;
	char *param[MAX_PARAMS];
	uint32_t cant_params;
} console_t;

// variables
t_log *log_consola;
t_log *log_safa;
config_t config;
pthread_t thread_servidor;
pthread_t thread_LTS;
pthread_t thread_STS;
uint32_t diego;
fd_set master; // conjunto maestro de descriptores de fichero


// funciones
config_t load_config();
void server();
void moduleHandler(uint32_t command, uint32_t socket);
void longTermScheduler();
void shortTermScheduler();

#endif /* SRC_S_AFA_H_ */
