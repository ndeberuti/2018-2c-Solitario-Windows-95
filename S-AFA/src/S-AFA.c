/*
 * S-AFA.c
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */
#include "S-AFA.h"

int main(void)
{
	system("clear");
	puts("PROCESO S-AFA\n");

	initializeVariables();

	log_info(schedulerLog, "Inicio del proceso");

	console();

	exit(EXIT_SUCCESS);
}

void initializeVariables()
{
	executedInstructions = 0;
	dma_executedInstructions = 0;
	actualProcesses = 0;
	totalProcesses = 0;
	exit_executedInstructions = 0;
	totalConnectedCpus =  0;

	newQueue = list_create();
	readyQueue = list_create();
	blockedQueue = list_create();
	executingQueue = list_create();
	finishedQueue = list_create();
	connectedCPUs = list_create();

	consoleLog = init_log(PATH_LOG, "Consola S-AFA", false, LOG_LEVEL_INFO);
	schedulerLog = init_log(PATH_LOG, "Proceso S-AFA", true, LOG_LEVEL_INFO);
	config = load_config();

	pthread_attr_t* threadAttributes;
	pthread_attr_init(threadAttributes);
	pthread_attr_setdetachstate(threadAttributes, PTHREAD_CREATE_DETACHED);

	pthread_create(&serverThread, threadAttributes, (void *)server, NULL);

	pthread_create(&stsThread, threadAttributes, (void *)shortTermScheduler, NULL);

	pthread_create(ltsThread, threadAttributes, (void *)longTermScheduler, NULL);
}


config_t load_config()
{
	t_config *config = config_create(PATH_CONFIG);

	config_t configFile;
	configFile.port = config_get_int_value(config, "PUERTO");
	configFile.schedulingAlgorithm = strdup(config_get_string_value(config, "ALGORITMO"));
	configFile.quantum = config_get_int_value(config, "QUANTUM");
	configFile.multiprogrammingLevel = config_get_int_value(config, "MULTIPROGRAMACION");
	configFile.schedulingDelay = config_get_int_value(config, "RETARDO_PLANIF");

	log_info(schedulerLog, "---- Configuracion ----");
	log_info(schedulerLog, "PUERTO = %d", configFile.port);
	log_info(schedulerLog, "ALGORITMO = %s", configFile.schedulingAlgorithm);
	log_info(schedulerLog, "QUANTUM = %d", configFile.quantum);
	log_info(schedulerLog, "MULTIPROGRAMACION = %d", configFile.multiprogrammingLevel);
	log_info(schedulerLog, "RETARDO_PLANIF = %d milisegundos", configFile.schedulingDelay);
	log_info(schedulerLog, "-----------------------");

	config_destroy(config);
	return configFile;
}

void server()
{
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
	uint32_t servidor = build_server(config.port, consoleLog);

	// añadir listener al conjunto maestro
	FD_SET(servidor, &master);
	// seguir la pista del descriptor de fichero mayor
	fdmax = servidor; // por ahora es éste

	// bucle principal
	while (true)
	{
		read_fds = master; // cópialo
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
		{
			log_error(consoleLog, "Error en select");
			exit(EXIT_FAILURE);
		}

		// explorar conexiones existentes en busca de datos que leer
		for (uint32_t i = 0; i <= fdmax; i++)
		{
			if (FD_ISSET(i, &read_fds))
			{ // ¡¡tenemos datos!!
				if (i == servidor)
				{
					// gestionar nuevas conexiones
					addrlen = sizeof(remoteaddr);
					if ((newfd = accept(servidor, (struct sockaddr *) &remoteaddr, &addrlen)) == -1)
						log_error(consoleLog, "Error en accept");
					else
					{
						FD_SET(newfd, &master); // añadir al conjunto maestro
						if (newfd > fdmax) // actualizar el máximo
							fdmax = newfd;
						//log_info(log_consola, "Nueva conexion desde %s en el socket %d", inet_ntoa(remoteaddr.sin_addr), newfd);
					}
				}
				else
				{
					// gestionar datos de un cliente
					if ((nbytes = receive_int(i, &command)) <= 0)
					{
						// error o conexión cerrada por el cliente
						if (nbytes == 0)
						{
							// conexión cerrada
							if (i == dmaSocket)
								log_info(consoleLog, "DMA desconectado");
							else
								log_info(consoleLog, "CPU desconectada");
						}
						else
							log_error(consoleLog, "recv (comando)");

						close(i); // ¡Hasta luego!
						FD_CLR(i, &master); // eliminar del conjunto maestro
					}
					else
					{
						// tenemos datos de algún cliente
						moduleHandler(command, i);
					}
				}
			} // if (FD_ISSET(i, &read_fds))
		}
	} // while (true)
}

