/*
 * CPU.c
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */
#include "CPU.h"

void initializeVariables()
{
	cpuLog = init_log("../../Logs/CPU.log", "Proceso CPU", true, LOG_LEVEL_INFO);

	terminateModule = false;

	getConfigs();

	pthread_attr_t* threadAttributes = NULL;
	pthread_attr_init(threadAttributes);
	pthread_attr_setdetachstate(threadAttributes, PTHREAD_CREATE_DETACHED);

	pthread_create(&serverThread, threadAttributes, (void *)server, NULL);

	connectToServers();
}

void executeProcesses()
{
	int32_t nbytes;
	uint32_t task;

	while(!terminateModule)
	{
		if((nbytes = receive_int(schedulerServerSocket, &task)) <= 0)
		{
			if(nbytes == 0)
				log_error(cpuLog, "EL planificador fue desconectado al intentar recibir una tarea del mismo\n");
			if(nbytes < 0)
				log_error(cpuLog, "Error al intentar recibir una tarea del planificador\n");

			log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
			exit(EXIT_FAILURE);
		}

		switch(task)
		{
			case INITIALIZE_PROCESS:
				initializeProcess();
			break;

			case EXECUTE_PROCESS:
				executeProcess();
			break;

			default:
				log_error(cpuLog, "Se recibio un id de tarea incorrecto del planificador!\n");
			break;
		}
	}

	//TODO - main execution loop
	//should use a "while(true)" wait for the scheduler to tell this module to execute or initialize a process and, in case the process is
	//being executed, it should stop reading instructions from the script the memory sent if the serverThread set a boolean variable
	//telling this main thread to kil or block (one for each case) the process in execution

}

int main(void)
{
	system("clear");
	puts("PROCESO CPU\n");

	initializeVariables();

	log_info(cpuLog, "Inicio del proceso\n");

	executeProcesses();

	exit(EXIT_SUCCESS);
}
