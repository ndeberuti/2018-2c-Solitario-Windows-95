/*
 * FM9.c
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */
#include "FM9.h"

int main(void) {




	system("clear");
	puts("PROCESO FM9\n");

	log_consola = init_log(PATH_LOG, "Consola FM9", true, LOG_LEVEL_INFO);
	log_fm9 = init_log(PATH_LOG, "Proceso FM9", true, LOG_LEVEL_INFO);
	log_info(log_fm9, "Inicio del proceso");

	config = load_config();
	


	inicializar_diccionario();


	setear_modo();




	//close_process_segmentacion_paginada(diego, 10);
	//guardar_archivo_segmentacion_paginada(11 ,222,8,"fallaaa\0");

/*
	for(int j = 0; j < 4; j++){

			int muestra = bitarray_test_bit(bitarray_memoria, j);
			printf("prueba bitarray : %d \n", muestra);
		}

*/

	/*
	bitarray_set_bit(bitarray_memoria, 1);
	bitarray_set_bit(bitarray_memoria, 5);
	bitarray_set_bit(bitarray_memoria, 15);
	bitarray_set_bit(bitarray_memoria, 20);

	printf("tamanio bitarray : %d \n", max_bit);





	int max_bit = bitarray_get_max_bit(bitarray_memoria);
	int largo = 1;



	int base = 0;
	int otra_base = 0;

	while(base < max_bit && otra_base- base < largo ){

		base=otra_base;
		while(bitarray_test_bit(bitarray_memoria, base) && base < max_bit){

			base++;

		}

		base++;
		otra_base=base;
		otra_base++;
		while(!bitarray_test_bit(bitarray_memoria, otra_base) && otra_base < max_bit){

			otra_base++;

		}
	}
	for(int j = 0; j < 30; j++){

		int muestra = bitarray_test_bit(bitarray_memoria, j);
		printf("prueba bitarray : %d \n", muestra);
	}

printf("BASE: %d OTRABASE :%d", base, otra_base);
/*
		if(!bitarray_test_bit(bitarray_memoria, base)){


			otra_base= base;
			otra_base++;
			while(!bitarray_test_bit(bitarray_memoria, otra_base) && otra_base < max_bit){

				otra_base++;

			}

		}else{

			base++;
		}


		base=otra_base;

	}
*/

	pthread_create(&thread_consola, NULL, (void *) consola, NULL);

	server();

	pthread_join(thread_consola, NULL);

	//pthread_join(thread_servidor, NULL);

	liberear_estructuras();
	exit(EXIT_SUCCESS);
	



}

config_t load_config() {
	t_config *config = config_create(PATH_CONFIG);

	config_t miConfig;
	miConfig.PUERTO = config_get_int_value(config, "PUERTO");
	miConfig.MODO = strdup(config_get_string_value(config, "MODO"));
	miConfig.TAMANIO = config_get_int_value(config, "TAMANIO");
	miConfig.MAX_LINEA = config_get_int_value(config, "MAX_LINEA");
	miConfig.TAM_PAGINA = config_get_int_value(config, "TAM_PAGINA");
	miConfig.TRANSFER_SIZE = config_get_int_value(config, "TRANSFER_SIZE");

	log_info(log_fm9, "---- Configuracion ----");
	log_info(log_fm9, "PUERTO = %d", miConfig.PUERTO);
	log_info(log_fm9, "MODO = %s", miConfig.MODO);
	log_info(log_fm9, "TAMANIO = %d", miConfig.TAMANIO);
	log_info(log_fm9, "MAX_LINEA = %d", miConfig.MAX_LINEA);
	log_info(log_fm9, "TAM_PAGINA = %d", miConfig.TAM_PAGINA);
	log_info(log_fm9, "-----------------------");

	config_destroy(config);
	return miConfig;
}


void server() {
	fd_set master; // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
	struct sockaddr_in remoteaddr; // dirección del cliente
	int fdmax; // número máximo de descriptores de fichero
	int newfd; // descriptor de socket de nueva conexión aceptada
	int command; // comando del cliente
	int nbytes;
	int addrlen;
	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);

	// obtener socket a la escucha
	int servidor = build_server(config.PUERTO, log_consola);

	// añadir listener al conjunto maestro
	FD_SET(servidor, &master);
	// seguir la pista del descriptor de fichero mayor
	fdmax = servidor; // por ahora es éste

	// bucle principal
	while (true) {
		read_fds = master; // cópialo
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			log_error(log_consola, "select");
			exit(EXIT_FAILURE);
		}
		// explorar conexiones existentes en busca de datos que leer
		for (int i = 0; i <= fdmax; i++)
			if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
				if (i == servidor) {
					// gestionar nuevas conexiones
					addrlen = sizeof(remoteaddr);
					if ((newfd = accept(servidor, (struct sockaddr *) &remoteaddr, &addrlen)) == -1)
						log_error(log_consola, "accept");
					else {
						FD_SET(newfd, &master); // añadir al conjunto maestro
						if (newfd > fdmax) // actualizar el máximo
							fdmax = newfd;
						//log_info(log_consola, "Nueva conexion desde %s en el socket %d", inet_ntoa(remoteaddr.sin_addr), newfd);
					}
				}
				else
					// gestionar datos de un cliente
					if ((nbytes = receive_int(i, &command)) <= 0) {
						// error o conexión cerrada por el cliente
						if (nbytes == 0)
							// conexión cerrada
							if (i == diego)
								log_info(log_consola, "Se desconecto El Diego");
							else
								log_info(log_consola, "CPU desconectada");
						else
							log_error(log_consola, "recv (comando)");

						close(i); // ¡Hasta luego!
						FD_CLR(i, &master); // eliminar del conjunto maestro
					}
					else
						// tenemos datos de algún cliente
						command_handler(command, i);
			} // Si, ahí está bien, pero si por ejemplo querés que ese valor contra el que comparas sea dinámico en base a un valor que recibis en la función, tenés que hacer referencia a esa variable y poner la función condicional en la función que hace el list_findif (FD_ISSET(i, &read_fds))
	} // while (true)
}

void command_handler(int command, int socket) {
	switch (command) {
	case NEW_DMA_CONNECTION:
		log_info(log_consola, "Nueva conexion desde El Diego");
		diego = socket;
		send(diego, &config.MAX_LINEA, sizeof(int), MSG_WAITALL);
		break;
	case NEW_CPU_CONNECTION:
		log_info(log_consola, "Nueva conexion de CPU");
		send(socket, &config.MAX_LINEA, sizeof(int),MSG_WAITALL);
			break;
	case CARGAR_ARCHIVO:
		log_info(log_consola, "Cargando archivo en memoria");
		guardar_archivo(socket);
			break;
	case LEER_ARCHIVO:
		log_info(log_consola, "Leyendo archivo");
		abrir_archivo(socket);
			break;
	case ASIGNAR:
		log_info(log_consola, "Modificando linea");
		modificar_linea(socket);
			break;
	case FLUSH:
		log_info(log_consola, "Flushing");
		flush(socket);
			break;
	case CLOSE_FILE:
		log_info(log_consola, "Closing file");
		close_file(socket);
		break;
	case CLOSE_PROCESS:
		log_info(log_consola, "Closing process");
		close_process(socket);
		break;

	default:
		log_warning(log_consola, "%d: Comando recibido incorrecto", command);
	}
}

void consola() {
	char *linea;
	char *token;
	char* pid_char;
	int pid;
	console_t *consola;

	while (true) {
		linea = readline("FM9> ");

		if (strlen(linea) > 0) {
			add_history(linea);
			log_info(log_consola, "Linea leida: %s", linea);
			consola = malloc(sizeof(console_t));

			if (consola != NULL) {
				consola->comando = strdup(strtok(linea, " "));
				consola->cant_params = 0;

				while (consola->cant_params < MAX_PARAMS && (token = strtok(NULL, " ")) != NULL)
					consola->param[consola->cant_params++] = strdup(token);

				if (str_eq(consola->comando, "clear"))
					system("clear");

				else if (str_eq(consola->comando, "dump"))
					if (consola->cant_params < 1)
						print_c(log_consola, "%s: falta el parametro ID del DTB\n", consola->comando);
					else {
					   pid_char = strdup(consola->param[0]);
					   pid =  atoi(pid_char);

						dump(pid);
					}

				else
					print_c(log_consola, "%s: Comando incorrecto\n", consola->comando);

				free(consola->comando);
				for (uint i = 0; i < consola->cant_params; i++)
					free(consola->param[i]);
				free(consola);
			}
		}
		free(linea);
	}
}

