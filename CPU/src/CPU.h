/*
 * CPU.h
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */

#ifndef SRC_CPU_H_
#define SRC_CPU_H_

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
char *PATH_LOG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Logs/logCPU.txt";
char *PATH_CONFIG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/CPU/config.txt";

#define CONEXION_CPU 4

// estructuras
typedef struct {
	char *IP_SAFA;
	uint32_t PUERTO_SAFA;
	char *IP_DIEGO;
	uint32_t PUERTO_DIEGO;
	char *IP_FM9;
	uint32_t PUERTO_FM9;
	uint32_t RETARDO;
} config_t;

// variables
t_log *log_cpu;
config_t config;
uint32_t safa;
uint32_t diego;
uint32_t fm9;

// funciones
config_t load_config();

#endif /* SRC_CPU_H_ */
