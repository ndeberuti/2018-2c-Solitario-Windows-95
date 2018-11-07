/*
 * FM9.c
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */
#include "FM9.h"

int main(void) {
	//simulacion de serializacion/deserializacion
	void* buffer_envio;



	system("clear");
	puts("PROCESO FM9\n");

	log_consola = init_log(PATH_LOG, "Consola FM9", false, LOG_LEVEL_INFO);
	log_fm9 = init_log(PATH_LOG, "Proceso FM9", true, LOG_LEVEL_INFO);
	log_info(log_fm9, "Inicio del proceso");

	config = load_config();
	
	setear_modo();
	//inicializar_memoria();



	//serializar(buffer_envio);


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
			} // if (FD_ISSET(i, &read_fds))
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

void inicializar_memoria_segmentacion_simple(){
	//tabla de segmentos

	tabla_de_segmentos = list_create();

	//alocacion de memoria
	numero_lineas_memoria = obtener_cantidad_lineas(config.TAMANIO);

	puntero_memoria_segmentada = malloc(config.TAMANIO);





	if(puntero_memoria_segmentada == NULL){
		log_error(log_fm9, "Puntero de memoria a NULL");
	}

	log_info(log_fm9, "Alocacion exitosa");


}

void guardar_proceso_segmentacion_simple(int pid ,int longitud_paquete, char* buffer_recepcion){

	segmento_offset_t* segmento = malloc(sizeof(segmento_offset_t));
	segmento_tabla_t* entrada_tabla = malloc(sizeof(segmento_tabla_t));




	segmento->segmento = pid;
	segmento->offset = longitud_paquete;

	if(segmento->offset > entrada_tabla->limite){
	log_error(log_fm9, "Segmentation Fault. El offset es mas grande que el limite");
	}

	if(list_size(tabla_de_segmentos) == 0){



							entrada_tabla->pid = segmento->segmento;
							entrada_tabla->id = 0;
							entrada_tabla->base = 0;
							entrada_tabla->limite = segmento->offset;
						 	list_add(tabla_de_segmentos, entrada_tabla);
	}else{

							entrada_tabla->pid = segmento->segmento;
							entrada_tabla->limite = segmento->offset;
							entrada_tabla->base = obtener_base_de_tabla(segmento->offset);
							entrada_tabla->id = obtener_id_de_tabla();
							list_add(tabla_de_segmentos, entrada_tabla);







							while(segmento->offset * config.MAX_LINEA <= longitud_paquete){
							memcpy(puntero_memoria_segmentada+ entrada_tabla->base, buffer_recepcion,config.MAX_LINEA);
							entrada_tabla->base+= config.MAX_LINEA;

							}
	}



}

	int obtener_limite_de_tabla(int pid){
		int limite;
		segmento_tabla_t* buffer = malloc(sizeof(segmento_tabla_t));

		buffer = list_find(tabla_de_segmentos,  pid_segmento(segmento, pid_segmento));
		limite = buffer->limite;


		return limite;
	}

	int pid_segmento(segmento_tabla_t segmento, int pid_segmento){
		if (pid_segmento == segmento.id){

			return 1;

		}else{

			return 0;

		}

	}

	int obtener_base_de_tabla(int pid){
		int base;
		//TODO
		return base;
	}




void setear_segmentacion_simple(){
	log_info(log_fm9, "Segmentación Simple seteada");
	inicializar_memoria_segmentacion_simple();
}

void setear_paginacion_invertida(){
	log_info(log_fm9, "Tablas de Paginación Invertida seteada");
}

void setear_segmentacion_paginada(){
	log_info(log_fm9, "Segmentación Paginada seteada");
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
/*
void recibir_proceso(int socket){
	int pid,longitud_paquete= 0;
	void * buffer_recepcion;

	recv(socket, &pid, sizeof(int), MSG_WAITALL);
	recv(socket, &longitud_paquete, sizeof(int), MSG_WAITALL);


	buffer_recepcion = malloc(longitud_paquete);


	recv(socket, buffer_recepcion, longitud_paquete, MSG_WAITALL);

	//guardar_proceso(pid, longitud_paquete, buffer_recepcion);
}





void devolver_proceso(int pid, int longitud_paquete){

	int offset = sizeof(int)*2;
	int cantidad_lineas = obtener_cantidad_lineas(longitud_paquete);
	void * buffer_envio = malloc((cantidad_lineas * config.MAX_LINEA) + sizeof(int)*2);

	memcpy(buffer_envio, &pid, sizeof(int));
	memcpy(buffer_envio + sizeof(int), &longitud_paquete, sizeof(int));

	int lineas_copiadas = 0;
	while (lineas_copiadas != cantidad_lineas) {
		//memcpy(buffer_envio + offset, memory_pointer, config.MAX_LINEA);
		offset += config.MAX_LINEA;
		lineas_copiadas++;
	}

	send(diego, buffer_envio, (cantidad_lineas * config.MAX_LINEA) + sizeof(int)*2, MSG_WAITALL);
	//TODO Enviar el proceso de a partes con tamaño transfer_size
	//TODO Descomentar esto cuando se implemente el transfer_size en ElDiego
	//send(diego, buffer_envio, config.TRANSFER_SIZE, MSG_WAITALL);
	free(buffer_envio);
}




*/



/*

void serializar(void * buffer_envio){

	int longitud = 5;
	char * palabra = "hola";



	buffer_envio = malloc(sizeof(int)+ 5);

	memcpy(buffer_envio, &longitud, sizeof(int));

	memcpy(buffer_envio + sizeof(int), &palabra, 5);

}

void deserializar(void* buffer, int tamanio){
int offset = 0;
void* buffer_envio;


serializar(buffer_envio);

memcpy(&prueba->numero, buffer_envio, sizeof(int));

offset =+ sizeof(int);

memcpy(&prueba->palabra, buffer_envio + offset, prueba->numero);

free(buffer_envio);

log_info(log_fm9,"%d", prueba->numero);

}

*/



