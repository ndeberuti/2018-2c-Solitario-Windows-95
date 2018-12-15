#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <commons/log.h>
#include "../../Bibliotecas/servidor/servidor.h"


typedef struct
{
	uint32_t pid;
	char* scriptPathInFS;
	uint32_t programCounter;	//Contains the line number (in the script file) of the last executed instruction
	bool wasInitialized;
	bool canBeScheduled;
	uint32_t executionState;
	uint32_t cpuProcessIsAssignedTo; //To get the socket of that cpu and send it a message, for example, if i need to
	uint32_t remainingQuantum;
	//t_list* fileTable;			//According to the assignment's text, the PCB has to have a file table...need to ask the teachers

	//Metrics
	uint32_t newQueueArrivalTime;
	uint32_t newQueueLeaveTime;
	uint32_t totalInstructionsExecuted;
	uint32_t instructionsExecutedOnLastExecution;
	uint32_t instructionsUntilIoOrEnd;

	uint32_t completedDmaCalls;	//How many calls to the DMA were made; this is incremented when a process is unblocked by a request from the DMA
	uint32_t lastIOStartTime;
	uint32_t responseTimes;		//All the response times added in one variable (ResponseTime = ioStartTime - ioStopTime = processBlockTime - processUnblockTime)

} PCB_t;


fd_set master;
t_log* log;
uint32_t totalProcesses = 0;

void* serializePCB(PCB_t*);
uint32_t sendPCB(uint32_t, PCB_t*);
uint32_t sendPCBReloaded(uint32_t, PCB_t*);
void server();
void command_handler();
void createAndSendPCB(uint32_t);
void showPCBData(PCB_t*);
int main(void);



void* serializePCB(PCB_t* pcb)
{
	uint32_t scriptNameSize = strlen(pcb->scriptPathInFS) + 1;
	void* buffer = calloc(1, (sizeof(uint32_t) * 17) + scriptNameSize);

	memcpy(buffer, &(pcb->pid), sizeof(uint32_t));
	memcpy(buffer + sizeof(uint32_t), &scriptNameSize, sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 2), pcb->scriptPathInFS, scriptNameSize);
	memcpy(buffer + (sizeof(uint32_t) * 3) + scriptNameSize, &(pcb->programCounter), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 4) + scriptNameSize, &(pcb->wasInitialized), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 5) + scriptNameSize, &(pcb->canBeScheduled), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 6) + scriptNameSize, &(pcb->executionState), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 7) + scriptNameSize, &(pcb->cpuProcessIsAssignedTo), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 8) + scriptNameSize, &(pcb->remainingQuantum), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 9) + scriptNameSize, &(pcb->newQueueArrivalTime), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 10) + scriptNameSize, &(pcb->newQueueLeaveTime), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 11) + scriptNameSize, &(pcb->totalInstructionsExecuted), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 12) + scriptNameSize, &(pcb->instructionsExecutedOnLastExecution), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 13) + scriptNameSize, &(pcb->instructionsUntilIoOrEnd), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 14) + scriptNameSize, &(pcb->completedDmaCalls), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 15) + scriptNameSize, &(pcb->lastIOStartTime), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 16) + scriptNameSize, &(pcb->responseTimes), sizeof(uint32_t));

	log_info(log, "PCB serializado correctamente");

	return buffer;
}

uint32_t sendPCB(uint32_t _socket, PCB_t* pcb)
{
	void* buffer = serializePCB(pcb);
	uint32_t scriptNameSize = strlen(pcb->scriptPathInFS) + 1;
	uint32_t bufferSize = scriptNameSize + (sizeof(uint32_t) * 17);

	return send(_socket, buffer, bufferSize, MSG_WAITALL);

	log_info(log, "PCB enviado correctamente");
}

