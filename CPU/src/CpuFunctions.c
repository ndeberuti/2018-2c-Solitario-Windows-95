#include "CPU.h"


int32_t getConfigs()
{
	char *configurationPath = calloc(22, sizeof(char));

	if(configurationPath == NULL)
		return MALLOC_ERROR;

	strcpy(configurationPath, "../../Configs/CPU.cfg"); //strcpy stops copying characters when it encounters a '\0', memcpy stops when it copies the defined amount

	t_config* configFile = config_create(configurationPath);
	config_t tempConfig;

	if(config_has_property(configFile, "IP_SAFA"))
	{
		tempConfig.schedulerIp = strdup(config_get_string_value(configFile, "IP_SAFA"));
	}
	else
	{
		log_error(cpuLog, "No se definio ninguna propiedad para la 'IP de SAFA' en el archivo de configuracion\n");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "Puerto_SAFA"))
	{
		tempConfig.schedulerPort = config_get_int_value(configFile, "Puerto_SAFA");
	}
	else
	{
		log_error(cpuLog, "No se definio ninguna propiedad para el 'Puerto de SAFA' en el archivo de configuracion\n");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "IP_FM9"))
	{
		tempConfig.memoryIp = strdup(config_get_string_value(configFile, "IP_FM9"));
	}
	else
	{
		log_error(cpuLog, "No se definio ninguna propiedad para la 'IP de FM9' en el archivo de configuracion\n");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "Puerto_FM9"))
	{
		tempConfig.memoryPort = config_get_int_value(configFile, "Puerto_FM9");
	}
	else
	{
		log_error(cpuLog, "No se definio ninguna propiedad para el 'Puerto de FM9' en el archivo de configuracion\n");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "IP_Diego"))
	{
		tempConfig.dmaIp = strdup(config_get_string_value(configFile, "IP_Diego"));
	}
	else
	{
		log_error(cpuLog, "So se  definio ninguna propiedad para la 'IP de El Diego' en el archivo de configuracion\n");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "Puerto_Diego"))
	{
		tempConfig.dmaPort = config_get_int_value(configFile, "Puerto_Diego");
	}
	else
	{
		log_error(cpuLog, "No se definio ninguna propiedad para el 'Puerto de el Diego' en el archivo de configuracion\n");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "IP_CPU"))
	{
		tempConfig.cpuIp = strdup(config_get_string_value(configFile, "IP_CPU"));
	}
	else
	{
		log_error(cpuLog, "No se definio ninguna propiedad para la 'IP de la CPU' en el archivo de configuracion\n");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "Puerto_CPU"))
	{
		tempConfig.cpuPort = config_get_int_value(configFile, "Puerto_CPU");
	}
	else
	{
		log_error(cpuLog, "No se definio ninguna propiedad para el 'Puerto de la CPU' en el archivo de configuracion\n");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "RetardoEjecucion"))
	{
		tempConfig.executionDelay = config_get_int_value(configFile, "RetardoEjecucion");
	}
	else
	{
		log_error(cpuLog, "No se definio ninguna propiedad para el 'Retardo de Ejecucion' en el archivo de configuracion\n");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	free(configurationPath);
	config_destroy(configFile);

	config = tempConfig;	//This is to avoid modifying the original config values if an error occurs after modifying the configFile in runtime

	return 0;
}

void showConfigs()
{
	log_info(cpuLog, "---- Configuracion ----\n");
	log_info(cpuLog, "IP_SAFA = %s", config.schedulerIp);
	log_info(cpuLog, "Puerto_SAFA = %d", config.schedulerPort);
	log_info(cpuLog, "IP_DAM = %s", config.dmaIp);
	log_info(cpuLog, "Puerto_DAM = %d", config.dmaPort);
	log_info(cpuLog, "IP_FM9 = %s", config.memoryIp);
	log_info(cpuLog, "Puerto_FM9 = %d", config.memoryPort);
	log_info(cpuLog, "IP_CPU = %s", config.cpuIp);
	log_info(cpuLog, "Puerto_CPU = %d", config.cpuPort);
	log_info(cpuLog, "Retardo de Ejecucion = %d", config.executionDelay);
	log_info(cpuLog, "-----------------------\n");
}

