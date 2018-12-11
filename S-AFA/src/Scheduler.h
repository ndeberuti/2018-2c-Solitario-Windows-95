#ifndef SRC_SCHEDULER_H_
#define SRC_SCHEDULER_H_

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "funciones/funciones.h"
#include <commons/config.h>
#include <commons/collections/dictionary.h>
#include "servidor/servidor.h"
#include "PCB.h"				
#include "enums.h"
#include <stdint.h>
#include <semaphore.h>
#include <sys/inotify.h>


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
} fileTableReg;

typedef struct
{
	uint32_t cpuId;
	bool isFree;
	uint32_t currentProcess; //the process that a cpu is executing; 0 if it is free
	uint32_t clientSocket;
	uint32_t serverSocket;
	uint32_t serverPort;
	char* serverIp;
	//maybe new fields?
} cpu_t;

typedef struct
{
	uint32_t memoryStartAddress;	//Address where the file starts in memory
	//uint32_t fileSizeInMemory;		//If the file gets modified, the CPU should tell this module what is the new size; difficult to do
	uint32_t processFileIsAssignedTo;
	t_list* processesWaitingForFile;	//When the file stopts being used by a process, the
										//first one from this list should be removed from the blockedQueue
	//maybe new fields?
}fileTableData;

typedef struct
{
	uint32_t semaphoreValue;
	t_list* processesUsingTheSemaphore;
	t_list* waitingProcesses;				//Processes waiting for the semaphore to be freed
	//maybe new fields
}semaphoreData;


t_log* consoleLog;
t_log* schedulerLog;
config_t config;
pthread_t serverThread;
pthread_t ltsThread;
pthread_t stsThread;
uint32_t dmaSocket;
uint32_t configFileInotifyFD;
fd_set master; //Master File Descriptors Set
uint32_t executedInstructions; //A System's actual time measured in executed instructions
uint32_t dma_executedInstructions;
uint32_t killProcessInstructions;		//Instructions that caused abnormal process terminations
uint32_t systemResponseTime;			//Sum of all response times from all processes
uint32_t actualProcesses;
uint32_t totalProcesses;
uint32_t totalConnectedCpus;
uint32_t activeProcesses;
t_list* newQueue;
t_list* readyQueue;
t_list* blockedQueue;
t_list* executionQueue;
t_list* finishedQueue;
t_list* connectedCPUs;
t_list* ioReadyQueue;

t_list* fileTableKeys;		//Because we could not add a keys list to the dictionary implementation provided by the teachers due to problems with
t_list* semaphoreListKeys;	//what we tried to implement (adding a t_list* to the dictionary struct, and modifying the corresponding functions to
							//add or remove keys from that list), to avoid problems we chose to create the key lists outside the dictionary
t_dictionary* fileTable;
t_dictionary* semaphoreList;
bool killThreads;
bool STSAlreadyExecuting;
bool LTSAlreadyExecuting;
bool terminateModule;
sem_t shortTermScheduler;
sem_t schedulerNotRunning;
sem_t longTermScheduler;
pthread_mutex_t configFileMutex;
pthread_mutex_t readyQueueMutex;
pthread_mutex_t newQueueMutex;
pthread_mutex_t blockedQueueMutex;
pthread_mutex_t executionQueueMutex;
pthread_mutex_t ioReadyQueueMutex;
pthread_mutex_t cpuListMutex;
pthread_mutex_t finishedQueueMutex;
pthread_mutex_t fileTableMutex;
pthread_mutex_t semaphoreListMutex;
pthread_mutex_t processIOTimesTableMutex;
pthread_mutex_t metricsGlobalvariablesMutex; //mutex for the "executedInstructions", "dma_executedInstructions"
											 //"killProcessInstructions", "systemResponseTime" variables


void (*scheduleProcesses)(); //Plointer to a function that takes an returns no values


//Scheduler functions
void _checkExecProc_and_algorithm();
void initializeVariables();
void longTermSchedulerThread();
void shortTermSchedulerThread();

//SchedulerFunctions
int32_t getConfigs();
void checkAlgorithm();
void addProcessToNewQueue(PCB_t*);
uint32_t addProcessToReadyQueue(PCB_t*);
void moveProcessToReadyQueue(PCB_t*, bool);
void addProcessToBlockedQueue(PCB_t*);
PCB_t* createProcess(char*);
void terminateExecutingProcess(uint32_t);
void unblockProcess(uint32_t, bool);
void blockProcess(uint32_t, bool);
uint32_t getFreeCPUsQty();
t_list* getFreeCPUs();
void executeProcess(PCB_t*, cpu_t*);
void killProcess(uint32_t);
void closeSocketAndRemoveCPU(uint32_t);
void checkAndFreeProcessFiles(uint32_t);
void checkAndFreeProcessSemaphores(uint32_t);
int32_t send_int_with_delay(uint32_t, uint32_t);
void freeCpuElement(cpu_t*);
void freePCB(PCB_t*);
void freeFileTableData(fileTableData*);
void freeSemaphoreListData(semaphoreData*);
t_list* getSchedulableProcesses();
void removeKeyFromList(t_list*, char*);
void showConfigs();
int32_t send_PCB_with_delay(PCB_t*, uint32_t);

//Algorithms
void roundRobinScheduler();
void virtualRoundRobinScheduler();
void customScheduler();
uint32_t countProcessInstructions(PCB_t*, cpu_t*);

//Console functions (to avoid "conflicting types" error)
void console();
void executeScript(char*);
void getProcessMetrics(uint32_t);
void getSystemMetrics();
void getProcessStatus(uint32_t);
void getQueuesStatus();
void terminateProcessConsole(uint32_t);
PCB_t* getProcessFromSchedulingQueues(uint32_t, char*);

//ServerThread functions
void server();
void moduleHandler(uint32_t, uint32_t);
void dmaTaskHandler(uint32_t, uint32_t);
void cpuTaskHandler(uint32_t, uint32_t);
void blockProcessInit(uint32_t);
void _blockProcess(uint32_t);
void _killProcess(uint32_t);
void processQuantumEnd(uint32_t);
cpu_t* findCPUBySocket(uint32_t);
void checkAndInitializeProcesses(cpu_t*);
uint32_t updatePCBInExecutionQueue(PCB_t*);
void checkIfFileOpen(uint32_t);
void saveFileDataToFileTable(uint32_t, uint32_t);
void closeFile(uint32_t);
void unlockProcess(uint32_t);
void signalResource(uint32_t);
void waitResource(uint32_t);
void freeCPUBySocket(uint32_t);
cpu_t* findCPUBy_socket(uint32_t);
void terminateProcess(uint32_t);
void handleConfigFileChanged();
void handleCpuConnection(uint32_t);


#endif /* SRC_SCHEDULER_H_ */