void setear_segmentacion_simple(){
	log_info(log_fm9, "Segmentación Simple seteada");
	inicializar_memoria_segmentacion_simple();
}

void setear_paginacion_invertida(){
	log_info(log_fm9, "Tablas de Paginación Invertida seteada");
	inicializar_memoria_paginacion_invertida();
}

void setear_segmentacion_paginada(){
	log_info(log_fm9, "Segmentación Paginada seteada");
	inicializar_memoria_segmentacion_paginada();
}

void setear_modo(){
	if(strcmp("SEG", config.MODO)== 0){
		setear_segmentacion_simple();
	}
	else if(strcmp("TPI", config.MODO)== 0){
		setear_paginacion_invertida();
	}
	else if(strcmp("SPA", config.MODO)== 0){
		setear_segmentacion_paginada();
	}
	else {
		log_error(log_fm9, "Modo de Gestión de Memoria desconocido");
	}

}

int obtener_cantidad_lineas(int longitud_paquete){


	return (longitud_paquete + config.MAX_LINEA - 1) / config.MAX_LINEA;

}

void guardar_archivo(int socket_diego){
	int resultado;
	int pid = recibir_int(socket_diego);
	printf("PID: %d\n",pid);

	int longitud_path = recibir_int(socket_diego);
	printf("longitud_path: %d\n",longitud_path);
	char* buffer  = recibir_char(socket_diego, longitud_path);
	printf("path: %s\n",buffer);

	int id = transformar_path(buffer, pid);



	free(buffer);

	int cantidad_lineas = recibir_int(socket_diego);
	printf("cantidad_lineas: %d\n",cantidad_lineas);
	int longitud = (cantidad_lineas * config.MAX_LINEA) +1;
	printf("longitud: %d\n",longitud);
	char* buffer_recepcion = recibir_char(socket_diego, longitud);

	printf("\nbuffer_recepcion: %s\n",buffer_recepcion);





	if(strcmp("SEG", config.MODO)== 0){
		resultado = guardar_archivo_segmentacion_simple(pid ,id , cantidad_lineas,buffer_recepcion);
		}
		else if(strcmp("TPI", config.MODO)== 0){

		//resultado = crearPid(pid,id,cantidad_lineas,buffer_recepcion);
		resultado = guardar_archivo_paginas_invertidas(pid, id, cantidad_lineas, buffer_recepcion);
		}
		else if(strcmp("SPA", config.MODO)== 0){
		resultado = guardar_archivo_segmentacion_paginada(pid, id ,cantidad_lineas, buffer_recepcion);
		}
		else {
			log_error(log_fm9, "Modo de Gestión de Memoria desconocido");
		}

	free(buffer_recepcion);
	send(socket_diego, &resultado, sizeof(int), MSG_WAITALL);
}

void liberear_estructuras(){

	dictionary_clean(diccionario);

	if(strcmp("SEG", config.MODO)== 0){


	 list_destroy(tabla_de_segmentos);
				}
				else if(strcmp("TPI", config.MODO)== 0){

					list_destroy(tabla_de_paginas);

				}
				else if(strcmp("SPA", config.MODO)== 0){
					segmento_paginado_t* segmento;

					int tamanio = list_size(tabla_de_segmentos_sp);

					for(int i = 0; i < tamanio; i++){

						segmento = list_get(tabla_de_segmentos_sp, i);
						list_destroy(segmento->tabla_de_paginas_segmento);

					}

					list_destroy(tabla_de_segmentos_sp);

				}
				else {
				log_error(log_fm9, "Modo de Gestión de Memoria desconocido");
				}




}

void abrir_archivo(int socket_cpu){


	int pid = recibir_int(socket_cpu);
		int longitud_path = recibir_int(socket_cpu);
		char* buffer = recibir_char(socket_cpu, longitud_path);
		int id = transformar_path(buffer, pid);
		free(buffer);




	if(strcmp("SEG", config.MODO)== 0){
			abrir_archivo_segmentacion_simple(socket_cpu, id);
			}
			else if(strcmp("TPI", config.MODO)== 0){
			
			//abrir_archivo_paginas_invertidas(socket_cpu, id);
			abrir_archivo_paginacion(socket_cpu, id);
			}
			else if(strcmp("SPA", config.MODO)== 0){
			abrir_archivo_segmentacion_paginada(socket_cpu, id);
			}
			else {
			log_error(log_fm9, "Modo de Gestión de Memoria desconocido");
			}



}

void close_file(int socket_cpu){
	int resultado;

	int pid = recibir_int(socket_cpu);
	int longitud_path = recibir_int(socket_cpu);
	char* path = recibir_char(socket_cpu, longitud_path);
	int id = transformar_path(path, pid);




		if(strcmp("SEG", config.MODO)== 0){
				resultado = close_file_segmentacion_simple(socket_cpu, id);
				}
				else if(strcmp("TPI", config.MODO)== 0){


					resultado= close_file_paginacion(socket_cpu, id);

				}
				else if(strcmp("SPA", config.MODO)== 0){
				resultado = close_file_segmentacion_paginada(socket_cpu, id);
				}
				else {
				log_error(log_fm9, "Modo de Gestión de Memoria desconocido");
				}

		send(socket_cpu, &resultado, sizeof(int), MSG_WAITALL);

		bool eliminacionOk = eliminar_id_segmento_de_diccionario(id, path, pid);

		if(!eliminacionOk)
			//TODO mostrar un error y romper todo

		free(path);
}

void close_process(int socket_cpu){

		int resultado;
		int pid = recibir_int(socket_cpu);



		if(strcmp("SEG", config.MODO)== 0){
				resultado = close_process_segmentacion_simple(socket_cpu, pid);
				}
				else if(strcmp("TPI", config.MODO)== 0){

				//resultado = close_process_paginas_invertidas(socket_cpu, pid);
					resultado = close_process_paginacion(socket_cpu, pid);
				}
				else if(strcmp("SPA", config.MODO)== 0){
				resultado = close_process_segmentacion_paginada(socket_cpu,pid);
				}
				else {
				log_error(log_fm9, "Modo de Gestión de Memoria desconocido");
				}
		send(socket_cpu, &resultado, sizeof(int), MSG_WAITALL);
}


void modificar_linea(int socket_cpu){

	int pid = recibir_int(socket_cpu);
	int longitud_path = recibir_int(socket_cpu);
	char* buffer = recibir_char(socket_cpu, longitud_path);
	int id = transformar_path(buffer, pid);
	free(buffer);
	int numero_linea = recibir_int(socket_cpu);
	int longitud_linea = recibir_int(socket_cpu);
	char* nueva_linea = recibir_char(socket_cpu, longitud_linea);
	char* linea_tratada = malloc(config.MAX_LINEA);

	memset(linea_tratada, '\n', config.MAX_LINEA);
	memcpy(linea_tratada, nueva_linea, longitud_linea);

	free(nueva_linea);

	if(strcmp("SEG", config.MODO)== 0){
			modificar_linea_segmentacion_simple(socket_cpu, id,numero_linea, linea_tratada);
			}
			else if(strcmp("TPI", config.MODO)== 0){

			modificar_linea_paginacion(socket_cpu, id, numero_linea, linea_tratada);
			}
			else if(strcmp("SPA", config.MODO)== 0){
			modificar_linea_segmentacion_paginada(socket_cpu, id, numero_linea, linea_tratada);

			}
			else {
			 log_error(log_fm9, "Modo de Gestión de Memoria desconocido");
			}


free(linea_tratada);
}

