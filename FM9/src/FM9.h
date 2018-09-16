/*
 * FM9.h
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */

#ifndef SRC_FM9_H_
#define SRC_FM9_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
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
char *PATH_LOG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Logs/logFM9.txt";
char *PATH_CONFIG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/FM9/config.txt";

// estructuras
typedef struct {
	uint32_t PUERTO;
	char *MODO;
	uint32_t TAMANIO;
	uint32_t MAX_LINEA;
	uint32_t TAM_PAGINA;
} config_t;



typedef struct {
	int numero;
	char* palabra;
} prueba_t;

/*enum MODOS_EJECUCION {
	SEGMENTACION_SIMPLE= "SEG",
	PAGINACION_INVERTIDA = "TPI",
	SEGMENTACION_PAGINADA = "SPI"
};*/



// variables
t_log *log_fm9;
config_t config;
pthread_t thread_servidor;
void* memory_pointer; //puntero a primer direccion de FM9
//prueba_t *prueba;


// funciones
config_t load_config();
void server();
void command_handler(uint32_t command);
void inicializar_memoria();
void setear_modo();
void setear_segmentacion_simple();
void setear_paginacion_invertida();
void setear_segmentacion_paginada();

void deserializar(void* buffer, int tamanio);
void serializar(void* buffer_envio);

#endif /* SRC_FM9_H_ */
