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
	pathActual = strdup(config->PUNTO_MONTAJE);
	pathConsola = strdup("/");

	pthread_create(&thread_servidor, NULL, (void *) server, NULL);

	pthread_create(&thread_consola, NULL, (void *) consola, NULL);

	pthread_join(thread_consola, NULL);

	pthread_join(thread_servidor, NULL);

	exit(EXIT_SUCCESS);
}

config_t *load_config() {
	t_config *aux_config = config_create(PATH_CONFIG);

	config_t *miConfig = malloc(sizeof(config_t));
	miConfig->PUERTO = config_get_int_value(aux_config, "PUERTO");
	miConfig->PUNTO_MONTAJE = strdup(config_get_string_value(aux_config, "PUNTO_MONTAJE"));
	miConfig->RETARDO = config_get_int_value(aux_config, "RETARDO");
	//config_destroy(aux_config);

	log_info(log_mdj, "---- Configuracion ----");
	log_info(log_mdj, "PUERTO = %d", miConfig->PUERTO);
	log_info(log_mdj, "PUNTO_MONTAJE = %s", miConfig->PUNTO_MONTAJE);
	log_info(log_mdj, "RETARDO = %d milisegundos", miConfig->RETARDO);
	log_info(log_mdj, "-----------------------");

	return miConfig;
}

void crear_estructura_directorios() {
	if (mkdir(config->PUNTO_MONTAJE, 0777) != 0)
		if (errno != EEXIST) {
			log_error(log_mdj, "Permiso denegado al intentar montar el FS en %s", config->PUNTO_MONTAJE);
			exit(EXIT_FAILURE);
		}

	log_info(log_consola, "Punto de montaje OK en: %s", config->PUNTO_MONTAJE);

	char *path;
	int32_t largo_mnt = strlen(config->PUNTO_MONTAJE);
	if ((path = malloc(sizeof(char) * (largo_mnt + 10))) == NULL) {
		log_error(log_mdj, "Error al intentar crear la estructura de directorios");
		exit(EXIT_FAILURE);
	}

	strcpy(path, config->PUNTO_MONTAJE);
	strcat(path, "Metadata/");
	mkdir(path, 0777);

	log_info(log_consola, "Carpeta Metadata OK en: %s", path);
	free(path);

	if ((path = malloc(sizeof(char) * (largo_mnt + 22))) == NULL) {
		log_error(log_mdj, "Error al intentar crear la estructura de directorios");
		exit(EXIT_FAILURE);
	}

	strcpy(path, config->PUNTO_MONTAJE);
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

	fs_config = malloc(sizeof(config_fs));
	fs_config->TAMANIO_BLOQUES = config_get_int_value(aux_config, "TAMANIO_BLOQUES");
	fs_config->CANTIDAD_BLOQUES = config_get_int_value(aux_config, "CANTIDAD_BLOQUES");
	fs_config->MAGIC_NUMBER = strdup(config_get_string_value(aux_config, "MAGIC_NUMBER"));
	//config_destroy(aux_config);

	log_info(log_mdj, "----- File System -----");
	log_info(log_mdj, "TAMANIO_BLOQUES = %d", fs_config->TAMANIO_BLOQUES);
	log_info(log_mdj, "CANTIDAD_BLOQUES = %d", fs_config->CANTIDAD_BLOQUES);
	log_info(log_mdj, "MAGIC_NUMBER = %s", fs_config->MAGIC_NUMBER);
	log_info(log_mdj, "-----------------------");
	log_info(log_consola, "Archivo Metadata.bin OK en: %s", path);
	free(path);

	if ((bitmap = malloc(sizeof(char) * (fs_config->CANTIDAD_BLOQUES + 1))) == NULL) {
		log_error(log_mdj, "Error al intentar crear la estructura de directorios");
		exit(EXIT_FAILURE);
	}

	if ((path = malloc(sizeof(char) * (largo_mnt + 20))) == NULL) {
		log_error(log_mdj, "Error al intentar crear la estructura de directorios");
		exit(EXIT_FAILURE);
	}

	strcpy(path, config->PUNTO_MONTAJE);
	strcat(path, "Metadata/");
	strcat(path, "Bitmap.bin");
	path_bitmap = strdup(path);

	FILE *fptr = fopen(path, "rb");
	if (fptr == NULL) {
		memset(bitmap, '\0', fs_config->CANTIDAD_BLOQUES + 1);
		memset(bitmap, '0', fs_config->CANTIDAD_BLOQUES);
	}
	else {
		char *aux_bitmap = malloc(sizeof(char) * (fs_config->CANTIDAD_BLOQUES + 1));
		fread(aux_bitmap, fs_config->CANTIDAD_BLOQUES, 1, fptr);
		if (strlen(aux_bitmap) == fs_config->CANTIDAD_BLOQUES)
			strcpy(bitmap, aux_bitmap);
		else {
			rewind(fptr);
			char *binario;
			strcpy(bitmap, "");
			int32_t entero = fgetc(fptr);
			while (!feof(fptr)) {
				binario = int_to_bin(entero);
				strcat(bitmap, binario);
				free(binario);
				entero = fgetc(fptr);
			}
		}
		free(aux_bitmap);
		fclose(fptr);
	}

	log_info(log_consola, "Archivo Bitmap.bin OK en: %s", path);
	bitarray = bitarray_create(bitmap, strlen(bitmap));
	fptr = fopen(path, "w");
	fputs(bitmap, fptr);
	fclose(fptr);
	free(path);

	if ((path = malloc(sizeof(char) * (largo_mnt + 10))) == NULL) {
		log_error(log_mdj, "Error al intentar crear la estructura de directorios");
		exit(EXIT_FAILURE);
	}

	strcpy(path, config->PUNTO_MONTAJE);
	strcat(path, "Archivos/");
	carpeta_archivos = string_substring_until(path, largo_mnt + 8);
	mkdir(path, 0777);

	log_info(log_consola, "Carpeta Archivos OK en: %s", path);
	free(path);

	if ((path = malloc(sizeof(char) * (largo_mnt + 9))) == NULL) {
		log_error(log_mdj, "Error al intentar crear la estructura de directorios");
		exit(EXIT_FAILURE);
	}

	strcpy(path, config->PUNTO_MONTAJE);
	strcat(path, "Bloques/");
	carpeta_bloques = strdup(path);
	mkdir(path, 0777);

	log_info(log_consola, "Carpeta Bloques OK en: %s", path);
	free(path);

	char *nro_bloque;
	char *nombre_bloque;
	for (int32_t i = 0; i < bitarray->size; i++)
		if (!bitarray_test_bit(bitarray, i * CHAR_BIT)) {
			nro_bloque = string_itoa(i);
			nombre_bloque = malloc(sizeof(char) * (strlen(carpeta_bloques) + strlen(nro_bloque) + 5));
			strcpy(nombre_bloque, carpeta_bloques);
			strcat(nombre_bloque, nro_bloque);
			strcat(nombre_bloque, ".bin");
			remove(nombre_bloque);
			free(nombre_bloque);
			free(nro_bloque);
		}
}