uint32_t sendPCBReloaded(uint32_t _socket, PCB_t* pcb)
{
	int32_t bytesSent = 0, nbytes = 0;

	if((nbytes = send_int(_socket, pcb->pid)) < 0)
	{
		log_info(log, "Error al enviar pid del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		log_info(log, "Se envio pid del PCB");
	}

	if((nbytes = send_string(_socket, pcb->scriptPathInFS)) < 0)
	{
		log_info(log, "Error al enviar scriptPathInFS del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		log_info(log, "Se envio scriptPathInFS del PCB");
	}

	if((nbytes = send_int(_socket, pcb->programCounter)) < 0)
	{
		log_info(log, "Error al enviar programCounter del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		log_info(log, "Se envio programCounter del PCB");
	}

	if((nbytes = send_int(_socket, pcb->wasInitialized)) < 0)
	{
		log_info(log, "Error al enviar wasInitialized del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		log_info(log, "Se envio wasInitialized del PCB");
	}

	if((nbytes = send_int(_socket, pcb->canBeScheduled)) < 0)
	{
		log_info(log, "Error al enviar canBeScheduled del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		log_info(log, "Se envio canBeScheduled del PCB");
	}

	if((nbytes = send_int(_socket, pcb->executionState)) < 0)
	{
		log_info(log, "Error al enviar executionState del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		log_info(log, "Se envio executionState del PCB");
	}

	if((nbytes = send_int(_socket, pcb->cpuProcessIsAssignedTo)) < 0)
	{
		log_info(log, "Error al enviar cpuProcessIsAssignedTo del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		log_info(log, "Se envio cpuProcessIsAssignedTo del PCB");
	}

	if((nbytes = send_int(_socket, pcb->remainingQuantum)) < 0)
	{
		log_info(log, "Error al enviar remainingQuantum del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		log_info(log, "Se envio remainingQuantum del PCB");
	}

	if((nbytes = send_int(_socket, pcb->newQueueArrivalTime)) < 0)
	{
		log_info(log, "Error al enviar newQueueArrivalTime del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		log_info(log, "Se envio newQueueArrivalTime del PCB");
	}

	if((nbytes = send_int(_socket, pcb->newQueueLeaveTime)) < 0)
	{
		log_info(log, "Error al enviar newQueueLeaveTime del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		log_info(log, "Se enviar newQueueLeaveTime del PCB");
	}

	if((nbytes = send_int(_socket, pcb->totalInstructionsExecuted)) < 0)
	{
		log_info(log, "Error al enviar totalInstructionsExecuted del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		log_info(log, "Se envio totalInstructionsExecuted del PCB");
	}

	if((nbytes = send_int(_socket, pcb->instructionsExecutedOnLastExecution)) < 0)
	{
		log_info(log, "Error al enviar instructionsExecutedOnLastExecution del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		log_info(log, "Se envio instructionsExecutedOnLastExecution del PCB");
	}

	if((nbytes = send_int(_socket, pcb->instructionsUntilIoOrEnd)) < 0)
	{
		log_info(log, "Error al enviar instructionsUntilIoOrEnd del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		log_info(log, "Se envio instructionsUntilIoOrEnd del PCB");
	}

	if((nbytes = send_int(_socket, pcb->completedDmaCalls)) < 0)
	{
		log_info(log, "Error al enviar dmaCalls del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		log_info(log, "Se envio dmaCalls del PCB");
	}

	if((nbytes = send_int(_socket, pcb->lastIOStartTime)) < 0)
	{
		log_info(log, "Error al enviar lastIOStartTime del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		log_info(log, "Se envio lastIOStartTime del PCB");
	}

	if((nbytes = send_int(_socket, pcb->responseTimes)) < 0)
	{
		log_info(log, "Error al enviar responseTimes del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		log_info(log, "Se envio responseTimes del PCB");
	}

	log_info(log, "PCB enviado correctamente");

	return bytesSent;
}


void server()
{
	fd_set read_fds; //Temporal FD set used in the select function
	struct sockaddr_in remoteaddr; //Client address
	uint32_t fdmax; //Number of the maximum FD
	uint32_t newfd; //New connection socket
	int32_t command; //Client command
	uint32_t nbytes;
	uint32_t addrlen;
	FD_ZERO(&master); //Set to zero all sockets in the master and read FD sets
	FD_ZERO(&read_fds);

	//Get listener socket
	uint32_t servidor = build_server(8000, log);

	//Add listener to the master FD set
	FD_SET(servidor, &master);
	//Keep track of the maximum GD
	fdmax = servidor;

	//Main loop
	while (true)
	{
		read_fds = master;
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
		{
			log_error(log, "Error de select!. El proceso sera abortado...");
			exit(EXIT_FAILURE);
		}
		//Check existing connections and wait for data to read fom any of them
		for (uint32_t _socket = 0; _socket <= fdmax; _socket++)
			if (FD_ISSET(_socket, &read_fds))
			{ //Data received
				if (_socket == servidor)
				{
					//Manage new connections
					addrlen = sizeof(remoteaddr);
					if ((newfd = accept(servidor, (struct sockaddr *) &remoteaddr, &addrlen)) == -1)
						log_error(log, "Error de accept!");
					else
					{
						FD_SET(newfd, &master); //Add the FD to the master set
						if (newfd > fdmax) //Update the maximum fd
						{
							fdmax = newfd;
						}
					}
				}
				else
					//Manage client data
					if ((nbytes = receive_int(_socket, &command)) <= 0)
					{
						//An error occurred or a connection was closed
						if (nbytes == 0)
						{
							log_error(log, "Se desconecto un cliente al intentar recibir un tarea...");
						}
						else
							log_error(log, "Error en recv (comando)");

						close(_socket);
						FD_CLR(_socket, &master); //Delete the socket from the master FdSet
					}
					else
					{
						//Receive data from a client
						command_handler(command, _socket);
					}
			} // if (FD_ISSET(i, &read_fds))
	} // while (true)
}

void command_handler(uint32_t command, uint32_t _socket)
{
	switch(command)
	{
		case 1:
			createAndSendPCB(_socket);
		break;
	}
}

void createAndSendPCB(uint32_t _socket)
{

	PCB_t* process = calloc(1, sizeof(PCB_t));

	if(process == NULL)//no se pudo reservar memoria
	{
		log_error(log, "Error al reserver memoria para el PCB");
		return;
	}

	process->newQueueArrivalTime = 5;
	process->pid = ++totalProcesses;
	process->newQueueLeaveTime = 0;
	process->programCounter = 0;
	process->scriptPathInFS = strdup("HolaSoyUnPath"); // scriptName es parte de una estructura que va a ser liberada
	process->wasInitialized = false;
	process->canBeScheduled = false;
	process->executionState = 2;
	process->cpuProcessIsAssignedTo = 6;
	process->completedDmaCalls = 7;
	process->responseTimes = 50;
	process->lastIOStartTime = 60;
	process->totalInstructionsExecuted = 9;
	process->instructionsExecutedOnLastExecution = 2;
	process->instructionsUntilIoOrEnd = 40;

	showPCBData(process);

	//sendPCB(_socket, process);

	sendPCBReloaded(_socket, process);
}

void showPCBData(PCB_t* process)
{
	char *queueName = "READY";
	char *scriptName = NULL, *wasInitialized = NULL, *canBeScheduled = NULL;
	uint32_t pid, programCounter, cpuProcessIsAssignedTo, remainingQuantum, newQueueArrivalTime;
	uint32_t newQueueLeaveTime, instructionsExecuted, completedDmaCalls, lastIOStartTime, responseTimes;
	uint32_t instructionsExecutedOnLastExecution, instructionsUntilIoOrEnd, executionState;


	if(process != NULL)
	{
		pid = process->pid;
		programCounter = process->programCounter;
		cpuProcessIsAssignedTo = process->cpuProcessIsAssignedTo;
		remainingQuantum = process->remainingQuantum;
		newQueueArrivalTime = process->newQueueArrivalTime;
		newQueueLeaveTime = process->newQueueLeaveTime;
		instructionsExecuted = process->totalInstructionsExecuted;
		instructionsExecutedOnLastExecution = process->instructionsExecutedOnLastExecution;
		instructionsUntilIoOrEnd = process->instructionsUntilIoOrEnd;
		completedDmaCalls = process->completedDmaCalls;
		lastIOStartTime = process->lastIOStartTime;
		responseTimes = process->responseTimes;
		scriptName = process->scriptPathInFS;
		executionState = process->executionState;

		if(process->wasInitialized)
			wasInitialized = "TRUE";
		else
			wasInitialized = "FALSE";

		if(process->canBeScheduled)
			canBeScheduled = "TRUE";
		else
			canBeScheduled = "FALSE";


		log_info(log, "-----Valores de las variables del PCB con id %d-----", pid);
		log_info(log, "\tscriptName: %s", scriptName);
		log_info(log, "\tprogramCounter: %d", programCounter);
		log_info(log, "\twasInitialized: %s", wasInitialized);
		log_info(log, "\tcanBeScheduled: %s", canBeScheduled);
		log_info(log, "\texecutionState: %s", queueName);
		log_info(log, "\tcpuProcessIsAssignedTo: %d", cpuProcessIsAssignedTo);
		log_info(log, "\tremainingQuantum: %d", remainingQuantum);
		log_info(log, "\tnewQueueArrivalTime: %d", newQueueArrivalTime);
		log_info(log, "\tnewQueueLeaveTime: %d", newQueueLeaveTime);
		log_info(log, "\tinstructionsExecuted: %d", instructionsExecuted);
		log_info(log, "\tinstructionsExecutedOnLastExecution: %d", instructionsExecutedOnLastExecution);
		log_info(log, "\tinstructionsUntilIoOrEnd: %d", instructionsUntilIoOrEnd);
		log_info(log, "\tcompletedDmaCalls: %d", completedDmaCalls);
		log_info(log, "\tlastIOStartTime: %d", lastIOStartTime);
		log_info(log, "\tresponseTimes: %d", responseTimes);
		log_info(log, "\texecutionState: %d", executionState);
	}
}

int main(void)
{
	system("clear");

	printf("\n----Test de envio de PCBs----\n\n");

	log = log_create("../../Logs/TestServer.log", "TestServer", true, LOG_LEVEL_INFO);

	server();
}
