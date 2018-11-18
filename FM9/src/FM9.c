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

	log_consola = init_log(PATH_LOG, "Consola FM9", false, LOG_LEVEL_INFO);
	log_fm9 = init_log(PATH_LOG, "Proceso FM9", true, LOG_LEVEL_INFO);
	log_info(log_fm9, "Inicio del proceso");

	config = load_config();
	
	setear_modo();


	pthread_create(&thread_consola, NULL, (void *) consola, NULL);

	pthread_join(thread_consola, NULL);

	pthread_join(thread_servidor, NULL);

	exit(EXIT_SUCCESS);
	
	//free();


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
	uint32_t fdmax; // número máximo de descriptores de fichero
	uint32_t newfd; // descriptor de socket de nueva conexión aceptada
	uint32_t command; // comando del cliente
	uint32_t nbytes;
	uint32_t addrlen;
	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);

	// obtener socket a la escucha
	uint32_t servidor = build_server(config.PUERTO, log_consola);

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
		for (uint32_t i = 0; i <= fdmax; i++)
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

void command_handler(uint32_t command, uint32_t socket) {
	switch (command) {
	case NUEVA_CONEXION_DIEGO:
		log_info(log_consola, "Nueva conexion desde El Diego");
		diego = socket;
		break;
	case NUEVA_CONEXION_CPU:
		log_info(log_consola, "Nueva conexion de CPU");
		break;
	case GUARDAR_PROCESO:
		log_info(log_consola, "Guardando proceso...");
		guardar_proceso(socket);
		break;
	case DEVOLVER_PROCESO:
		log_info(log_consola, "Devolviendo proceso...");
		devolver_proceso(socket);
		break;
	default:
		log_warning(log_consola, "%d: Comando recibido incorrecto", command);
	}
}

void consola() {
	char *linea;
	char *token;
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
						// TODO: comando dump
					}

				else
					print_c(log_consola, "%s: Comando incorrecto\n", consola->comando);

				free(consola->comando);
				for (uint32_t i = 0; i < consola->cant_params; i++)
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
	else if(strcmp("SPI", config.MODO)== 0){
		setear_segmentacion_paginada();
	}
	else {
		log_error(log_fm9, "Modo de Gestión de Memoria desconocido");
	}
}

int obtener_cantidad_lineas(int longitud_paquete){


	return (longitud_paquete + config.MAX_LINEA - 1) / config.MAX_LINEA;

}

void guardar_proceso(int socket_diego){

	int pid = recibir_int(socket_diego);
	int cantidad_lineas = recibir_int(socket_diego);
	char* buffer_recepcion = malloc(cantidad_lineas * config.MAX_LINEA);
	buffer_recepcion= recibir_char(socket_diego, cantidad_lineas * config.MAX_LINEA);


	if(strcmp("SEG", config.MODO)== 0){
		guardar_proceso_segmentacion_simple(pid ,cantidad_lineas,buffer_recepcion);
		}
		else if(strcmp("TPI", config.MODO)== 0){
		//TODO guardar_proceso_paginas_invertidas(pid ,longitud_paquete,buffer_recepcion);
		}
		else if(strcmp("SPI", config.MODO)== 0){
		//TODO guardar_proceso_segmentacion_paginada(pid ,longitud_paquete, buffer_recepcion);
		}
		else {
			log_error(log_fm9, "Modo de Gestión de Memoria desconocido");
		}

}

void devolver_proceso(int socket_diego){


	int pid = recibir_int(socket_diego);



	if(strcmp("SEG", config.MODO)== 0){
			devolver_proceso_segmentacion_simple(socket_diego, pid);
			}
			else if(strcmp("TPI", config.MODO)== 0){
			//TODO devolver_proceso_paginas_invertidas(socket_diego, pid);
			}
			else if(strcmp("SPI", config.MODO)== 0){
			//TODO  devolver_proceso_segmentacion_paginada(socket_diego, pid);
			}
			else {
			//TODO  log_error(log_fm9, "Modo de Gestión de Memoria desconocido");
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

	log_error(log_fm9, "Error en la repcecepción de mensaje");
	return "Error";}

	else{

		return buffer_recepcion;
	}


}

//---------------------------------------------------------------------------------------------------------
//SEGMENTACION SIMPLE

