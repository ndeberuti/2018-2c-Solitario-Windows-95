#include "Scheduler.h"


void shortTermSchedulerThread()
{
	t_list* schedulableProcessesFromReadyQueue = NULL;
	uint32_t scheduleableProcessesQty = 0;

	while(!killThreads)
	{
		sem_wait(&shortTermScheduler);	//Wait until this scheduler is needed
		sem_wait(&schedulerNotRunning);	//Extra security measure to avoid 2 the schedulers running at the same time and thus, producing unexpected behavior

		//TODO: Review that last semaphore wait?

		pthread_mutex_lock(&canCommunicateWithCPUs);	//To avoid the other threads to message CPUs when this one needs to do so

		STSAlreadyExecuting = true;

		log_info(schedulerLog, "STS: Comenzando ejecucion");

		pthread_mutex_lock(&readyQueueMutex);
		pthread_mutex_lock(&ioReadyQueueMutex);

		if(getFreeCPUsQty() == 0)
		{
			log_info(schedulerLog, "STS: No hay CPUs disponibles para ejecutar procesos. Se debera esperar a que haya un proceso nuevo.");

			pthread_mutex_unlock(&readyQueueMutex);
			pthread_mutex_unlock(&ioReadyQueueMutex);
			pthread_mutex_unlock(&canCommunicateWithCPUs);
			sem_post(&schedulerNotRunning);

			return;
		}

		schedulableProcessesFromReadyQueue = getSchedulableProcesses();

		scheduleableProcessesQty = list_size(schedulableProcessesFromReadyQueue);
		scheduleableProcessesQty += list_size(ioReadyQueue);

		if(scheduleableProcessesQty == 0)
		{
			log_info(schedulerLog, "STS: No es posible planificar ya que no hay procesos listos. Se debe crear un nuevo proceso, o desbloquear uno ya existente, para poder planificar");
		}
		else
		{
			bool aProcessWasExecuted = (*scheduleProcesses)();

			if(aProcessWasExecuted)
				processesReadyToExecute--;
		}

		pthread_mutex_unlock(&readyQueueMutex);
		pthread_mutex_unlock(&ioReadyQueueMutex);

		pthread_mutex_unlock(&canCommunicateWithCPUs);

		STSAlreadyExecuting = false;

		sem_post(&schedulerNotRunning);
		log_info(schedulerLog, "STS: Ejecucion finalizada");
	}
}

void longTermSchedulerThread()
{
	PCB_t* processToInitialize = NULL;
	uint32_t processAccepted = 0;
	t_list* schedulableProcesses = NULL;

	while(!killThreads)
	{
		sem_wait(&longTermScheduler);	//Wait until this scheduler is needed
		sem_wait(&schedulerNotRunning);	//Extra security measure to avoid 2 the schedulers running at the same time and thus, producing unexpected behavior

		LTSAlreadyExecuting = true;

		pthread_mutex_lock(&canCommunicateWithCPUs);	//To avoid the other threads to message CPUs when this one needs to do so

		pthread_mutex_lock(&newQueueMutex);		//Need to prevent the threads accessing the readyQueue/newQueue
		pthread_mutex_lock(&readyQueueMutex); 	//at the same time. Otherwise, unexpected things may happen...

		log_info(schedulerLog, "LTS: Comenzando ejecucion...");

		if(list_size(newQueue) == 0)
		{
			log_info(schedulerLog, "LTS: No hay procesos para aceptar");

			schedulableProcesses = getSchedulableProcesses();

			if(list_size(schedulableProcesses) == 0)
				log_info(schedulerLog, "LTS: No hay procesos para planificar");
			else
			{
				pthread_mutex_lock(&executionQueueMutex);

				_checkExecProc_and_algorithm(); //Call the STS if there are no new processes to add, but there are ready ones

				pthread_mutex_unlock(&executionQueueMutex);
			}
		}
		else
		{
			pthread_mutex_lock(&cpuListMutex);	//As the connectedCPUs list is accessed multiple times in this function, to avoid results changing due to a CPU being modified, I must lock it

			for(uint32_t i = 0; i < list_size(newQueue); i++)
			{
				processToInitialize = list_remove(newQueue, 0);
				processAccepted = addProcessToReadyQueue(processToInitialize);

				if(!processAccepted)	//If a process was not accepted, it means there is no more room for more processes (due to the multiprogrammingLevel)
					break;
			}

			if(getFreeCPUsQty() == 0)
				log_info(schedulerLog, "LTS: No hay CPUs disponibles para inicializar el proceso con id %d. Se dejara en la cola de listos (sin poder ser planificado) a la espera de que se libere una CPU", processToInitialize->pid);
			else
			{
				checkAndInitializeProcessesLoop();	//Initializes all the processes waiting in the readyQueue to be initialized
			}

			/*
			pthread_mutex_lock(&executionQueueMutex);
				_checkExecProc_and_algorithm();			//This function is wrong; the STS should not be posted when an uninitialized process is added to the readyQueue
			pthread_mutex_unlock(&executionQueueMutex);
			*/

			pthread_mutex_unlock(&cpuListMutex);
		}

		pthread_mutex_unlock(&readyQueueMutex);
		pthread_mutex_unlock(&newQueueMutex);

		pthread_mutex_unlock(&canCommunicateWithCPUs);

		LTSAlreadyExecuting = false;
		log_info(schedulerLog, "LTS: Ejecucion finalizada");

		sem_post(&schedulerNotRunning);
	}
}