void flush(int socket_diego){

	int pid = recibir_int(socket_diego);
	int longitud_path = recibir_int(socket_diego);
		char* buffer = recibir_char(socket_diego, longitud_path);
		int id = transformar_path(buffer, pid);
		free(buffer);

	if(strcmp("SEG", config.MODO)== 0){
					flush_segmentacion_simple(socket_diego, id);
				}
				else if(strcmp("TPI", config.MODO)== 0){

					flush_paginacion_invertida(socket_diego, id);
				}
				else if(strcmp("SPA", config.MODO)== 0){
					flush_segmentacion_paginada(socket_diego,id);
				}
				else {
					log_error(log_fm9, "Modo de Gestión de Memoria desconocido");
				}

}

void dump(int pid){



	if(strcmp("SEG", config.MODO)== 0){
					dump_segmentacion_simple(pid);
				}
				else if(strcmp("TPI", config.MODO)== 0){

					dump_paginacion_invertida(pid);
				}
				else if(strcmp("SPA", config.MODO)== 0){
					dump_segmentacion_paginada(pid);
				}
				else {
					log_error(log_fm9, "Modo de Gestión de Memoria desconocido");
				}

}

int recibir_int(int socket){
	int buffer;
	int resultado_recv = recv(socket, &buffer, sizeof(int), MSG_WAITALL);

if(resultado_recv == -1){

	log_error(log_fm9, "Error en la repcecepción de mensaje");
	return -1;
}else{
return buffer;
}

}

char* recibir_char(int socket, int longitud_paquete){
char* buffer_recepcion = malloc(longitud_paquete);

int resultado = recv(socket, buffer_recepcion, longitud_paquete, MSG_WAITALL);

if(resultado == -1){

	log_error(log_fm9, "Error recibir char: %s", strerror(errno));

	return "Error";}

	else{

		return buffer_recepcion;
	}


}

//---------------------------------------------------------------------------------------------------------
//SEGMENTACION SIMPLE

void inicializar_memoria_segmentacion_simple(){
	//tabla de segmentos






	puntero_memoria_segmentada = malloc(config.TAMANIO);
	memset(puntero_memoria_segmentada, '~', config.TAMANIO);

	tabla_de_segmentos = list_create();

	b_m_s = calloc(1,config.TAMANIO / config.MAX_LINEA /8);
	bitarray_memoria = bitarray_create(b_m_s, config.TAMANIO / config.MAX_LINEA /8);


	int tamanio_bitarray = bitarray_get_max_bit(bitarray_memoria);
	int tamanio_bitarray2 = config.TAMANIO / config.MAX_LINEA / 8 ;

	log_info("Tamanio del bitarray = %d", tamanio_bitarray);
	log_info("Tamanio del bitarray = %d", tamanio_bitarray2);




	if(puntero_memoria_segmentada == NULL){

		log_error(log_fm9, "No se pudo inicializar la memoria");
	}else{

		log_info(log_fm9, "Inicialización de memoria exitosa");
	}

}

int guardar_archivo_segmentacion_simple(int pid ,int id,int cantidad_lineas, char* buffer_recepcion){

	int entra_memoria;

segmento_offset_t* segmento_nuevo = malloc(sizeof(segmento_offset_t));
segmento_tabla_t* entrada_tabla = malloc(sizeof(segmento_tabla_t));

int resultado;
//entrada_administrativa_segmentacion_t* entrada_administrativa = malloc(sizeof(entrada_administrativa_segmentacion_t));




//entrada_administrativa->pid = pid;

entra_memoria = entra_en_memoria(cantidad_lineas);

if(buffer_recepcion == NULL){

	log_error(log_fm9, "Buffer nulo");
	resultado = ERROR;
}else{


	if(list_is_empty(tabla_de_segmentos) && entra_memoria){


							//entrada_administrativa->id = pid;
							entrada_tabla->pid = pid;
							entrada_tabla->id = id;
							entrada_tabla->base = 0 ;
							entrada_tabla->limite = cantidad_lineas;


							reservar_bitarray(bitarray_memoria, entrada_tabla->base, entrada_tabla->limite);





							list_add(tabla_de_segmentos, entrada_tabla);

							printf("CANTIDAD LINEAS RECEPCION: %d\n", cantidad_lineas);
							printf("ESTA LLEGANDO ESTO DEL DMA: %s\n", buffer_recepcion);
							memcpy(puntero_memoria_segmentada, buffer_recepcion, cantidad_lineas * config.MAX_LINEA);

							resultado = OK;
							log_info(log_fm9, "Se guardo el segmento %d en memoria \n", id);
							return resultado;
	}else{



			if(entra_memoria){


								buscar_segmento_vacio(cantidad_lineas, segmento_nuevo);


								entrada_tabla->pid = pid;
								entrada_tabla->id = id;


								entrada_tabla->base = segmento_nuevo->segmento;

								entrada_tabla->limite = segmento_nuevo->offset;

								memcpy(puntero_memoria_segmentada + entrada_tabla->base * config.MAX_LINEA, buffer_recepcion, entrada_tabla->limite * config.MAX_LINEA);




								reservar_bitarray(bitarray_memoria, entrada_tabla->base, entrada_tabla->limite);


								list_add(tabla_de_segmentos, entrada_tabla);

								resultado = OK;
								log_info(log_fm9, "Se guardo el segmento %d en memoria \n", entrada_tabla->id);
								return resultado;


								}else{

									resultado = ESPACIO_INSUFICIENTE;
									log_error(log_fm9, "Archivo no entra en memoria \n");
									return resultado;
								}


	}
}
free(segmento_nuevo);

return 0;


}

int entra_en_memoria(int cantidad_lineas){

	int max_bit = bitarray_get_max_bit(bitarray_memoria);



	int base = 0;
	int otra_base = 0;

		while(base < max_bit && otra_base- base < cantidad_lineas ){

			base=otra_base;
			while(bitarray_test_bit(bitarray_memoria, base) && base < max_bit){

				base++;

			}


					otra_base=base;
					otra_base++;
			while(!bitarray_test_bit(bitarray_memoria, otra_base) && otra_base < max_bit){

				otra_base++;

			}


		}

	if(cantidad_lineas <= otra_base - base){


	return 1;
	}else{
		return 0;
	}


}

void abrir_archivo_segmentacion_simple(int socket_cpu,int id){
	segmento_tabla_t* segmento_linea;

	int resultado, tamanio, resultado_envio;

	bool es_id(segmento_tabla_t* entrada){

					return entrada->id == id;

		}


	segmento_linea = list_find(tabla_de_segmentos, (void*) es_id);




	if(segmento_linea != NULL){
		printf("base: %d \n", segmento_linea->base);
			printf("id: %d \n", segmento_linea->id);
			printf("limite: %d \n", segmento_linea->limite);
			printf("pid: %d \n", segmento_linea->pid);
		resultado = OK;

	void* buffer_envio = calloc(1, (segmento_linea->limite * config.MAX_LINEA) + (sizeof(int)*3));

	tamanio = segmento_linea->limite * config.MAX_LINEA;

	memcpy(buffer_envio,&resultado, sizeof(int));


	memcpy(buffer_envio+sizeof(int),&tamanio, sizeof(int));


	memcpy(buffer_envio+ sizeof(int)*2,puntero_memoria_segmentada + segmento_linea->base * config.MAX_LINEA , segmento_linea->limite * config.MAX_LINEA);
	memcpy(buffer_envio + sizeof(int)*2 + tamanio , &(segmento_linea->limite), sizeof(int));

	resultado_envio = send(socket_cpu, buffer_envio, tamanio + (sizeof(int)* 3), MSG_WAITALL);

	log_info(log_fm9, "Datos archivo enviado ->buffer: %s; tamanio: %d; resultado: %d\n", buffer_envio, tamanio, resultado);

	free(buffer_envio);

		if(resultado_envio != -1){
		log_info(log_fm9, "Se envió el archivo %d al CPU del proceso %d", segmento_linea->id, segmento_linea->pid);
		} else{

			log_error(log_fm9, "Error en el envio del archivo %d, del proceso %d", segmento_linea->id, segmento_linea->pid);

		}
	}else{

		resultado = ARCHIVO_NO_ABIERTO;

  resultado_envio =  send(socket_cpu, &resultado, sizeof(int), MSG_WAITALL);
   log_error(log_fm9, "No se encuentra el archivo en memoria");


	}

}

