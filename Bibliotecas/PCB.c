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

/* This SendPCB functions does not send PCBs properly (there are send errors)

uint32_t sendPCB(uint32_t _socket, PCB_t* pcb)
{
	void* buffer = serializePCB(pcb);
	uint32_t scriptNameSize = string_length(pcb->scriptPathInFS) + 1;
	uint32_t bufferSize = scriptNameSize + (sizeof(uint32_t) * 9);

	return send(_socket, buffer, bufferSize, 0);
}

*/

uint32_t sendPCB(uint32_t _socket, PCB_t* pcb)
{
	int32_t bytesSent = 0, nbytes = 0;

	if((nbytes = send_int(_socket, pcb->pid)) < 0)
	{
		//log_info(log, "Error al enviar pid del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		//log_info(log, "Se envio pid del PCB");
	}

	if((nbytes = send_string(_socket, pcb->scriptPathInFS)) < 0)
	{
		//log_info(log, "Error al enviar scriptPathInFS del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		//log_info(log, "Se envio scriptPathInFS del PCB");
	}

	if((nbytes = send_int(_socket, pcb->programCounter)) < 0)
	{
		//log_info(log, "Error al enviar programCounter del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		//log_info(log, "Se envio programCounter del PCB");
	}

	if((nbytes = send_int(_socket, pcb->wasInitialized)) < 0)
	{
		//log_info(log, "Error al enviar wasInitialized del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		//log_info(log, "Se envio wasInitialized del PCB");
	}

	if((nbytes = send_int(_socket, pcb->executionState)) < 0)
	{
		//log_info(log, "Error al enviar executionState del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		//log_info(log, "Se envio executionState del PCB");
	}

	if((nbytes = send_int(_socket, pcb->cpuProcessIsAssignedTo)) < 0)
	{
		//log_info(log, "Error al enviar cpuProcessIsAssignedTo del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		//log_info(log, "Se envio cpuProcessIsAssignedTo del PCB");
	}

	if((nbytes = send_int(_socket, pcb->remainingQuantum)) < 0)
	{
		//log_info(log, "Error al enviar remainingQuantum del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		//log_info(log, "Se envio remainingQuantum del PCB");
	}

	if((nbytes = send_int(_socket, pcb->newQueueArrivalTime)) < 0)
	{
		//log_info(log, "Error al enviar newQueueArrivalTime del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		//log_info(log, "Se envio newQueueArrivalTime del PCB");
	}

	if((nbytes = send_int(_socket, pcb->newQueueLeaveTime)) < 0)
	{
		//log_info(log, "Error al enviar newQueueLeaveTime del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		//log_info(log, "Se enviar newQueueLeaveTime del PCB");
	}

	if((nbytes = send_int(_socket, pcb->totalInstructionsExecuted)) < 0)
	{
		//log_info(log, "Error al enviar totalInstructionsExecuted del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		//log_info(log, "Se envio totalInstructionsExecuted del PCB");
	}

	if((nbytes = send_int(_socket, pcb->instructionsExecutedOnLastExecution)) < 0)
	{
		//log_info(log, "Error al enviar instructionsExecutedOnLastExecution del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		//log_info(log, "Se envio instructionsExecutedOnLastExecution del PCB");
	}

	if((nbytes = send_int(_socket, pcb->instructionsUntilIoOrEnd)) < 0)
	{
		//log_info(log, "Error al enviar instructionsUntilIoOrEnd del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		//log_info(log, "Se envio instructionsUntilIoOrEnd del PCB");
	}

	if((nbytes = send_int(_socket, pcb->completedDmaCalls)) < 0)
	{
		//log_info(log, "Error al enviar dmaCalls del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		//log_info(log, "Se envio dmaCalls del PCB");
	}

	if((nbytes = send_int(_socket, pcb->lastIOStartTime)) < 0)
	{
		//log_info(log, "Error al enviar lastIOStartTime del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		//log_info(log, "Se envio lastIOStartTime del PCB");
	}

	if((nbytes = send_int(_socket, pcb->responseTimes)) < 0)
	{
		//log_info(log, "Error al enviar responseTimes del PCB");
		return nbytes;
	}
	else
	{
		bytesSent += nbytes;
		//log_info(log, "Se envio responseTimes del PCB");
	}

	//log_info(log, "PCB enviado correctamente");

	return bytesSent;
}

