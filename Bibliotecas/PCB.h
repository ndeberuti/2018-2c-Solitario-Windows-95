#ifndef PCB_H_
#define PCB_H_

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "commons/collections/list.h"

typedef struct
{
	uint32_t pid;
	char* script;
	void* programCounter; //No se el tipo de dato
	uint32_t wasInitialized;
	t_list* fileTable;
	uint32_t newQueueArrivalTime;
	uint32_t newQueueLeaveTime;
	uint32_t executionState;
	uint32_t cpuProcessIsAssignedTo; //to get the socket of that cpu and send it a message, for example, if i need to
	uint32_t remainingQuantum;
} PCB_t;


uint32_t sendPCB(PCB_t* pcb, uint32_t socket);	//Returns an error code (< 0) or 0 if OK

uint32_t recvPCB(uint32_t socket, PCB_t* pcb); //Returns an error code (< 0) or 0 if OK
											   //'pcb' must be initialized before calling this function and retrieving the data


#endif /* PCB_H_ */
