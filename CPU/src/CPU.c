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
	log_info(cpuLog, "Inicio del proceso");

	getConfigs();

	pthread_attr_t* threadAttributes = NULL;
	pthread_attr_init(threadAttributes);
	pthread_attr_setdetachstate(threadAttributes, PTHREAD_CREATE_DETACHED);

	pthread_create(&serverThread, threadAttributes, (void *)server, NULL);

	connectToServers();
}

void executeProcesses()
{
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

	executeProcesses();

	exit(EXIT_SUCCESS);
}