void moduleHandler(uint32_t command, uint32_t socket)
{
	switch (command)
	{
		case NUEVA_CONEXION_DMA:
			log_info(consoleLog, "Nueva conexion desde el DMA");
			dmaSocket = socket;
			break;
		case NUEVA_CONEXION_CPU:
			log_info(consoleLog, "Nueva conexion de CPU");

			cpu_t* cpuConnection = calloc(1, sizeof(cpu_t));
			cpuConnection->cpuId = ++totalConnectedCpus;
			cpuConnection->currentProcess = 0;
			cpuConnection->isFree = true;

			list_add(connectedCPUs, cpuConnection);
			break;
		case MSJ_DESDE_CPU:
			log_info(consoleLog, "Mensaje desde CPU"); //TODO: por ahi tengo que informar que cpu me manda msj
			taskHandler("CPU desconectada", socket, cpuTaskHandler); //TODO: por ahi tengo que informar que CPU se murio
			break;
		case MSJ_DESDE_DMA:
			log_info(consoleLog, "Mensaje desde DMA");
			taskHandler("DMA desconectado", socket, dmaTaskHandler);
			break;
		default:
			log_warning(consoleLog, "%d: Comando recibido incorrecto\n", command);
	}
}


void taskHandler(char* errorMsg, int socket, void (*taskHandler)(uint32_t, uint32_t))
{
	uint32_t nbytes;
	uint32_t task;

	if ((nbytes = receive_int(socket, &task)) <= 0)
	{
		// error o conexión cerrada por el cliente
		if (nbytes == 0)
			log_info(consoleLog, errorMsg);
		else
			log_error(consoleLog, "recv (comando)");

		close(socket); // ¡Hasta luego!
		FD_CLR(socket, &master); // eliminar del conjunto maestro

		//Falta hacer algo mas? Limpiar laguna estructura o algo? Esto me marea un poquito
	}
	else
	{
		// tenemos datos de algún cliente
		taskHandler(task, socket);
	}
}


void dmaTaskHandler(uint32_t task, uint32_t socket)
{
	switch(task)
	{
		//TODO
		default:
			log_warning(consoleLog, "La tarea %d es incorrecta", task);
	}
}


void cpuTaskHandler(uint32_t task, uint32_t socket)
{
	switch(task)
	{
		//TODO
		default:
			log_warning(consoleLog, "La tarea %d es incorrecta", task);
	}
}


void console()
{
	char *line;
	char *token;
	console_t *console;

	while (true)
	{
		line = readline("S-AFA> ");

		if (strlen(line) > 0)
		{
			add_history(line);
			log_info(consoleLog, "Linea leida: %s", line);
			console = malloc(sizeof(console_t));

			if (console != NULL)
			{
				console->command = strdup(strtok(line, " "));
				console->paramsQty = 0;

				while (console->paramsQty < MAX_PARAMS && (token = strtok(NULL, " ")) != NULL)
					console->param[console->paramsQty++] = strdup(token);

				if (str_eq(console->command, "clear"))
					system("clear");

				else if (str_eq(console->command, "ejecutar"))
				{
					if (console->paramsQty < 1)
						print_c(consoleLog, "%s: falta la ruta del script que se desea ejecutar\n", console->command);
					else
					{
						executeScript(console->param[0]);
					}
				}

				else if (str_eq(console->command, "status"))
				{
					// TODO: comando status
				}

				else if (str_eq(console->command, "finalizar"))
				{
					if (console->paramsQty < 1)
						print_c(consoleLog, "%s: falta el ID correspondiente a un DT Block\n", console->command);
					else
					{
						// TODO: comando finalizar
					}
				}

				else if (str_eq(console->command, "metricas"))
				{
					// TODO: comando metricas
				}

				else
					print_c(consoleLog, "%s: Comando incorrecto\n", console->command);

				free(console->command);
				for (uint32_t i = 0; i < console->paramsQty; i++)
					free(console->param[i]);
				free(console);
			}
		}

		free(line);
	}
}


void executeScript(char* script)
{
	PCB_t* process = createPCB(script);

	if(process == NULL)
	{
		log_error("Ocurrio un problema al intentar crear el PCB para el nuevo proceso");
		return;
	}

	addProcessToNewQueue(process);
	//STS tiene que activarse si no hay procesos ejecutando y hay procesos en Ready
}


PCB_t* crearPCB(char* scriptName)
{
	PCB_t* process = calloc(1, sizeof(PCB_t));

	if(process == NULL)	//no se pudo reservar memoria
		return NULL;

	process->fileTable = list_create();
	process->newQueueArrivalTime = executedInstructions;
	process->newQueueLeaveTime = 0;
	process->pid = ++totalProcesses;
	process->programCounter = NULL;
	process->script = strdup(scriptName); // scriptName es parte de una estructura que va a ser liberada
	process->wasInitialized = false;

	return process;
}

void addProcessToNewQueue(PCB_t* process)
{
	list_add(newQueue, process);

	//TODO: Signal LTS -> semaphores needed
}

//TODO: regarding the sts, if a new cpu connects, it must be activated
//(that new cpu will be selected anyways, no need to specify one for the STS to assign a process to it)
