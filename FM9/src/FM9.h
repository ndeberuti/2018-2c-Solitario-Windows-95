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
#include <readline/readline.h>
#include <readline/history.h>
#include "funciones/funciones.h"
#include "commons/config.h"
#include "servidor/servidor.h"
#include <math.h>
#include "PCB.h"
#include "commons/bitarray.h"

// constantes
char *PATH_LOG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Logs/logFM9.txt";
char *PATH_CONFIG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/FM9/config.txt";

#define MAX_PARAMS 1

#define NUEVA_CONEXION_DIEGO 1
#define NUEVA_CONEXION_CPU 4
#define GUARDAR_PROCESO 5
#define ABRIR_PROCESO 6
#define DEVOLVER_PROCESO 7

// estructuras
typedef struct {
	uint32_t PUERTO;
	char *MODO;
	uint32_t TAMANIO;
	uint32_t MAX_LINEA;
	uint32_t TAM_PAGINA;
	uint32_t TRANSFER_SIZE;
} config_t;


typedef struct {
	char *comando;
	char *param[MAX_PARAMS];
	uint32_t cant_params;
} console_t;


typedef struct {
	int frame;
	int pid;
	int nroPag;
} t_tablaPaginaInvertida;

typedef struct{
	t_tablaPaginaInvertida *estructura_administrativa;
	char* frames;
	char* memoria;
} t_memoria_principal;


typedef struct{
	int pid;
	int id;
}entrada_administrativa_segmentacion_t;

typedef struct {

	int id;
	int limite;
	int base;

}segmento_tabla_t;


typedef struct {
	int segmento;
	int offset;

}segmento_offset_t;

typedef struct {
	int base;
	int offset;

}segmento_base_t;


/*enum MODOS_EJECUCION {
	SEGMENTACION_SIMPLE= "SEG",
	PAGINACION_INVERTIDA = "TPI",
	SEGMENTACION_PAGINADA = "SPI"
};*/


//listas

t_list* tabla_de_segmentos;
t_list* tabla_administrativa_segmentacion;

// variables
t_log *log_consola;
t_log *log_fm9;
config_t config;
pthread_t thread_servidor;

pthread_t thread_consola;
uint32_t diego;

void* buffer_envio;

//variables memoria segmentada

char* puntero_memoria_segmentada;

char* b_m_s;
t_bitarray* bitarray_memoria_segmentada;
int numero_lineas_memoria;
int id_segmento=0;


//funciones globales
config_t load_config();
void server();

void command_handler(uint32_t command, uint32_t socket);
void consola();


void setear_modo();
void setear_segmentacion_simple();
void setear_paginacion_invertida();
void setear_segmentacion_paginada();

void inicializar_memoria_segmentacion_simple();
void inicializar_memoria_paginacion_invertida();
void inicializar_tabla_de_paginas(int numero_lineas_memoria);

void recibir_proceso(int socket);
void devolver_proceso(int socket);

void guardar_proceso_segmentacion_simple(int pid ,int longitud_paquete, char* buffer_recepcion);
void guardar_proceso_paginas_invertidas(int pid ,int longitud_paquete, char* buffer_recepcion);
void guardar_proceso_segmentacion_paginada(int pid ,int longitud_paquete, char* buffer_recepcion);

void devolver_proceso_segmentacion_simple(int socket_diego, int pid);
void devolver_proceso_paginas_invertidas(int socket_diego,int pid);
void devolver_proceso_segmentacion_paginada(int socket_diego,int pid);

char* buscar_proceso_segmentacion_simple(int pid);
void liberar_segmento(int pid,int base, int limite);
//void devolver_proceso(int pid, int longitud_paquete);
//int obtener_cantidad_lineas(int longitud_paquete);


//segmentacion simple
int asignar_id();
int buscar_base(int offset);
void buscar_segmento(int pid, segmento_tabla_t* segmento_envio);


//paginas invertidas

void crearMemoriaPrincipal(int frames,int tamanio_pagina);
t_memoria_principal* puntero_memoria_paginada;


//bitarray
void reservar_bitarray(t_bitarray* bitarray_memoria_segmentada, int base, int limite);
void liberar_bitarray(t_bitarray* bitarray_memoria_segmentada,int base,int limite);



void guardar_proceso(int socket);


//comunicacion
char* recibir_char(int socket, int longitud_paquete);
int recibir_int(int socket);

#endif /* SRC_FM9_H_ */
