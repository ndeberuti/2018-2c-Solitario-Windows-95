#include "PCB.h"


void* serializePCB(PCB_t* pcb)
{
	uint32_t scriptNameSize = string_length(pcb->scriptName) + 1;
	void* buffer = calloc(1, (sizeof(uint32_t) * 9) + scriptNameSize);

	memcpy(buffer, pcb->pid, sizeof(uint32_t));
	memcpy(buffer + sizeof(uint32_t), &scriptNameSize, sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 2), pcb->scriptName, scriptNameSize);
	memcpy(buffer + (sizeof(uint32_t) * 2) + scriptNameSize, &(pcb->programCounter), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 3) + scriptNameSize, &(pcb->wasInitialized), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 4) + scriptNameSize, &(pcb->newQueueArrivalTime), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 5) + scriptNameSize, &(pcb->newQueueLeaveTime), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 6) + scriptNameSize, &(pcb->executionState), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 7) + scriptNameSize, &(pcb->cpuProcessIsAssignedTo), sizeof(uint32_t));
	memcpy(buffer + (sizeof(uint32_t) * 8) + scriptNameSize, &(pcb->remainingQuantum), sizeof(uint32_t));

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

	uint32_t pid, scriptLength, programCounter, wasInitialized, newQueueArrivalTime;
	uint32_t newQueueLeaveTime, executionState, cpuProcessIsAssignedTo, remainingQuantum;

	recv(socket, &pid, sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &scriptLength, sizeof(uint32_t), MSG_WAITALL);

	char* scriptName = calloc(1, scriptLength);
	recv(socket, scriptName, scriptLength, MSG_WAITALL);

	recv(socket, &programCounter, sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &wasInitialized, sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &newQueueArrivalTime, sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &newQueueLeaveTime, sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &executionState, sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &cpuProcessIsAssignedTo, sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &remainingQuantum, sizeof(uint32_t), MSG_WAITALL);

	pcb = calloc(1, sizeof(PCB_t));
	pcb->pid = pid;
	pcb->scriptName = scriptName;
	pcb->programCounter = programCounter;
	pcb->wasInitialized = wasInitialized;
	pcb->newQueueArrivalTime = newQueueArrivalTime;
	pcb->newQueueLeaveTime = newQueueLeaveTime;
	pcb->executionState = executionState;
	pcb->cpuProcessIsAssignedTo = cpuProcessIsAssignedTo;
	pcb->remainingQuantum = remainingQuantum;

	return 0;		//Falta hacer algo para agarrar los errores que puedan tirar los rcv de arriba
}