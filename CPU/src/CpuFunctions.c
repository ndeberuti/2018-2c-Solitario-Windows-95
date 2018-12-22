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
	//config_destroy(configFile);

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

void connectToServers()
{
	uint32_t _socket = 0;
	int32_t nbytes = 0;
	int32_t schedulerAlgorithm = 0;
	int32_t _memoryLineSize = 0;


	if ((_socket = connect_server(config.schedulerIp, config.schedulerPort, NEW_CPU_CONNECTION, cpuLog)) == 0)
	{
		log_error(cpuLog, "Error al conectar con S-AFA");
		exit(EXIT_FAILURE);
	}
	else if((nbytes = receive_int(_socket, &schedulerAlgorithm)) <= 0)
	{
		if(nbytes == 0)
		{
			log_error(cpuLog, "Error al recibir el tipo de algoritmo del planificador\n");
		}
		else
		{
			log_error(cpuLog, "Error al recibir el tipo de algoritmo del planificador\n");
		}

		log_error(socketErrorLog, "Receive error: %s", strerror(errno));
		log_error(cpuLog, "Debido a un error en la comunicacion con el planificador, se abortara el modulo");
		exit(EXIT_FAILURE);
	}
	else
	{
		if(schedulerAlgorithm == USE_NORMAL_ALGORITHM)
			usingCustomSchedulingAlgorithm = false;

		else if(schedulerAlgorithm == USE_CUSTOM_ALGORITHM)
			usingCustomSchedulingAlgorithm = true;
		else
		{
			log_error(cpuLog, "Se recibio un valor incorrecto al esperar el tipo de algoritmo usado por el planificador. Este proceso sera abortado...");
			exit(EXIT_FAILURE);
		}

		schedulerServerSocket = _socket;

		log_info(cpuLog, "El handshake y la conexion con el planificador se han realizado correctamente!\n");
	}

	if ((_socket = connect_server(config.dmaIp, config.dmaPort, NEW_CPU_CONNECTION, cpuLog)) == 0)
	{
		log_error(cpuLog, "Error al conectar con El Diego");
		exit(EXIT_FAILURE);
	}
	else
	{
		dmaServerSocket = _socket;
		log_info(cpuLog, "Conexion con El Diego exitosa");
	}

	if ((_socket = connect_server(config.memoryIp, config.memoryPort, NEW_CPU_CONNECTION, cpuLog)) == 0)
	{
		log_error(cpuLog, "Error al conectar con FM9");
		exit(EXIT_FAILURE);
	}
	else if((nbytes = receive_int(_socket, &_memoryLineSize)) <= 0)
	{
		if(nbytes == 0)
		{
			log_error(cpuLog, "La memoria se desconecto al intentar recibir el tamaño de linea de ella\n");
		}
		else
		{
			log_error(cpuLog, "Error al recibir el tamaño de linea de la memoria\n");
		}

		log_error(socketErrorLog, "Receive error: %s", strerror(errno));
		log_error(cpuLog, "Debido a un error en la comunicacion con la memoria, se abortara este modulo");
		exit(EXIT_FAILURE);
	}
	else
	{
		memoryServerSocket = _socket;
		memoryLineSize = _memoryLineSize;
		log_info(cpuLog, "Conexion con FM9 exitosa");
	}
}