void _checkExecProc_and_algorithm()
{
	int32_t semaphoreValue = 0;

	log_info(schedulerLog, "LTS: Se intentara activar el STS");

	if((list_size(executionQueue) == 0) || (getFreeCPUsQty() > 0))
	{
		sem_getvalue(&shortTermScheduler, &semaphoreValue);

		if(semaphoreValue == 0)		//If semaphore value < 0, the STS may already been executing
			sem_post(&shortTermScheduler); //If no process is executing at this moment, must activate the STS, no matter if
													   	   //the STS algorithm is preemptive or not
	}
	else
	{
		log_info(schedulerLog, "LTS: Debido a que no hay CPUs disponibles, no se puede activar el STS (ya que este no podra ejecutar ningun proceso)");

		//stsWantsToExecute = true; //This may generate problems with semaphores
	}
}

void initializeVariables()
{
	executedInstructions = 0;
	dma_executedInstructions = 0;
	actualProcesses = 0;
	totalProcesses = 0;
	totalConnectedCpus =  0;
	killProcessInstructions = 0;
	systemResponseTime = 0;
	configFileInotifyFD = 0;
	processesReadyToExecute = 0;

	killThreads = false;
	STSAlreadyExecuting = false;
	LTSAlreadyExecuting = false;
	terminateModule = false;

	//Inotify
	configFileInotifyFD = inotify_init();
	inotify_add_watch(configFileInotifyFD, "../../Configs/Scheduler.cfg", IN_MODIFY);	//Watch the file and send an event when the file is modified
																						//the event will be catched using a select in the "server()"
																						//function (ServerThread.c file)
	newQueue = list_create();
	readyQueue = list_create();
	blockedQueue = list_create();
	executionQueue = list_create();
	finishedQueue = list_create();
	connectedCPUs = list_create();
	ioReadyQueue = list_create();
	fileTableKeys = list_create();
	semaphoreListKeys = list_create();
	executingProcessesToKill = list_create();

	fileTable = dictionary_create();
	semaphoreList = dictionary_create();

	sem_init(&shortTermScheduler, 0, 0);
	sem_init(&longTermScheduler, 0, 0);
	sem_init(&schedulerNotRunning, 0, 1);

	consoleLog = init_log("../../Logs/S-AFA_Consola.log", "Consola S-AFA", true, LOG_LEVEL_INFO);
	schedulerLog = init_log("../../Logs/S-AFA_Planif.log", "Proceso S-AFA", true, LOG_LEVEL_INFO);

	int32_t result = getConfigs();
	showConfigs();

	if(result < 0)
	{
		if(result == MALLOC_ERROR)
		{
			log_error(schedulerLog, "Se aborta el proceso por un error de malloc al intentar obtener las configuraciones...");
		}
		else if (result == CONFIG_PROPERTY_NOT_FOUND)
		{
			log_error(schedulerLog, "Se aborta el proceso debido a que no se encontro una propiedad requerida en el archivo de configuracion...");
		}

		log_error(consoleLog, "Ocurrio un error al intentar obtener los datos del archivo de configuracion. El proceso sera abortado...");
		exit(CONFIG_LOAD_ERROR);
	}

	pthread_attr_t* threadAttributes = malloc(sizeof(pthread_attr_t));
	pthread_attr_init(threadAttributes);
	pthread_attr_setdetachstate(threadAttributes, PTHREAD_CREATE_DETACHED);

	pthread_create(&serverThread, threadAttributes, (void *)server, NULL);

	pthread_create(&stsThread, threadAttributes, (void *)shortTermSchedulerThread, NULL);

	pthread_create(&ltsThread, threadAttributes, (void *)longTermSchedulerThread, NULL);

	free(threadAttributes);
}

void freeResources()
{
	free(config.schedulingAlgorithm);

	list_destroy_and_destroy_elements(connectedCPUs, free);
	list_destroy_and_destroy_elements(newQueue, freePCB);
	list_destroy_and_destroy_elements(readyQueue, freePCB);
	list_destroy_and_destroy_elements(blockedQueue, freePCB);
	list_destroy_and_destroy_elements(executionQueue, freePCB);
	list_destroy_and_destroy_elements(finishedQueue, freePCB);
	list_destroy_and_destroy_elements(ioReadyQueue, freePCB);

	dictionary_destroy_and_destroy_elements(fileTable, freeFileTableData);
	dictionary_destroy_and_destroy_elements(fileTable, freeSemaphoreListData);

	list_destroy_and_destroy_elements(fileTableKeys, free);
	list_destroy_and_destroy_elements(semaphoreListKeys, free);

	log_destroy(consoleLog);
	log_destroy(schedulerLog);
}

int main(void)
{
	system("clear");
	puts("PROCESO S-AFA\n");

	initializeVariables();

	log_info(schedulerLog, "Inicio del proceso\n");

	console();

	freeResources();

	exit(EXIT_SUCCESS);
}