int32_t handshakeScheduler(uint32_t socket)
{
	int32_t nbytes;
	int32_t message;

	if((nbytes = send_int(socket, NEW_CPU_CONNECTION)) < 0)
	{
		log_error(cpuLog, "Error al indicar al planificador que se esta conectando una CPU\n");
		return nbytes;
		//TODO (Optional) - Send Error Handling
	}

	if((nbytes = receive_int(socket, &message)) <= 0)
	{
		if(nbytes == 0)
		{
			log_error(cpuLog, "Error al recibir mensaje de confirmacion de handshake del planificador\n");
			return nbytes;
		}
		else
		{
			log_error(cpuLog, "Error al recibir mensaje de confirmacion de handshake del planificador\n");

			close(socket);
			FD_CLR(socket, &master);
			return nbytes;
		}
	}

	if(message == USE_NORMAL_ALGORITHM)
		usingCustomSchedulingAlgorithm = false;

	else if(message == USE_CUSTOM_ALGORITHM)
		usingCustomSchedulingAlgorithm = true;
	else
	{
		log_error(cpuLog, "Se recibio un valor incorrecto al esperar el tipo de algoritmo usado por el planificador. Este proceso sera abortado...");
		exit(EXIT_FAILURE);
	}

	if((nbytes = send_string(socket, config.cpuIp)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador la direccion ip de la CPU\n");
		return nbytes;
		//TODO (Optional) - Send Error Handling
	}

	if((nbytes = send_int(socket, config.cpuPort)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador la el puerto de la CPU\n");
		return nbytes;
		//TODO (Optional) - Send Error Handling
	}

	if((nbytes = receive_int(socket, &message)) <= 0)
	{
		if(nbytes == 0)
		{
			log_error(cpuLog, "Error al recibir mensaje de confirmacion de handshake del planificador\n");
			return nbytes;
		}
		else
		{
			log_error(cpuLog, "Error al recibir mensaje de confirmacion de handshake del planificador\n");

			close(socket);
			FD_CLR(socket, &master);
			return nbytes;
		}
	}

	log_info(cpuLog, "El handshake con el planificador se ha realizado correctamente!\n");

	return 0;
}

void connectToServers()
{
	uint32_t socket;
	int32_t result;

	if ((socket = connect_server(config.schedulerIp, config.schedulerPort, -1, cpuLog)) == 0)
	{
		log_error(cpuLog, "Error al conectar con S-AFA");
		exit(EXIT_FAILURE);
	}
	else
	{
		if((result = handshakeScheduler(socket)) < 0)		//Error receiving handshake messages (for example, a recv call took too long to receive messages)
		{
			log_error(cpuLog, "Error al conectar con S-AFA");
			exit(EXIT_FAILURE);
		}

		schedulerServerSocket = socket;

		log_info(cpuLog, "Conexion con S-AFA exitosa");
	}

	if ((socket = connect_server(config.dmaIp, config.dmaPort, NEW_CPU_CONNECTION, cpuLog)) == 0)
	{
		log_error(cpuLog, "Error al conectar con El Diego");
		exit(EXIT_FAILURE);
	}
/*	else
	{
		if((result = handshakeProcess(socket)) < 0)		//Error receiving handshake messages (for example, a recv call took too long to receive messages)
		{
			log_error(cpuLog, "Error al conectar con El Diego");
			exit(EXIT_FAILURE);
		}

		dmaServerSocket = socket;

		log_info(cpuLog, "Conexion con El Diego exitosa");
	}*/

	log_info(cpuLog, "Conexion con El Diego exitosa");

	if ((socket = connect_server(config.memoryIp, config.memoryPort, NEW_CPU_CONNECTION, cpuLog)) == 0)
	{
		log_error(cpuLog, "Error al conectar con FM9");
		exit(EXIT_FAILURE);
	}

	log_info(cpuLog, "Conexion con FM9 exitosa");
/*	else
	{
		if((result = handshakeProcess(socket)) < 0)		//Error receiving handshake messages (for example, a recv call took too long to receive messages)
		{
			log_error(cpuLog, "Error al conectar con FM9");
			exit(EXIT_FAILURE);
		}

			memoryServerSocket = socket;

			log_info(cpuLog, "Conexion con FM9 exitosa");
		}*/
}

void executeProcess()
{
	uint32_t instructionsQty;
	int32_t nbytes;
	bool instructionWasExecuted;
	PCB_t* processToExecute = NULL;
	char* scriptContent = NULL; 	//The contents of a script file, which are in memory (all the lines of instructions of that script arranged in a string)
	char* lineToParse = NULL;
	t_list* parsedScript;	//This contains an element for each line/instruction the script has
	t_list* parsedLine;		//This contains the instruction (first element of the list) and its arguments (each is an element of the list)

	if((nbytes = recvPCB(schedulerServerSocket, processToExecute)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir un PCB del mismo\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir un PCB del planificador\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
	}

	processInExecutionPCB = processToExecute;
	instructionsExecuted = 0;
	currentProcessQuantum = processInExecutionPCB->remainingQuantum;
	currentProgramCounter = processInExecutionPCB->programCounter;	//The program counter is the number of line of the last instruction that was executed (in the script, lines index starts at 1)
																	//So, after receiving the PCB, to get the next instruction to execute, I
																	//must increment the programCounter by 1.
																	//If this is the first execution for that PCB, the currentProgramCounter == 0
	scriptContent = requestScriptFromMemory();

	parsedScript = parseScript(scriptContent);
	instructionsQty = list_size(parsedScript);	//How many instructions the script has


	//Execute instructions until the program counter (that starts at index 1) exceeds the number
	//of instructions of the script, or until the process runs out of quantum
	while((currentProcessQuantum != 0) && (currentProgramCounter < instructionsQty) && (!stopExecution))
	{
		//This variables get changed here and not at the end of the while, because if a PCB must be sent back to the scheduler inside
		//the "checkAndExecuteInstruction" function, when it gets updated, its variables get updated correctly with this ones

		//If this variables were to be updated after executing and instruction, I should check inside that function when and where to
		//modify this variables in order for the PCB to be updated correctly, should the PCB be sent back to the scheduler (so the variables would
		//be updated inside that function and in here, at the end of the 'while' loop), which would be more difficult

		currentProgramCounter++;
		currentProcessQuantum--;
		instructionsExecuted++;

		lineToParse = (char*) list_get(parsedScript, (currentProgramCounter - 1));	//The index in a script (for the instructions) starts at 1, but
																					//the index of a list starts at 0
		parsedLine = parseLine(lineToParse);

		instructionWasExecuted = checkAndExecuteInstruction(parsedLine);

		if(instructionWasExecuted)
		{
			if((nbytes = send_int(schedulerServerSocket, COMPLETED_INSTRUCTION)) < 0)
			{
				log_error(cpuLog, "Error al indicar al planificador que se ejecuto una instruccion\n");

				log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
				exit(EXIT_FAILURE);
				//TODO (Optional) - Send Error Handling
			}
		}
		else	//If no instruction was executed (an empty line was found), decrement the quantum and the instructionsExecuted counter
		{
			currentProcessQuantum--;
			instructionsExecuted++;
		}

		list_destroy_and_destroy_elements(parsedLine, free);	//Each element of the list is a "char*"; this deletes the list and its elements
	}

	if(currentProgramCounter >= instructionsQty)	//Reached the end of the script; there are no more instructions to execute
	{
		updatePCBAndSendExecutionEndMessage(END_OF_SCRIPT);

		tellMemoryToFreeProcessData();	//To make sure that the memory has no data of the process after it ends
										//(maybe a file did not get closed by an instruction), I send it a message telling it to
										//free all data related to the ending process...

		log_info(cpuLog, "El proceso %d finalizo su ejecucion (por fin de script) luego de ejecutar %d instrucciones", processInExecutionPCB->pid, instructionsExecuted);
	}
	else if(currentProcessQuantum == 0)	//Reached the end of the quantum
	{
		updatePCBAndSendExecutionEndMessage(QUANTUM_END);

		log_info(cpuLog, "El proceso %d finalizo su ejecucion (por fin de quantum) luego de ejecutar %d instrucciones", processInExecutionPCB->pid, instructionsExecuted);
	}
	else if(killExecutingProcess)
	{
		updatePCBAndSendExecutionEndMessage(PROCESS_ERROR);

		killExecutingProcess = false;
		stopExecution = false;

		log_info(cpuLog, "Se ha finalizado la ejecucion del proceso con ID %d de forma prematura por peticion del planificador\n", processInExecutionPCB->pid);
	}

	processInExecutionPCB = NULL;
	instructionsExecuted = 0;
	currentProcessQuantum = 0;
	currentProgramCounter = 0;


	free(processInExecutionPCB);  //After an execution, and after sending the PCB back to the scheduler, its structure must be cleaned
	list_destroy_and_destroy_elements(parsedScript, free);  //Free the parsed script and all its elements (that are "char*")
	free(scriptContent);  //Free the script received from the memory
}

void initializeProcess()
{
	int32_t nbytes;
	PCB_t* processToInitialize = NULL;

	if((nbytes = recvPCB(schedulerServerSocket, processToInitialize)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir un PCB del mismo\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir un PCB del planificador\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
	}
	if((nbytes = send_int(schedulerServerSocket, MESSAGE_RECEIVED)) < 0)
	{
		log_error(cpuLog, "Error al indicar al planificador que se recibio un PCB correctamente\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	sendOpenFileRequestToDMA(processToInitialize->scriptPathInFS);

	if((nbytes = send_int(schedulerServerSocket, BLOCK_PROCESS_INIT)) < 0)
	{
		log_error(cpuLog, "Error al indicar al planificador que debe bloquear un proceso\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	if((nbytes = send_int(schedulerServerSocket, processToInitialize->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el pid del proceso a bloquear\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
}

t_list* parseScript(char* script)
{
	//Receives the script that the memory sends and transforms it to a list; the line/instruction 1 of the script is the element with index 0 of the list, and so on...

	t_list* parsedScript =  string_split_to_list(script, "\n");

	//Remove the comment lines from the parsedScript list (those are the lines that begin with a '#')
	removeCommentsFromScript(parsedScript);

	return parsedScript;
}

t_list* parseLine(char* line)	//The first element of the list is the instruction, the other elements are the arguments in order of appearance
{
	return string_split_to_list(line, " ");
}

bool checkAndExecuteInstruction(t_list* parsedLine)	//The 'parsedLine' list should be destroyed after calling this function
{
	//Error handling in this function is optional, as the assignment states that no instructions with
	//sintax or semantic errors will appear in the script

	char* instruction = (char*) list_get(parsedLine, 0);
	uint32_t listSize = list_size(parsedLine);	//listSize == 1 -> Received only the instruction
												//Each increment after that indicates the number of arguments received for that instruction

	if(listSize == 0)
	{
		//Received a line without instructions; return and skip to the next line
		return false;
	}

	if(string_equals_ignore_case("abrir", instruction))
	{
		if(listSize == 1)	//Only the instruction was provided
		{
			//TODO (optional) - Error -> The instruction received no arguments
		}
		else if(listSize > 2)
		{
			//TODO (optional) - Error -> More than one argument received
			//					Should this error appear, or should I take the first argument and continue?
		}

		char* filePath = (char*) list_get(parsedLine, 1);

		log_info(cpuLog, "Se comenzara la ejecucion de la instruccion \"abrir\"...\n");

		openFile(filePath);
	}
	else if(string_equals_ignore_case("concentrar", instruction))
	{
		if(listSize > 1)
		{
			//TODO (optional) - Error -> Arguments received, should have no arguments for this instruction
			//					 Should I ignore this error, or should I execute the instruction and continue?
		}

		log_info(cpuLog, "Se comenzara la ejecucion de la instruccion \"concentrar\"...\n");

		//Do nothing. This instruction should only count as another instruction execution, but it does nothing; it only makes the thread sleep for
		//the time specified in the "ExecutionDelay" property of the config file (as any other instruction)
	}
	else if(string_equals_ignore_case("asignar", instruction))
	{
		if(listSize < 4)
		{
			//TODO (optional) - Error -> Arguments missing (received only the instruction, or less than 3 arguments)
		}
		else if(listSize > 4)
		{
			//TODO (optional) - Error -> Should not receive more than 3 arguments...
		}

		char* filePath = (char*) list_get(parsedLine, 1);
		char* lineNumber = (char*) list_get(parsedLine, 2);
		char* dataToAssign = (char*) list_get(parsedLine, 3);

		uint32_t lineNumberInt = (uint32_t) strtol(lineNumber, NULL, 10);

		log_info(cpuLog, "Se comenzara la ejecucion de la instruccion \"asignar\"...\n");

		modifyFileLineInMemory(filePath, lineNumberInt, dataToAssign);
	}
	else if(string_equals_ignore_case("wait", instruction))
	{
		if(listSize < 2)
		{
			//TODO (optional) - Error -> Arguments missing (received only the instruction)
		}
		else if(listSize > 2)
		{
			//TODO (optional) - Error -> Should not receive more than 1 arguments...
		}

		char* semaphoreName = (char*) list_get(parsedLine, 1);

		log_info(cpuLog, "Se comenzara la ejecucion de la instruccion \"wait\"...\n");

		waitSemaphore(semaphoreName);
	}
	else if(string_equals_ignore_case("signal", instruction))
	{
		if(listSize < 2)
		{
			//TODO (optional) - Error -> Arguments missing (received only the instruction)
		}
		else if(listSize > 2)
		{
			//TODO (optional) - Error -> Should not receive more than 1 arguments...
		}

		char* semaphoreName = (char*) list_get(parsedLine, 1);

		log_info(cpuLog, "Se comenzara la ejecucion de la instruccion \"signal\"...\n");

		signalSemaphore(semaphoreName);
	}
	else if(string_equals_ignore_case("flush", instruction))
	{
		if(listSize < 2)
		{
			//TODO (optional) - Error -> Arguments missing (received only the instruction)
		}
		else if(listSize > 2)
		{
			//TODO (optional) - Error -> Should not receive more than 1 arguments...
		}

		char* filePath = (char*) list_get(parsedLine, 1);

		log_info(cpuLog, "Se comenzara la ejecucion de la instruccion \"flush\"...\n");

		flushFile(filePath);
	}
	else if(string_equals_ignore_case("close", instruction))
	{
		if(listSize < 2)
		{
			//TODO (optional) - Error -> Arguments missing (received only the instruction)
		}
		else if(listSize > 2)
		{
			//TODO (optional) - Error -> Should not receive more than 1 arguments...
		}

		char* filePath = (char*) list_get(parsedLine, 1);

		log_info(cpuLog, "Se comenzara la ejecucion de la instruccion \"close\"...\n");

		closeFile(filePath);
	}
	else if(string_equals_ignore_case("crear", instruction))
	{
		if(listSize < 3)
		{
			//TODO (optional) - Error -> Arguments missing (received only the instruction, or only 1 argument of the 2 required)
		}
		else if(listSize > 3)
		{
			//TODO (optional) - Error -> Should not receive more than 2 arguments...
		}

		char* filePath = (char*) list_get(parsedLine, 1);
		char* fileLines = (char*) list_get(parsedLine, 2);

		uint32_t fileLinesInt = (uint32_t) strtol(fileLines, NULL, 10);

		log_info(cpuLog, "Se comenzara la ejecucion de la instruccion \"crear\"...\n");

		createFile(filePath, fileLinesInt);
	}
	else if(string_equals_ignore_case("borrar", instruction))
	{
		if(listSize < 2)
		{
			//TODO (optional) - Error -> Arguments missing (received only the instruction)
		}
		else if(listSize > 2)
		{
			//TODO (optional) - Error -> Should not receive more than 1 arguments...
		}

		char* filePath = (char*) list_get(parsedLine, 1);

		log_info(cpuLog, "Se comenzara la ejecucion de la instruccion \"borrar\"...\n");

		deleteFile(filePath);
	}
	else
	{
		//TODO (optional) - Received a wrong instruction, raise an error
		log_error(cpuLog, "Se ha intentado ejecutar una instruccion incorrecta");
	}

	//Sleep for some time (ExecutionDelay)
	log_info(cpuLog, "Retardo de ejecucion...\n");

	double milisecondsSleep = config.executionDelay / 1000;
	sleep(milisecondsSleep);

	log_info(cpuLog, "Se ha finalizado la ejecucion de la instruccion\n");

	return true;
}

void openFile(char* filePathInFS)
{
	//Check if the file was opened by the current process
	uint32_t message = (uint32_t) askSchedulerIfFileOpen(filePathInFS);

	switch(message)
	{
		case FILE_NOT_OPEN:
			//If the file is not open, I should send a message to the DMA telling it to open the file, and send a message to the scheduler telling it to block the current process
			handleFileNotOpen(filePathInFS, false);
		break;

		case FILE_OPENED_BY_ANOTHER_PROCESS:
			//If the file was opened by other process, I should tell the scheduler to block the current process and wait for the file to be liberated
			handleFileOpenedByOtherProcess(filePathInFS, false);
		break;

		case FILE_OPEN:
			//Do Nothing, no need to open the file again; go to the next instruction
			log_info(cpuLog, "Se solicito la apertura del archivo del path \"%s\", pero este ya fue abierto por el proceso %d\n", filePathInFS, processInExecutionPCB->pid);
		break;
	}
}

void modifyFileLineInMemory(char* filePathInFS, uint32_t lineNumber, char* dataToAssign)
{
	//Check if the file was opened by the current process
	uint32_t message = (uint32_t) askSchedulerIfFileOpen(filePathInFS);

	switch(message)
	{
		case FILE_NOT_OPEN:
				//If the file is not open, it means it is an error to try modifying a file not open by the current process -> Raise an error and tell the scheduler
			handleFileNotOpen(filePathInFS, true);
		break;

		case FILE_OPENED_BY_ANOTHER_PROCESS:
			//If the file was opened by other process, it means it is an error to try modifying a file not open by the current process -> Raise an error and tell the scheduler
			handleFileOpenedByOtherProcess(filePathInFS, true);
		break;

		case FILE_OPEN:
			//Tell the DMA to modify a file and send the filePath, the number of the line that should be modified, and the data to insert in that line
			//Wait for the memory to confirm the result of the operation before continuing execution
			handleModifyFile(filePathInFS, lineNumber, dataToAssign);
		break;
	}
}

void waitSemaphore(char* semaphoreName)
{
	int32_t nbytes;
	int32_t message;

	//Tell the scheduler to wait on that semaphore, then wait for the result
	if((nbytes = send_int(schedulerServerSocket, WAIT_RESOURCE)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al planificador que haga un WAIT sobre un semaforo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(schedulerServerSocket, processInExecutionPCB->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el pid del proceso para el cual debe hacer un WAIT\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(schedulerServerSocket, semaphoreName)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el nombre del semaforo al cual debe hacerle WAIT\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Receive the scheduler response
	if((nbytes = receive_int(schedulerServerSocket, &message)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir la confirmacion de si se pudo realizar una operacion de WAIT sobre el semaforo \"%s\"\n", semaphoreName);
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir la confirmacion de si se pudo realizar una operacion de WAIT sobre el semaforo \"%s\"\n", semaphoreName);

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
	}

	if(message == WAIT_OK)
	{
		//If the scheduler could wait on the semaphore, continue execution (so, do nothing here)...
		log_info(cpuLog, "Se realizo con exito la operacion WAIT sobre el semaforo \"%s\"\n", semaphoreName);
	}
	else if(message == WAIT_ERROR)
	{
		//Send the scheduler a blockProcess request, so the process waits for the semaphore to become free
		log_info(cpuLog, "Se intento realizar la operacion WAIT sobre el semaforo \"%s\", pero el mismo se encuentra ocupado ocupado (no presenta mas instancias disponibles)\nEl proceso sera bloqueado...\n", semaphoreName);
		tellSchedulerToBlockProcess(false);
	}
}

void signalSemaphore(char* semaphoreName)
{
	int32_t nbytes;
	int32_t message;

	//Tell the scheduler to wait on that semaphore, then wait for the result
	if((nbytes = send_int(schedulerServerSocket, SIGNAL_RESOURCE)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al planificador que haga un SIGNAL sobre un semaforo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(schedulerServerSocket, processInExecutionPCB->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el pid del proceso para el cual debe hacer un SIGNAL\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(schedulerServerSocket, semaphoreName)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el nombre del semaforo al cual debe hacerle SIGNAL\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Receive the scheduler response
	if((nbytes = receive_int(schedulerServerSocket, &message)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir la confirmacion de si se pudo realizar una operacion de SIGNAL sobre el semaforo \"%s\"\n", semaphoreName);
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir la confirmacion de si se pudo realizar una operacion de SIGNAL sobre el semaforo \"%s\"\n", semaphoreName);

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
	}

	if(message == SIGNAL_OK)
	{
		//If the scheduler could wait on the semaphore, continue execution (so, do nothing here)...
		log_info(cpuLog, "Se realizo con exito la operacion SIGNAL sobre el semaforo \"%s\"\n", semaphoreName);
	}
	else if(message == SIGNAL_ERROR)
	{
		//This is error is a placeholder; the scheduler never sends a "SIGNAL_ERROR" message, because a signal should never produce an error

		log_info(cpuLog, "Se intento realizar la operacion SIGNAL sobre el semaforo \"%s\", pero ocurrio un error\nEl proceso sera terminado...\n", semaphoreName);
		handleProcessError();
	}
}

void flushFile(char* filePathInFS)
{
	//Check if the file was opened by the current process
	uint32_t message = (uint32_t) askSchedulerIfFileOpen(filePathInFS);

	switch(message)
	{
		case FILE_NOT_OPEN:
			//If the file is not open, it means it is an error to try flushing a file not open by the current process -> Raise an error and tell the scheduler
			handleFileNotOpen(filePathInFS, true);
		break;

		case FILE_OPENED_BY_ANOTHER_PROCESS:
			//If the file was opened by other process, it means it is an error to try flushing a file not open by the current process -> Raise an error and tell the scheduler
			handleFileOpenedByOtherProcess(filePathInFS, true);
		break;

		case FILE_OPEN:
			//Tell the DMA to flush the file and send the filePath. Then tell the scheduler to block the process
			handleFlushFile(filePathInFS);
		break;
	}
}

void closeFile(char* filePathInFS)
{
	//Check if the file was opened by the current process
	uint32_t message = (uint32_t) askSchedulerIfFileOpen(filePathInFS);

	switch(message)
	{
		case FILE_NOT_OPEN:
			//If the file is not open, it means it is an error to try closing a file not open by the current process -> Raise an error and tell the scheduler
			handleFileNotOpen(filePathInFS, true);
		break;

		case FILE_OPENED_BY_ANOTHER_PROCESS:
			//If the file was opened by other process, it means it is an error to try closing a file not open by the current process -> Raise an error and tell the scheduler
			handleFileOpenedByOtherProcess(filePathInFS, true);
		break;

		case FILE_OPEN:
			//Tell the scheduler to close the file and send the filePath. Then tell the memory to remove the file's data from it
			handleCloseFile(filePathInFS);
		break;
	}
}

void createFile(char* filePathInFS, uint32_t numberOfLines)
{
	int32_t nbytes;

	//Tell the DMA to create the file in the specified path and with the specified number of lines; also send the ID of the process that asks for the file creation
	if((nbytes = send_int(dmaServerSocket, CREATE_FILE)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al DMA que cree un archivo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(dmaServerSocket, processInExecutionPCB->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar al DMA el pid del proceso para el cual se debe crear un archivo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(dmaServerSocket, filePathInFS)) < 0)
	{
		log_error(cpuLog, "Error al enviar al DMA el nombre del archivo que debe crear\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(dmaServerSocket, numberOfLines)) < 0)
	{
		log_error(cpuLog, "Error al enviar al DMA la cantidad de lineas que debe contener el archivo que se desea crear\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	tellSchedulerToBlockProcess(true);
}

void deleteFile(char* filePathInFS)
{
	int32_t nbytes;

	//Tell the DMA to delete the file in the specified path
	if((nbytes = send_int(dmaServerSocket, DELETE_FILE)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al DMA que elimine un archivo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(dmaServerSocket, processInExecutionPCB->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar al DMA el pid del proceso para el cual se debe elimninar un archivo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(dmaServerSocket, filePathInFS)) < 0)
	{
		log_error(cpuLog, "Error al enviar al DMA el nombre del archivo que debe eliminar\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	tellSchedulerToBlockProcess(true);
}

uint32_t askSchedulerIfFileOpen(char* filePathInFS)
{
	int32_t nbytes;
	int32_t message;

	//Send the scheduler the taskCode, the current process' ID, and the path of the file to open (in order for the scheduler to locate it in the fileTable)
	if((nbytes = send_int(schedulerServerSocket, CHECK_IF_FILE_OPEN)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al planificador que verifique si un archivo esta abierto\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(schedulerServerSocket, processInExecutionPCB->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el pid del proceso para el cual verificar si un archivo esta abierto\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(schedulerServerSocket, filePathInFS)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el nombre del archivo que debe verificar si se encuentra abierto\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Receive a message from the scheduler telling me if the file was open or not
	if((nbytes = receive_int(schedulerServerSocket, &message)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir la confirmacion de si un archivo se encuentra abierto\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir la confirmacion de si un archivo se encuentra abierto del planificador\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
	}

	return message;
}

void handleFileNotOpen(char* filePathInFS, bool raiseError)
{
	//If this function does not raise an error, it means it should handle the FILE_NOT_OPEN message when trying to execute the openFile instruction
	//If it raises an error, it has to handle the FILE_NOT_OPEN message when trying to execute another instruction related to files

	if(!raiseError)
	{
		sendOpenFileRequestToDMA(filePathInFS);
		tellSchedulerToBlockProcess(false);
	}
	else
	{
		log_error(cpuLog, "Se solicito una operacion sobre un archivo no abierto por el proceso %d\nEl proceso sera terminado..\n", processInExecutionPCB->pid);
		handleProcessError();
	}
}

void handleFileOpenedByOtherProcess(char* filePathInFS, bool raiseError)
{
	//If this function does not raise an error, it means it should handle the FILE_OPENED_BY_ANOTHER_PROCESS message when trying to execute the openFile instruction
	//If it raises an error, it has to handle the FILE_OPENED_BY_ANOTHER_PROCESS message when trying to execute another instruction related to files

	if(!raiseError)
	{
		log_info(cpuLog, "Se solicito la apertura del archivo del path \"%s\" para el proceso %d, pero dicho archivo ya fue abierto por otro proceso\nEl proceso actual sera bloqueado\n", filePathInFS, processInExecutionPCB->pid);
		tellSchedulerToBlockProcess(false);
	}
	else
	{
		log_error(cpuLog, "Se solicito una operacion sobre un archivo no abierto por el proceso %d\nEl proceso sera terminado..\n", processInExecutionPCB->pid);
		handleProcessError();
	}
}

void handleProcessError()
{
	//Send a message to the scheduler telling it to kill the current process due to an error
	//Also, tell the memory there was an error, so it removes all files opened by this process and discards the changes

	int32_t nbytes;
	int32_t message;

	updateCurrentPCB();

	if((nbytes = send_int(schedulerServerSocket, PROCESS_ERROR)) < 0)
	{
		log_error(cpuLog, "Error al indicar al planificador que ocurrio un error en la ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = sendPCB(schedulerServerSocket, processInExecutionPCB)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el PCB del proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	//The scheduler sends a message telling the CPU it received the PCB
	if((nbytes = receive_int(schedulerServerSocket, &message)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir la confirmacion de recepcion de un PCB");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir la confirmacion de recepcion de un PCB del planificador");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
	}


	tellMemoryToFreeProcessData();

	log_info(cpuLog, "El proceso %d finalizo su ejecucion (debido a un error) luego de ejecutar %d instrucciones", processInExecutionPCB->pid, instructionsExecuted);
}

void tellMemoryToFreeProcessData()
{
	int32_t nbytes;
	int32_t message;

	if((nbytes = send_int(memoryServerSocket, CLOSE_PROCESS)) < 0)
	{
		log_error(cpuLog, "Error al indicar a la memoria que ocurrio un error en la ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
			//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(memoryServerSocket, processInExecutionPCB->pid)) < 0)
	{
		log_error(cpuLog, "Error al indicar a la memoria que ocurrio un error en la ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Wait for the memory to confirm the result of the operation before continuing execution
	if((nbytes = receive_int(memoryServerSocket, &message)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "La memoria fue desconectada al intentar recibir el resultado de una operacion de cierre de archivos para un proceso");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir el resultado de una operacion de cierre de archivos para un proceso de la memoria");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara");
		exit(EXIT_FAILURE);
	}

	if(message == ARCHIVO_NO_ABIERTO)
	{
		log_warning(cpuLog, "Se intentaron quitar archivos de la memoria que no se encuentran en ella");
	}
	else if(message == PROCESO_NO_ABIERTO)
	{
		log_warning(cpuLog, "Se intentaron quitar archivos de la memoria para un proceso que no tiene archivos en ella");
	}
	else if(message == ERROR)
	{
		log_warning(cpuLog, "Ocurrio un error al intentar quitar los archivos en memoria de un proceso");
	}
	else if(message == OK)
	{
		log_info(cpuLog, "Se han quitado de la memoria todos los archivos para el proceso %d de forma exitosa", processInExecutionPCB->pid);
	}
}

void tellSchedulerToBlockProcess(bool isDmaCall)
{
	int32_t nbytes;
	int32_t message;

	updateCurrentPCB();

	//Send a message to the scheduler telling it to block the process, and the PCB
	if((nbytes = send_int(schedulerServerSocket, BLOCK_PROCESS)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al planificador que bloquee el proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = sendPCB(schedulerServerSocket, processInExecutionPCB)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el PCB del proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(schedulerServerSocket, isDmaCall)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al planificador que bloquee el proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	//The scheduler sends a message telling the CPU it received the PCB
	if((nbytes = receive_int(schedulerServerSocket, &message)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir la confirmacion de recepcion de un PCB");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir la confirmacion de recepcion de un PCB del planificador");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
	}

	log_info(cpuLog, "El proceso %d finalizo su ejecucion (por bloqueo), luego de ejecutar %d instrucciones", processInExecutionPCB->pid, instructionsExecuted);
}

void handleModifyFile(char* filePathInFS, uint32_t lineNumber, char* dataToAssign)
{
	int32_t nbytes;
	int32_t message;

	//Tell the memory to modify a file and send the filePath, the number of the line that should be modified, and the data to insert in that line
	if((nbytes = send_int(memoryServerSocket, ASIGNAR)) < 0)
	{
		log_error(cpuLog, "Error al solicitar a la memoria que modifique un archivo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(memoryServerSocket, filePathInFS)) < 0)
	{
		log_error(cpuLog, "Error al enviar a la memoria el path del archivo a modificar\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(memoryServerSocket, lineNumber)) < 0)
	{
		log_error(cpuLog, "Error al indicar a la memoria el numero de linea a modificar en el archivo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(memoryServerSocket, dataToAssign)) < 0)
	{
		log_error(cpuLog, "Error al enviar a la memoria el dato a asignar en la linea del archivo especificado\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Wait for the memory to confirm the result of the operation before continuing execution
	if((nbytes = receive_int(schedulerServerSocket, &message)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir la confirmacion de si un archivo se encuentra abierto\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir la confirmacion de si un archivo se encuentra abierto del planificador\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
	}

	if(message == ARCHIVO_NO_ABIERTO)
	{
		log_info(cpuLog, "Se intento modificar un archivo en memoria que no se encuentra en ella. El proceso sera terminado...\n");

		handleProcessError();
	}
	else if(message == OK)
	{
		log_info(cpuLog, "Se ha modificado extosamente la linea %d del archivo en la ruta \"%s\"", lineNumber, filePathInFS);
	}
	else if(message == SEGMENTATION_FAULT)
	{
		log_info(cpuLog, "Ocurrio un error de segmentation fault al intentar modificar el contenido de un archivo en memoria. El proceso sera terminado...\n");

		handleProcessError();
	}
}

void handleFlushFile(char* filePathInFS)
{
	int32_t nbytes;

	//Tell the DMA to flush the file and send the filePath
	if((nbytes = send_int(dmaServerSocket, FLUSH_FILE)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al planificador que bloquee el proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(dmaServerSocket, processInExecutionPCB->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el PCB del proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(dmaServerSocket, filePathInFS)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el PCB del proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	log_info(cpuLog, "Se solicito al DMA la descarga de la informacion en memoria del archivo del path \"%s\"\n", filePathInFS);

	tellSchedulerToBlockProcess(true);
}

void handleCloseFile(char* filePathInFS)
{
	int32_t nbytes;
	int32_t message;

	//Tell the scheduler to wait on that semaphore, then wait for the result
	if((nbytes = send_int(schedulerServerSocket, CLOSE_FILE)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al planificador que cierre un archivo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(schedulerServerSocket, processInExecutionPCB->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el pid del proceso para el cual debe cerrar un archivo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(schedulerServerSocket, filePathInFS)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el nombre del archivo que debe cerrar\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Tell the memory to delete the file's data from it
	if((nbytes = send_int(memoryServerSocket, CLOSE_FILE)) < 0)
	{
		log_error(cpuLog, "Error al solicitar a la memoria que cierre un archivo\n");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(memoryServerSocket, filePathInFS)) < 0)
	{
		log_error(cpuLog, "Error al enviar a la memoria el nombre del archivo que debe cerrar\n");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Wait for the memory to confirm the result of the operation before continuing execution
	if((nbytes = receive_int(memoryServerSocket, &message)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "La memoria fue desconectada al intentar recibir el resultado de una operacion de cierre de archivo");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir el resultado de una operacion de cierre de archivo de la memoria");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara");
		exit(EXIT_FAILURE);
	}

	if(message == ARCHIVO_NO_ABIERTO)
	{
		log_info(cpuLog, "El archivo \"%s\" que se intento quitar de memoria no se encuentra en ella. El proceso sera terminado...", filePathInFS);

		handleProcessError();
	}
	else if(message == OK)
	{
		log_info(cpuLog, "Se ha cerrado exitosamente el archivo de la ruta \"%s\"", filePathInFS);
	}
}

void updateCurrentPCB()
{
	processInExecutionPCB->totalInstructionsExecuted += instructionsExecuted;
	processInExecutionPCB->instructionsExecutedOnLastExecution = instructionsExecuted;
	processInExecutionPCB->programCounter += instructionsExecuted;
	processInExecutionPCB->remainingQuantum -= instructionsExecuted;

	if(usingCustomSchedulingAlgorithm)
	{
		if(processInExecutionPCB->instructionsUntilIoOrEnd < instructionsExecuted)
		{
			//More instructions were executed than the ones left to the next IO which means the process already called an IO instruction
			//Or it may be that the scheduling algorithm used when this process started executing did not require to check the instructions left until IO or end of script
			processInExecutionPCB->instructionsUntilIoOrEnd = 0;
		}
		else
			processInExecutionPCB-> instructionsUntilIoOrEnd -= instructionsExecuted;

	}
}

void updatePCBAndSendExecutionEndMessage(uint32_t messageCode)
{
	int32_t nbytes;
	int32_t message;

	updateCurrentPCB();

	if((nbytes = send_int(schedulerServerSocket, messageCode)) < 0)
	{
		log_error(cpuLog, "Error al indicar al planificador que se termino el quantum de un proceso\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = sendPCB(schedulerServerSocket, processInExecutionPCB)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el PCB del proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	//The scheduler sends a message telling the CPU it received the PCB
	if((nbytes = receive_int(schedulerServerSocket, &message)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir la confirmacion de recepcion de un PCB");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir la confirmacion de recepcion de un PCB del planificador");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
	}
}

void removeCommentsFromScript(t_list* script)
{
	uint32_t linesQty = list_size(script);
	char* line;

	for(uint32_t i = 0; i < linesQty; i++)
	{
		line = (char*) list_get(script, i);

		//If the line starts with a '#' character, it is a comment, it must be removed from the parsed script
		if(line[0] == '#')
		{
			list_remove(script, i);
		}
	}
}

uint32_t countInstructionsUntilEndOrIO(t_list* parsedScript)
{
	uint32_t linesQty = list_size(parsedScript);
	uint32_t instructionsCount = 0;
	char* line = NULL;
	t_list* parsedLine;

	//Counts how many instructions are left until the next IO operation or end of script

	for(uint32_t i = 0; i < linesQty; i++)
	{
		line = (char*) list_get(parsedScript, i);

		parsedLine = parseLine(line);

		//If an IO instruction or the end of script is found, stop counting lines
		if((list_size(parsedLine) == 0) || isIoInstruction(parsedLine))
			break;

		instructionsCount++;
	}

	return instructionsCount;
}

bool isIoInstruction(t_list* parsedLine)	//Receives a list that represents the parsed line and its arguments
{
	char* instruction = (char*) list_get(parsedLine, 0);

	if((string_equals_ignore_case("abrir", instruction)) || (string_equals_ignore_case("flush", instruction)) ||
			(string_equals_ignore_case("crear", instruction)) || (string_equals_ignore_case("borrar", instruction)))
	{
		return true;
	}

	return false;
}

void countProcessInstructions()
{
	int32_t nbytes;
	int32_t processId;
	int32_t programCounter;
	char* scriptPath;
	t_list* parsedScript;
	char* scriptContent;
	uint32_t instructionsCounted = 0;

	//Receive the pid, programCounter and scriptPath of a process to count instructions
	if((nbytes = receive_int(schedulerServerSocket, &processId)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir un pid\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir un pid del planificador\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
	}
	if((nbytes = receive_int(schedulerServerSocket, &programCounter)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir un program counter\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir un program counter del planificador\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
	}
	if((nbytes = receive_string(schedulerServerSocket, &scriptPath)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir un path de script\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir un path de script del planificador\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
	}


	scriptContent = requestScriptFromMemory();
	parsedScript = parseScript(scriptContent);
	instructionsCounted = countInstructionsUntilEndOrIO(parsedScript);


	//Send the instructions counted to the scheduler
	if((nbytes = send_int(schedulerServerSocket, instructionsCounted)) < 0)
	{
		log_error(cpuLog, "Error al informar al planificador la cantidad de instrucciones que le faltan a un proceso antes de una operacion de IO\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
}

char* requestScriptFromMemory()
{
	int32_t nbytes;
	char* scriptContent = NULL;

	//Ask the memory for the script
	if((nbytes = send_int(memoryServerSocket, REQUEST_SCRIPT)) < 0)
	{
		log_error(cpuLog, "Error al solicitar a la memoria que envie un script\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(memoryServerSocket, processInExecutionPCB->scriptPathInFS)) < 0)
	{
		log_error(cpuLog, "Error al enviar a la memoria el nombre del script requerido\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	if((nbytes = receive_string(memoryServerSocket, &scriptContent)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir un PCB del mismo\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir un PCB del planificador\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
	}

	return scriptContent;
}

void sendOpenFileRequestToDMA(char* filePathInFS)
{
	int32_t nbytes;

	//Send the task to the DMA, the process ID and the path of the file to open
	if((nbytes = send_int(dmaServerSocket, OPEN_FILE)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al planificador que bloquee el proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(dmaServerSocket, processInExecutionPCB->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el PCB del proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(dmaServerSocket, filePathInFS)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el PCB del proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	log_info(cpuLog, "Se solicito al DMA la apertura del archivo del path \"%s\", para el proceso %d\n", filePathInFS, processInExecutionPCB->pid);
}