void inicializar_memoria_segmentacion_simple(){
	//tabla de segmentos

	tabla_de_segmentos = list_create();


	puntero_memoria_segmentada = malloc(config.TAMANIO);
	bitarray_memoria = bitarray_create(b_m_s,config.TAMANIO / config.MAX_LINEA);

	for(int i = 0; i < config.TAMANIO / config.MAX_LINEA; i++){
	bitarray_clean_bit(bitarray_memoria,  i);
	}

	if(puntero_memoria_segmentada == NULL){

		log_error(log_fm9, "No se pudo inicializar la memoria");
	}else{

		log_info(log_fm9, "Inicialización de memoria exitosa");
	}

}

void guardar_proceso_segmentacion_simple(int pid ,int cantidad_lineas, char* buffer_recepcion){

segmento_offset_t* segmento = malloc(sizeof(segmento_offset_t));
segmento_offset_t* segmento_nuevo = malloc(sizeof(segmento_offset_t));
segmento_tabla_t* entrada_tabla = malloc(sizeof(segmento_tabla_t));
entrada_administrativa_segmentacion_t* entrada_administrativa = malloc(sizeof(entrada_administrativa_segmentacion_t));


segmento->offset = cantidad_lineas;

entrada_administrativa->pid = pid;




	if(list_is_empty(tabla_de_segmentos)){



							entrada_administrativa->id = 0;

							entrada_tabla->id = 0;
							entrada_tabla->base = 0 ;
							entrada_tabla->limite = segmento->offset;


							reservar_bitarray(bitarray_memoria, entrada_tabla->base, entrada_tabla->limite);

							list_add(tabla_administrativa_segmentacion, entrada_administrativa);
							list_add(tabla_de_segmentos, entrada_tabla);
	}else{

				if(entra_en_memoria(cantidad_lineas) == 1){
					int corrimiento = 0;

					while( cantidad_lineas != 0){


					entrada_tabla->id = asignar_id();

					entrada_administrativa->id = entrada_tabla->id;


					segmento_nuevo = buscar_segmento();
					entrada_tabla->base = segmento_nuevo->segmento;

								if((segmento_nuevo->segmento - segmento_nuevo->offset) > cantidad_lineas){


									entrada_tabla->limite = cantidad_lineas;

								}else{

									entrada_tabla->limite = segmento_nuevo->offset;
								}

					memcpy(puntero_memoria_segmentada + entrada_tabla->base * config.MAX_LINEA, buffer_recepcion + corrimiento,entrada_tabla->limite * config.MAX_LINEA);



					reservar_bitarray(bitarray_memoria, entrada_tabla->base, entrada_tabla->limite);


					list_add(tabla_de_segmentos, entrada_tabla);
					list_add(tabla_administrativa_segmentacion, entrada_administrativa);

					cantidad_lineas =- entrada_tabla ->limite;

					corrimiento =+ entrada_tabla->limite;
					}







					}else{


					}log_error(log_fm9, "Archivo no entra en memoria");

free(segmento);
free(entrada_tabla);

}

}

int entra_en_memoria(int cantidad_lineas){
int espacio_libre = 0;


	for(int j = 0; j < config.TAMANIO; j++){

		if(bitarray_test_bit(bitarray_memoria, j))
			espacio_libre++;

	}
		if( espacio_libre < cantidad_lineas ){

		return 0;
		}else{

			return 1;
		}
}



void liberar_segmento(int pid, int base, int limite){
	int id_segmento;

	entrada_administrativa_segmentacion_t* entrada_admin;


	liberar_bitarray(bitarray_memoria, base, limite);


	bool id_pid(entrada_administrativa_segmentacion_t* entrada){

				return entrada_admin->pid == pid;
			}



	entrada_admin = list_find(tabla_administrativa_segmentacion,(void*)id_pid);
	id_segmento = entrada_admin->id;
	free(entrada_admin);

	bool es_pid(segmento_tabla_t* entrada_segmento){


						return entrada_segmento->id == id_segmento;
					}



	list_remove_by_condition(tabla_de_segmentos,(void*) id_pid);
	list_remove_by_condition(tabla_administrativa_segmentacion, (void*)es_pid);

	liberar_bitarray(bitarray_memoria, base, limite);
}


