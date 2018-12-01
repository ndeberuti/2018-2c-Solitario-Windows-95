#ifndef PCB_H_
#define PCB_H_


#include <stdint.h>
#include "servidor/servidor.h"
#include <commons/collections/list.h>
#include <commons/string.h>

typedef struct
{
	uint32_t pid;
	char* scriptName;											  
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
	uint32_t instructionsExecuted;

	uint32_t completedDmaCalls;	//How many calls to the DMA were made; this is incremented when a process is unblocked by a request from the DMA
	uint32_t lastIOStartTime;
	uint32_t responseTimes;		//All the response times added in one variable (ResponseTime = ioStartTime - ioStopTime = processBlockTime - processUnblockTime)

} PCB_t;

/*
  In order to get each process' responseTime, I have to track each I/O operation's start and end times
  To do that, a list could be added to the PCB using the above struct, but to avoid making PCB
  transfers complex (using a list and a struct makes it rather complex to serialize/deserialize),
  I will be using a dictionary where the keys are the processIDs, and the data is a list with
  all the ioTracking structs for all the I/O operations of that process
  This decision to avoid putting that data in the PCB is also backed by the fact that only this module
  uses that data, not the other ones, so there is no point on including that in the PCB and thus
  making PCB transferring more complex

  The above is ok, but managing another structure is pointless and means more overhead
  Instead of that structure, I had the idea to have a variable in the PCB that counts the DMA
  calls, a variable to track the last IO operation start time, and a variable that
  has the sum of all the ResponseTimes. So, to add to the ResponseTimes, when the process blocks
  the ioStartTime variable must be set with the actual time. When the process unlocks from a DMA
  call, that variable is subtracted to the actual time and that value added to all the ResponseTimes
  then the ioStartTime variable is set to cero
  also, I need to track the quantity of DMA calls to get the median ResponseTime value for the metrics
*/


uint32_t sendPCB(PCB_t* pcb, uint32_t socket);	//Returns an error code (< 0) or the number of bytes sent, if OK

uint32_t recvPCB(PCB_t* pcb, uint32_t socket); //Returns an error code (<= 0) or the number of bytes received, if OK
											   //The 'pcb' pointer variable must be created before calling this function and retrieving the data (no need to malloc it)

#endif /* PCB_H_ */