void modificar_linea_segmentacion_simple(int socket_cpu,int id,int numero_linea, char* linea_nueva){
	segmento_tabla_t* segmento_linea;
	int resultado;

		bool es_id(segmento_tabla_t* entrada){

						return entrada->id == id;

			}

		printf("id de segmento a buscar %d: \n", id);

		segmento_linea = list_find(tabla_de_segmentos,(void*) es_id);



		if(segmento_linea == NULL){

			log_error(log_fm9, "El segmento %d no se encuentra en memoria", id);
		resultado = ARCHIVO_NO_ABIERTO ;


		}


		else if(numero_linea > segmento_linea->limite){

			log_error(log_fm9, "Segmentation fault");
			resultado = SEGMENTATION_FAULT;
		}else{

			log_info(log_fm9, "Modificando linea");
			memcpy(puntero_memoria_segmentada + (segmento_linea->base *config.MAX_LINEA) + ((numero_linea -1 ) * config.MAX_LINEA),linea_nueva,config.MAX_LINEA);

			resultado = OK ;
		}











   send(socket_cpu, &resultado, sizeof(int), MSG_WAITALL);

}



void flush_segmentacion_simple(int socket_diego, int id){
	int resultado, cantidad_lineas,tamanio;

	segmento_tabla_t* segmento;


	bool es_id(segmento_tabla_t* entrada_segmento){


								return entrada_segmento->id == id;
							}


		segmento = list_find(tabla_de_segmentos, (void*) es_id);


		if(segmento != NULL){
		resultado = OK;
		tamanio = segmento->limite * config.MAX_LINEA;
		cantidad_lineas = segmento->limite;

		//TODO - Revisa este envio... comparandolo con el abrir, esta mal... y agregame el envio de cantidad de lineas al final, igual que en el abrir

		char* buffer_envio = malloc(sizeof(int)*3 +(segmento-> limite * config.MAX_LINEA));

		memcpy(buffer_envio, &resultado, sizeof(int));
		memcpy(buffer_envio+ sizeof(int), &tamanio,sizeof(int));
		memcpy(buffer_envio+ sizeof(int) *2 , puntero_memoria_segmentada + (segmento->base * config.MAX_LINEA), (segmento->limite * config.MAX_LINEA));
		memcpy(buffer_envio+ sizeof(int)*2 + segmento->limite * config.MAX_LINEA, &cantidad_lineas,sizeof(int));

		send(socket_diego, buffer_envio, sizeof(int)*3+ segmento->limite * config.MAX_LINEA, MSG_WAITALL);


		free(buffer_envio);

		log_info(log_fm9, "Se completo el flush para el archivo %d" , id );
}else{

		resultado = ARCHIVO_NO_ABIERTO;

		send(socket_diego, &resultado, sizeof(int), MSG_WAITALL);
		log_error(log_fm9, "El archivo no está en memoria");
}

}

void dump_segmentacion_simple(int pid){

	int aciertos;
	int contador = 0;
			t_list* lista_filtrada = NULL;
			segmento_tabla_t* segmento = NULL;

			if(list_size(tabla_de_segmentos) > 0){
					bool es_pid(segmento_tabla_t* entrada_segmento){


												return entrada_segmento->pid == pid;
											}



							lista_filtrada= list_filter(tabla_de_segmentos, (void*) es_pid);
							aciertos = list_size(lista_filtrada);

							printf("ACIERTOS DUMP: %d", aciertos);

				if(aciertos > 0){

					log_info(log_fm9, "---- Dump ----");

						while(contador < aciertos){

							segmento = list_get(lista_filtrada, contador);
							char* buffer_muestra = malloc(segmento->limite * config.MAX_LINEA);

							if(segmento != NULL){

								log_info(log_fm9, "PID PROCESO = %d", segmento->pid);
								log_info(log_fm9, "ID SEGMENTO = %d", segmento->id);
								log_info(log_fm9, "BASE = %d", segmento->base );
								log_info(log_fm9, "LIMITE = %d", segmento->limite);

								memcpy(buffer_muestra,puntero_memoria_segmentada + segmento->base * config.MAX_LINEA, segmento->limite * config.MAX_LINEA );
								log_info(log_fm9, "DATOS EN MEMORIA REAL = %s", buffer_muestra);



												}else{

													log_error(log_fm9, "El archivo %d no se encuentra en memoria", segmento->id);


												}

						contador++;
						free(buffer_muestra);
						}

						list_destroy(lista_filtrada);

				}else{


					log_error(log_fm9, "El proceso %d no se encuentra en memoria", pid);
				}


			}else{

				log_error(log_fm9, "La tabla de segmentos y la memoria estan vacias");
			}
}


int close_file_segmentacion_simple(int socket_cpu, int id){
	int resultado;
	segmento_tabla_t* segmento;


			bool es_id(segmento_tabla_t* entrada_segmento){


										return entrada_segmento->id == id;
									}


				segmento = list_find(tabla_de_segmentos, (void*) es_id);


				if(segmento != NULL){


					memset(puntero_memoria_segmentada + segmento->base * config.MAX_LINEA,'/n' , segmento->limite*config.MAX_LINEA);

					liberar_segmento(segmento->id, segmento->base, segmento->limite);
					log_info(log_fm9, "Se cerró el archivo ID: %d en memoria", id);
					resultado = OK;
				}else{

					log_error(log_fm9, "El archivo no se encuentra en memoria");
					resultado = ARCHIVO_NO_ABIERTO;

				}

				return resultado;



}

int close_process_segmentacion_simple(int socket_cpu,int pid){
		t_list* lista_filtrada;
		segmento_tabla_t* segmento;
		int aciertos;
		int resultado;

				bool es_pid(segmento_tabla_t* entrada_segmento){


											return entrada_segmento->pid == pid;
										}



						lista_filtrada= list_filter(tabla_de_segmentos, (void*) es_pid);
						aciertos = list_size(lista_filtrada);

			if(aciertos > 0){

				log_info(log_fm9, "Cerrando proceso %d \n", pid);

				for(int i = 0; i < aciertos; i++){

					segmento = list_get(lista_filtrada, i);

					resultado = close_file_segmentacion_simple(socket_cpu, segmento->id);


				}
				}else{

					log_error(log_fm9, "El proceso %d no se encuentra en memoria \n", pid);
					resultado = PROCESO_NO_ABIERTO;
				}

list_destroy(lista_filtrada);
return resultado;
}
//--
void liberar_segmento(int pid, int base, int limite){



	liberar_bitarray(bitarray_memoria, base, limite);


	bool es_pid(segmento_tabla_t* entrada){

				return entrada->id == pid;
			}


	list_remove_by_condition(tabla_de_segmentos,(void*) es_pid);


}







void buscar_segmento_vacio(int cantidad_lineas, segmento_offset_t* segmento){

	int max_bit = bitarray_get_max_bit(bitarray_memoria);



	int base = 0;
	int otra_base = 0;

	while(base < max_bit && otra_base- base < cantidad_lineas ){

		base=otra_base;
		while(bitarray_test_bit(bitarray_memoria, base) && base < max_bit){

			base++;

		}


				otra_base=base;
				otra_base++;
		while(!bitarray_test_bit(bitarray_memoria, otra_base) && otra_base < max_bit){

			otra_base++;

		}


	}








segmento->offset = cantidad_lineas;
segmento->segmento = base;


}










//-------------------------------------------------------------------------------------------------------------------
//SEGMENTACION PAGINADA

