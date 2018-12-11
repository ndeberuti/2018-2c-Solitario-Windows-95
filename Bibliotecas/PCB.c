#include "PCB.h"


void* serializePCB(PCB_t* pcb)
{
	uint32_t scriptNameSize = string_length(pcb->scriptPathInFS) + 1;
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

	return buffer;
}

uint32_t sendPCB(uint32_t _socket, PCB_t* pcb)
{
	void* buffer = serializePCB(pcb);
	uint32_t scriptNameSize = string_length(pcb->scriptPathInFS) + 1;
	uint32_t bufferSize = scriptNameSize + (sizeof(uint32_t) * 9);

	return send(_socket, buffer, bufferSize, 0);
}

uint32_t recvPCB(uint32_t _socket, PCB_t* pcb)	//El pcb es una variable que se declara pero se inicializa en la funcion;
{												//esta hecho asi para permitir detectar errores y actuar acorde a ellos

	uint32_t pid, scriptLength, programCounter, wasInitialized, canBeScheduled, newQueueArrivalTime;
	uint32_t newQueueLeaveTime, executionState, cpuProcessIsAssignedTo, remainingQuantum, lastIOStartTime;
	uint32_t dmaCalls, totalInstructionsExecuted, responseTimes, bytesReceived, nbytes;
	uint32_t instructionsExecutedOnLastExecution, instructionsUntilIoOrEnd;

	char* scriptPathInFS = calloc(1, scriptLength);

	bytesReceived = 0;

	if((nbytes = recv(_socket, &pid, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((bytesReceived = recv(_socket, &scriptLength, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(_socket, scriptPathInFS, scriptLength, MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(_socket, &programCounter, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(_socket, &wasInitialized, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(_socket, &canBeScheduled, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(_socket, &executionState, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(_socket, &cpuProcessIsAssignedTo, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(_socket, &remainingQuantum, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(_socket, &newQueueArrivalTime, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(_socket, &newQueueLeaveTime, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(_socket, &totalInstructionsExecuted, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(_socket, &instructionsExecutedOnLastExecution, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(_socket, &instructionsUntilIoOrEnd, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(_socket, &dmaCalls, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(_socket, &lastIOStartTime, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;

	if((nbytes = recv(_socket, &responseTimes, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		return nbytes;
	}
	else
		bytesReceived += nbytes;


	pcb = calloc(1, sizeof(PCB_t));
	pcb->pid = pid;
	pcb->scriptPathInFS = scriptPathInFS;
	pcb->programCounter = programCounter;
	pcb->wasInitialized = wasInitialized;
	pcb->canBeScheduled = canBeScheduled;
	pcb->executionState = executionState;
	pcb->cpuProcessIsAssignedTo = cpuProcessIsAssignedTo;
	pcb->remainingQuantum = remainingQuantum;
	pcb->newQueueArrivalTime = newQueueArrivalTime;
	pcb->newQueueLeaveTime = newQueueLeaveTime;
	pcb->totalInstructionsExecuted = totalInstructionsExecuted;
	pcb->instructionsExecutedOnLastExecution = instructionsExecutedOnLastExecution;
	pcb->instructionsUntilIoOrEnd = instructionsUntilIoOrEnd;
	pcb->completedDmaCalls = dmaCalls;
	pcb->lastIOStartTime = lastIOStartTime;
	pcb->responseTimes = responseTimes;

	return bytesReceived;
}
