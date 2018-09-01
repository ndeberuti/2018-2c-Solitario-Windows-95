/*
 * ElDiego.c
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */
#include "ElDiego.h"

int main(void) {
	system("clear");
	puts("PROCESO EL DIEGO\n");

	log_diego = init_log(PATH_LOG, "Proceso El Diego", true, LOG_LEVEL_INFO);
	log_info(log_diego, "Inicio del proceso");

	config = load_config();

	safa = connect_server(config.IP_SAFA, config.PUERTO_SAFA, log_diego);
	mdj = connect_server(config.IP_MDJ, config.PUERTO_MDJ, log_diego);
	fm9 = connect_server(config.IP_FM9, config.PUERTO_FM9, log_diego);

	pthread_create(&thread_servidor, NULL, (void *) server, NULL);

	pthread_join(thread_servidor, NULL);

	exit(EXIT_SUCCESS);
}

config_t load_config() {
	t_config *config = config_create(PATH_CONFIG);

	config_t miConfig;
	miConfig.PUERTO = config_get_int_value(config, "PUERTO");
	miConfig.IP_SAFA = strdup(config_get_string_value(config, "IP_SAFA"));
	miConfig.PUERTO_SAFA = config_get_int_value(config, "PUERTO_SAFA");
	miConfig.IP_MDJ = strdup(config_get_string_value(config, "IP_MDJ"));
	miConfig.PUERTO_MDJ = config_get_int_value(config, "PUERTO_MDJ");
	miConfig.IP_FM9 = strdup(config_get_string_value(config, "IP_FM9"));
	miConfig.PUERTO_FM9 = config_get_int_value(config, "PUERTO_FM9");
	miConfig.TRANSFER_SIZE = config_get_int_value(config, "TRANSFER_SIZE");

	log_info(log_diego, "---- Configuracion ----");
	log_info(log_diego, "PUERTO = %d", miConfig.PUERTO);
	log_info(log_diego, "IP_SAFA = %s", miConfig.IP_SAFA);
	log_info(log_diego, "PUERTO_SAFA = %d", miConfig.PUERTO_SAFA);
	log_info(log_diego, "IP_MDJ = %s", miConfig.IP_MDJ);
	log_info(log_diego, "PUERTO_MDJ = %d", miConfig.PUERTO_MDJ);
	log_info(log_diego, "IP_FM9 = %s", miConfig.IP_FM9);
	log_info(log_diego, "PUERTO_FM9 = %d", miConfig.PUERTO_FM9);
	log_info(log_diego, "TRANSFER_SIZE = %d", miConfig.TRANSFER_SIZE);
	log_info(log_diego, "-----------------------");

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
	uint32_t servidor = build_server(config.PUERTO, log_diego);

	// añadir listener al conjunto maestro
	FD_SET(servidor, &master);
	// seguir la pista del descriptor de fichero mayor
	fdmax = servidor; // por ahora es éste

	// bucle principal
	while (true) {
		read_fds = master; // cópialo
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			log_error(log_diego, "select");
			exit(EXIT_FAILURE);
		}
		// explorar conexiones existentes en busca de datos que leer
		for (uint32_t i = 0; i <= fdmax; i++)
			if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
				if (i == servidor) {
					// gestionar nuevas conexiones
					addrlen = sizeof(remoteaddr);
					if ((newfd = accept(servidor, (struct sockaddr *) &remoteaddr, &addrlen)) == -1)
						log_error(log_diego, "accept");
					else {
						FD_SET(newfd, &master); // añadir al conjunto maestro
						if (newfd > fdmax) // actualizar el máximo
							fdmax = newfd;
						log_info(log_diego, "Nueva conexion desde %s en el socket %d", inet_ntoa(remoteaddr.sin_addr), newfd);
					}
				}
				else
					// gestionar datos de un cliente
					if ((nbytes = receive_int(i, &command)) <= 0) {
						// error o conexión cerrada por el cliente
						if (nbytes == 0)
							// conexión cerrada
							log_info(log_diego, "Socket %d colgado", i);
						else
							log_error(log_diego, "recv (comando)");

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
		log_warning(log_diego, "%d: Comando recibido incorrecto", command);
	}
}
