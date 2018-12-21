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
#include <openssl/md5.h>
#include "funciones/funciones.h"
#include "commons/config.h"
#include "commons/bitarray.h"
#include "servidor/servidor.h"
#include "enums.h"

// constantes
char *PATH_LOG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Logs/logMDJ.txt";
char *PATH_CONFIG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Configs/MDJ.cfg";

#define MAX_PARAMS 1

// estructuras
typedef struct {
	int32_t PUERTO;
	char *PUNTO_MONTAJE;
	int32_t RETARDO;
} config_t;

typedef struct {
	int32_t TAMANIO_BLOQUES;
	int32_t CANTIDAD_BLOQUES;
	char *MAGIC_NUMBER;
} config_fs;

typedef struct {
	char *comando;
	char *param[MAX_PARAMS];
	int32_t cant_params;
} console_t;

typedef struct {
	char *tamanio;
	char *bloques;
} f_config;

// variables
t_log *log_consola;
t_log *log_mdj;
config_t *config;
config_fs *fs_config;
char *path_bitmap;
char *bitmap;
t_bitarray *bitarray;
char *carpeta_archivos;
char *carpeta_bloques;
char *pathActual;
char *pathConsola;
pthread_t thread_servidor;
pthread_t thread_consola;
int32_t tamanioLineasMemoria;

// funciones
config_t *load_config();
void crear_estructura_directorios();
void server();
void command_handler(int32_t socket, int32_t command);
void validar_archivo(int32_t socket);
void crear_archivo(int32_t socket);
void obtener_datos(int32_t socket);
void guardar_datos(int32_t socket);
void borrar_archivo(int32_t socket);
char *int_to_bin(int32_t i);
int32_t calcular_cant_bloques(int32_t bytes);
void *proximo_bloque_libre(int32_t bloque_inicial);
void set_bitarray(int32_t posicion);
void clean_bitarray(int32_t posicion);
void save_file(char *path, int32_t tamanio, char *bloques);
void crear_path_completo(char *path_completo);
char *obtener_todo(char *path, int32_t offset);
f_config *file_create(char *path);
void file_destroy(f_config *config);
void borrar_todo(char *path);
char *formatear_path(char *path);
char *convertir_punto_punto(char *path_completo);
bool es_carpeta_archivos(char *path);
void consola();

#endif /* SRC_MDJ_H_ */