void server() {
	fd_set master; // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
	struct sockaddr_in remoteaddr; // dirección del cliente
	int32_t fdmax; // número máximo de descriptores de fichero
	int32_t newfd; // descriptor de socket de nueva conexión aceptada
	int32_t command; // comando del cliente
	int32_t nbytes;
	int32_t addrlen;
	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);

	// obtener socket a la escucha
	int32_t servidor = build_server(config->PUERTO, log_consola);

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
		for (int32_t i = 0; i <= fdmax; i++)
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

void command_handler(int32_t socket, int32_t command) {
	switch (command) {
	case NEW_DMA_CONNECTION:
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
	case OBTENER_DATOS:
		log_info(log_consola, "Operacion OBTENER_DATOS recibida");
		obtener_datos(socket);
		break;
	case GUARDAR_DATOS:
		log_info(log_consola, "Operacion GUARDAR_DATOS recibida");
		guardar_datos(socket);
		break;
	case BORRAR_ARCHIVO:
		log_info(log_consola, "Operacion BORRAR_ARCHIVO recibida");
		borrar_archivo(socket);
		break;
	default:
		log_warning(log_consola, "%d: Comando recibido incorrecto", command);
	}
}

void validar_archivo(int32_t socket) {
	char *path;
	int32_t rta;

	if (receive_string(socket, &path) <= 0) {
		log_error(log_consola, "recv path (validar_archivo)");
		rta = ERROR_OPERACION;
	}
	else {
		log_info(log_consola, "Validar archivo %s", path);

		char *aux_path = malloc(sizeof(char) * (strlen(carpeta_archivos) + strlen(path) + 1));
		strcpy(aux_path, carpeta_archivos);
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

	usleep(config->RETARDO * 1000);

	if (send_int(socket, rta) == -1)
		log_error(log_consola, "send (validar_archivo)");
}

void crear_archivo(int32_t socket) {
	char *path;
	int32_t rta;
	int32_t bytes;

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

			int32_t cant_bloques = calcular_cant_bloques(bytes);

			log_info(log_consola, "El archivo ocupa %d bloques", cant_bloques);

			int32_t *prox_bloque;
			int32_t pos_actual = 0;
			int32_t bloque_inicial = 0;
			int32_t bloques[cant_bloques];

			while (pos_actual < cant_bloques && (prox_bloque = proximo_bloque_libre(bloque_inicial)) != NULL) {
				bloques[pos_actual] = *prox_bloque;
				bloque_inicial = *prox_bloque + 1;
				pos_actual++;
			}

			if (prox_bloque == NULL) {
				log_warning(log_consola, "No hay suficientes bloques libres para guardar el archivo");
				rta = BLOQUES_INSUFICIENTES;
			}
			else {
				int32_t bytes_del_bloque;
				int32_t bytes_restantes = bytes;
				char *relleno;
				char *nro_bloque;
				int32_t largo_lista_bloques = 1;
				char *nombre_bloque;
				FILE *fptr;

				for (int32_t i = 0; i < cant_bloques; i++) {
					//CREAR EL BLOQUE
					bytes_del_bloque = bytes_restantes > fs_config->TAMANIO_BLOQUES ? fs_config->TAMANIO_BLOQUES : bytes_restantes;
					relleno = malloc(sizeof(char) * (bytes_del_bloque + 1));
					bytes_restantes -= bytes_del_bloque;

					nro_bloque = string_itoa(bloques[i]);
					largo_lista_bloques += strlen(nro_bloque) + 1;
					nombre_bloque = malloc(sizeof(char) * (strlen(carpeta_bloques) + strlen(nro_bloque) + 5));
					strcpy(nombre_bloque, carpeta_bloques);
					strcat(nombre_bloque, nro_bloque);
					strcat(nombre_bloque, ".bin");

					fptr = fopen(nombre_bloque, "w");
					memset(relleno, '\0', bytes_del_bloque + 1);
					memset(relleno, '\n', bytes_del_bloque);
					fputs(relleno, fptr);
					fclose(fptr);

					log_info(log_consola, "Se creo el bloque %s.bin", nro_bloque);

					free(relleno);
					free(nro_bloque);
					free(nombre_bloque);

					//ACTUALIZAR BITARRAY
					set_bitarray(bloques[i]);
					log_info(log_consola, "Se actualizo el bit %d del bitmap", bloques[i]);
				}
				//CREAR EL ARCHIVO
				char *lista_bloques = malloc(sizeof(char) * (largo_lista_bloques + 1));

				strcpy(lista_bloques, "[");
				for (int32_t i = 0; i < cant_bloques; i++) {
					strcat(lista_bloques, string_itoa(bloques[i]));
					if (i < cant_bloques - 1)
						strcat(lista_bloques, ",");
				}
				strcat(lista_bloques, "]");

				char *path_archivo = malloc(sizeof(char) * (strlen(carpeta_archivos) + strlen(path) + 1));
				strcpy(path_archivo, carpeta_archivos);
				strcat(path_archivo, path);

				save_file(path_archivo, bytes, lista_bloques);

				free(path_archivo);
				free(lista_bloques);

				log_info(log_consola, "Se creo el archivo %s", path);
				log_info(log_consola, "Operacion finalizada con exito");
				rta = OPERACION_OK;
			}
		}
		free(path);
	}

	usleep(config->RETARDO * 1000);

	if (send_int(socket, rta) == -1)
		log_error(log_consola, "send (crear_archivo)");
}

