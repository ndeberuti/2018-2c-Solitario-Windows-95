/*
 * CPU.h
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */

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
#include "servidor/servidor.h"
#include "enums.h"

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


t_log *cpuLog;
config_t config;
uint32_t schedulerServerSocket;
uint32_t dmaServerSocket;
uint32_t memoryServerSocket;
uint32_t schedulerClientSocket;
fd_set master; //Master File Descriptors Set
pthread_t serverThread;

//CPU.c Functions
void initializeVariables();

//ServerThread.c functions
void server();
int32_t handshakeProcess(uint32_t);
void connectToServers();
int32_t getConfigs();

#endif /* SRC_CPU_H_ */
