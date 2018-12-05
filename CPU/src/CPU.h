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
#include "funciones/funciones.h"
#include <commons/config.h>
#include "commons/string.h"
#include "servidor/servidor.h"
#include "enums.h"
#include "PCB.h"

#define CONEXION_CPU 4

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
config_t config;
uint32_t schedulerServerSocket;	//The one obtained when this module connects to the scheduler
uint32_t schedulerClientSocket;	//The one obtained when the scheduler connects to this module's server
uint32_t dmaServerSocket;
uint32_t memoryServerSocket;
uint32_t instructionsExecuted;	//Instructions executed for the actual process
uint32_t currentProcessQuantum;
uint32_t currentProgramCounter;
PCB_t* processInExecutionPCB;	//Has the PCB of the process being executed
bool terminateModule;			//Used to tell all the threads the process must be closed
bool stopExecution;				//Used to stop execution when needing to kill a process from the scheduler's console
bool killExecutingProcess;
bool blockExecutingProcess;
bool usingCustomSchedulingAlgorithm;
fd_set master; //Master File Descriptors Set
pthread_t serverThread;


//CPU.c Functions
void initializeVariables();

//ServerThread.c functions
void server();
void moduleHandler(uint32_t, uint32_t);
void schedulerTaskHandler(uint32_t, uint32_t);
void killProcess(uint32_t);
void blockProcess(uint32_t);

//CpuFunctions.c Functions
int32_t getConfigs();
int32_t handshakeScheduler(uint32_t socket);
void connectToServers();
void executeProcess();
void initializeProcess();
t_list* parseScript(char*);
t_list* parseLine(char*);
bool checkAndExecuteInstruction(t_list*);
void openFile(char*);
void modifyFileLineInMemory(char*, uint32_t, char*);
void waitSemaphore(char*);
void signalSemaphore(char*);
void flushFile(char*);
void closeFile(char*);
void createFile(char*, uint32_t);
void deleteFile(char*);
uint32_t askSchedulerIfFileOpen(char*);
void handleFileNotOpen(char*, bool);
void handleFileOpenedByOtherProcess(char*, bool);
void handleProcessError();
void tellMemoryToFreeProcessData();
void tellSchedulerToBlockProcess(bool);
void handleModifyFile(char*, uint32_t, char*);
void handleFlushFile(char*);
void handleCloseFile(char*);
void updateCurrentPCB();
void updatePCBAndSendExecutionEndMessage(uint32_t);
void removeCommentsFromScript(t_list*);
uint32_t countInstructionsUntilEndOrIO(t_list*);
bool isIoInstruction(t_list*);
void countProcessInstructions();
char* requestScriptFromMemory();


#endif /* SRC_CPU_H_ */