void inicializar_memoria_segmentacion_paginada(){

	int forzar_bitarray = (config.TAMANIO / config.TAM_PAGINA / 8);

tamanio_bitarray_sp = config.TAMANIO / config.TAM_PAGINA;

if(config.TAMANIO % config.TAM_PAGINA > 0){


	tamanio_bitarray_sp ++;

}


if(tamanio_bitarray_sp % 8 > 0){

				forzar_bitarray++;




}


		tabla_de_segmentos_sp = list_create();

		puntero_memoria_sp = malloc(config.TAMANIO);
		memset(puntero_memoria_sp, '~', config.TAMANIO);


		b_m_s = calloc(1,forzar_bitarray);

		bitarray_memoria = bitarray_create(b_m_s,forzar_bitarray);

		int max_bit = bitarray_get_max_bit(bitarray_memoria);
		printf("Tamanio del bitarray: %d\n", max_bit);

		printf("Cantidad de paginas: %d \n", tamanio_bitarray_sp);
		if(puntero_memoria_sp == NULL){

			log_error(log_fm9, "No se pudo inicializar la memoria");
		}else{

			log_info(log_fm9, "Inicialización de memoria exitosa");
		}

}

int guardar_archivo_segmentacion_paginada(int pid ,int id,int cantidad_lineas,char* buffer_recepcion){
	int resultado;



	int paginas = cantidad_lineas * config.MAX_LINEA / config.TAM_PAGINA;


	if((cantidad_lineas * config.MAX_LINEA) % config.TAM_PAGINA > 0 ){
		paginas++;

	}

	printf("CANTIDAD DE PAGINAS : %d \n", paginas);
	int entra_memoria = entra_memoria_sp(paginas);

	printf("ENTRA MEMORIA : %d\n", entra_memoria);

		if(entra_memoria == 1){



		paginar_segmento(pid, id, cantidad_lineas, buffer_recepcion);
		log_info(log_fm9,"Se guardo el archivo ID: %d en memoria \n", id);
		resultado = OK;

		}else{

			log_error(log_fm9, "El archivo ID: %d no entra en memoria \n", id);
			resultado = ESPACIO_INSUFICIENTE;

		}




return resultado;






}

void paginar_segmento(int pid, int id, int cantidad_lineas, char* buffer_recepcion){

	segmento_paginado_t* segmento_nuevo = malloc(sizeof(segmento_paginado_t));
	int cantidad_paginas;


	segmento_nuevo->pid = pid;
	segmento_nuevo->id = id;
	segmento_nuevo->tabla_de_paginas_segmento = list_create();

	list_add(tabla_de_segmentos_sp,segmento_nuevo);





	cantidad_paginas = cantidad_lineas * config.MAX_LINEA / config.TAM_PAGINA  ;


	if((cantidad_lineas * config.MAX_LINEA) % config.TAM_PAGINA  > 0){

		cantidad_paginas++;
	}


	printf("CANTIDAD DE PAGINAS %d :  ID %d \n", cantidad_paginas, id);


	char*buffer_envio= malloc(cantidad_paginas * config.TAM_PAGINA);

	memcpy(buffer_envio, buffer_recepcion, cantidad_lineas * config.MAX_LINEA);

	asignar_segmento_paginado_vacio(cantidad_paginas, segmento_nuevo,  buffer_envio);





}

int entra_memoria_sp(int cantidad_paginas){
	int pagina = 0;
	int espacios_vacios=0;

	while (pagina < tamanio_bitarray_sp){

			if(!bitarray_test_bit(bitarray_memoria, pagina)){

				espacios_vacios++;
			}

			pagina++;
			}

	printf("CANTIDAD DE PAGINAS LIBRES %d \n", espacios_vacios);
			if(espacios_vacios < cantidad_paginas){


				return 0;
			}else{


				return 1;
			}

}
void asignar_segmento_paginado_vacio(int cantidad_paginas,segmento_paginado_t* segmento_nuevo, char* buffer_envio){

	int numero_frame= 0;
	int offset = 0;
	int acierto=0;


		while(numero_frame < tamanio_bitarray_sp && acierto < cantidad_paginas){

			if(!(bitarray_test_bit(bitarray_memoria, numero_frame))){

				acierto++;

				list_add(segmento_nuevo->tabla_de_paginas_segmento, numero_frame);

				log_info(log_fm9,  "Guardando pagina: %d\n", numero_frame);
				bitarray_set_bit(bitarray_memoria, numero_frame);

				memcpy(puntero_memoria_sp + (numero_frame) * config.TAM_PAGINA, buffer_envio + config.TAM_PAGINA * offset, config.TAM_PAGINA);
				offset++;
			}

			numero_frame++;

		}



}


void abrir_archivo_segmentacion_paginada(int socket_cpu, int id){
	segmento_paginado_t * segmento_buscado;
	int resultado, frame, tamanio, paginas, cantidad_lineas;
	int contador = 0;
	int offset = 0;




	bool es_id(segmento_paginado_t* entrada){

							return entrada->id == id;

				}

			segmento_buscado = list_find(tabla_de_segmentos_sp, (void*) es_id);

			if(segmento_buscado == NULL){

				resultado = ARCHIVO_NO_ABIERTO ;
				log_error(log_fm9, "El segmento no se encuentra en la tabla");
				send(socket_cpu, &resultado, sizeof(int)* 2, MSG_WAITALL);

			}else{

				log_info("Abriendo archivo %d", id);
			paginas = list_size(segmento_buscado->tabla_de_paginas_segmento);


			tamanio = config.TAM_PAGINA * paginas;
			cantidad_lineas = tamanio / config.MAX_LINEA;
			resultado = OK;

			char* buffer_envio = calloc(1, (sizeof(int)* 3) + tamanio + 1);

			memcpy(buffer_envio, &resultado, sizeof(int));
			memcpy(buffer_envio + sizeof(int), &tamanio, sizeof(int));

			while(contador < paginas){

			frame = list_get(segmento_buscado->tabla_de_paginas_segmento, contador);



			printf("FRAME DE PAGINA: %d", frame);


			memcpy(buffer_envio + sizeof(int)*2 + offset, puntero_memoria_sp + (frame * config.TAM_PAGINA), config.TAM_PAGINA);


			offset += config.TAM_PAGINA;
			contador++;
			}


			memcpy(buffer_envio+ sizeof(int)*2 + tamanio, &cantidad_lineas, sizeof(int));
			send(socket_cpu, buffer_envio, sizeof(int)* 3 + tamanio, MSG_WAITALL);
			free(buffer_envio);



}


}