void obtener_datos(int32_t socket) {
	char *path;
	int32_t rta;
	int32_t size;
	int32_t offset;
	char *sub_datos;

	if (receive_string(socket, &path) <= 0) {
		log_error(log_consola, "recv path (obtener_datos)");
		rta = ERROR_OPERACION;
	}
	else {
		if (receive_int(socket, &offset) <= 0) {
			log_error(log_consola, "recv offset (obtener_datos)");
			rta = ERROR_OPERACION;
		}
		else {
			if (receive_int(socket, &size) <= 0) {
				log_error(log_consola, "recv size (obtener_datos)");
				rta = ERROR_OPERACION;
			}
			else {
				log_info(log_consola, "Obtener datos del archivo %s desde el byte %d y largo %d", path, offset, size);

				char *path_archivo = malloc(sizeof(char) * (strlen(carpeta_archivos) + strlen(path) + 1));
				strcpy(path_archivo, carpeta_archivos);
				strcat(path_archivo, path);

				char *datos = obtener_todo(path_archivo, offset);
				free(path_archivo);

				if (datos != NULL) {
					if (size < 0)
						sub_datos = string_substring_from(datos, offset);
					else
						sub_datos = string_substring(datos, offset, size);

					log_info(log_consola, "Operacion finalizada con exito");
					rta = OPERACION_OK;
					free(datos);
				}
				else {
					log_warning(log_consola, "El offset es mayor que el tamanio del archivo");
					rta = OPERACION_FAIL;
				}
			}
		}
		free(path);
	}

	usleep(config->RETARDO * 1000);

	if (send_int(socket, rta) == -1)
		log_error(log_consola, "send (obtener_datos)");

	if (rta == OPERACION_OK) {
		if (send_string(socket, sub_datos) == -1)
			log_error(log_consola, "send datos (obtener_datos)");

		free(sub_datos);
	}
}

