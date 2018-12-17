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
	#include <errno.h>

	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <pthread.h>
	#include <readline/readline.h>
	#include <readline/history.h>

	#include "commons/config.h"
	#include "commons/collections/list.h"
	#include "servidor/servidor.h"
	#include "commons/collections/dictionary.h"
	#include "enums.h"


	#include "commons/bitarray.h"
	#include <stdint.h>

	// constantes
	char *PATH_LOG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Logs/logFM9.txt";
	char *PATH_CONFIG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/FM9/config.txt";

	#define MAX_PARAMS 1


	//ESTRUCTURAS
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

		int pid;
		int id;
		int base;
		int limite;

	}segmento_tabla_t;


	typedef struct {
		int segmento;
		int offset;

	}segmento_offset_t;

	typedef struct {
		int pid;
		int id;
		t_list* tabla_de_paginas_segmento;


	}segmento_paginado_t;




	/*enum MODOS_EJECUCION {
		SEGMENTACION_SIMPLE= "SEG",
		PAGINACION_INVERTIDA = "TPI",
		SEGMENTACION_PAGINADA = "SPI"
	};*/

	//-------------------------------------------------------------------------------------------------------------------

	//listas



	// variables
	t_log *log_consola;
	t_log *log_fm9;
	config_t config;
	pthread_t thread_servidor;

	pthread_t thread_consola;
	uint32_t diego;
	uint32_t cpu;

	void* buffer_envio;

	//variables memoria segmentada

	char* puntero_memoria_segmentada;
	char* puntero_memoria_sp;

	char* b_m_s;
	t_bitarray* bitarray_memoria;
	int numero_lineas_memoria;
	int id_segmento=0;


	//funciones globales
	config_t load_config();
	void server();

	void command_handler(uint32_t command, uint32_t socket);
	void consola();

	void inicializar_diccionario();
	void setear_modo();

	void guardar_archivo(int socket);
	void abrir_archivo(int socket);
	void modificar_linea(int socket);
	void flush (int socket);
	void dump(int pid);
	void close_file(int socket_cpu);
	void close_process(int socket_cpu);

	void liberear_estructuras();

	void setear_segmentacion_simple();
	void setear_paginacion_invertida();
	void setear_segmentacion_paginada();

	void inicializar_memoria_segmentacion_simple();
	void inicializar_memoria_paginacion_invertida();
	void inicializar_memoria_segmentacion_paginada();


	void inicializar_tabla_de_paginas(int numero_lineas_memoria);

	void recibir_proceso(int socket);



	void guardar_proceso_paginas_invertidas(int pid ,int longitud_paquete, char* buffer_recepcion);



	void devolver_proceso_paginas_invertidas(int socket_diego,int pid);
	void devolver_proceso_segmentacion_paginada(int socket_diego,int pid);





//SEGMENTACION SIMPLE
	t_list* tabla_de_segmentos;





	int entra_en_memoria(int cantidad_lineas);

	void buscar_segmento_vacio(int cantidad_lineas, segmento_offset_t* segmento);





	int guardar_archivo_segmentacion_simple(int pid ,int id,int cantidad_lineas, char* buffer_recepcion);
	void abrir_archivo_segmentacion_simple(int socket_diego, int id);
	void modificar_linea_segmentacion_simple(int socket_cpu,int pid, int numero_linea, char* linea_nueva);
	void flush_segmentacion_simple(int socket_diego,int id);
	void dump_segmentacion_simple(int pid);
	int close_file_segmentacion_simple(int socket_cpu,int id);
	int close_process_segmentacion_simple(int socket_cpu,int pid);


	char* buscar_proceso_segmentacion_simple(int pid);

	int obtener_cantidad_lineas(int longitud_paquete);

	void buscar_segmento_2(int pid, segmento_tabla_t* segmento_envio);
	void liberar_segmento(int pid,int base, int limite);

	//PAGINACION
	t_list* tabla_de_paginas;

	typedef struct {

			int pid;
			int id;
			int frame;
		}entrada_tabla_invertida_t;



	char* puntero_memoria_paginada;

	int tamanio_bitarray_paginada;

	int guardar_archivo_paginas_invertidas(int pid,int id,int cantidad_lineas,char* buffer_recepcion);
	void abrir_archivo_paginacion(int socket_cpu,int id);
	int close_file_paginacion(int socket_cpu,int id);
	int close_process_paginacion(int socket_cpu,int pid);
	void modificar_linea_paginacion(int socket_cpu,int id,int numero_linea,char* linea_tratada);
	void flush_paginacion_invertida(int socket_diego,int id);
	void dump_paginacion_invertida(int pid);

	void paginar(int pid, int id, int cantidad_lineas, char* buffer_recepcion);
	int entra_memoria_paginada(int cantidad_paginas);

	

	
	//SEGMENTACION PAGINADA

	t_list* tabla_de_procesos_sp;
	t_list* tabla_de_segmentos_sp;
	t_list* tabla_de_paginas_sp;

	int guardar_archivo_segmentacion_paginada(int pid ,int id,int longitud_paquete, char* buffer_recepcion);
	void abrir_archivo_segmentacion_paginada(int socket_cpu, int id);
	void modificar_linea_segmentacion_paginada(int socket_cpu,int id,int numero_linea,char* linea_tratada);
	void flush_segmentacion_paginada(int socket_diego,int id);
	void dump_segmentacion_paginada(int pid);
	int close_file_segmentacion_paginada(int socket_cpu,int id);
	int close_process_segmentacion_paginada(int socket_cpu,int pid);


	void asignar_segmento_paginado_vacio(int cantidad_paginas,segmento_paginado_t* segmento_nuevo, char* buffer_envio);
	int entra_memoria_sp(int cantidad_paginas);
	void paginar_segmento(int pid,int id, int cantidad_lineas, char* buffer_recepecion);

	//bitarray
	void reservar_bitarray(t_bitarray* bitarray_memoria_segmentada, int base, int limite);
	void liberar_bitarray(t_bitarray* bitarray_memoria_segmentada,int base,int limite);

	int tamanio_bitarray_sp;

	//comunicacion
	char* recibir_char(int socket, int longitud_paquete);
	int recibir_int(int socket);


	//diccionario
	void inicializar_diccionario();
	int transformar_path(char* path);

	int id_nuevo();
	int id_global = 0;
	t_dictionary *diccionario;

	#endif /* SRC_FM9_H_ */