void modificar_linea_segmentacion_paginada(int socket_cpu,int id,int numero_linea,char* linea_tratada){

	segmento_paginado_t * segmento_buscado;
	int resultado, frame;

	int numero_pagina = (config.MAX_LINEA * numero_linea / config.TAM_PAGINA );

		if((config.MAX_LINEA * numero_linea) % config.TAM_PAGINA > 0 ){

			numero_pagina++;

		}

		numero_pagina--;


		int corrimiento = numero_linea - (numero_pagina*config.TAM_PAGINA / config.MAX_LINEA) -1;

		bool es_id(segmento_paginado_t* entrada){

								return entrada->id == id;

					}

				segmento_buscado = list_find(tabla_de_segmentos_sp, (void*) es_id);

				if(segmento_buscado == NULL){

					resultado = ARCHIVO_NO_ABIERTO ;
					log_error(log_fm9, "El segmento ID: no se encuentra en la tabla\n");
				}else{

					if(list_size(segmento_buscado->tabla_de_paginas_segmento) * config.TAM_PAGINA/ config.MAX_LINEA < numero_linea ){

						log_error(log_fm9, "Segmentation Fault");
						resultado =SEGMENTATION_FAULT;
					}else{

					frame = list_get(segmento_buscado->tabla_de_paginas_segmento, numero_pagina);


					resultado = OK;

					log_info(log_fm9, "Modificando linea %d\n", numero_linea);




				memcpy(puntero_memoria_sp + (frame * config.TAM_PAGINA)+ (corrimiento * config.MAX_LINEA),linea_tratada, config.MAX_LINEA);

				}
				send(socket_cpu, &resultado, sizeof(int), MSG_WAITALL);

}
}
void flush_segmentacion_paginada(int socket_diego,int id){

	segmento_paginado_t * segmento_buscado = malloc(sizeof(segmento_paginado_t));
	int resultado, frame, cantidad_lineas;

	int offset = 0;
	int paginas;
	int numero_pagina = 0;




			bool es_id(segmento_paginado_t* entrada){

									return entrada->id == id;

						}

					segmento_buscado = list_find(tabla_de_segmentos_sp, (void*) es_id);

					if(segmento_buscado == NULL){

						resultado = ARCHIVO_NO_ABIERTO ;
						log_error(log_fm9, "El segmento no se encuentra en la tabla");
						send(socket_diego, &resultado, sizeof(int), MSG_WAITALL);

					}else{
						resultado = OK;

						log_info(log_fm9, "El segmento se encuentra en memoria");

					paginas = list_size(segmento_buscado->tabla_de_paginas_segmento);

					int tamanio_paginas = paginas * config.TAM_PAGINA;
					cantidad_lineas = tamanio_paginas / config.MAX_LINEA;
					char* buffer_envio = malloc((sizeof(int) * 3) + (paginas * config.TAM_PAGINA) + 1);

					memcpy(buffer_envio, &resultado, sizeof(int));
					memcpy(buffer_envio+ sizeof(int), &tamanio_paginas, sizeof(int));

					while(numero_pagina < paginas ){

						frame = list_get(segmento_buscado->tabla_de_paginas_segmento, numero_pagina);



						memcpy(buffer_envio + sizeof(int) *2 + offset ,puntero_memoria_sp + (frame * config.TAM_PAGINA), sizeof(config.TAM_PAGINA));



						numero_pagina++;
						offset += config.TAM_PAGINA;


					}

					memcpy(buffer_envio + (sizeof(int)*2) + (paginas*config.TAM_PAGINA), &cantidad_lineas, sizeof(int) );


					send(socket_diego, buffer_envio, (sizeof(int)*3) + (paginas* config.TAM_PAGINA), MSG_WAITALL);
					free(buffer_envio);



					}





}

void dump_segmentacion_paginada(int pid){
	segmento_paginado_t * segmento_buscado;
		t_list* lista_filtrada;

		int frame, aciertos;

		int offset = 0;
		int contador = 0;

		int paginas;
		int numero_pagina;




						bool es_pid(segmento_paginado_t* entrada){

													return entrada->pid == pid;

											}

						lista_filtrada = list_filter(tabla_de_segmentos_sp, (void*) es_pid);


						aciertos = list_size(lista_filtrada);

						if(aciertos >0){
						log_info(log_fm9, "------DUMP------\n");

						log_info(log_fm9, "PID PROCESO = %d\n", pid);

						char * buffer_muestra =  malloc(config.TAM_PAGINA);

						while(contador < aciertos){




								segmento_buscado = list_get(lista_filtrada, contador);



								log_info(log_fm9, "ID ARCHIVO = %d\n", segmento_buscado->id);



								paginas = list_size(segmento_buscado->tabla_de_paginas_segmento);






								numero_pagina=0;



								while(numero_pagina < paginas){

											frame = list_get(segmento_buscado->tabla_de_paginas_segmento, numero_pagina);




											log_info(log_fm9,"FRAME : %d de PAGINA %d \n", frame, numero_pagina);

											memcpy(buffer_muestra, puntero_memoria_sp + (frame * config.TAM_PAGINA), config.TAM_PAGINA);


											offset += config.TAM_PAGINA;
											printf("offset: %d\n", offset);
											log_info(log_fm9,"BUFFER PAGINA %d: %s \n",numero_pagina, buffer_muestra);
											numero_pagina++;
											}




								contador++;


						}

						}else{

							log_error(log_fm9, "El proceso: %d no se encuentra en memoria\n", pid);
						}

}



int close_file_segmentacion_paginada(int socket_cpu,int id){
	segmento_paginado_t * segmento_buscado;
	int resultado;
	int frame;

	int offset = 0;
	int paginas;
	int numero_pagina = 0;





				bool es_id(segmento_paginado_t* entrada){

										return entrada->id == id;

							}

						segmento_buscado = list_find(tabla_de_segmentos_sp, (void*) es_id);

						if(segmento_buscado == NULL){

							resultado = ARCHIVO_NO_ABIERTO;
							log_error(log_fm9, "El archivo ID:%d no se encuentra en la tabla \n", id);
							send(socket_cpu, &resultado, sizeof(int), MSG_WAITALL);

						}else{
							resultado = OK;

							log_info(log_fm9, "Se ha cerrado el archivo ID:%d \n", id);

						paginas = list_size(segmento_buscado->tabla_de_paginas_segmento);




						while(numero_pagina < paginas ){

						frame = list_get(segmento_buscado->tabla_de_paginas_segmento, numero_pagina);


						printf("FRAME BORRADO: %d \n", frame);


						memset(puntero_memoria_sp + frame * config.TAM_PAGINA, '~', config.TAM_PAGINA);

						bitarray_clean_bit(bitarray_memoria, frame);


						numero_pagina++;
						offset += config.TAM_PAGINA;


						}

						list_destroy(segmento_buscado->tabla_de_paginas_segmento);
						list_remove_by_condition(tabla_de_segmentos_sp, (void*) es_id);






						}


return resultado;




}

int close_process_segmentacion_paginada(int socket_cpu,int pid){

	segmento_paginado_t * segmento_buscado = malloc(sizeof(segmento_paginado_t));
	t_list* lista_filtrada;

	int resultado, aciertos;

	int contador = 0;





					bool es_pid(segmento_paginado_t* entrada){

												return entrada->pid == pid;

										}

					lista_filtrada = list_filter(tabla_de_segmentos_sp, (void*) es_pid);

					aciertos = list_size(lista_filtrada);

					if(aciertos == 0){
						log_error(log_fm9,"El proceso PID: %d no se encuentra en memoria", pid);
						resultado=ERROR;

					}else{
						log_info(log_fm9, "Se ha cerrado el proceso PID: %d", pid );

						while(contador < aciertos){

						segmento_buscado = list_get(lista_filtrada, contador);

						resultado = close_file_segmentacion_paginada(socket_cpu,segmento_buscado->id);


						contador++;

							}
						}




	list_destroy(lista_filtrada);
	return resultado;







}


//-------------------------------------------------------------------------------------------------------------------
//BITARRAY

void reservar_bitarray(t_bitarray* bitarray_memoria,int base,int limite){



	for(int i= base; i < (base + limite); i++){

	bitarray_set_bit(bitarray_memoria, i);
	printf("BIT SETEADO: %d \n", i);
	}


}

void liberar_bitarray(t_bitarray* bitarray_memoria,int base,int limite){

	for(int i= base; i < (base +limite); i++){

		bitarray_clean_bit(bitarray_memoria, i);
		log_info("BITARRAY LIBERADO : %d", i);

	}

}

//TABLA DE PAGINAS INVERTIDAS

void inicializar_memoria_paginacion_invertida(){


	int forzar_bitarray = (config.TAMANIO / config.TAM_PAGINA / 8);

tamanio_bitarray_paginada = config.TAMANIO / config.TAM_PAGINA;

if(config.TAMANIO % config.TAM_PAGINA > 0){


	tamanio_bitarray_sp ++;

}


if(tamanio_bitarray_sp % 8 > 0){

				forzar_bitarray++;




}



			tabla_de_paginas = list_create();

			puntero_memoria_paginada = malloc(config.TAMANIO);
			memset(puntero_memoria_paginada, '~', config.TAMANIO);


			b_m_s = calloc(1,forzar_bitarray);

			bitarray_memoria = bitarray_create(b_m_s,forzar_bitarray);

			int max_bit = bitarray_get_max_bit(bitarray_memoria);
			printf("Tamanio del bitarray: %d\n", max_bit);

			printf("Cantidad de paginas: %d \n", tamanio_bitarray_paginada);
			if(puntero_memoria_paginada == NULL){

				log_error(log_fm9, "No se pudo inicializar la memoria");
			}else{

				log_info(log_fm9, "Inicialización de memoria exitosa");
			}

	}



