#include "Scheduler.h"


void shortTermSchedulerThread()
{
	uint32_t semaphoreValue;

	while(!killThreads)
	{
		sem_wait(&shortTermScheduler);	//Wait until this scheduler is needed
		sem_wait(&schedulerNotRunning);	//Extra security measure to avoid 2 the schedulers running at the same time and thus, producing unexpected behavior

		//TODO: Review that last semaphore wait?

		STSAlreadyExecuting = true;

		if(list_size(readyQueue) == 0)
		{
			log_info(schedulerLog, "No es posible planificar ya que no hay procesos listos\nSe debe crear un nuevo proceso, o desbloquear uno ya existente, para poder planificar\n");
		}
		else
		{
			pthread_mutex_lock(&readyQueueMutex);	//To schedule, I need to access the readyQueue.
													//Can not do so when it is being accessed by other thread
			(*scheduleProcesses)();

			pthread_mutex_unlock(&readyQueueMutex);
		}

		STSAlreadyExecuting = false;

		sem_post(&schedulerNotRunning);
	}
}

void longTermSchedulingThread()
{
	PCB_t* processToAccept;
	uint32_t processAccepted;
	uint32_t semaphoreValue;



	while(!killThreads)
	{
		sem_wait(&longTermScheduler);	//Wait until this scheduler is needed
		sem_wait(&schedulerNotRunning);	//Extra security measure to avoid 2 the schedulers running at the same time and thus, producing unexpected behavior

		LTSAlreadyExecuting = true;

		pthread_mutex_lock(&newQueueMutex);		//Need to prevent the threads accessing the readyQueue/newQueue
		pthread_mutex_lock(&readyQueueMutex); 	//at the same time. Otherwise, unexpected things may happen...

		if(list_size(newQueue) == 0)
		{
			log_info(schedulerLog, "LTS: No hay procesos para aceptar\n");

			if(list_size(readyQueue) == 0)
				log_info(schedulerLog, "LTS: No hay procesos para planificar\n");
			else
				_checkExecProc_and_algorithm(); //Call the STS if there are no new processes to add, but there are ready ones
		}
		else
		{
			for(uint32_t i = 0; i < list_size(newQueue); i++)
			{
				processToAccept = list_remove(newQueue, 0);
				processAccepted = addProcessToReadyQueue(processToAccept);

				//TODO
				//lo tengo que agregar a la cola de listos para guardar el espacio, pero en realidad
				//no deberia estar ahi hasta que la cpu no me confirma que se inicializo el PCB
				//correctamente (es decir que se cargo el script en memoria y eso)
				//entonces, para evitar que el planificador me planifique un proceso que esta condicional
				//en la cola de listos, le tengo que agregar una variable al PCB para indicar si es
				//planificable... cuando la CPU me confirma que se inicializo el proceso, se cambia
				//la variable de ese proceso en la cola de listos a planificable... si hubo un error
				//en lka inicializacion, tengo que sacar el PCB de la cola de listos

				if(!processAccepted)
					break;
			}

			_checkExecProc_and_algorithm();
		}

		pthread_mutex_unlock(&readyQueueMutex);
		pthread_mutex_unlock(&newQueueMutex);

		LTSAlreadyExecuting = false;

		sem_post(&schedulerNotRunning);
	}
}

void _checkExecProc_and_algorithm()
{
	uint32_t semaphoreValue;

	log_info(schedulerLog, "LTS: Se encontraron procesos en la cola de listos. Se intentara activar el STS\n");

	if((list_size(executionQueue) == 0) || (getFreeCpusQty() > 0))
	{
		sem_getvalue(&shortTermScheduler, &semaphoreValue);

		if(semaphoreValue == 0)		//If semaphore value < 0, the STS may already been executing
			sem_post(&shortTermScheduler); //If no process is executing at this moment, must activate the STS, no matter if
													   	   //the STS algorithm is preemptive or not
	}
}

void initializeVariables()
{
	executedInstructions = 0;
	dma_executedInstructions = 0;
	actualProcesses = 0;
	totalProcesses = 0;
	exit_executedInstructions = 0;
	totalConnectedCpus =  0;

	killThreads = false;
	STSAlreadyExecuting = false;
	LTSAlreadyExecuting = false;

	newQueue = list_create();
	readyQueue = list_create();
	blockedQueue = list_create();
	executionQueue = list_create();
	finishedQueue = list_create();
	connectedCPUs = list_create();
	ioReadyQueue = list_create();

	sem_init(&shortTermScheduler, 0, 0);
	sem_init(&longTermScheduler, 0, 0);
	sem_init(&schedulerNotRunning, 0, 1);

	consoleLog = init_log(PATH_LOG, "Consola S-AFA", false, LOG_LEVEL_INFO);
	schedulerLog = init_log(PATH_LOG, "Proceso S-AFA", true, LOG_LEVEL_INFO);
	config = getConfigs();

	pthread_attr_t* threadAttributes;
	pthread_attr_init(threadAttributes);
	pthread_attr_setdetachstate(threadAttributes, PTHREAD_CREATE_DETACHED);

	pthread_create(&serverThread, threadAttributes, (void *)server, NULL);

	pthread_create(&stsThread, threadAttributes, (void *)shortTermSchedulerThread, NULL);

	pthread_create(ltsThread, threadAttributes, (void *)longTermSchedulerThread, NULL);
}

int main(void)
{
	system("clear");
	puts("PROCESO S-AFA\n");

	initializeVariables();

	log_info(schedulerLog, "Inicio del proceso\n");

	console();

	exit(EXIT_SUCCESS);
}
