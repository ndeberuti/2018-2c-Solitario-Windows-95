/*
 * MDJ.c
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */
#include "MDJ.h"

int main(void) {
	system("clear");
	puts("PROCESO MDJ\n");

	log_consola = init_log(PATH_LOG, "Consola MDJ", false, LOG_LEVEL_INFO);
	log_mdj = init_log(PATH_LOG, "Proceso MDJ", true, LOG_LEVEL_INFO);
	log_info(log_mdj, "Inicio del proceso");

	config = load_config();
	crear_estructura_directorios();
	pathActual = strdup(config.PUNTO_MONTAJE);
	pathConsola = strdup("/");

	pthread_create(&thread_servidor, NULL, (void *) server, NULL);

	pthread_create(&thread_consola, NULL, (void *) consola, NULL);

	pthread_join(thread_consola, NULL);

	pthread_join(thread_servidor, NULL);

	exit(EXIT_SUCCESS);
}

config_t load_config() {
	t_config *aux_config = config_create(PATH_CONFIG);

	config_t miConfig;
	miConfig.PUERTO = config_get_int_value(aux_config, "PUERTO");
	miConfig.PUNTO_MONTAJE = strdup(config_get_string_value(aux_config, "PUNTO_MONTAJE"));
	miConfig.RETARDO = config_get_int_value(aux_config, "RETARDO");
	config_destroy(aux_config);

	log_info(log_mdj, "---- Configuracion ----");
	log_info(log_mdj, "PUERTO = %d", miConfig.PUERTO);
	log_info(log_mdj, "PUNTO_MONTAJE = %s", miConfig.PUNTO_MONTAJE);
	log_info(log_mdj, "RETARDO = %d milisegundos", miConfig.RETARDO);
	log_info(log_mdj, "-----------------------");

	return miConfig;
}

