#include "PCB.h"


void* serializePCB(PCB_t* pcb)
{
	uint32_t scriptNameSize = string_length(pcb->scriptName) + 1;
	void* buffer = calloc(1, (sizeof(uint32_t) * 15) + scriptNameSize);

	memcpy(buffer, &(pcb->pid), sizeof(uint32_t));
	memcpy(buffer + sizeof(uint32_t), &scriptNameSize, sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 2), pcb->scriptName, scriptNameSize);
	memcpy(buffer + (sizeof(uint32_t) * 2) + scriptNameSize, &(pcb->programCounter), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 3) + scriptNameSize, &(pcb->wasInitialized), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 4) + scriptNameSize, &(pcb->canBeScheduled), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 5) + scriptNameSize, &(pcb->executionState), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 6) + scriptNameSize, &(pcb->cpuProcessIsAssignedTo), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 7) + scriptNameSize, &(pcb->remainingQuantum), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 8) + scriptNameSize, &(pcb->newQueueArrivalTime), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 9) + scriptNameSize, &(pcb->newQueueLeaveTime), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 10) + scriptNameSize, &(pcb->instructionsExecuted), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 12) + scriptNameSize, &(pcb->completedDmaCalls), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 13) + scriptNameSize, &(pcb->lastIOStartTime), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 14) + scriptNameSize, &(pcb->responseTimes), sizeof(uint32_t));

	return buffer;
}

uint32_t sendPCB(PCB_t* pcb, uint32_t socket)
{
	void* buffer = serializePCB(pcb);
	uint32_t scriptNameSize = string_length(pcb->scriptName) + 1;
	uint32_t bufferSize = scriptNameSize + (sizeof(uint32_t) * 9);

	return send(socket, buffer, bufferSize, 0);
}

uint32_t recvPCB(PCB_t* pcb, uint32_t socket)	//El pcb es una variable que se declara pero se inicializa en la funcion;
{												//esta hecho asi para permitir detectar errores y actuar acorde a ellos

	uint32_t pid, scriptLength, programCounter, wasInitialized, canBeScheduled, newQueueArrivalTime;
	uint32_t newQueueLeaveTime, executionState, cpuProcessIsAssignedTo, remainingQuantum, lastIOStartTime;
	uint32_t dmaCalls, instructionsExecuted, responseTimes, bytesReceived, nbytes;

	bytesReceived = 0;

	if((nbytes = recv(socket, &pid, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((bytesReceived = recv(socket, &scriptLength, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	char* scriptName = calloc(1, scriptLength);
	if((nbytes = recv(socket, scriptName, scriptLength, MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(socket, &programCounter, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(socket, &wasInitialized, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(socket, &canBeScheduled, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(socket, &executionState, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(socket, &cpuProcessIsAssignedTo, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(socket, &remainingQuantum, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(socket, &newQueueArrivalTime, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(socket, &newQueueLeaveTime, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(socket, &instructionsExecuted, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(socket, &dmaCalls, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(socket, &lastIOStartTime, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(socket, &responseTimes, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;


	pcb = calloc(1, sizeof(PCB_t));
	pcb->pid = pid;
	pcb->scriptName = scriptName;
	pcb->programCounter = programCounter;
	pcb->wasInitialized = wasInitialized;
	pcb->canBeScheduled = canBeScheduled;
	pcb->executionState = executionState;
	pcb->cpuProcessIsAssignedTo = cpuProcessIsAssignedTo;
	pcb->remainingQuantum = remainingQuantum;
	pcb->newQueueArrivalTime = newQueueArrivalTime;
	pcb->newQueueLeaveTime = newQueueLeaveTime;
	pcb->instructionsExecuted = instructionsExecuted;
	pcb->completedDmaCalls = dmaCalls;
	pcb->lastIOStartTime = lastIOStartTime;
	pcb->responseTimes = responseTimes;

	return bytesReceived;
}