void devolver_proceso_segmentacion_simple(int socket_diego, int pid){

	int offset = 0;
	void* buffer_envio;
	segmento_tabla_t* segmento_envio;

	buscar_segmento_2(pid, segmento_envio);

	buffer_envio = malloc(segmento_envio->limite + (sizeof(int)*2+ sizeof(char*)));

	//serializo PID
	memcpy(buffer_envio + offset, &pid, sizeof(int));
	offset += sizeof(int);
	//serializo longitud

	memcpy(buffer_envio + offset , &segmento_envio->limite, sizeof(int));
	offset += sizeof(int);

	//serializo segmento

	memcpy(buffer_envio + offset, puntero_memoria_segmentada + segmento_envio->base, segmento_envio->limite);

	//TODO enviar
	liberar_segmento(pid, segmento_envio->base, segmento_envio->limite);


	free(buffer_envio);

}



void buscar_segmento_2(int pid, segmento_tabla_t* segmento){

	entrada_administrativa_segmentacion_t* entrada_administrativa = malloc(sizeof(entrada_administrativa_segmentacion_t));
	int id_segmento;




		bool id_pid(entrada_administrativa_segmentacion_t* entrada){
			int pid_entrada;
			entrada = malloc(sizeof(entrada_administrativa_segmentacion_t));
			pid_entrada = entrada->pid;
			free(entrada);

			return pid == pid_entrada;
		}



		entrada_administrativa = list_find(tabla_administrativa_segmentacion, (void*)id_pid);
		id_segmento = entrada_administrativa->id;
		free(entrada_administrativa);

		bool es_pid(segmento_tabla_t* entrada_segmento){


					return entrada_segmento->id == id_segmento;
				}
		segmento = list_find(tabla_de_segmentos, (void*)es_pid);



}


segmento_offset_t buscar_segmento(){

segmento_offset_t segmento;



int base=0;
int otra_base;



	while (!(bitarray_test_bit(bitarray_memoria, base)) && base < config.TAMANIO ){


	base++;

	}

	otra_base = base;

	while(bitarray_test_bit(bitarray_memoria, base) &&  base < config.TAMANIO ){

	otra_base++;


	}

	segmento.segmento = base;
	segmento.offset = otra_base;

return segmento;
}


int asignar_id(){
	id_segmento++;

	return id_segmento;
}





//-------------------------------------------------------------------------------------------------------------------------
//PAGINACION INVERTIDA



void inicializar_memoria_paginacion_invertida(){
	//Logear que inicializamos
	int frames = config.TAMANIO / config.TAM_PAGINA;
	crearMemoriaPrincipal(frames, config.TAM_PAGINA);
	crearEstructurasAdministrativas(frames);
	//Necesito una estructura que me guarde la ultima pagina de cada proceso
	inicializarEstructuraAdicional();

}

void crearMemoriaPrincipal(int frames,int tamanio_pagina){




	puntero_memoria_paginada = malloc(sizeof(t_memoria_principal));

	puntero_memoria_paginada->memoria=malloc(config.TAMANIO);
	puntero_memoria_paginada->estructura_administrativa= puntero_memoria_paginada->memoria; // hito en la humanida'
	puntero_memoria_paginada->frames= puntero_memoria_paginada->memoria;



}








/*
void inicializar_ta---------------------------------------------------------------------------
//SEbla_de_paginas(int numero_lineas_memoria){
	segmento_tabla_t* entrada_vacia = malloc(sizeof(segmento_tabla_t));

	entrada_vacia->base=0;
	entrada_vacia->limite=0;



	for(int i=0; i < numero_lineas_memoria; i++){
		int j= 0;

		entrada_vacia->id = j;
		list_add(tabla_de_segmentos, entrada_vacia);
		j++;

	}

free(entrada_vacia);

}
*/



//-------------------------------------------------------------------------------------------------------------------
//SEGMENTACION PAGINADA

void inicializar_memoria_segmentacion_paginada(){



}

//-------------------------------------------------------------------------------------------------------------------
//BITARRAY

void reservar_bitarray(t_bitarray* bitarray_memoria_segmentada,int base,int limite){

	for(int i= 0; i <= limite; i++){

		bitarray_set_bit(bitarray_memoria_segmentada, i);

	}


}

void liberar_bitarray(t_bitarray* bitarray_memoria_segmentada,int base,int limite){

	for(int i= 0; i <= limite; i++){

		bitarray_clean_bit(bitarray_memoria_segmentada, i);

	}

}