void crear_estructura_directorios() {
	if (mkdir(config.PUNTO_MONTAJE, 0777) != 0)
		if (errno != EEXIST) {
			log_error(log_mdj, "Permiso denegado al intentar montar el FS en %s", config.PUNTO_MONTAJE);
			exit(EXIT_FAILURE);
		}

	char *path;
	uint32_t largo_mnt = strlen(config.PUNTO_MONTAJE);
	if ((path = malloc(sizeof(char) * (largo_mnt + 10))) == NULL) {
		log_error(log_mdj, "Error al intentar crear la estructura de directorios");
		exit(EXIT_FAILURE);
	}

	strcpy(path, config.PUNTO_MONTAJE);
	strcat(path, "Metadata/");
	mkdir(path, 0777);
	free(path);

	if ((path = malloc(sizeof(char) * (largo_mnt + 22))) == NULL) {
		log_error(log_mdj, "Error al intentar crear la estructura de directorios");
		exit(EXIT_FAILURE);
	}

	strcpy(path, config.PUNTO_MONTAJE);
	strcat(path, "Metadata/");
	strcat(path, "Metadata.bin");

	t_config *aux_config;
	if ((aux_config = config_create(path)) == NULL) {
		aux_config = malloc(sizeof(t_config));
		aux_config->path = strdup(path);
		aux_config->properties = dictionary_create();
		config_set_value(aux_config, "TAMANIO_BLOQUES", "64");
		config_set_value(aux_config, "CANTIDAD_BLOQUES", "5192");
		config_set_value(aux_config, "MAGIC_NUMBER", "FIFA");
		config_save(aux_config);
	}

	fs_config.TAMANIO_BLOQUES = config_get_int_value(aux_config, "TAMANIO_BLOQUES");
	fs_config.CANTIDAD_BLOQUES = config_get_int_value(aux_config, "CANTIDAD_BLOQUES");
	fs_config.MAGIC_NUMBER = strdup(config_get_string_value(aux_config, "MAGIC_NUMBER"));
	config_destroy(aux_config);

	log_info(log_mdj, "----- File System -----");
	log_info(log_mdj, "TAMANIO_BLOQUES = %d", fs_config.TAMANIO_BLOQUES);
	log_info(log_mdj, "CANTIDAD_BLOQUES = %d", fs_config.CANTIDAD_BLOQUES);
	log_info(log_mdj, "MAGIC_NUMBER = %s", fs_config.MAGIC_NUMBER);
	log_info(log_mdj, "-----------------------");
	free(path);

	if ((path = malloc(sizeof(char) * (largo_mnt + 20))) == NULL) {
		log_error(log_mdj, "Error al intentar crear la estructura de directorios");
		exit(EXIT_FAILURE);
	}

	if ((bitmap = malloc(sizeof(char) * (fs_config.CANTIDAD_BLOQUES + 1))) == NULL) {
		log_error(log_mdj, "Error al intentar crear la estructura de directorios");
		exit(EXIT_FAILURE);
	}

	strcpy(path, config.PUNTO_MONTAJE);
	strcat(path, "Metadata/");
	strcat(path, "Bitmap.bin");
	path_bitmap = strdup(path);

	FILE *fptr = fopen(path, "r");
	if (fptr == NULL) {
		fptr = fopen(path, "w");
		memset(bitmap, '\0', fs_config.CANTIDAD_BLOQUES + 1);
		memset(bitmap, '0', fs_config.CANTIDAD_BLOQUES);
		fputs(bitmap, fptr);
	}
	else
		fgets(bitmap, fs_config.CANTIDAD_BLOQUES + 1, fptr);

	bitarray = bitarray_create(bitmap, strlen(bitmap));
	fclose(fptr);
	free(path);

	if ((path = malloc(sizeof(char) * (largo_mnt + 10))) == NULL) {
		log_error(log_mdj, "Error al intentar crear la estructura de directorios");
		exit(EXIT_FAILURE);
	}

	strcpy(path, config.PUNTO_MONTAJE);
	strcat(path, "Archivos/");
	carpeta_archivos = strdup(path);
	mkdir(path, 0777);
	free(path);

	if ((path = malloc(sizeof(char) * (largo_mnt + 9))) == NULL) {
		log_error(log_mdj, "Error al intentar crear la estructura de directorios");
		exit(EXIT_FAILURE);
	}

	strcpy(path, config.PUNTO_MONTAJE);
	strcat(path, "Bloques/");
	carpeta_bloques = strdup(path);
	mkdir(path, 0777);
	free(path);
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
							log_info(log_consola, "Se desconecto El Diego");
						else
							log_error(log_consola, "recv (comando)");

						close(i); // ¡Hasta luego!
						FD_CLR(i, &master); // eliminar del conjunto maestro
					}
					else
						// tenemos datos de algún cliente
						command_handler(i, command);
			} // if (FD_ISSET(i, &read_fds))
	} // while (true)
}

void command_handler(uint32_t socket, uint32_t command) {
	switch (command) {
	case NUEVA_CONEXION_DIEGO:
		log_info(log_consola, "Nueva conexion desde El Diego");
		break;
	case VALIDAR_ARCHIVO:
		log_info(log_consola, "Operacion VALIDAR_ARCHIVO recibida");
		validar_archivo(socket);
		break;
	case CREAR_ARCHIVO:
		log_info(log_consola, "Operacion CREAR_ARCHIVO recibida");
		crear_archivo(socket);
		break;
	default:
		log_warning(log_consola, "%d: Comando recibido incorrecto", command);
	}
}

void validar_archivo(uint32_t socket) {
	char *path;
	uint32_t rta;

	if (receive_string(socket, &path) <= 0) {
		log_error(log_consola, "recv path (validar_archivo)");
		rta = ERROR_OPERACION;
	}
	else {
		char *aux_path = malloc(sizeof(char) * (strlen(pathActual) + strlen(path) + 1));
		strcpy(aux_path, pathActual);
		strcat(aux_path, path);
		if (isFileExists(aux_path)) {
			log_info(log_consola, "Existe el archivo %s", path);
			rta = OPERACION_OK;
		}
		else {
			log_info(log_consola, "No existe el archivo %s", path);
			rta = OPERACION_FAIL;
		}
		free(aux_path);
		free(path);
	}

	if (send_int(socket, rta) == -1)
		log_error(log_consola, "send (validar_archivo)");
}

