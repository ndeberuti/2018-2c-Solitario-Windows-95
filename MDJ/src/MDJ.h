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
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "funciones/funciones.h"
#include "commons/config.h"
#include "commons/bitarray.h"
#include "servidor/servidor.h"

// constantes
char *PATH_LOG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Logs/logMDJ.txt";
char *PATH_CONFIG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/MDJ/config.txt";

#define MAX_PARAMS 1

#define NUEVA_CONEXION_DIEGO 1
#define VALIDAR_ARCHIVO 2
#define CREAR_ARCHIVO 3

#define OPERACION_FAIL 10
#define OPERACION_OK 11

#define ERROR_OPERACION 99

// estructuras
typedef struct {
	uint32_t PUERTO;
	char *PUNTO_MONTAJE;
	uint32_t RETARDO;
} config_t;

typedef struct {
	uint32_t TAMANIO_BLOQUES;
	uint32_t CANTIDAD_BLOQUES;
	char *MAGIC_NUMBER;
} config_fs;

typedef struct {
	char *comando;
	char *param[MAX_PARAMS];
	uint32_t cant_params;
} console_t;

// variables
t_log *log_consola;
t_log *log_mdj;
config_t config;
config_fs fs_config;
char *path_bitmap;
char *bitmap;
t_bitarray *bitarray;
char *carpeta_archivos;
char *carpeta_bloques;
char *pathActual;
char *pathConsola;
pthread_t thread_servidor;
pthread_t thread_consola;

// funciones
config_t load_config();
void crear_estructura_directorios();
void server();
void command_handler(uint32_t socket, uint32_t command);
void validar_archivo(uint32_t socket);
void crear_archivo(uint32_t socket);
void *proximo_bloque_libre(uint32_t bloque_inicial);
void set_bitarray(uint32_t posicion);
void consola();

#endif /* SRC_MDJ_H_ */
