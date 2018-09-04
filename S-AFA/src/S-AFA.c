/*
 * S-AFA.c
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */
#include "S-AFA.h"

int main(void) {
	system("clear");
	puts("PROCESO S-AFA\n");

	log_consola = init_log(PATH_LOG, "Consola S-AFA", false, LOG_LEVEL_INFO);
	log_safa = init_log(PATH_LOG, "Proceso S-AFA", true, LOG_LEVEL_INFO);
	log_info(log_safa, "Inicio del proceso");

	config = load_config();

	pthread_create(&thread_servidor, NULL, (void *) server, NULL);

	pthread_create(&thread_consola, NULL, (void *) consola, NULL);

	pthread_join(thread_consola, NULL);

	pthread_join(thread_servidor, NULL);

	exit(EXIT_SUCCESS);
}

config_t load_config() {
	t_config *config = config_create(PATH_CONFIG);

	config_t miConfig;
	miConfig.PUERTO = config_get_int_value(config, "PUERTO");
	miConfig.ALGORITMO = strdup(config_get_string_value(config, "ALGORITMO"));
	miConfig.QUANTUM = config_get_int_value(config, "QUANTUM");
	miConfig.MULTIPROGRAMACION = config_get_int_value(config, "MULTIPROGRAMACION");
	miConfig.RETARDO_PLANIF = config_get_int_value(config, "RETARDO_PLANIF");

	log_info(log_safa, "---- Configuracion ----");
	log_info(log_safa, "PUERTO = %d", miConfig.PUERTO);
	log_info(log_safa, "ALGORITMO = %s", miConfig.ALGORITMO);
	log_info(log_safa, "QUANTUM = %d", miConfig.QUANTUM);
	log_info(log_safa, "MULTIPROGRAMACION = %d", miConfig.MULTIPROGRAMACION);
	log_info(log_safa, "RETARDO_PLANIF = %d milisegundos", miConfig.RETARDO_PLANIF);
	log_info(log_safa, "-----------------------");

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
		linea = readline("S-AFA> ");

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

				else if (str_eq(consola->comando, "ejecutar"))
					if (consola->cant_params < 1)
						print_c(log_consola, "%s: falta la ruta del script que se desea ejecutar\n", consola->comando);
					else {
						// TODO: comando ejecutar
					}

				else if (str_eq(consola->comando, "status")) {
					// TODO: comando status
				}

				else if (str_eq(consola->comando, "finalizar"))
					if (consola->cant_params < 1)
						print_c(log_consola, "%s: falta el ID correspondiente a un DT Block\n", consola->comando);
					else {
						// TODO: comando finalizar
					}

				else if (str_eq(consola->comando, "metricas")) {
					// TODO: comando metricas
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