void crear_archivo(uint32_t socket) {
	char *path;
	uint32_t rta;
	uint32_t bytes;

	if (receive_string(socket, &path) <= 0) {
		log_error(log_consola, "recv path (validar_archivo)");
		rta = ERROR_OPERACION;
	}
	else {
		if (receive_int(socket, &bytes) <= 0) {
			log_error(log_consola, "recv bytes (validar_archivo)");
			rta = ERROR_OPERACION;
		}
		else {
			log_info(log_consola, "Crear archivo %s de %d bytes", path, bytes);

			uint32_t cant_bloques = bytes / fs_config.TAMANIO_BLOQUES;
			if (bytes % fs_config.TAMANIO_BLOQUES > 0)
				cant_bloques++;

			uint32_t *prox_bloque;
			uint32_t pos_actual = 0;
			uint32_t bloque_inicial = 0;
			uint32_t bloques[cant_bloques];

			while (pos_actual < cant_bloques && (prox_bloque = proximo_bloque_libre(bloque_inicial)) != NULL) {
				bloques[pos_actual] = *prox_bloque;
				bloque_inicial = *prox_bloque + 1;
				pos_actual++;
			}

			if (prox_bloque == NULL) {
				log_warning(log_consola, "No hay suficientes bloques libres para guardar el archivo");
				rta = OPERACION_FAIL;
			}
			else {
				uint32_t bytes_del_bloque;
				uint32_t bytes_restantes = bytes;
				char *relleno;
				char *nro_bloque;
				uint32_t largo_lista_bloques = 1;
				char *nombre_bloque;

				for (uint32_t i = 0; i < cant_bloques; i++) {
					//CREAR EL BLOQUE
					bytes_del_bloque = bytes_restantes > fs_config.TAMANIO_BLOQUES ? fs_config.TAMANIO_BLOQUES : bytes_restantes;
					relleno = malloc(sizeof(char) * (bytes_del_bloque + 1));
					bytes_restantes -= fs_config.TAMANIO_BLOQUES;

					nro_bloque = string_itoa(bloques[i]);
					largo_lista_bloques += strlen(nro_bloque) + 1;
					nombre_bloque = malloc(sizeof(char) * (strlen(carpeta_bloques) + strlen(nro_bloque) + 5));
					strcpy(nombre_bloque, carpeta_bloques);
					strcat(nombre_bloque, nro_bloque);
					strcat(nombre_bloque, ".bin");

					FILE *fptr = fopen(nombre_bloque, "w");
					memset(relleno, '\0', bytes_del_bloque + 1);
					memset(relleno, '\n', bytes_del_bloque);
					fputs(relleno, fptr);
					fclose(fptr);
					free(relleno);
					free(nro_bloque);
					free(nombre_bloque);

					//ACTUALIZAR BITARRAY
					set_bitarray(bloques[i]);
				}
				//CREAR EL ARCHIVO
				char *lista_bloques = malloc(sizeof(char) * (largo_lista_bloques + 1));

				strcpy(lista_bloques, "[");
				for (uint32_t i = 0; i < cant_bloques; i++) {
					strcat(lista_bloques, string_itoa(bloques[i]));
					if (i < cant_bloques - 1)
						strcat(lista_bloques, ",");
				}
				strcat(lista_bloques, "]");

				char *path_archivo = malloc(sizeof(char) * (strlen(carpeta_archivos) + strlen(path) + 1));
				strcpy(path_archivo, carpeta_archivos);
				strcat(path_archivo, path);

				crear_path_completo(path_archivo);

				t_config *aux_config = malloc(sizeof(t_config));
				aux_config->path = strdup(path_archivo);
				aux_config->properties = dictionary_create();
				config_set_value(aux_config, "TAMANIO", string_itoa(bytes));
				config_set_value(aux_config, "BLOQUES", lista_bloques);
				config_save(aux_config);
				config_destroy(aux_config);
				free(path_archivo);
				free(lista_bloques);

				log_info(log_consola, "Operacion finalizada con exito");
				rta = OPERACION_OK;
			}
		}
		free(path);
	}

	if (send_int(socket, rta) == -1)
		log_error(log_consola, "send (crear_archivo)");
}

