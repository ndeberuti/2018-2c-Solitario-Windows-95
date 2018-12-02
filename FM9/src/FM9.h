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
	#include "commons/collections/list.h"
	#include "servidor/servidor.h"
	#include <math.h>
	#include "PCB.h"
	#include "commons/bitarray.h"
	#include <stdint.h>

	// constantes
	char *PATH_LOG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Logs/logFM9.txt";
	char *PATH_CONFIG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/FM9/config.txt";

	#define MAX_PARAMS 1

	#define NUEVA_CONEXION_DIEGO 1
	#define NUEVA_CONEXION_CPU 4
	#define CARGAR_PROCESO 6
	#define ABRIR_LINEA 7
	#define MODIFICAR_LINEA 8
	#define OK 10
	#define ERROR 11
	#define FLUSH 12

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

		int id;
		int base;
		int limite;

	}segmento_tabla_t;


	typedef struct {
		int segmento;
		int offset;

	}segmento_offset_t;

	typedef struct {
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


	void setear_modo();

	void guardar_proceso(int socket);
	void abrir_linea(int socket);
	void modificar_linea(int socket);
	void flush (int socket);
	void dump(int pid);

	void setear_paginacion_invertida();
	void setear_segmentacion_paginada();


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


	void setear_segmentacion_simple();
	void inicializar_memoria_segmentacion_simple();

	int guardar_proceso_segmentacion_simple(int pid ,int cantidad_lineas, char* buffer_recepcion);
	void abrir_linea_segmentacion_simple(int socket_diego, int pid, int numero_linea);
	void modificar_linea_segmentacion_simple(int socket_cpu,int pid, int numero_linea, char* linea_nueva);
	void flush_segmentacion_simple(int socket_diego,int pid);
	void dump_segmentacion_simple(int pid);


	char* buscar_proceso_segmentacion_simple(int pid);

	int obtener_cantidad_lineas(int longitud_paquete);

	void buscar_segmento_2(int pid, segmento_tabla_t* segmento_envio);
	void liberar_segmento(int pid,int base, int limite);

	//PAGINACION

	#define FRAME_ADM -1
	#define PAGINALIBRE -2
	#define FRAMELIBRE -3

	int ESTRUCTURA_ADM_SIZE;
	int ESTRUCTURA_ADM_CANT_FRAMES;
	int MARCOS;
	int MARCO_SIZE;
	int CANTIDADLINEASxPAG;

	typedef struct {
		int frame;
		int pid;
		int nroPag;
	} t_tablaPaginaInvertida;

	typedef struct memoria_principal{
		t_tablaPaginaInvertida *estructura_administrativa;
		char* frames;
		char* memoria;
	} t_memoria_principal;

	
	t_tablaPaginaInvertida * tablaInvertida;
	t_memoria_principal* memoria; // nuevo

	
	void inicializarMEMpaginada();

	void inicializarEstructuraAdicional();
	void crearMemoriaPrincipalPaginacion(int marcos, int marco_size);
	int leer_pagina(char*, char**);
	int crearEstructurasAdministrativas();
	int calcularPosicion(int frame);
	int buscarFrame(int unPid, int pagina);
	int asignarPaginasIniciales(int unPid, int paginas, char * buffer) ;
	int almacenarLinea(int unPid, int pagina, int offset, int tamanio, char * buffer);
	int solicitarLinea(int unPid, int pagina, int offset, int tamanio, char *buffer);
	int hash(int unPid, int pagina) ;
	int asignarPaginas(int pid, int paginas,char *buffer);
	int crearPid(int pid,int lineas,char *buffer);
	int eliminarPid(int pid);
	void eliminarPagina(int unPid, int nroPag);
	void crearMemoriaPrincipal(int frames,int tamanio_pagina);
	void abrir_linea_paginas_invertidas(int socket_cpu,int pid,int numero_linea);
	void modificar_linea_paginas_invertidas(int socket_cpu, int pid,int nroLinea,char* buffer);
	


	//SEGMENTACION PAGINADA

	t_list* tabla_de_procesos_sp;
	t_list* tabla_de_segmentos_sp;
	t_list* tabla_de_paginas_sp;

	int guardar_proceso_segmentacion_paginada(int pid ,int longitud_paquete, char* buffer_recepcion);
	void abrir_linea_segmentacion_paginada(int socket_cpu, int pid, int numero_linea);
	void modificar_linea_segmentacion_paginada(int socket_cpu,int pid,int numero_linea,char* linea_tratada);
	void flush_segmentacion_paginada(int socket_diego,int pid);
	void dump_segmentacion_paginada(int pid);


	void asignar_segmento_paginado_vacio(int cantidad_paginas,segmento_paginado_t* segmento_nuevo, char* buffer_recepcion);
	int entra_memoria_sp(int cantidad_paginas);
	void paginar_segmento(int id, int cantidad_lineas, char* buffer_recepecion);

	//bitarray
	void reservar_bitarray(t_bitarray* bitarray_memoria_segmentada, int base, int limite);
	void liberar_bitarray(t_bitarray* bitarray_memoria_segmentada,int base,int limite);






	//comunicacion
	char* recibir_char(int socket, int longitud_paquete);
	int recibir_int(int socket);

	#endif /* SRC_FM9_H_ */
