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

	log_fm9 = init_log(PATH_LOG, "Proceso FM9", true, LOG_LEVEL_INFO);
	log_info(log_fm9, "Inicio del proceso");

	config = load_config();

	inicializar_memoria();

	//serializar(buffer_envio);

	pthread_create(&thread_servidor, NULL, (void *) server, NULL);
	pthread_join(thread_servidor, NULL);

	exit(EXIT_SUCCESS);

	free(memory_pointer);
}

config_t load_config() {
	t_config *config = config_create(PATH_CONFIG);

	config_t miConfig;
	miConfig.PUERTO = config_get_int_value(config, "PUERTO");
	miConfig.MODO = strdup(config_get_string_value(config, "MODO"));
	miConfig.TAMANIO = config_get_int_value(config, "TAMANIO");
	miConfig.MAX_LINEA = config_get_int_value(config, "MAX_LINEA");
	miConfig.TAM_PAGINA = config_get_int_value(config, "TAM_PAGINA");

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
	uint32_t servidor = build_server(config.PUERTO, log_fm9);

	// añadir listener al conjunto maestro
	FD_SET(servidor, &master);
	// seguir la pista del descriptor de fichero mayor
	fdmax = servidor; // por ahora es éste

	// bucle principal
	while (true) {
		read_fds = master; // cópialo
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			log_error(log_fm9, "select");
			exit(EXIT_FAILURE);
		}
		// explorar conexiones existentes en busca de datos que leer
		for (uint32_t i = 0; i <= fdmax; i++)
			if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
				if (i == servidor) {
					// gestionar nuevas conexiones
					addrlen = sizeof(remoteaddr);
					if ((newfd = accept(servidor, (struct sockaddr *) &remoteaddr, &addrlen)) == -1)
						log_error(log_fm9, "accept");
					else {
						FD_SET(newfd, &master); // añadir al conjunto maestro
						if (newfd > fdmax) // actualizar el máximo
							fdmax = newfd;
						log_info(log_fm9, "Nueva conexion desde %s en el socket %d", inet_ntoa(remoteaddr.sin_addr), newfd);
					}
				}
				else
					// gestionar datos de un cliente
					if ((nbytes = receive_int(i, &command)) <= 0) {
						// error o conexión cerrada por el cliente
						if (nbytes == 0)
							// conexión cerrada
							log_info(log_fm9, "Socket %d colgado", i);
						else
							log_error(log_fm9, "recv (comando)");

						close(i); // ¡Hasta luego!
						FD_CLR(i, &master); // eliminar del conjunto maestro
					}
					else
						// tenemos datos de algún cliente
						command_handler(command);
			} // if (FD_ISSET(i, &read_fds))
	} // while (true)
}

void command_handler(uint32_t command) {
	switch (command) {

	default:
		log_warning(log_fm9, "%d: Comando recibido incorrecto", command);
	}
}


void inicializar_memoria(){
	//alocacion de memoria

	memory_pointer  = malloc(config.TAMANIO);

	if(memory_pointer == NULL){
		log_error(log_fm9, "Puntero de memoria a NULL");
	}

	log_info(log_fm9, "Alocacion exitosa");

	setear_modo();
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

void setear_segmentacion_simple(){
	log_info(log_fm9, "Segmentación Simple seteada");
}

void setear_paginacion_invertida(){
	log_info(log_fm9, "Tablas de Paginación Invertida seteada");
}

void setear_segmentacion_paginada(){
	log_info(log_fm9, "Segmentación Paginada seteada");
}


void recibir_proceso(int socket){
	int pid,longitud_paquete= 0;
	void * buffer_recepcion;

	recv(socket, &pid, sizeof(int), MSG_WAITALL);
	recv(socket, &longitud_paquete, sizeof(int), MSG_WAITALL);
	recv(socket, buffer_recepcion, longitud_paquete, MSG_WAITALL);

	guardar_proceso(pid, longitud_paquete, buffer_recepcion);
}

void guardar_proceso(int pid ,int longitud_paquete, void * buffer_recepcion){

	int offset = 0 ;

#warning REDONDEAR PARA ARRIBA!!!!
	//TODO REDONDEAR PARA ARRIBA!!!!
	int cantidad_lineas_proceso= longitud_paquete / config.MAX_LINEA;

	//TODO verificar que haya memoria disponible
	//TODO guardar pid, linea y asociar la estructura administrativa
	while(offset <= longitud_paquete){
		memcpy(memory_pointer, buffer_recepcion + offset,config.MAX_LINEA);
		offset+= config.MAX_LINEA;
	}
}


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