int guardar_archivo_paginas_invertidas(int pid,int id,int cantidad_lineas,char* buffer_recepcion){
	int resultado;

	int paginas = cantidad_lineas * config.MAX_LINEA / config.TAM_PAGINA;


		if((cantidad_lineas * config.MAX_LINEA) % config.TAM_PAGINA > 0 ){
			paginas++;

		}

		printf("CANTIDAD DE PAGINAS : %d \n", paginas);
		int entra_memoria = entra_memoria_paginada(paginas);

		printf("ENTRA MEMORIA : %d\n", entra_memoria);

			if(entra_memoria == 1){

				paginar(pid, id,cantidad_lineas,buffer_recepcion);

			//paginar_segmento(pid, id, cantidad_lineas, buffer_recepcion);
			log_info(log_fm9,"Se guardo el archivo ID: %d en memoria \n", id);
			resultado = OK;

			}else{

				log_error(log_fm9, "El archivo ID: %d no entra en memoria \n", id);
				resultado = ESPACIO_INSUFICIENTE;

			}




	return resultado;








}

void abrir_archivo_paginacion(int socket_cpu,int id){
		t_list* paginas_encontradas = NULL;
		entrada_tabla_invertida_t* entrada_encontrada;

		int resultado, frame, tamanio, paginas, cantidad_lineas;
		int contador = 0;
		int offset = 0;




		bool es_id(entrada_tabla_invertida_t* entrada){

								return entrada->id == id;

					}

				paginas_encontradas = list_filter(tabla_de_paginas, (void*) es_id);
				paginas= list_size(paginas_encontradas);

				if(paginas ==0){

					resultado = ARCHIVO_NO_ABIERTO ;
					log_error(log_fm9, "El archivo: %d no se encuentra en la tabla\n", id);
				}else{

					log_info(log_fm9,"Abriendo archivo %d\n", id);


				tamanio = config.TAM_PAGINA * paginas;

				cantidad_lineas = tamanio / config.MAX_LINEA;
				resultado = OK;

				char* buffer_envio = calloc(1,sizeof(int)* 3 + tamanio);

				memcpy(buffer_envio, &resultado, sizeof(int));
				memcpy(buffer_envio + sizeof(int), &tamanio, sizeof(int));

				while(contador < paginas){

					entrada_encontrada = list_get(paginas_encontradas, contador);


				frame = entrada_encontrada->frame;



				memcpy(buffer_envio + sizeof(int)*2 + offset, puntero_memoria_paginada + (frame * config.TAM_PAGINA), config.TAM_PAGINA);


				offset += config.TAM_PAGINA;
				contador++;
				}

				memcpy(buffer_envio+ sizeof(int)*2 + offset, &(cantidad_lineas), sizeof(int));

				send(socket_cpu, buffer_envio, sizeof(int)* 3 + offset, MSG_WAITALL);

				printf("BUFFER ABRIR: %s\n", buffer_envio+ sizeof(int)*3);


	}

	free(buffer_envio);
	}




int close_file_paginacion(int socket_cpu,int id){

	t_list* paginas_encontradas = NULL;
	entrada_tabla_invertida_t * entrada_buscada;
	int resultado;
	int frame;
	int paginas;


	int numero_pagina = 0;





					bool es_id(entrada_tabla_invertida_t* entrada){

											return entrada->id == id;

								}

					paginas_encontradas = list_filter(tabla_de_paginas,  es_id);

					paginas= list_size(paginas_encontradas);

							if(paginas == 0){

								resultado = ARCHIVO_NO_ABIERTO;
								log_error(log_fm9, "El archivo ID:%d no se encuentra en la tabla \n", id);
								send(socket_cpu, &resultado, sizeof(int), MSG_WAITALL);

							}else{
								resultado = OK;

								log_info(log_fm9, "Se ha cerrado el archivo ID:%d \n", id);



							while(numero_pagina < paginas ){

							entrada_buscada = list_get(paginas_encontradas, numero_pagina);

							frame = entrada_buscada->frame;


							memset(puntero_memoria_paginada + (frame * config.TAM_PAGINA),'~',config.TAM_PAGINA);

							bitarray_clean_bit(bitarray_memoria, frame);


							numero_pagina++;



							}

							list_destroy(paginas_encontradas);
							list_remove_by_condition(tabla_de_paginas, es_id);






							}


	return resultado;







}


int close_process_paginacion(int socket_cpu,int pid){

	int resultado, aciertos;
	entrada_tabla_invertida_t * entrada_buscada = malloc(sizeof(entrada_tabla_invertida_t));
		t_list* lista_filtrada = NULL;



		int contador = 0;





						bool es_pid(entrada_tabla_invertida_t* entrada){

													return entrada->pid == pid;

											}

						lista_filtrada = list_filter(tabla_de_paginas, (void*) es_pid);

						aciertos = list_size(lista_filtrada);

						if(aciertos == 0){
							log_error(log_fm9,"El proceso PID: %d no se encuentra en memoria", pid);
							resultado=ERROR;

						}else{
							log_info(log_fm9, "Se ha cerrado el proceso PID: %d", pid );

							while(contador < aciertos){

							entrada_buscada = list_get(lista_filtrada, contador);


							 resultado = close_file_paginacion(socket_cpu,entrada_buscada->id);


							contador++;

								}
							}



		return resultado;








}


void modificar_linea_paginacion(int socket_cpu,int id,int numero_linea,char* linea_tratada){
	t_list* lista_filtrada = NULL;
	entrada_tabla_invertida_t* entrada_encontrada;
	int resultado, frame;


	int numero_pagina = (config.MAX_LINEA * numero_linea / config.TAM_PAGINA );

		if((config.MAX_LINEA * numero_linea) % config.TAM_PAGINA > 0 ){

			numero_pagina++;

		}

		numero_pagina--;



	int corrimiento = numero_linea - (numero_pagina*config.TAM_PAGINA / config.MAX_LINEA) -1;



			bool es_id(entrada_tabla_invertida_t* entrada){

									return entrada->id == id;

						}

			lista_filtrada = list_filter(tabla_de_paginas, (void*) es_id);

					if(list_size(lista_filtrada) == 0){

						resultado = ARCHIVO_NO_ABIERTO ;
						log_error(log_fm9, "El archivo ID:%d no se encuentra en la tabla\n", id);
					}else{
						if(list_size(lista_filtrada)*config.TAM_PAGINA / config.MAX_LINEA < numero_linea ){
							log_error(log_fm9, "Segmentation Fault");
							resultado = SEGMENTATION_FAULT;


						}else{


						log_info(log_fm9, "Modificando linea %d\n", numero_linea);
						entrada_encontrada =list_get(lista_filtrada, numero_pagina );

						frame = entrada_encontrada->frame;
						printf("TAMANIO: %d\n", list_size(lista_filtrada));
						printf("FRAME: %d\n",frame);
						printf("NUMERO DE PAGINA: %d\n", numero_pagina);
						printf("CORRIMIENTO: %d\n", corrimiento);
                    resultado = OK;

					memcpy(puntero_memoria_paginada + (frame * config.TAM_PAGINA)+ (corrimiento * config.MAX_LINEA),linea_tratada, config.MAX_LINEA);

					}
					send(socket_cpu, &resultado, sizeof(int), MSG_WAITALL);


}


}