void executeProcess()
{
	uint32_t instructionsQty = 0;
	int32_t nbytes = 0;
	uint32_t instructionExecutionResult = 0;
	uint32_t scriptLines = 0;
	PCB_t* processToExecute = NULL;
	char* scriptContent = NULL; 	//The contents of a script file, which are in memory (all the lines of instructions of that script arranged in a string)
	char* lineToParse = NULL;
	t_list* parsedScript = NULL;	//This contains an element for each line/instruction the script has
	t_list* parsedLine = NULL;		//This contains the instruction (first element of the list) and its arguments (each is an element of the list)

	log_info(cpuLog, "El planificador solicita la ejecucion de un proceso");

	if((nbytes = recvPCB(schedulerServerSocket, &processToExecute)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir un PCB del mismo\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir un PCB del planificador\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Receive error: %s", strerror(errno));

		exit(EXIT_FAILURE);
	}

	showPCBInfo(processToExecute);

	processInExecutionPCB = &processToExecute;

	log_info(cpuLog, "Se recibio del planificador el PCB correspondiente al proceso %d", (*processInExecutionPCB)->pid);


	instructionsExecuted = 0;
	currentProcessQuantum = (*processInExecutionPCB)->remainingQuantum;
	currentProgramCounter = (*processInExecutionPCB)->programCounter;	//The program counter is the number of line of the last instruction that was executed (in the script, lines index starts at 1)
																	    //So, after receiving the PCB, to get the next instruction to execute, I
																	    //must increment the programCounter by 1.
																		//If this is the first execution for that PCB, the currentProgramCounter == 0
	scriptContent = requestScriptFromMemory(&scriptLines);

	log_info(cpuLog, "Me llego de memoria un script con %d lineas", scriptLines);

	if(scriptContent == NULL)	//Error getting the file from the memory
	{
		log_error(cpuLog, "Error al obtener un archivo de memoria. El proceso sera abortado...");
		handleProcessError();
		return;
	}

	parsedScript = parseScript(scriptContent, scriptLines);
	instructionsQty = list_size(parsedScript);	//How many instructions the script has

	log_info(cpuLog, "Quedan %d lineas a ejecutar para el proceso %d", (instructionsQty - currentProgramCounter), (*processInExecutionPCB)->pid);
	log_info(cpuLog, "El proceso %d que va a ejecutar tiene un quantum de %d", (*processInExecutionPCB)->pid, (*processInExecutionPCB)->remainingQuantum);

	//Execute instructions until the program counter (that starts at index 1) exceeds the number
	//of instructions of the script, or until the process runs out of quantum.
	//As the last line in a script is a '\n' char, I need to read lines until I get to the line with number 'instructionsQty - 1'
	while((currentProcessQuantum != 0) && (currentProgramCounter < instructionsQty))
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

		printf("\nlinea a parsear: %s\n\n", lineToParse);

		if(lineToParse[0] == '#')	//Comment lines start with '#' and must no be executed
		{
			currentProcessQuantum++;
			instructionsExecuted--;
			continue;
		}

		parsedLine = parseLine(lineToParse);

		instructionExecutionResult = checkAndExecuteInstruction(parsedLine);


		if((instructionExecutionResult == INSTRUCTION_EXECUTED))
		{
			if((nbytes = send_int(schedulerServerSocket, COMPLETED_INSTRUCTION)) < 0)
			{
				log_error(cpuLog, "Error al indicar al planificador que se ejecuto una instruccion\n");

				log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

				log_error(socketErrorLog, "Send error: %s", strerror(errno));

				exit(EXIT_FAILURE);
				//TODO (Optional) - Send Error Handling
			}
		}
		else if(instructionExecutionResult == PROCESS_BLOCKED)
		{
			if((nbytes = send_int(schedulerServerSocket, COMPLETED_INSTRUCTION)) < 0)
			{
				log_error(cpuLog, "Error al indicar al planificador que se ejecuto una instruccion\n");
				log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

				log_error(socketErrorLog, "Send error: %s", strerror(errno));
				exit(EXIT_FAILURE);
				//TODO (Optional) - Send Error Handling
			}

			free(*processInExecutionPCB);
			list_destroy_and_destroy_elements(parsedScript, free);
			free(scriptContent);

			return;
		}
		else if(instructionExecutionResult == PROCESS_KILLED)
		{
			//Do nothing, the error was treated before. Just end the execution

			free(*processInExecutionPCB);
			list_destroy_and_destroy_elements(parsedScript, free);
			free(scriptContent);

			return;
		}
		else if(instructionExecutionResult == INSTRUCTION_NOT_EXECUTED) //An empty line was read
		{
			currentProcessQuantum++;
			instructionsExecuted--;
		}

		//list_destroy_and_destroy_elements(parsedLine, free);	//Each element of the list is a "char*"; this deletes the list and its elements
	}

	if(currentProgramCounter >= instructionsQty)	//Reached the end of the script; there are no more instructions to execute
	{
		updatePCBAndSendExecutionEndMessage(END_OF_SCRIPT);

		tellMemoryToFreeProcessData();	//To make sure that the memory has no data of the process after it ends
										//(maybe a file did not get closed by an instruction), I send it a message telling it to
										//free all data related to the ending process...

		log_info(cpuLog, "El proceso %d finalizo su ejecucion (por fin de script) luego de ejecutar %d instrucciones", (*processInExecutionPCB)->pid, instructionsExecuted);
	}
	else if(currentProcessQuantum == 0)	//Reached the end of the quantum
	{
		updatePCBAndSendExecutionEndMessage(QUANTUM_END);

		log_info(cpuLog, "El proceso %d finalizo su ejecucion (por fin de quantum) luego de ejecutar %d instrucciones", (*processInExecutionPCB)->pid, instructionsExecuted);
	}

	free(*processInExecutionPCB);  //After an execution, and after sending the PCB back to the scheduler, its structure must be cleaned
	list_destroy_and_destroy_elements(parsedScript, free);  //Free the parsed script and all its elements (that are "char*")
	free(scriptContent);  //Free the script received from the memory

	processInExecutionPCB = NULL;
	instructionsExecuted = 0;
	currentProcessQuantum = 0;
	currentProgramCounter = 0;
}