void *proximo_bloque_libre(uint32_t bloque_inicial) {
	uint32_t i = bloque_inicial;

	while (i < bitarray->size && bitarray_test_bit(bitarray, i * CHAR_BIT))
		i++;

	return i >= bitarray->size ? NULL : &i;
}

void set_bitarray(uint32_t posicion) {
	bitarray_set_bit(bitarray, posicion * CHAR_BIT);

	FILE *fptr = fopen(path_bitmap, "w");
	fputs(bitarray->bitarray, fptr);
	fclose(fptr);
}

void crear_path_completo(char *path_completo) {
	char *path = malloc(sizeof(char) * strlen(path_completo));
	char *aux_path = strdup(path_completo);
	char *sub_path1 = strdup(strtok(aux_path, "/"));
	char *token = strtok(NULL, "/");
	char *sub_path2;

	if (token != NULL) {
		sub_path2 = strdup(token);
		strcpy(path, "/");

		while (token != NULL) {
			strcat(path, sub_path1);
			strcat(path, "/");
			free(sub_path1);
			sub_path1 = strdup(sub_path2);
			token = strtok(NULL, "/");
			if (token != NULL)
				sub_path2 = strdup(token);
		}

		mkdir(path, 0777);
		free(sub_path2);
	}
	free(sub_path1);
	free(aux_path);
	free(path);
}

void consola() {
	char *linea;
	char *token;
	console_t *consola;

	while (true) {
		linea = readline(pathConsola);

		if (strlen(linea) > 0) {
			add_history(linea);
			log_info(log_consola, "Linea leida: %s", linea);
			consola = malloc(sizeof(console_t));

			if (consola != NULL) {
				consola->comando = strdup(strtok(linea, " "));
				consola->cant_params = 0;

				while (consola->cant_params < MAX_PARAMS && (token = strtok(NULL, " ")) != NULL)
					consola->param[consola->cant_params++] = strdup(token);

				if (consola->cant_params > 0) {
					//TODO: Formatear path teniendo en cuenta el . y el ..
					//char *pathFormateado = formatearPath(consola->param[0]);
				}

				if (str_eq(consola->comando, "clear"))
					system("clear");

				else if (str_eq(consola->comando, "ls")) {
					char *aux;
					if (consola->cant_params == 0) {
						aux = malloc(sizeof(char) * (strlen(pathActual) + 7));
						strcpy(aux, "ls -l ");
						strcat(aux, pathActual);
						system(aux);
					}
					else {
						aux = malloc(sizeof(char) * (strlen(consola->param[0]) + strlen(pathActual) + 1));
						strcpy(aux, pathActual);
						strcat(aux, consola->param[0]);
						if (isDirectoryExists(aux)) {
							free(aux);
							aux = malloc(sizeof(char) * (strlen(consola->param[0]) + strlen(pathActual) + 7));
							strcpy(aux, "ls -l ");
							strcat(aux, pathActual);
							strcat(aux, consola->param[0]);
							system(aux);
						}
						else
							print_c(log_consola, "%s: No existe el directorio %s", consola->comando, consola->param[0]);
					}
					free(aux);
				}

				else if (str_eq(consola->comando, "cd"))
					if (consola->cant_params < 1)
						print_c(log_consola, "%s: falta el parametro <directorio>", consola->comando);
					else {
						// TODO: comando cd
					}

				else if (str_eq(consola->comando, "md5"))
					if (consola->cant_params < 1)
						print_c(log_consola, "%s: falta el parametro <archivo>", consola->comando);
					else {
						// TODO: comando md5
					}

				else if (str_eq(consola->comando, "cat"))
					if (consola->cant_params < 1)
						print_c(log_consola, "%s: falta el parametro <archivo>", consola->comando);
					else {
						// TODO: comando cat
					}

				else
					print_c(log_consola, "%s: Comando incorrecto", consola->comando);

				free(consola->comando);
				for (uint32_t i = 0; i < consola->cant_params; i++)
					free(consola->param[i]);
				free(consola);
			}
		}
		free(linea);
	}
}
