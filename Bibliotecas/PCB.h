#ifndef PCB_H_
#define PCB_H_

				  
#include <stdio.h>
#include <string.h>			   
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <commons/collections/list.h>

typedef struct
{
	uint32_t pid;
	char* scriptName;											  
	uint32_t programCounter;
	bool wasInitialized;
	bool canBeScheduled;							
	uint32_t executionState;
	uint32_t cpuProcessIsAssignedTo; //To get the socket of that cpu and send it a message, for example, if i need to
	uint32_t remainingQuantum;
	//t_list* fileTable;			//According to the assignment's text, the PCB has to have a file table...need to ask the teachers

	//Metrics
	uint32_t newQueueArrivalTime;
	uint32_t newQueueLeaveTime;
	uint32_t readyQueueArrivalTime; //Should be set when the process is initialized (when the CPU sends the confirmation)
	uint32_t dmaCalls;	//How many calls to the DMA -> The process tried to access files not in memory (need to check the file table for that)
	uint32_t instructionsExecuted;
	uint32_t firstExecutionTime; //To calculate Response Time = FirstExecutiontime - ReadyQueueArrivalTime
	bool normalTermination; 	//To allow to measure how many instructions led to a process termination
								//(instead of a normal exit); used for the metrics
} PCB_t;


uint32_t sendPCB(PCB_t* pcb, uint32_t socket);	//Returns an error code (< 0) or 0 if OK

uint32_t recvPCB(PCB_t* pcb, uint32_t socket); //Returns an error code (< 0) or 0 if OK
											   //'pcb' pointer variable must be created before calling this function and retrieving the data

#endif /* PCB_H_ */