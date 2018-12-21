#ifndef SRC_DMA_H_
#define SRC_DMA_H_

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
#include "commons/config.h"
#include "servidor/servidor.h"
#include "enums.h"


typedef struct
{
	uint32_t port;
	char* schedulerIp;
	uint32_t schedulerPort;
	char* fileSystemIp;
	uint32_t fileSystemPort;
	char* memoryIp;
	uint32_t memoryPort;
	uint32_t transferSize;
	//
} config_t;


t_log* dmaLog;
fd_set master; //Master FD set
config_t config;
uint32_t schedulerServerSocket;
uint32_t fileSystemServerSocket;
uint32_t memoryServerSocket;
uint32_t memoryLineSize;	//How many bytes does a memory line take
pthread_t serverThread;


//DMA
void initializeVariables();
void connectToServers();

//DMAFunctions
int32_t getConfigs();
void showConfigs();
bool checkIfFileIsInFS(char*);
char* getFileFromFS(char*);
void sendProcessErrorMessageToScheduler(uint32_t);
t_list* parseScriptFromMemory(char*, uint32_t);
t_list* parseScriptFromFS(char*);
char* convertParsedFileToMemoryBuffer(t_list*, uint32_t*);
bool sendFileToMemory(char*, uint32_t, char*, uint32_t, uint32_t);
char* convertParsedFileToFileSystemBuffer(t_list*);
char* getFileFromMemory(char*, uint32_t*);
int32_t sendFileToFileSystem(char*, char*);
void tellSchedulerToUnblockProcess(uint32_t, char*, bool, char*);

//ServerThread
void server();
void commandHandler(uint32_t, uint32_t);
void openFile(uint32_t, bool);
void deleteFile(uint32_t);
void createFile(uint32_t);
void flushFile(uint32_t);


#endif /* SRC_DMA_H_ */