void guardar_datos(int32_t socket) {
	char *path;
	char *buffer;
	int32_t rta;
	int32_t size;
	int32_t offset;

	if (receive_string(socket, &path) <= 0) {
		log_error(log_consola, "recv path (guardar_datos)");
		rta = ERROR_OPERACION;
	}
	else {
		if (receive_int(socket, &offset) <= 0) {
			log_error(log_consola, "recv offset (guardar_datos)");
			rta = ERROR_OPERACION;
		}
		else {
			if (receive_int(socket, &size) <= 0) {
				log_error(log_consola, "recv size (guardar_datos)");
				rta = ERROR_OPERACION;
			}
			else {
				if (receive_string(socket, &buffer) <= 0) {
					log_error(log_consola, "recv buffer (guardar_datos)");
					rta = ERROR_OPERACION;
				}
				else {
					log_info(log_consola, "Guardar %s en el archivo %s desde el byte %d y largo %d", buffer, path, offset, size);

					char *path_archivo = malloc(sizeof(char) * (strlen(carpeta_archivos) + strlen(path) + 1));
					strcpy(path_archivo, carpeta_archivos);
					strcat(path_archivo, path);

					char *datos = obtener_todo(path_archivo, offset);

					if (datos != NULL) {
						char *cabeza = string_substring_until(datos, offset);
						char *new_datos = malloc(sizeof(char) * (strlen(datos) - size + strlen(buffer) + 1));
						strcpy(new_datos, cabeza);
						strcat(new_datos, buffer);
						free(cabeza);

						if (offset + size < strlen(datos)) {
							char *cola = string_substring_from(datos, offset + size);
							strcat(new_datos, cola);
							free(cola);
						}

						borrar_todo(path_archivo);
						log_info(log_consola, "Se elimino el archivo original");

						// CREAR TODOO
						int32_t cant_bloques = calcular_cant_bloques(strlen(new_datos));

						log_info(log_consola, "El archivo modificado ocupa %d bloques", cant_bloques);

						int32_t *prox_bloque;
						int32_t pos_actual = 0;
						int32_t bloque_inicial = 0;
						int32_t bloques[cant_bloques];

						while (pos_actual < cant_bloques && (prox_bloque = proximo_bloque_libre(bloque_inicial)) != NULL) {
							bloques[pos_actual] = *prox_bloque;
							bloque_inicial = *prox_bloque + 1;
							pos_actual++;
						}

						if (prox_bloque == NULL) {
							log_warning(log_consola, "No hay suficientes bloques libres para guardar el archivo modificado");
							log_info(log_consola, "Se va a restaurar el archivo original");
							rta = BLOQUES_INSUFICIENTES;
							free(new_datos);
							new_datos = strdup(datos);

							cant_bloques = calcular_cant_bloques(strlen(new_datos));

							pos_actual = 0;
							bloque_inicial = 0;

							while (pos_actual < cant_bloques && (prox_bloque = proximo_bloque_libre(bloque_inicial)) != NULL) {
								bloques[pos_actual] = *prox_bloque;
								bloque_inicial = *prox_bloque + 1;
								pos_actual++;
							}
						}
						else
							rta = OPERACION_OK;

						int32_t bytes_del_bloque;
						int32_t bytes_restantes = strlen(new_datos);
						char *relleno;
						char *nro_bloque;
						int32_t largo_lista_bloques = 1;
						char *nombre_bloque;
						FILE *fptr;

						for (int32_t i = 0; i < cant_bloques; i++) {
							//CREAR EL BLOQUE
							bytes_del_bloque = bytes_restantes > fs_config->TAMANIO_BLOQUES ? fs_config->TAMANIO_BLOQUES : bytes_restantes;
							bytes_restantes -= bytes_del_bloque;

							nro_bloque = string_itoa(bloques[i]);
							largo_lista_bloques += strlen(nro_bloque) + 1;
							nombre_bloque = malloc(sizeof(char) * (strlen(carpeta_bloques) + strlen(nro_bloque) + 5));
							strcpy(nombre_bloque, carpeta_bloques);
							strcat(nombre_bloque, nro_bloque);
							strcat(nombre_bloque, ".bin");

							fptr = fopen(nombre_bloque, "w");
							relleno = string_substring(new_datos, fs_config->TAMANIO_BLOQUES * i, bytes_del_bloque);
							fputs(relleno, fptr);
							fclose(fptr);

							log_info(log_consola, "Se creo el bloque %s.bin", nro_bloque);

							free(relleno);
							free(nro_bloque);
							free(nombre_bloque);

							//ACTUALIZAR BITARRAY
							set_bitarray(bloques[i]);
							log_info(log_consola, "Se actualizo el bit %d del bitmap", bloques[i]);
						}
						//CREAR EL ARCHIVO
						char *lista_bloques = malloc(sizeof(char) * (largo_lista_bloques + 1));

						strcpy(lista_bloques, "[");
						for (int32_t i = 0; i < cant_bloques; i++) {
							strcat(lista_bloques, string_itoa(bloques[i]));
							if (i < cant_bloques - 1)
								strcat(lista_bloques, ",");
						}
						strcat(lista_bloques, "]");

						save_file(path_archivo, strlen(new_datos), lista_bloques);

						free(lista_bloques);
						free(new_datos);
						free(datos);

						if (rta == OPERACION_OK) {
							log_info(log_consola, "Se modifico el archivo %s", path);
							log_info(log_consola, "Operacion finalizada con exito");
						}
					}
					else {
						log_warning(log_consola, "El offset es mayor que el tamanio del archivo");
						rta = OPERACION_FAIL;
					}
					free(path_archivo);
					free(buffer);
				}
			}
		}
		free(path);
	}

	usleep(config->RETARDO * 1000);

	if (send_int(socket, rta) == -1)
		log_error(log_consola, "send (guardar_datos)");
}

