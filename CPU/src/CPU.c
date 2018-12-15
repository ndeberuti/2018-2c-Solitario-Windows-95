#include "CPU.h"

void initializeVariables()
{
	cpuLog = init_log("../../Logs/CPU.log", "Proceso CPU", true, LOG_LEVEL_INFO);
	socketErrorLog = init_log("../../Logs/CPUSocketErrors.log", "CPU", false, LOG_LEVEL_INFO);

	terminateModule = false;
	usingCustomSchedulingAlgorithm = false;

	processInExecutionPCB = NULL;
	instructionsExecuted = 0;
	currentProcessQuantum = 0;
	currentProgramCounter = 0;


	int32_t result = getConfigs();
	showConfigs();

	if(result < 0)
	{
		if(result == MALLOC_ERROR)
		{
			log_error(cpuLog, "Se aborta el proceso por un error de malloc al intentar obtener las configuraciones...");
		}
		else if (result == CONFIG_PROPERTY_NOT_FOUND)
		{
			log_error(cpuLog, "Se aborta el proceso debido a que no se encontro una propiedad requerida en el archivo de configuracion...");
		}

		log_error(cpuLog, "Ocurrio un error al intentar obtener los datos del archivo de configuracion. El proceso sera abortado...");
		exit(CONFIG_LOAD_ERROR);
	}

/*	pthread_attr_t* threadAttributes = malloc(sizeof(pthread_attr_t));
	pthread_attr_init(threadAttributes);
	pthread_attr_setdetachstate(threadAttributes, PTHREAD_CREATE_DETACHED);	//CPUs cannot be servers

	pthread_create(&serverThread, threadAttributes, (void *)server, NULL);
*/

	connectToServers();
}

void executeProcesses()
{
	int32_t nbytes;
	uint32_t task;

	while(!terminateModule)
	{
		//No need to ask this module (from the scheduler) to kill a process, because that message will be gotten when a process finished its execution
		//(so there will be no process to kill in that CPU)

		if((nbytes = receive_int(schedulerServerSocket, &task)) <= 0)
		{
			if(nbytes == 0)
				log_error(cpuLog, "El planificador se desconecto al intentar recibir una tarea del mismo");
			else
				log_error(cpuLog, "Error al intentar recibir una tarea del planificador");

			exit(EXIT_FAILURE);
		}

		switch(task)
		{
			case INITIALIZE_PROCESS:
				initializeProcess();
			break;

			case EXECUTE_PROCESS:
				executeProcess();
			break;

			case COUNT_INSTRUCTIONS:		 //Receives the scriptPath, program counter and pid from a PCB (No need to send the complete pcb), and
				countProcessInstructions();	 //returns the number of instructions left before an IO instruction or the end of the script
			break;

			default:
				log_error(cpuLog, "Se recibio un id de tarea incorrecto del planificador!\n");
			break;
		}
	}

	freePCB(processInExecutionPCB);

	//TODO - main execution loop
	//should use a "while(true)" wait for the scheduler to tell this module to execute or initialize a process and, in case the process is
	//being executed, it should stop reading instructions from the script the memory sent if the serverThread set a boolean variable
	//telling this main thread to kil or block (one for each case) the process in execution

}

int main(void)
{
	system("clear");
	puts("PROCESO CPU\n");

	initializeVariables();

	log_info(cpuLog, "Inicio del proceso\n");

	executeProcesses();

	exit(EXIT_SUCCESS);
}
