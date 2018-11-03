/*
 * S-AFA.h
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */

#ifndef SRC_SCHEDULER_H_
#define SRC_SCHEDULER_H_

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
#include <semaphore.h>

// constantes
char *PATH_LOG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Logs/logSAFA.txt";
char *PATH_CONFIG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/S-AFA/config.txt";

#define MAX_PARAMS 1

enum ExecutionStates
{
	SYSTEM_PROCESS = 0,
	NEW = 1,
	READY = 2,
	READY_IO = 3,
	EXECUTING = 4,
	BLOCKED = 5,
	TERMINATED = 6
};

enum SchedulingAlgorithmCodes
{
	RR = 1,
	VRR = 2,
	Custom = 3
};


typedef struct
{
	uint32_t port;
	char* schedulingAlgorithm;
	uint32_t schedulingAlgorithmCode;
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
	uint32_t cpuId;
	bool isFree;
	uint32_t currentProcess; //the process that a cpu is executing; 0 if it is free
	uint32_t socket;
	//TODO: maybe new fields?
} cpu_t;


t_log* consoleLog;
t_log* schedulerLog;
config_t config;
pthread_t serverThread;
pthread_t ltsThread;
pthread_t stsThread;
uint32_t dmaSocket;
fd_set master; //Master File Descriptors Set
uint32_t executedInstructions; //AKA System's actual time measured in executed instructions
uint32_t dma_executedInstructions;
uint32_t actualProcesses;
uint32_t totalProcesses;
uint32_t totalConnectedCpus;
uint32_t exit_executedInstructions;
uint32_t activeProcesses;
t_list* newQueue;
t_list* readyQueue;
t_list* blockedQueue;
t_list* executionQueue;
t_list* finishedQueue;
t_list* connectedCPUs;
t_list* ioReadyQueue;
bool killThreads;
bool STSAlreadyExecuting;
bool LTSAlreadyExecuting;
sem_t shortTermScheduler;
sem_t schedulerNotRunning;
sem_t longTermScheduler;
pthread_mutex_t readyQueueMutex;
pthread_mutex_t newQueueMutex;


void (*scheduleProcesses)(); //'scheduler' is a pointer to a function that takes an returns no values


config_t getConfigs();
void server();
void moduleHandler(uint32_t, uint32_t);
void longTermSchedulerThread();
void shortTermSchedulerThread();
void addProcessToNewQueue(PCB_t*);
PCB_t* createProcess(char*);
void terminateProcess(PCB_t*);
void console();
void roundRobinScheduler();
void virtualRoundRobinScheduler();
void customScheduler();
void fifoScheduler();
void sendPCB(PCB_t*, uint32_t);


#endif /* SRC_SCHEDULER_H_ */