void initializeProcess()
{
	int32_t nbytes = 0;
	PCB_t* processToInitialize = NULL;
	uint32_t message = 0;

	log_info(cpuLog, "El planificador solicita la incializacion de un proceso");

	if((nbytes = recvPCB(schedulerServerSocket, &processToInitialize)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir un PCB del mismo\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir un PCB del planificador\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Receive error: %s", strerror(errno));

		exit(EXIT_FAILURE);
	}

	processInExecutionPCB = &processToInitialize;

	log_info(cpuLog, "Se recibio del planificador el PCB correspondiente al proceso %d", (*processInExecutionPCB)->pid);

	if((nbytes = send_int(schedulerServerSocket, BLOCK_PROCESS_INIT)) < 0)
	{
		log_error(cpuLog, "Error al indicar al planificador que debe bloquear un proceso\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	if((nbytes = send_int(schedulerServerSocket, processToInitialize->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el pid del proceso a bloquear\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Wait for the scheduler to confirm the process was blocked, so the DMA will never try to unblock a process that was not blocked
	if((nbytes = receive_int(schedulerServerSocket, &message)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "El planificador fue desconectado al intentar recibir la confirmacion del bloqueo de un proceso\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir la confirmacion del bloqueo de un proceso del planificador\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		log_error(socketErrorLog, "Receive error: %s", strerror(errno));

		exit(EXIT_FAILURE);
	}

	sendOpenFileRequestToDMA(processToInitialize->scriptPathInFS, OPEN_SCRIPT);
}

/* The below function, I think, does not work for the type of information I need to handle, because
 * all the built-in linux functions to work with strings stop reading when they reach a '\0' character
 * and I need to read past those characters (because the '\0' are like paddings to fill a memoryLine
 * not filled with actual data). After a trail of '\0' chars there may be more information to be read,
 * that the built-in functions like 'strtok' do not get to because they stop when they reach a '\0')
 *
t_list* parseScript(char* script)
{
	//Receives the script that the memory sends and transforms it to a list; the line/instruction 1 of the script is the element with index 0 of the list, and so on...

	t_list* parsedScript =  string_split_to_list(script, "\n");

	//Remove the comment lines from the parsedScript list (those are the lines that begin with a '#')
	removeCommentsFromScript(parsedScript);

	return parsedScript;
}
*/


//TODO - This is ok for segmentation but maybe not for pagination; this function may be needed to be changed to work well with pagination
t_list* parseScript(char* script, uint32_t scriptLines)
{
	char* buffer = NULL;
	uint32_t currentLineOffset = 0;
	uint32_t currentMemoryLineStart = 0;
	uint32_t lineLength = 0;
	uint32_t charsToCopy = 0;
	t_list* parsedScript = NULL;
	parsedScript = list_create();

	for(uint32_t i = 0; i < scriptLines; i++)
	{
		currentMemoryLineStart = i * memoryLineSize; //The beginning of a memoryLine inside the script received from the memory
		currentLineOffset = currentMemoryLineStart;	 //That memoryLine contains an instruction line ending in '\n' (so I need to read until I find that char)


		//This is to see how many chars until a '\n'
		while(script[currentLineOffset] != '\n')
			currentLineOffset++;

		lineLength = currentLineOffset - currentMemoryLineStart;  //I need to subtract them because the offset starts with the number of the
																  //currentLine (see above, at the beginning of the 'for' loop!)

		if(lineLength > 0)	//The line contains more chars besides the '\n'
		{
			charsToCopy = lineLength;

			buffer = calloc(1, lineLength + 1);	//This string has the size of the line calculated above (length including the '\n' char)

			//'lineLength - 1' is the length of the file without the '\n' char.
			//I need to exclude the '\n' char from the string (in its place there will be a '\0' char)
			memcpy(buffer, (script + currentMemoryLineStart), charsToCopy);

			printf("\ncopie la linea %d -> %s\n", i+1, buffer);
		}
		else //The line only has a '\n' char
		{
			printf("hola");
			buffer = calloc(1, 1);
		}

		list_add(parsedScript, buffer);
	}

	printf("\n");

	return parsedScript;
}

t_list* parseLine(char* line)	//The first element of the list is the instruction, the other elements are the arguments in order of appearance
{
	return string_n_split_to_list(line, 4, " ");	//As the maximum number of arguments an instruction can have is 3, each line
													//must be separated in 4 strings (instruction + 3 arguments)
}

uint32_t checkAndExecuteInstruction(t_list* parsedLine)	//The 'parsedLine' list should be destroyed after calling this function
{
	//Error handling in this function is optional, as the assignment states that no instructions with
	//sintax or semantic errors will appear in the script

	uint32_t executionResult = 0;
	char* instruction = (char*) list_get(parsedLine, 0);
	uint32_t listSize = list_size(parsedLine);	//listSize == 1 -> Received only the instruction
												//Each increment after that indicates the number of arguments received for that instruction

	if(listSize == 0)
	{
		//Received a line without instructions; return and skip to the next line
		return INSTRUCTION_NOT_EXECUTED;
	}

	log_info(cpuLog, "Instruccion a ejecutar: %s", instruction);

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
		executionResult = openFile(filePath);
	}
	else if(string_equals_ignore_case("concentrar", instruction))
	{
		if(listSize > 1)
		{
			//TODO (optional) - Error -> Arguments received, should have no arguments for this instruction
			//					 Should I ignore this error, or should I execute the instruction and continue?
		}

		log_info(cpuLog, "Se comenzara la ejecucion de la instruccion \"concentrar\"...\n");
		executionDelay();
		executionResult = INSTRUCTION_EXECUTED;
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
		executionResult = modifyFileLineInMemory(filePath, lineNumberInt, dataToAssign);
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

		executionResult = waitSemaphore(semaphoreName);
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

		executionResult = signalSemaphore(semaphoreName);
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

		executionResult = flushFile(filePath);
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

		executionResult = closeFile(filePath);
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

		executionResult = createFile(filePath, fileLinesInt);
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

		executionResult = deleteFile(filePath);
	}
	else
	{
		//TODO (optional) - Received a wrong instruction, raise an error
		log_error(cpuLog, "Se ha intentado ejecutar una instruccion incorrecta");
		executionResult = handleProcessError();
	}

	if(executionResult == PROCESS_KILLED)
	{
		log_info(cpuLog, "La ultima instruccion que se intento ejecutar para el proceso %d produjo un error, por lo que se ordeno la terminacion de dicho proceso...", (*processInExecutionPCB)->pid);
	}
	else if(executionResult == 0)
	{
		log_error(cpuLog, "Se recibio un mensaje incorrecto de algun modulo. Se debe revisar la comunicacion con los demas modulos. Este modulo sera abortado debido a ello; se deberian abortar los demas modulos...");
		exit(EXIT_FAILURE);
	}

	return executionResult;
}

uint32_t openFile(char* filePathInFS)
{
	//Check if the file was opened by the current process
	uint32_t message = (uint32_t) askSchedulerIfFileOpen(filePathInFS);

	switch(message)
	{
		case FILE_NOT_OPEN:
			//If the file is not open, I should send a message to the DMA telling it to open the file, and send a message to the scheduler telling it to block the current process
			return handleFileNotOpen(filePathInFS, false);
		break;

		case FILE_OPENED_BY_ANOTHER_PROCESS:
			//If the file was opened by other process, I should tell the scheduler to block the current process and wait for the file to be liberated
			return handleFileOpenedByOtherProcess(filePathInFS, false);
		break;

		case FILE_OPEN:
			//Do Nothing, no need to open the file again; go to the next instruction
			log_info(cpuLog, "Se solicito la apertura del archivo del path \"%s\", pero este ya fue abierto por el proceso %d\n", filePathInFS, (*processInExecutionPCB)->pid);
			return INSTRUCTION_EXECUTED;
		break;
	}

	return 0; //It never reaches this part
}

uint32_t modifyFileLineInMemory(char* filePathInFS, uint32_t lineNumber, char* dataToAssign)
{
	//Check if the file was opened by the current process
	uint32_t message = (uint32_t) askSchedulerIfFileOpen(filePathInFS);

	switch(message)
	{
		case FILE_NOT_OPEN:
				//If the file is not open, it means it is an error to try modifying a file not open by the current process -> Raise an error and tell the scheduler
			return handleFileNotOpen(filePathInFS, true);
		break;

		case FILE_OPENED_BY_ANOTHER_PROCESS:
			//If the file was opened by other process, it means it is an error to try modifying a file not open by the current process -> Raise an error and tell the scheduler
			return handleFileOpenedByOtherProcess(filePathInFS, true);
		break;

		case FILE_OPEN:
			//Tell the DMA to modify a file and send the filePath, the number of the line that should be modified, and the data to insert in that line
			//Wait for the memory to confirm the result of the operation before continuing execution
			executionDelay();
			return handleModifyFile(filePathInFS, lineNumber, dataToAssign);
		break;
	}

	return 0;
}

uint32_t waitSemaphore(char* semaphoreName)
{
	int32_t nbytes = 0;
	int32_t message = 0;

	//Tell the scheduler to wait on that semaphore, then wait for the result
	if((nbytes = send_int(schedulerServerSocket, WAIT_RESOURCE)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al planificador que haga un WAIT sobre un semaforo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(schedulerServerSocket, (*processInExecutionPCB)->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el pid del proceso para el cual debe hacer un WAIT\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(schedulerServerSocket, semaphoreName)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el nombre del semaforo al cual debe hacerle WAIT\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

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

		log_error(socketErrorLog, "Receive error: %s", strerror(errno));

		exit(EXIT_FAILURE);
	}

	if(message == WAIT_OK)
	{
		//If the scheduler could wait on the semaphore, continue execution (so, do nothing here)...
		log_info(cpuLog, "Se realizo con exito la operacion WAIT sobre el semaforo \"%s\"\n", semaphoreName);
		executionDelay();
		return INSTRUCTION_EXECUTED;
	}
	else if(message == WAIT_ERROR)
	{
		//Send the scheduler a blockProcess request, so the process waits for the semaphore to become free
		log_info(cpuLog, "Se intento realizar la operacion WAIT sobre el semaforo \"%s\", pero el mismo se encuentra ocupado ocupado (no presenta mas instancias disponibles)\nEl proceso sera bloqueado...\n", semaphoreName);
		executionDelay();
		return tellSchedulerToBlockProcess(false);
	}

	return 0;
}

uint32_t signalSemaphore(char* semaphoreName)
{
	int32_t nbytes = 0;
	int32_t message = 0;

	//Tell the scheduler to wait on that semaphore, then wait for the result
	if((nbytes = send_int(schedulerServerSocket, SIGNAL_RESOURCE)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al planificador que haga un SIGNAL sobre un semaforo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(schedulerServerSocket, (*processInExecutionPCB)->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el pid del proceso para el cual debe hacer un SIGNAL\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(schedulerServerSocket, semaphoreName)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el nombre del semaforo al cual debe hacerle SIGNAL\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

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

		log_error(socketErrorLog, "Receive error: %s", strerror(errno));

		exit(EXIT_FAILURE);
	}

	if(message == SIGNAL_OK)
	{
		//If the scheduler could wait on the semaphore, continue execution (so, do nothing here)...
		log_info(cpuLog, "Se realizo con exito la operacion SIGNAL sobre el semaforo \"%s\"\n", semaphoreName);
		executionDelay();
		return INSTRUCTION_EXECUTED;
	}
	else if(message == SIGNAL_ERROR)
	{
		//This is error is a placeholder; the scheduler never sends a "SIGNAL_ERROR" message, because a signal should never produce an error

		log_info(cpuLog, "Se intento realizar la operacion SIGNAL sobre el semaforo \"%s\", pero ocurrio un error\nEl proceso sera terminado...\n", semaphoreName);
		return handleProcessError();
	}

	return 0;
}

uint32_t flushFile(char* filePathInFS)
{
	//Check if the file was opened by the current process
	uint32_t message = (uint32_t) askSchedulerIfFileOpen(filePathInFS);

	switch(message)
	{
		case FILE_NOT_OPEN:
			//If the file is not open, it means it is an error to try flushing a file not open by the current process -> Raise an error and tell the scheduler
			return handleFileNotOpen(filePathInFS, true);
		break;

		case FILE_OPENED_BY_ANOTHER_PROCESS:
			//If the file was opened by other process, it means it is an error to try flushing a file not open by the current process -> Raise an error and tell the scheduler
			return handleFileOpenedByOtherProcess(filePathInFS, true);
		break;

		case FILE_OPEN:
			//Tell the DMA to flush the file and send the filePath. Then tell the scheduler to block the process
			return handleFlushFile(filePathInFS);

		break;
	}

	return 0; //It never reaches this part
}

uint32_t closeFile(char* filePathInFS)
{
	//Check if the file was opened by the current process
	uint32_t message = (uint32_t) askSchedulerIfFileOpen(filePathInFS);

	switch(message)
	{
		case FILE_NOT_OPEN:
			//If the file is not open, it means it is an error to try closing a file not open by the current process -> Raise an error and tell the scheduler
			return handleFileNotOpen(filePathInFS, true);
		break;

		case FILE_OPENED_BY_ANOTHER_PROCESS:
			//If the file was opened by other process, it means it is an error to try closing a file not open by the current process -> Raise an error and tell the scheduler
			return handleFileOpenedByOtherProcess(filePathInFS, true);
		break;

		case FILE_OPEN:
			//Tell the scheduler to close the file and send the filePath. Then tell the memory to remove the file's data from it
			return handleCloseFile(filePathInFS);
		break;
	}

	return 0;
}

uint32_t createFile(char* filePathInFS, uint32_t numberOfLines)
{
	int32_t nbytes = 0;
	uint32_t processId = (*processInExecutionPCB)->pid;

	uint32_t result = tellSchedulerToBlockProcess(true);

	//Tell the DMA to create the file in the specified path and with the specified number of lines; also send the ID of the process that asks for the file creation
	if((nbytes = send_int(dmaServerSocket, CREATE_FILE)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al DMA que cree un archivo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(dmaServerSocket, processId)) < 0)
	{
		log_error(cpuLog, "Error al enviar al DMA el pid del proceso para el cual se debe crear un archivo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

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

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	log_info(cpuLog, "Se solicito al DMA la creacion del archivo \"%s\" con %d lineas para el proceso %d", filePathInFS, numberOfLines, processId);
	executionDelay();
	return result;
}

uint32_t deleteFile(char* filePathInFS)
{
	int32_t nbytes = 0;
	uint32_t processId = (*processInExecutionPCB)->pid;

	uint32_t result = tellSchedulerToBlockProcess(true);

	//Tell the DMA to delete the file in the specified path
	if((nbytes = send_int(dmaServerSocket, DELETE_FILE)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al DMA que elimine un archivo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(dmaServerSocket, processId)) < 0)
	{
		log_error(cpuLog, "Error al enviar al DMA el pid del proceso para el cual se debe elimninar un archivo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(dmaServerSocket, filePathInFS)) < 0)
	{
		log_error(cpuLog, "Error al enviar al DMA el nombre del archivo que debe eliminar\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	log_info(cpuLog, "Se solicito al DMA la eliminacion del archivo \"%s\" para el proceso %d", filePathInFS, processId);
	executionDelay();
	return result;
}

uint32_t askSchedulerIfFileOpen(char* filePathInFS)
{
	int32_t nbytes = 0;
	int32_t message = 0;

	//Send the scheduler the taskCode, the current process' ID, and the path of the file to open (in order for the scheduler to locate it in the fileTable)
	if((nbytes = send_int(schedulerServerSocket, CHECK_IF_FILE_OPEN)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al planificador que verifique si un archivo esta abierto\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(schedulerServerSocket, (*processInExecutionPCB)->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el pid del proceso para el cual verificar si un archivo esta abierto\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(schedulerServerSocket, filePathInFS)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el nombre del archivo que debe verificar si se encuentra abierto\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

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

		log_error(socketErrorLog, "Receive error: %s", strerror(errno));

		exit(EXIT_FAILURE);
	}

	return message;
}

uint32_t handleFileNotOpen(char* filePathInFS, bool raiseError)
{
	//If this function does not raise an error, it means it should handle the FILE_NOT_OPEN message when trying to execute the openFile instruction
	//If it raises an error, it has to handle the FILE_NOT_OPEN message when trying to execute another instruction related to files

	if(!raiseError)
	{
		executionDelay();

		uint32_t result = tellSchedulerToBlockProcess(false);

		sendOpenFileRequestToDMA(filePathInFS, OPEN_FILE);
		return result;
	}
	else
	{
		log_error(cpuLog, "Se solicito una operacion sobre un archivo no abierto por el proceso %d\nEl proceso sera terminado..\n", (*processInExecutionPCB)->pid);
		return handleProcessError();
	}
}

uint32_t handleFileOpenedByOtherProcess(char* filePathInFS, bool raiseError)
{
	//If this function does not raise an error, it means it should handle the FILE_OPENED_BY_ANOTHER_PROCESS message when trying to execute the openFile instruction
	//If it raises an error, it has to handle the FILE_OPENED_BY_ANOTHER_PROCESS message when trying to execute another instruction related to files

	if(!raiseError)
	{
		log_info(cpuLog, "Se solicito la apertura del archivo del path \"%s\" para el proceso %d, pero dicho archivo ya fue abierto por otro proceso\nEl proceso actual sera bloqueado\n", filePathInFS, (*processInExecutionPCB)->pid);
		return tellSchedulerToBlockProcess(false);
	}
	else
	{
		log_error(cpuLog, "Se solicito una operacion sobre un archivo no abierto por el proceso %d\nEl proceso sera terminado..\n", (*processInExecutionPCB)->pid);
		return handleProcessError();
	}
}

uint32_t handleProcessError()
{
	//Send a message to the scheduler telling it to kill the current process due to an error
	//Also, tell the memory there was an error, so it removes all files opened by this process and discards the changes

	int32_t nbytes = 0;

	updateCurrentPCB();

	if((nbytes = send_int(schedulerServerSocket, PROCESS_ERROR)) < 0)
	{
		log_error(cpuLog, "Error al indicar al planificador que ocurrio un error en la ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = sendPCB(schedulerServerSocket, (*processInExecutionPCB))) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el PCB del proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}



	tellMemoryToFreeProcessData();

	log_info(cpuLog, "El proceso %d finalizo su ejecucion (debido a un error) luego de ejecutar %d instrucciones", (*processInExecutionPCB)->pid, instructionsExecuted);
	return PROCESS_KILLED;
}

void tellMemoryToFreeProcessData()
{
	int32_t nbytes = 0;
	int32_t message = 0;

	if((nbytes = send_int(memoryServerSocket, CLOSE_PROCESS)) < 0)
	{
		log_error(cpuLog, "Error al indicar a la memoria que ocurrio un error en la ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
			//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(memoryServerSocket, (*processInExecutionPCB)->pid)) < 0)
	{
		log_error(cpuLog, "Error al indicar a la memoria que ocurrio un error en la ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

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

		log_error(socketErrorLog, "Receive error: %s", strerror(errno));

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
		log_info(cpuLog, "Se han quitado de la memoria todos los archivos para el proceso %d de forma exitosa", (*processInExecutionPCB)->pid);
	}
}

uint32_t tellSchedulerToBlockProcess(bool isDmaCall)
{
	int32_t nbytes = 0;
	int32_t message = 0;

	updateCurrentPCB();

	//Send a message to the scheduler telling it to block the process, and the PCB
	if((nbytes = send_int(schedulerServerSocket, BLOCK_PROCESS)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al planificador que bloquee el proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = sendPCB(schedulerServerSocket, (*processInExecutionPCB))) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el PCB del proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(schedulerServerSocket, isDmaCall)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al planificador que bloquee el proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Wait for the scheduler to tell me it blocked the current process (that is to avoid the dma trying to unlock the process before it is blocked...just in case)
	if((nbytes = receive_int(schedulerServerSocket, &message)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "El planificador fue desconectado al intentar recibir la confirmacion del bloqueo de un proceso\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir la confirmacion del bloqueo de un proceso del planificador\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");
		log_error(socketErrorLog, "Receive error: %s", strerror(errno));

		exit(EXIT_FAILURE);
	}


	log_info(cpuLog, "El proceso %d finalizo su ejecucion (por bloqueo), luego de ejecutar %d instrucciones", (*processInExecutionPCB)->pid, instructionsExecuted);
	return PROCESS_BLOCKED;
}

uint32_t handleModifyFile(char* filePathInFS, uint32_t lineNumber, char* dataToAssign)
{
	int32_t nbytes = 0;
	int32_t message = 0;

	//Tell the memory to modify a file and send the filePath, the number of the line that should be modified, and the data to insert in that line
	if((nbytes = send_int(memoryServerSocket, ASIGNAR)) < 0)
	{
		log_error(cpuLog, "Error al solicitar a la memoria que modifique un archivo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(memoryServerSocket, (*processInExecutionPCB)->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar a la memoria el id del proceso que solicita modificar un archivo\n");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(memoryServerSocket, filePathInFS)) < 0)
	{
		log_error(cpuLog, "Error al enviar a la memoria el path del archivo a modificar\n");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(memoryServerSocket, lineNumber)) < 0)
	{
		log_error(cpuLog, "Error al indicar a la memoria el numero de linea a modificar en el archivo\n");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(memoryServerSocket, dataToAssign)) < 0)
	{
		log_error(cpuLog, "Error al enviar a la memoria el dato a asignar en la linea del archivo especificado\n");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Wait for the memory to confirm the result of the operation before continuing execution
	if((nbytes = receive_int(memoryServerSocket, &message)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "La memoria fue desconectada al intentar recibir el resultado de modificar un archivo en ella\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir el resultado de modificar un archivo de la memoria\n");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara\n");

		log_error(socketErrorLog, "Receive error: %s", strerror(errno));

		exit(EXIT_FAILURE);
	}

	if(message == ARCHIVO_NO_ABIERTO)
	{
		log_info(cpuLog, "Se intento modificar un archivo en memoria que no se encuentra en ella. El proceso sera terminado...\n");

		return handleProcessError();
	}
	else if(message == OK)
	{
		log_info(cpuLog, "Se ha modificado extosamente la linea %d del archivo en la ruta \"%s\"", lineNumber, filePathInFS);
		return INSTRUCTION_EXECUTED;
	}
	else if(message == SEGMENTATION_FAULT)
	{
		log_info(cpuLog, "Ocurrio un error de segmentation fault al intentar modificar el contenido de un archivo en memoria. El proceso sera terminado...\n");

		return handleProcessError();
	}

	return 0;
}

uint32_t handleFlushFile(char* filePathInFS)
{
	int32_t nbytes = 0;

	uint32_t result = tellSchedulerToBlockProcess(true);

	//Tell the DMA to flush the file and send the filePath
	if((nbytes = send_int(dmaServerSocket, FLUSH_FILE)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al planificador que bloquee el proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(dmaServerSocket, (*processInExecutionPCB)->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el PCB del proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(dmaServerSocket, filePathInFS)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el PCB del proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	log_info(cpuLog, "Se solicito al DMA la descarga de la informacion en memoria del archivo del path \"%s\"\n", filePathInFS);
	executionDelay();
	return result;
}

uint32_t handleCloseFile(char* filePathInFS)
{
	int32_t nbytes = 0;
	int32_t message = 0;

	//Tell the scheduler to close a file
	if((nbytes = send_int(schedulerServerSocket, CLOSE_FILE)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al planificador que cierre un archivo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(schedulerServerSocket, (*processInExecutionPCB)->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el id del proceso que solicita cerrar un archivo\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(schedulerServerSocket, filePathInFS)) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el nombre del archivo que debe cerrar\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Tell the memory to delete the file's data from it
	if((nbytes = send_int(memoryServerSocket, CLOSE_FILE)) < 0)
	{
		log_error(cpuLog, "Error al solicitar a la memoria que cierre un archivo\n");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(memoryServerSocket, (*processInExecutionPCB)->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar a la memoria el id del proceso que solicita cerrar un archivo\n");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(memoryServerSocket, filePathInFS)) < 0)
	{
		log_error(cpuLog, "Error al enviar a la memoria el nombre del archivo que debe cerrar\n");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

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

		log_error(socketErrorLog, "Receive error: %s", strerror(errno));

		exit(EXIT_FAILURE);
	}

	if(message == ARCHIVO_NO_ABIERTO)
	{
		log_info(cpuLog, "El archivo \"%s\" que se intento quitar de memoria no se encuentra en ella. El proceso sera terminado...", filePathInFS);
		return handleProcessError();
	}
	else if(message == OK)
	{
		log_info(cpuLog, "Se ha cerrado exitosamente el archivo de la ruta \"%s\"", filePathInFS);
		executionDelay();
		return INSTRUCTION_EXECUTED;
	}

	return 0; //It should never reach this part
}

void updateCurrentPCB()
{
	(*processInExecutionPCB)->totalInstructionsExecuted += instructionsExecuted;
	(*processInExecutionPCB)->instructionsExecutedOnLastExecution = instructionsExecuted;
	(*processInExecutionPCB)->programCounter += instructionsExecuted;
	(*processInExecutionPCB)->remainingQuantum -= instructionsExecuted;

	if(usingCustomSchedulingAlgorithm)
	{
		if((*processInExecutionPCB)->instructionsUntilIoOrEnd < instructionsExecuted)
		{
			//More instructions were executed than the ones left to the next IO which means the process already called an IO instruction
			//Or it may be that the scheduling algorithm used when this process started executing did not require to check the instructions left until IO or end of script
			(*processInExecutionPCB)->instructionsUntilIoOrEnd = 0;
		}
		else
			(*processInExecutionPCB)-> instructionsUntilIoOrEnd -= instructionsExecuted;

	}
}

void updatePCBAndSendExecutionEndMessage(uint32_t messageCode)
{
	int32_t nbytes = 0;
	int32_t message = 0;

	updateCurrentPCB();

	if((nbytes = send_int(schedulerServerSocket, messageCode)) < 0)
	{
		log_error(cpuLog, "Error al indicar al planificador que se termino el quantum de un proceso\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = sendPCB(schedulerServerSocket, (*processInExecutionPCB))) < 0)
	{
		log_error(cpuLog, "Error al enviar al planificador el PCB del proceso en ejecucion\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

}

void removeCommentsFromScript(t_list** script)
{
	char* line = NULL;

	bool _line_is_comment(char* _line)
	{
		return (_line == '#');
	}

	line = list_remove_by_condition((*script), _line_is_comment);

	while(line != NULL)
	{
		free(line);
		line = list_remove_by_condition((*script), _line_is_comment);
	}
}

uint32_t countInstructionsUntilEndOrIO(t_list* parsedScript)
{
	uint32_t linesQty = list_size(parsedScript);
	uint32_t instructionsCount = 0;
	char* line = NULL;
	t_list* parsedLine = NULL;

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
	int32_t nbytes = 0;
	int32_t processId = 0;
	int32_t programCounter = 0;
	char* scriptPath = NULL;
	t_list* parsedScript = NULL;
	char* scriptContent = NULL;
	uint32_t instructionsCounted = 0;
	uint32_t scriptLines = 0;

	//Receive the pid, programCounter and scriptPath of a process to count instructions
	if((nbytes = receive_int(schedulerServerSocket, &processId)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir un pid\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir un pid del planificador\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Receive error: %s", strerror(errno));

		exit(EXIT_FAILURE);
	}
	if((nbytes = receive_int(schedulerServerSocket, &programCounter)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir un program counter\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir un program counter del planificador\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Receive error: %s", strerror(errno));

		exit(EXIT_FAILURE);
	}
	if((nbytes = receive_string(schedulerServerSocket, &scriptPath)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "EL planificador fue desconectado al intentar recibir un path de script\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir un path de script del planificador\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Receive error: %s", strerror(errno));

		exit(EXIT_FAILURE);
	}


	scriptContent = requestScriptFromMemory(&scriptLines);
	parsedScript = parseScript(scriptContent, scriptLines);
	instructionsCounted = countInstructionsUntilEndOrIO(parsedScript);


	//Send the instructions counted to the scheduler
	if((nbytes = send_int(schedulerServerSocket, instructionsCounted)) < 0)
	{
		log_error(cpuLog, "Error al informar al planificador la cantidad de instrucciones que le faltan a un proceso antes de una operacion de IO\n");

		log_info(cpuLog, "Debido a una desconexion del planificador, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
}

char* requestScriptFromMemory(uint32_t* scriptLines)
{
	int32_t nbytes = 0;
	char* scriptContent = NULL;
	int32_t result = 0;
	int32_t _scriptLines = 0;
	int32_t bufferSize = 0;

	//Ask the memory for the script
	if((nbytes = send_int(memoryServerSocket, LEER_ARCHIVO)) < 0)
	{
		log_error(cpuLog, "Error al solicitar a la memoria que envie un script\n");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(memoryServerSocket, (*processInExecutionPCB)->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar a la memoria el id del proceso que solicita un archivo\n");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(memoryServerSocket, (*processInExecutionPCB)->scriptPathInFS)) < 0)
	{
		log_error(cpuLog, "Error al enviar a la memoria el nombre del script requerido\n");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	if((nbytes = receive_int(memoryServerSocket, &result)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "La memoria fue desconectada al intentar recibir el resultado de pedirle un archivo\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir el resultado de pedirle un archivo a la memoria\n");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara\n");
		log_error(socketErrorLog, "Receive error: %s", strerror(errno));

		exit(EXIT_FAILURE);
	}

	if(result != OK)
	{
		log_error(cpuLog, "La memoria devolvio un error (result = %d) al intentar recibir un archivo de ella. El proceso sera abortado", result);
		return NULL;
	}

	if((nbytes = receive_string(memoryServerSocket, &scriptContent)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "La memoria fue desconectada al intentar recibir un archivo\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir un archivo de la memoria\n");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara\n");

		log_error(socketErrorLog, "Receive error: %s", strerror(errno));

		exit(EXIT_FAILURE);
	}
	if((nbytes = receive_int(memoryServerSocket, &_scriptLines)) <= 0)
	{
		if(nbytes == 0)
			log_error(cpuLog, "La memoria fue desconectada al intentar recibir el resultado de pedirle un archivo\n");
		if(nbytes < 0)
			log_error(cpuLog, "Error al intentar recibir el resultado de pedirle un archivo a la memoria\n");

		log_info(cpuLog, "Debido a una desconexion de la memoria, este proceso se cerrara\n");
		log_error(socketErrorLog, "Receive error: %s", strerror(errno));

		exit(EXIT_FAILURE);
	}

	printf("\nscript que llega de memoria: %s\n\n", scriptContent);

	(*scriptLines) = (uint32_t) _scriptLines;

	return scriptContent;
}

void sendOpenFileRequestToDMA(char* filePathInFS, uint32_t messageCode)
{
	int32_t nbytes = 0;

	//Send the task to the DMA, the process ID and the path of the file to open
	if((nbytes = send_int(dmaServerSocket, messageCode)) < 0)
	{
		log_error(cpuLog, "Error al solicitar al DMA que abra un archivo\n");

		log_info(cpuLog, "Debido a una desconexion del DMA, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(dmaServerSocket, (*processInExecutionPCB)->pid)) < 0)
	{
		log_error(cpuLog, "Error al enviar al DMA elid del proceso para el cual debe abrir un archivo\n");

		log_info(cpuLog, "Debido a una desconexion del DMA, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(dmaServerSocket, filePathInFS)) < 0)
	{
		log_error(cpuLog, "Error al enviar al DMA el path del archivo que debe abrir\n");

		log_info(cpuLog, "Debido a una desconexion del DMA, este proceso se cerrara\n");

		log_error(socketErrorLog, "Send error: %s", strerror(errno));

		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	log_info(cpuLog, "Se solicito al DMA la apertura del archivo del path \"%s\", para el proceso %d\n", filePathInFS, (*processInExecutionPCB)->pid);
}

void freePCB(PCB_t* pcb)
{
	free(pcb->scriptPathInFS);
	free(pcb);
}

void executionDelay()
{
	//Sleep for some time (ExecutionDelay)
	log_info(cpuLog, "Retardo de ejecucion...\n");

	double milisecondsSleep = config.executionDelay / 1000;
	sleep(milisecondsSleep);

	log_info(cpuLog, "Se ha finalizado la ejecucion de la instruccion\n");
}

void showPCBInfo(PCB_t* pcb)
{
	printf("\n\n----Info del PCB----\n\n");
	printf("\tpid: %d\n\tscript: %s\n\tprogram counter: %d\n", pcb->pid, pcb->scriptPathInFS, pcb->programCounter);
	printf("\tcpu: %d\n\tquantum: %d\n\n", pcb->cpuProcessIsAssignedTo, pcb->remainingQuantum);
}
