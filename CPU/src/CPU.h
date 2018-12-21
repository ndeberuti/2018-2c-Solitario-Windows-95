#ifndef SRC_CPU_H_
#define SRC_CPU_H_

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
#include <errno.h>
#include "funciones/funciones.h"
#include "commons/config.h"
#include "commons/string.h"
#include "servidor/servidor.h"
#include <semaphore.h>
#include "enums.h"
#include "PCB.h"


typedef struct
{
	char *schedulerIp;
	uint32_t schedulerPort;
	char *dmaIp;
	uint32_t dmaPort;
	char *memoryIp;
	uint32_t memoryPort;
	char *cpuIp;	  //This is used to tell the other modules (in the handshake) this module's ip/port, so they can connect to this module's server if the want to
	uint32_t cpuPort; //(for example, the scheduler needs to connect to this module's server in order to send special requests, like killing a process in execution)
	uint32_t executionDelay;
	//
} config_t;


t_log* cpuLog;
t_log* socketErrorLog;		//select/recv/send error log
config_t config;
uint32_t schedulerServerSocket;	//The one obtained when this module connects to the scheduler
//uint32_t schedulerClientSocket;	//The one obtained when the scheduler connects to this module's server
uint32_t dmaServerSocket;
uint32_t memoryServerSocket;
uint32_t instructionsExecuted;	//Instructions executed for the actual process
uint32_t currentProcessQuantum;
uint32_t currentProgramCounter;
uint32_t memoryLineSize;

PCB_t** processInExecutionPCB;	//Has the PCB of the process being executed
bool terminateModule;			//Used to tell all the threads the process must be closed
bool usingCustomSchedulingAlgorithm;
fd_set master; //Master File Descriptors Set
pthread_t serverThread;


//CPU
void initializeVariables();

//ServerThread
/*
void server();
void moduleHandler(uint32_t, uint32_t);
void schedulerTaskHandler(uint32_t, uint32_t);
void killProcess(uint32_t);
void blockProcess(uint32_t);
*/

//CpuFunctions
int32_t getConfigs();
int32_t handshakeScheduler(uint32_t socket);
void connectToServers();
void executeProcess();
void initializeProcess();
t_list* parseScript(char*, uint32_t);
t_list* parseLine(char*);
uint32_t checkAndExecuteInstruction(t_list*);
uint32_t openFile(char*);
uint32_t modifyFileLineInMemory(char*, uint32_t, char*);
uint32_t waitSemaphore(char*);
uint32_t signalSemaphore(char*);
uint32_t flushFile(char*);
uint32_t closeFile(char*);
uint32_t createFile(char*, uint32_t);
uint32_t deleteFile(char*);
uint32_t askSchedulerIfFileOpen(char*);
uint32_t handleFileNotOpen(char*, bool);
uint32_t handleFileOpenedByOtherProcess(char*, bool);
uint32_t handleProcessError();
void tellMemoryToFreeProcessData();
uint32_t tellSchedulerToBlockProcess(bool);
uint32_t handleModifyFile(char*, uint32_t, char*);
uint32_t handleFlushFile(char*);
uint32_t handleCloseFile(char*);
void updateCurrentPCB();
void updatePCBAndSendExecutionEndMessage(uint32_t);
void removeCommentsFromScript(t_list**);
uint32_t countInstructionsUntilEndOrIO(t_list*);
bool isIoInstruction(t_list*);
void countProcessInstructions();
char* requestScriptFromMemory(uint32_t*);
void showConfigs();
void sendOpenFileRequestToDMA(char*, uint32_t);
void freePCB(PCB_t*);
void executionDelay();


#endif /* SRC_CPU_H_ */