void borrar_archivo(int32_t socket) {
	char *path;
	int32_t rta;

	if (receive_string(socket, &path) <= 0) {
		log_error(log_consola, "recv path (borrar_archivo)");
		rta = ERROR_OPERACION;
	}
	else {
		log_info(log_consola, "Borrar archivo %s", path);

		char *aux_path = malloc(sizeof(char) * (strlen(carpeta_archivos) + strlen(path) + 1));
		log_info(log_consola, "Operacion finalizada con exito");
		strcpy(aux_path, carpeta_archivos);
		strcat(aux_path, path);
		borrar_todo(aux_path);
		rta = OPERACION_OK;
		free(aux_path);
		free(path);
	}

	usleep(config->RETARDO * 1000);

	if (send_int(socket, rta) == -1)
		log_error(log_consola, "send (borrar_archivo)");
}

char *int_to_bin(int32_t i) {
	char *bits = malloc(sizeof(char) * 9);
	int32_t bits_index = 7;
	memset(bits, '\0', 9);
	memset(bits, '0', 8);

	while (i > 0) {
		bits[bits_index--] = (i & 1) + '0';
		i = (i >> 1);
	}

	return bits;
}

int32_t calcular_cant_bloques(int32_t bytes) {
	int32_t cant_bloques = bytes / fs_config->TAMANIO_BLOQUES;

	if (bytes % fs_config->TAMANIO_BLOQUES > 0)
		cant_bloques++;

	return cant_bloques;
}

