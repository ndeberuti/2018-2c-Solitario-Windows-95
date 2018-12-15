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


t_log* log;
uint32_t serverSocket;


uint32_t recvPCB(uint32_t _socket, PCB_t** pcb)	//El pcb es una variable que se declara pero se inicializa en la funcion;
{												//esta hecho asi para permitir detectar errores y actuar acorde a ellos

	uint32_t pid, programCounter, wasInitialized, canBeScheduled, newQueueArrivalTime;
	uint32_t newQueueLeaveTime, executionState, cpuProcessIsAssignedTo, remainingQuantum, lastIOStartTime;
	uint32_t dmaCalls, totalInstructionsExecuted, responseTimes, bytesReceived, nbytes;
	uint32_t instructionsExecutedOnLastExecution, instructionsUntilIoOrEnd;

	char* scriptPathInFS = NULL;

	bytesReceived = 0;

	if((nbytes = recv(_socket, &pid, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		log_info(log, "Error al recibir pid del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		log_info(log, "Se recibio pid del PCB");
	}

	if((nbytes = receive_string(_socket, &scriptPathInFS)) <= 0)
	{
		log_info(log, "Error al recibir scriptPathInFS del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		log_info(log, "Se recibio scriptPathInFS del PCB");
	}

	if((nbytes = recv(_socket, &programCounter, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		log_info(log, "Error al recibir programCounter del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		log_info(log, "Se recibio programCounter del PCB");
	}

	if((nbytes = recv(_socket, &wasInitialized, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		log_info(log, "Error al recibir wasInitialized del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		log_info(log, "Se recibio wasInitialized del PCB");
	}

	if((nbytes = recv(_socket, &canBeScheduled, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		log_info(log, "Error al recibir canBeScheduled del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		log_info(log, "Se recibio canBeScheduled del PCB");
	}

	if((nbytes = recv(_socket, &executionState, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		log_info(log, "Error al recibir executionState del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		log_info(log, "Se recibio executionState del PCB");
	}

	if((nbytes = recv(_socket, &cpuProcessIsAssignedTo, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		log_info(log, "Error al recibir cpuProcessIsAssignedTo del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		log_info(log, "Se recibio cpuProcessIsAssignedTo del PCB");
	}

	if((nbytes = recv(_socket, &remainingQuantum, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		log_info(log, "Error al recibir remainingQuantum del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		log_info(log, "Se recibio remainingQuantum del PCB");
	}

	if((nbytes = recv(_socket, &newQueueArrivalTime, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		log_info(log, "Error al recibir newQueueArrivalTime del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		log_info(log, "Se recibio newQueueArrivalTime del PCB");
	}

	if((nbytes = recv(_socket, &newQueueLeaveTime, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		log_info(log, "Error al recibir newQueueLeaveTime del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		log_info(log, "Se recibio newQueueLeaveTime del PCB");
	}

	if((nbytes = recv(_socket, &totalInstructionsExecuted, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		log_info(log, "Error al recibir totalInstructionsExecuted del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		log_info(log, "Se recibio totalInstructionsExecuted del PCB");
	}

	if((nbytes = recv(_socket, &instructionsExecutedOnLastExecution, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		log_info(log, "Error al recibir instructionsExecutedOnLastExecution del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		log_info(log, "Se recibio instructionsExecutedOnLastExecution del PCB");
	}

	if((nbytes = recv(_socket, &instructionsUntilIoOrEnd, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		log_info(log, "Error al recibir instructionsUntilIoOrEnd del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		log_info(log, "Se recibio instructionsUntilIoOrEnd del PCB");
	}

	if((nbytes = recv(_socket, &dmaCalls, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		log_info(log, "Error al recibir dmaCalls del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		log_info(log, "Se recibio dmaCalls del PCB");
	}

	if((nbytes = recv(_socket, &lastIOStartTime, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		log_info(log, "Error al recibir lastIOStartTime del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		log_info(log, "Se recibio lastIOStartTime del PCB");
	}

	if((nbytes = recv(_socket, &responseTimes, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		log_info(log, "Error al recibir responseTimes del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		log_info(log, "Se recibio responseTimes del PCB");
	}


	PCB_t* process = calloc(1, sizeof(PCB_t));
	process->pid = pid;
	process->scriptPathInFS = scriptPathInFS;
	process->programCounter = programCounter;
	process->wasInitialized = wasInitialized;
	process->canBeScheduled = canBeScheduled;
	process->executionState = executionState;
	process->cpuProcessIsAssignedTo = cpuProcessIsAssignedTo;
	process->remainingQuantum = remainingQuantum;
	process->newQueueArrivalTime = newQueueArrivalTime;
	process->newQueueLeaveTime = newQueueLeaveTime;
	process->totalInstructionsExecuted = totalInstructionsExecuted;
	process->instructionsExecutedOnLastExecution = instructionsExecutedOnLastExecution;
	process->instructionsUntilIoOrEnd = instructionsUntilIoOrEnd;
	process->completedDmaCalls = dmaCalls;
	process->lastIOStartTime = lastIOStartTime;
	process->responseTimes = responseTimes;

	(*pcb) = process;

	log_info(log, "PCB recibido por completo de forma exitosa!");

	return bytesReceived;
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

	printf("\n----Test de recepcion de PCBs----\n\n");

	log = log_create("../../Logs/TestClient.log", "TestClient", true, LOG_LEVEL_INFO);

	serverSocket = connect_server("127.0.0.1", 8000, 2, log);

	log_info(log, "Se recibira un PCB del servidor");

	send_int(serverSocket, 1);

	PCB_t* process = NULL;

	recvPCB(serverSocket, &process);

	showPCBData(process);


	printf("\n\n Test terminado, presione enter para salir...");
	getchar();
}
