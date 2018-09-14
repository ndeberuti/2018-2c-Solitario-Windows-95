/*
 * S-AFA.h
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */

#ifndef SRC_S_AFA_H_
#define SRC_S_AFA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "funciones/funciones.h"
#include "commons/config.h"
#include "commons/collections/list.h"
#include "servidor/servidor.h"
#include "enums.h"
#include <stdint.h>

// constantes
char *PATH_LOG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Logs/logSAFA.txt";
char *PATH_CONFIG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/S-AFA/config.txt";

#define MAX_PARAMS 1

// estructuras
typedef struct
{
	uint32_t port;
	char* schedulingAlgorithm;
	uint32_t quantum;
	uint32_t multiprogrammingLevel;
	uint32_t schedulingDelay;
} config_t;

typedef struct
{
	char *command;
	char *param[MAX_PARAMS];
	uint32_t paramsQty;
} console_t;

typedef struct
{
	char* fileName;
	char* filePath;
	void* fileStartAddress; //No se el tipo de dato
} fileTableReg;

typedef struct
{
	uint32_t pid;
	char* script;
	void* programCounter; //No se el tipo de dato
	bool wasInitialized;
	t_list* fileTable;
	uint32_t newQueueArrivalTime;
	uint32_t newQueueLeaveTime;
} PCB_t;

typedef struct
{
	uint32_t cpuId;
	bool isFree;
	uint32_t currentProcess; //the process that a cpu is executing; 0 if it is free
	//TODO: maybe new fields?
} cpu_t;

// variables
t_log* consoleLog;
t_log* schedulerLog;
config_t config;
pthread_t serverThread;
pthread_t ltsThread;
pthread_t stsThread;
uint32_t dmaSocket;
fd_set master; // conjunto maestro de descriptores de fichero
uint32_t executedInstructions; //AKA tiempo actual del sistema medido en instrucciones ejecutadas
uint32_t dma_executedInstructions;
uint32_t actualProcesses;
uint32_t totalProcesses;
uint32_t totalConnectedCpus;
uint32_t exit_executedInstructions;
t_list* newQueue;
t_list* readyQueue;
t_list* blockedQueue;
t_list* executingQueue;
t_list* finishedQueue;
t_list* connectedCPUs;



// funciones
config_t load_config();
void server();
void moduleHandler(uint32_t command, uint32_t socket);
void longTermScheduler();
void shortTermScheduler();

#endif /* SRC_S_AFA_H_ */