void *proximo_bloque_libre(int32_t bloque_inicial) {
	int32_t i = bloque_inicial;

	while (i < bitarray->size && bitarray_test_bit(bitarray, i * CHAR_BIT))
		i++;

	return i >= bitarray->size ? NULL : &i;
}

void set_bitarray(int32_t posicion) {
	bitarray_set_bit(bitarray, posicion * CHAR_BIT);

	FILE *fptr = fopen(path_bitmap, "w");
	fputs(bitarray->bitarray, fptr);
	fclose(fptr);
}

void clean_bitarray(int32_t posicion) {
	bitarray_clean_bit(bitarray, posicion * CHAR_BIT);

	FILE *fptr = fopen(path_bitmap, "w");
	fputs(bitarray->bitarray, fptr);
	fclose(fptr);
}

void save_file(char *path, int32_t tamanio, char *bloques) {
	crear_path_completo(path);

	FILE* file = fopen(path, "wb+");
	char* lines = string_new();
	string_append_with_format(&lines, "TAMANIO=%s\n", string_itoa(tamanio));
	string_append_with_format(&lines, "BLOQUES=%s\n", bloques);

	fwrite(lines, strlen(lines), 1, file);
	fclose(file);
	free(lines);
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

char *obtener_todo(char *path, int32_t offset) {
	f_config *aux_config = file_create(path);
	int32_t tamanio = atoi(aux_config->tamanio);
	char **bloques = string_get_string_as_array(aux_config->bloques);
	int32_t cant_bloques = calcular_cant_bloques(tamanio);
	file_destroy(aux_config);
	char *rta = NULL;

	if (offset < tamanio) {
		char *nombre_bloque;
		FILE *fptr;
		char *parte;
		char *contenido;
		rta = malloc(sizeof(char) * (tamanio + 1));

		for (int32_t i = 0; i < cant_bloques; i++) {
			nombre_bloque = malloc(sizeof(char) * (strlen(carpeta_bloques) + strlen(bloques[i]) + 5));
			strcpy(nombre_bloque, carpeta_bloques);
			strcat(nombre_bloque, bloques[i]);
			strcat(nombre_bloque, ".bin");
			fptr = fopen(nombre_bloque, "r");
			parte = malloc(sizeof(char) * (fs_config->TAMANIO_BLOQUES + 1));
			contenido = malloc(sizeof(char) * (fs_config->TAMANIO_BLOQUES + 1));
			fgets(parte, fs_config->TAMANIO_BLOQUES + 1, fptr);
			strcpy(contenido, parte);

			if (i == 0) strcpy(rta, parte);
			else strcat(rta, parte);

			while (!feof(fptr) && strlen(contenido) < fs_config->TAMANIO_BLOQUES && strlen(rta) < tamanio) {
				free(parte);
				parte = malloc(sizeof(char) * (fs_config->TAMANIO_BLOQUES + 1));
				fgets(parte, fs_config->TAMANIO_BLOQUES + 1, fptr);
				strcat(contenido, parte);
                strcat(rta, parte);
			}
			free(nombre_bloque);
			free(contenido);
			fclose(fptr);
			free(parte);
		}
	}

	for (int32_t i = 0; i < cant_bloques; i++)
		free(bloques[i]);
	free(bloques);

	return rta;
}

f_config *file_create(char *path) {
	FILE* file = fopen(path, "r");

	if (file == NULL) {
		return NULL;
	}

	struct stat stat_file;
	stat(path, &stat_file);

	f_config *config = malloc(sizeof(f_config));

	char* buffer = calloc(1, stat_file.st_size + 1);
	fread(buffer, stat_file.st_size, 1, file);

	if (strstr(buffer, "\r\n")) {
		printf("\n\nconfig_create - WARNING: the file %s contains a \\r\\n sequence "
		 "- the Windows new line sequence. The \\r characters will remain as part "
		 "of the value, as Unix newlines consist of a single \\n. You can install "
		 "and use `dos2unix` program to convert your files to Unix style.\n\n", path);
	}

	char** lines = string_split(buffer, "\n");

	void add_configuration(char *line) {
		if (!string_starts_with(line, "#")) {
			char** keyAndValue = string_n_split(line, 2, "=");

			if (str_eq(keyAndValue[0], "TAMANIO"))
				config->tamanio = strdup(keyAndValue[1]);
			else if (str_eq(keyAndValue[0], "BLOQUES"))
				config->bloques = strdup(keyAndValue[1]);

			free(keyAndValue[0]);
			free(keyAndValue[1]);
			free(keyAndValue);
		}
	}
	string_iterate_lines(lines, add_configuration);
	string_iterate_lines(lines, (void*) free);

	free(lines);
	free(buffer);
	fclose(file);

	return config;
}

void file_destroy(f_config *config) {
	free(config->tamanio);
	free(config->bloques);
	free(config);
}

void borrar_todo(char *path) {
	f_config *aux_config = file_create(path);
	int32_t tamanio = atoi(aux_config->tamanio);
	char **bloques = string_get_string_as_array(aux_config->bloques);
	int32_t cant_bloques = calcular_cant_bloques(tamanio);
	file_destroy(aux_config);
	char *nombre_bloque;

	for (int32_t i = 0; i < cant_bloques; i++) {
		nombre_bloque = malloc(sizeof(char) * (strlen(carpeta_bloques) + strlen(bloques[i]) + 5));
		strcpy(nombre_bloque, carpeta_bloques);
		strcat(nombre_bloque, bloques[i]);
		strcat(nombre_bloque, ".bin");

		clean_bitarray(atoi(bloques[i]));

		remove(nombre_bloque);
		free(nombre_bloque);
		free(bloques[i]);
	}
	free(bloques);
	remove(path);
}

char *formatear_path(char *path) {
	char *primer_caracter = string_substring_until(path, 1);
	char *rta;

	if (str_eq(primer_caracter, "/")) {
		rta = malloc(sizeof(char) * (strlen(config->PUNTO_MONTAJE) + strlen(path) + 1));
		strcpy(rta, config->PUNTO_MONTAJE);
	}
	else {
		rta = malloc(sizeof(char) * (strlen(pathActual) + strlen(path) + 1));
		strcpy(rta, pathActual);
	}

	char *sub_path;
	bool sin_barra;
	char *nuevo_path;
	free(primer_caracter);
	char *aux_path = strdup(path);
	char *token = strtok(aux_path, "/");

	while (token != NULL) {
		sub_path = strdup(token);
		sin_barra = false;

		if (strlen(sub_path) == 0 || str_eq(sub_path, "."))
			sin_barra = true;
		else if (str_eq(sub_path, "..")) {
			nuevo_path = convertir_punto_punto(rta);
			strcpy(rta, nuevo_path);
			free(nuevo_path);
		}
		else
			strcat(rta, sub_path);

		token = strtok(NULL, "/");
		if (token != NULL && !sin_barra)
			strcat(rta, "/");
		free(sub_path);
	}
	free(aux_path);

	if (string_ends_with(rta, "/")) {
		char *new_rta = string_substring_until(rta, strlen(rta) - 1);
		free(rta);
		return new_rta;
	}
	else
		return rta;
}

char *convertir_punto_punto(char *path_completo) {
	char *path;

	if (str_eq(path_completo, config->PUNTO_MONTAJE))
		path = string_substring_until(path_completo, strlen(path_completo) - 1);
	else {
		int32_t i = strlen(path_completo) - 2;
		bool salir = false;
		char *aux;

		while (i > 0 && !salir) {
			aux = string_substring(path_completo, i, 1);
			if (str_eq(aux, "/"))
				salir = true;
			else
				i--;
			free(aux);
		}
		path = string_substring_until(path_completo, i);
	}

	return path;
}

bool es_carpeta_archivos(char *path) {
	char *aux_path = string_substring_until(path, strlen(carpeta_archivos) + 1);
	char *aux_arch = malloc(sizeof(char) * (strlen(carpeta_archivos) + 2));
	strcpy(aux_arch, carpeta_archivos);
	strcat(aux_arch, "/");
	bool esCarpetaArchivos = str_eq(aux_path, aux_arch);
	free(aux_path);
	free(aux_arch);

	return esCarpetaArchivos;
}

void consola() {
	char *aux;
	char *linea;
	char *token;
	char *prompt;
	console_t *consola;
	char *pathFormateado;
	char *pathFormateadoCorto;

	while (true) {
		prompt = malloc(sizeof(char) * (strlen(pathConsola) + 3));
		strcpy(prompt, pathConsola);
		strcat(prompt, "> ");

		linea = readline(prompt);

		if (strlen(linea) > 0) {
			add_history(linea);
			log_info(log_consola, "Linea leida: %s", linea);
			consola = malloc(sizeof(console_t));

			if (consola != NULL) {
				consola->comando = strdup(strtok(linea, " "));
				consola->cant_params = 0;

				while (consola->cant_params < MAX_PARAMS && (token = strtok(NULL, " ")) != NULL)
					consola->param[consola->cant_params++] = strdup(token);

				if (consola->cant_params > 0)
					pathFormateado = formatear_path(consola->param[0]);
				else
					pathFormateado = strdup(pathActual);

				pathFormateadoCorto = string_substring_from(pathFormateado, strlen(config->PUNTO_MONTAJE) - 1);

				if (str_eq(consola->comando, "clear"))
					system("clear");

				else if (str_eq(consola->comando, "ls")) {
					if (isDirectoryExists(pathFormateado)) {
						aux = malloc(sizeof(char) * (strlen(pathFormateado) + 7));
						strcpy(aux, "ls -l ");
						strcat(aux, pathFormateado);
						system(aux);
						free(aux);
					}
					else
						print_c(log_consola, "%s: No existe el directorio %s", consola->comando, pathFormateadoCorto);
				}

				else if (str_eq(consola->comando, "cd")) {
					if (consola->cant_params < 1)
						print_c(log_consola, "%s: falta el parametro <directorio>", consola->comando);
					else {
						if (isDirectoryExists(pathFormateado)) {
							char *aux = malloc(sizeof(char) * (strlen(pathFormateado) + 2));
							strcpy(aux, pathFormateado);
							strcat(aux, "/");

							free(pathActual);
							pathActual = strdup(aux);

							free(pathConsola);
							pathConsola = string_substring_from(aux, strlen(config->PUNTO_MONTAJE) - 1);

							log_info(log_consola, "Se cambio el path a %s", pathConsola);

							free(aux);
						}
						else
							print_c(log_consola, "%s: No existe el directorio %s", consola->comando, pathFormateadoCorto);
					}
				}

				else if (str_eq(consola->comando, "md5")) {
					if (es_carpeta_archivos(pathFormateado)) {
						if (consola->cant_params < 1)
							print_c(log_consola, "%s: falta el parametro <archivo>", consola->comando);
						else {
							if (isFileExists(pathFormateado)) {
								aux = obtener_todo(pathFormateado, 0);
								void *content = strdup(aux);
								unsigned char *digest = malloc(MD5_DIGEST_LENGTH);
								MD5_CTX context;
								MD5_Init(&context);
								MD5_Update(&context, content, strlen(content) + 1);
								MD5_Final(digest, &context);
								for(int32_t i = 0; i < MD5_DIGEST_LENGTH; i++)
									printf("%02x", digest[i]);
								printf("\n");
								free(content);
								free(digest);
								free(aux);
							}
							else
								print_c(log_consola, "%s: No existe el archivo %s", consola->comando, pathFormateadoCorto);
						}
					}
					else
						print_c(log_consola, "%s: Comando incorrecto en este contexto", consola->comando);
				}

				else if (str_eq(consola->comando, "cat")) {
					if (consola->cant_params < 1)
						print_c(log_consola, "%s: falta el parametro <archivo>", consola->comando);
					else {
						if (isFileExists(pathFormateado)) {
							if (es_carpeta_archivos(pathFormateado)) {
								aux = obtener_todo(pathFormateado, 0);
								printf("%s", aux);
							}
							else {
								aux = malloc(sizeof(char) * (strlen(pathFormateado) + 5));
								strcpy(aux, "cat ");
								strcat(aux, pathFormateado);
								system(aux);
							}
							free(aux);
						}
						else
							print_c(log_consola, "%s: No existe el archivo %s", consola->comando, pathFormateadoCorto);
					}
				}

				else
					print_c(log_consola, "%s: Comando incorrecto", consola->comando);

				free(consola->comando);
				for (int32_t i = 0; i < consola->cant_params; i++)
					free(consola->param[i]);
				free(consola);
				free(pathFormateado);
				free(pathFormateadoCorto);
			}
		}
		free(linea);
		free(prompt);
	}
}