uint32_t recvPCB(uint32_t _socket, PCB_t** pcb)	//El pcb es una variable que se declara pero se inicializa en la funcion;
{												//esta hecho asi para permitir detectar errores y actuar acorde a ellos

	uint32_t pid, programCounter, wasInitialized, newQueueArrivalTime;
	uint32_t newQueueLeaveTime, executionState, cpuProcessIsAssignedTo, remainingQuantum, lastIOStartTime;
	uint32_t dmaCalls, totalInstructionsExecuted, responseTimes, bytesReceived, nbytes;
	uint32_t instructionsExecutedOnLastExecution, instructionsUntilIoOrEnd;

	char* scriptPathInFS = NULL;

	bytesReceived = 0;

	if((nbytes = recv(_socket, &pid, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		//log_info(log, "Error al recibir pid del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		//log_info(log, "Se recibio pid del PCB");
	}

	if((nbytes = receive_string(_socket, &scriptPathInFS)) <= 0)
	{
		//log_info(log, "Error al recibir scriptPathInFS del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		//log_info(log, "Se recibio scriptPathInFS del PCB");
	}

	if((nbytes = recv(_socket, &programCounter, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		//log_info(log, "Error al recibir programCounter del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		//log_info(log, "Se recibio programCounter del PCB");
	}

	if((nbytes = recv(_socket, &wasInitialized, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		//log_info(log, "Error al recibir wasInitialized del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		//log_info(log, "Se recibio wasInitialized del PCB");
	}

	if((nbytes = recv(_socket, &executionState, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		//log_info(log, "Error al recibir executionState del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		//log_info(log, "Se recibio executionState del PCB");
	}

	if((nbytes = recv(_socket, &cpuProcessIsAssignedTo, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		//log_info(log, "Error al recibir cpuProcessIsAssignedTo del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		//log_info(log, "Se recibio cpuProcessIsAssignedTo del PCB");
	}

	if((nbytes = recv(_socket, &remainingQuantum, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		//log_info(log, "Error al recibir remainingQuantum del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		//log_info(log, "Se recibio remainingQuantum del PCB");
	}

	if((nbytes = recv(_socket, &newQueueArrivalTime, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		//log_info(log, "Error al recibir newQueueArrivalTime del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		//log_info(log, "Se recibio newQueueArrivalTime del PCB");
	}

	if((nbytes = recv(_socket, &newQueueLeaveTime, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		//log_info(log, "Error al recibir newQueueLeaveTime del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		//log_info(log, "Se recibio newQueueLeaveTime del PCB");
	}

	if((nbytes = recv(_socket, &totalInstructionsExecuted, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		//log_info(log, "Error al recibir totalInstructionsExecuted del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		//log_info(log, "Se recibio totalInstructionsExecuted del PCB");
	}

	if((nbytes = recv(_socket, &instructionsExecutedOnLastExecution, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		//log_info(log, "Error al recibir instructionsExecutedOnLastExecution del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		//log_info(log, "Se recibio instructionsExecutedOnLastExecution del PCB");
	}

	if((nbytes = recv(_socket, &instructionsUntilIoOrEnd, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		//log_info(log, "Error al recibir instructionsUntilIoOrEnd del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		//log_info(log, "Se recibio instructionsUntilIoOrEnd del PCB");
	}

	if((nbytes = recv(_socket, &dmaCalls, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		//log_info(log, "Error al recibir dmaCalls del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		//log_info(log, "Se recibio dmaCalls del PCB");
	}

	if((nbytes = recv(_socket, &lastIOStartTime, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		//log_info(log, "Error al recibir lastIOStartTime del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		//log_info(log, "Se recibio lastIOStartTime del PCB");
	}

	if((nbytes = recv(_socket, &responseTimes, sizeof(uint32_t), MSG_WAITALL)) <= 0)
	{
		//log_info(log, "Error al recibir responseTimes del PCB");
		return nbytes;
	}
	else
	{
		bytesReceived += nbytes;
		//log_info(log, "Se recibio responseTimes del PCB");
	}


	PCB_t* process = calloc(1, sizeof(PCB_t));
	process->pid = pid;
	process->scriptPathInFS = scriptPathInFS;
	process->programCounter = programCounter;
	process->wasInitialized = wasInitialized;
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

	//log_info(log, "PCB recibido por completo de forma exitosa!");

	return bytesReceived;
}