void flush_paginacion_invertida(int socket_diego,int id){
			t_list* paginas_encontradas = NULL;
			entrada_tabla_invertida_t* entrada_encontrada;

			int resultado, frame, tamanio, paginas, cantidad_lineas;
			int contador = 0;
			int offset = 0;

			bool es_id(entrada_tabla_invertida_t* entrada){

									return entrada->id == id;

						}

					paginas_encontradas = list_filter(tabla_de_paginas, (void*) es_id);
					paginas= list_size(paginas_encontradas);

					if(paginas ==0){

						resultado = ARCHIVO_NO_ABIERTO ;
						log_error(log_fm9, "El archivo: %d no se encuentra en la tabla\n", id);

						send(socket_diego, &resultado,sizeof(int), MSG_WAITALL );
					}else{

						log_info("Abriendo archivo %d\n", id);


					tamanio = config.TAM_PAGINA * paginas;
					cantidad_lineas = tamanio / config.MAX_LINEA;
					resultado = OK;

					char* buffer_envio = malloc(sizeof(int)* 3 + tamanio +1);

					memcpy(buffer_envio, &resultado, sizeof(int));
					memcpy(buffer_envio + sizeof(int), &tamanio, sizeof(int));

					while(contador < paginas){

						entrada_encontrada = list_get(paginas_encontradas, contador);


					frame = entrada_encontrada->frame;



					memcpy(buffer_envio + sizeof(int)*2 + offset, puntero_memoria_paginada + (frame * config.TAM_PAGINA), config.TAM_PAGINA);


					offset += config.TAM_PAGINA;
					contador++;
					}


					memcpy(buffer_envio + sizeof(int)*2 + offset, &cantidad_lineas, sizeof(int));
					send(socket_diego, buffer_envio, sizeof(int)* 3 + config.MAX_LINEA * paginas, MSG_WAITALL);
					free(buffer_envio);
					list_destroy(paginas_encontradas);


		}









}


void dump_paginacion_invertida(int pid){
	entrada_tabla_invertida_t * entrada_buscada;
			t_list* lista_filtrada;







			int offset = 0;
							int contador = 0;

							int paginas;





							bool es_pid(entrada_tabla_invertida_t* entrada){

														return entrada->pid == pid;

												}

							lista_filtrada = list_filter(tabla_de_paginas, (void*) es_pid);




							paginas = list_size(lista_filtrada);

							printf("TAMANIO LISTA : %d", paginas);

							if(paginas >0){
							log_info(log_fm9, "------DUMP------\n");

							log_info(log_fm9, "PID PROCESO = %d\n", pid);

							char * buffer_muestra = malloc(config.TAM_PAGINA);

							while(contador < paginas){
								int frame = 0;




								entrada_buscada = list_get(lista_filtrada, contador);

								frame = entrada_buscada->frame;
								log_info(log_fm9, "ID ARCHIVO = %d\n", entrada_buscada->id);


								log_info(log_fm9,"FRAME : %d de PAGINA %d \n", frame, contador);

								memcpy(buffer_muestra, puntero_memoria_paginada + (frame * config.TAM_PAGINA), config.TAM_PAGINA);


								log_info(log_fm9,"CONTENIDO FRAME %d: %s \n",frame, buffer_muestra);


								contador++;
								offset+= config.TAM_PAGINA;

							}


							}else{

								log_error(log_fm9, "El proceso: %d no se encuentra en memoria\n", pid);
							}


list_destroy(lista_filtrada);


}



int entra_memoria_paginada(int cantidad_paginas){
	int pagina = 0;
	int espacios_vacios=0;

	while (pagina < tamanio_bitarray_paginada){

			if(!bitarray_test_bit(bitarray_memoria, pagina)){

				espacios_vacios++;
			}

			pagina++;
			}

	printf("CANTIDAD DE PAGINAS LIBRES %d \n", espacios_vacios);
			if(espacios_vacios < cantidad_paginas){


				return 0;
			}else{


				return 1;
			}

}

void paginar(int pid, int id, int cantidad_lineas, char* buffer_recepcion){

	int offset=0;
	int contador=0;
	int n_frame=0;




	int cantidad_paginas = cantidad_lineas * config.MAX_LINEA / config.TAM_PAGINA;

	if((cantidad_lineas * config.MAX_LINEA) % config.TAM_PAGINA  > 0){

			cantidad_paginas++;
		}


		printf("CANTIDAD DE PAGINAS %d :  ID %d \n", cantidad_paginas, id);


		char*buffer_envio= malloc(cantidad_paginas * config.TAM_PAGINA);

		memset(buffer_envio, "~", cantidad_paginas*config.TAM_PAGINA);

		memcpy(buffer_envio, buffer_recepcion, cantidad_lineas * config.MAX_LINEA);

		printf("ESTO SE ESTA GUARDANDO: %s\n", buffer_envio);




		while(n_frame < tamanio_bitarray_paginada && contador < cantidad_paginas){



					if(!(bitarray_test_bit(bitarray_memoria, n_frame))){

						entrada_tabla_invertida_t* entrada_tabla= malloc(sizeof(entrada_tabla_invertida_t));

						entrada_tabla->pid = pid;
						entrada_tabla->id = id;


						entrada_tabla->frame = n_frame;

						list_add(tabla_de_paginas, entrada_tabla);

						log_info(log_fm9,  "Guardando pagina en frame: %d \n", entrada_tabla->frame);
						bitarray_set_bit(bitarray_memoria, n_frame);

						memcpy(puntero_memoria_paginada + (n_frame * config.TAM_PAGINA), buffer_envio +  offset, config.TAM_PAGINA);
						offset+= config.TAM_PAGINA;
						contador++;

					}

					n_frame++;

				}

















}



//---------------------------------------------------------------------------------------------------------------
//DICCIONARIO


void inicializar_diccionario(){


	diccionario = list_create();






}

int transformar_path(char* path, int pid)
{
	int id;
	char* path_diccionario = strdup(path);

	bool _path_coincide_al_dado(dataDePath* data)
	{
		return (strcmp(data->path, path_diccionario) == 0);
	}

	bool _path_esta_asignado_al_proceso_dado(dataDePath* data)
	{
		return (data->pid == pid);
	}

	t_list* filtroPorPath = NULL;
	t_list* filtroPathPorPid = NULL;
	dataDePath* data = NULL;

	filtroPorPath = list_filter(diccionario, _path_coincide_al_dado);

	if(filtroPorPath == 0)	//El archivo no esta en el diccionario
	{
		id = agregarPathAlDiccionario(path_diccionario, pid);
	}
	else if(filtroPorPath > 0) //El archivo esta en el diccionario. Tengo que ver si esta asignado al proceso dado; si no lo esta, agrego una entrada
	{
		filtroPathPorPid = list_filter(filtroPorPath, _path_esta_asignado_al_proceso_dado);

		if(list_size(filtroPathPorPid) == 0)
		{
			id = agregarPathAlDiccionario(path_diccionario, pid);
		}
		else
		{
			data = list_get(filtroPathPorPid, 0);
			id = data->idSegmento;
		}
	}

	return id;
}

int agregarPathAlDiccionario(char* path, int pid)
{
	dataDePath* data = calloc(1, sizeof(dataDePath));

	data->idSegmento = id_nuevo();
	data->path = path;
	data->pid = pid;

	list_add(diccionario, data);

	return data->idSegmento;
}

int id_nuevo(){

	id_global++;

	return id_global;
}

bool eliminar_id_segmento_de_diccionario(int idSegmento, char* path, int pid)
{
	dataDePath* datax = NULL;

	bool _es_id(dataDePath* data)
	{
		return data->idSegmento == idSegmento;
	}

	datax = list_remove_by_condition(diccionario, _es_id);

	if((datax->pid == pid) && (strcmp(datax->path, path) == 0))	//Si el id que saque de la lista esta correcto, deberia terner el mismo path y pid que los pasados a esta funcion
	{
		log_info(log_fm9, "Se elimino una entrada del diccionario de bloques perteneciente al bloque %d", datax->idSegmento);
		return true;	//Si es asi, salio todo bien
	}
	else
	{
		log_warning(log_fm9, "Se intento eliminar la entrada del bloque %d del diccionario de bloques, pero su path y pid no coinciden con los solicitados", datax->idSegmento);
		list_add(diccionario, datax);	//Si el path y pid del elemento obtenido no coinciden con los pasados a esta funcion, vuelvo a meter el
		return false;					//elemento en el diccionario y lo informo (devolviendo falso)
	}
}

