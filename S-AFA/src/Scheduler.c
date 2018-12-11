#include "Scheduler.h"


void shortTermSchedulerThread()
{
	t_list* schedulableProcesses;

	while(!killThreads)
	{
		sem_wait(&shortTermScheduler);	//Wait until this scheduler is needed
		sem_wait(&schedulerNotRunning);	//Extra security measure to avoid 2 the schedulers running at the same time and thus, producing unexpected behavior

		//TODO: Review that last semaphore wait?

		STSAlreadyExecuting = true;

		pthread_mutex_lock(&readyQueueMutex);
		pthread_mutex_lock(&ioReadyQueueMutex);

		schedulableProcesses = getSchedulableProcesses();

		if(list_size(schedulableProcesses) == 0)
		{
			log_info(schedulerLog, "No es posible planificar ya que no hay procesos listos\nSe debe crear un nuevo proceso, o desbloquear uno ya existente, para poder planificar");
		}
		else
		{
			(*scheduleProcesses)();
		}

		pthread_mutex_unlock(&readyQueueMutex);
		pthread_mutex_unlock(&ioReadyQueueMutex);

		STSAlreadyExecuting = false;

		sem_post(&schedulerNotRunning);
	}
}

void longTermSchedulerThread()
{
	PCB_t* processToInitialize;
	uint32_t processAccepted;
	t_list* schedulableProcesses;

	while(!killThreads)
	{
		sem_wait(&longTermScheduler);	//Wait until this scheduler is needed
		sem_wait(&schedulerNotRunning);	//Extra security measure to avoid 2 the schedulers running at the same time and thus, producing unexpected behavior

		LTSAlreadyExecuting = true;

		pthread_mutex_lock(&newQueueMutex);		//Need to prevent the threads accessing the readyQueue/newQueue
		pthread_mutex_lock(&readyQueueMutex); 	//at the same time. Otherwise, unexpected things may happen...

		if(list_size(newQueue) == 0)
		{
			log_info(schedulerLog, "LTS: No hay procesos para aceptar");

			schedulableProcesses = getSchedulableProcesses();

			if(list_size(schedulableProcesses) == 0)
				log_info(schedulerLog, "LTS: No hay procesos para planificar");
			else
				_checkExecProc_and_algorithm(); //Call the STS if there are no new processes to add, but there are ready ones
		}
		else
		{
			for(uint32_t i = 0; i < list_size(newQueue); i++)
			{
				processToInitialize = list_remove(newQueue, 0);
				processAccepted = addProcessToReadyQueue(processToInitialize);

				t_list* freeCPUs = getFreeCPUs();

				if(list_size(freeCPUs) == 0)
					log_info(schedulerLog, "No hay CPUs disponibles para inicializar el proceso con id %d. Se dejara en la cola de listos (sin poder ser planificado) a la espera de que se libere una CPU", processToInitialize->pid);
				else
				{
					cpu_t* freeCPU = list_get(freeCPUs, 0);
					executeProcess(processToInitialize, freeCPU);
				}

				//tengo que agregar el nuevo pcb a la cola de listos para guardar el espacio y despues a la cola de ejecucion, para poder mandar el proceso a la cola de bloqueados,
				//en realidad se pasa a la cola de ejecucion y se asigna a una cpu, pero se le avisa a la cpu que es para inicializar, y no se hace pasar el proceso por el STS
				//El problema es si no hay ninguna CPU libre para inicializar el proceso... en ese caso tendria que
				//dejar el pcb en la cola de listos, y cuando se libera o se conecta una nueva cpu, se tiene que verificar si hay procesos pendientes de inicializacion e inicializarlos
				//(para inicializar procesos, hacer una funcion que busque en la cola de listos todos los procesos que tengan el bool que indica que no se puede planificar, y luego se asigna
				//cada uno de esos procesos a la cpu liberada/nueva; esto deberia llamarse cuando se libera una cpu y cuando se conecta una nueva)

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
	int32_t semaphoreValue;

	log_info(schedulerLog, "LTS: Se encontraron procesos en la cola de listos. Se intentara activar el STS");

	if((list_size(executionQueue) == 0) || (getFreeCPUsQty() > 0))
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
	totalConnectedCpus =  0;
	killProcessInstructions = 0;
	systemResponseTime = 0;
	configFileInotifyFD = 0;

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

	fileTable = dictionary_create();
	semaphoreList = dictionary_create();

	sem_init(&shortTermScheduler, 0, 0);
	sem_init(&longTermScheduler, 0, 0);
	sem_init(&schedulerNotRunning, 0, 1);

	consoleLog = init_log("../../Logs/S-AFA_Consola.log", "Consola S-AFA", true, LOG_LEVEL_INFO);
	schedulerLog = init_log("../../Logs/S-AFA_Planif.log", "Proceso S-AFA", false, LOG_LEVEL_INFO);

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

	pthread_attr_t* threadAttributes = NULL;
	pthread_attr_init(threadAttributes);
	pthread_attr_setdetachstate(threadAttributes, PTHREAD_CREATE_DETACHED);

	pthread_create(&serverThread, threadAttributes, (void *)server, NULL);

	pthread_create(&stsThread, threadAttributes, (void *)shortTermSchedulerThread, NULL);

	pthread_create(&ltsThread, threadAttributes, (void *)longTermSchedulerThread, NULL);
}

void freeResources()
{
	free(config.schedulingAlgorithm);

	list_destroy_and_destroy_elements(connectedCPUs, freeCpuElement);
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
