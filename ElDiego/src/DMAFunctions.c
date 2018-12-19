#include "DMA.h"

int32_t getConfigs()
{
	char *configurationPath = calloc(22, sizeof(char));

	if(configurationPath == NULL)
		return MALLOC_ERROR;

	strcpy(configurationPath, "../../Configs/DMA.cfg"); //strcpy stops copying characters when it encounters a '\0', memcpy stops when it copies the defined amount

	t_config* configFile = config_create(configurationPath);
	config_t tempConfig;

	if(config_has_property(configFile, "PUERTO"))
	{
		tempConfig.port = config_get_int_value(configFile, "PUERTO");
	}
	else
	{
		log_error(dmaLog, "No se definio ninguna propiedad para el 'Puerto de El Diego' en el archivo de configuracion");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "IP_SAFA"))
	{
		tempConfig.schedulerIp = strdup(config_get_string_value(configFile, "IP_SAFA"));
	}
	else
	{
		log_error(dmaLog, "No se definio ninguna propiedad para la 'IP de SAFA' en el archivo de configuracion");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "PUERTO_SAFA"))
	{
		tempConfig.schedulerPort = config_get_int_value(configFile, "PUERTO_SAFA");
	}
	else
	{
		log_error(dmaLog, "No se definio ninguna propiedad para el 'Puerto de SAFA' en el archivo de configuracion");
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
		log_error(dmaLog, "No se definio ninguna propiedad para la 'IP de FM9' en el archivo de configuracion");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "PUERTO_FM9"))
	{
		tempConfig.memoryPort = config_get_int_value(configFile, "PUERTO_FM9");
	}
	else
	{
		log_error(dmaLog, "No se definio ninguna propiedad para el 'Puerto de FM9' en el archivo de configuracion");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "IP_MDJ"))
	{
		tempConfig.fileSystemIp = strdup(config_get_string_value(configFile, "IP_MDJ"));
	}
	else
	{
		log_error(dmaLog, "No se definio ninguna propiedad para la 'IP de MDJ' en el archivo de configuracion");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "PUERTO_MDJ"))
	{
		tempConfig.fileSystemPort = config_get_int_value(configFile, "PUERTO_MDJ");
	}
	else
	{
		log_error(dmaLog, "No se definio ninguna propiedad para el 'Puerto de MDJ' en el archivo de configuracion");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "TAMAÑO_TRANSFERENCIA"))
	{
		tempConfig.transferSize = config_get_int_value(configFile, "TAMAÑO_TRANSFERENCIA");
	}
	else
	{
		log_error(dmaLog, "No se definio ninguna propiedad para el 'Tamaño de Transferencia' en el archivo de configuracion");
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
	log_info(dmaLog, "---- Configuracion ----");
	log_info(dmaLog, "PUERTO = %d", config.port);
	log_info(dmaLog, "IP_SAFA = %s", config.schedulerIp);
	log_info(dmaLog, "PUERTO_SAFA = %d", config.schedulerPort);
	log_info(dmaLog, "IP_MDJ = %s", config.fileSystemIp);
	log_info(dmaLog, "PUERTO_MDJ = %d", config.fileSystemPort);
	log_info(dmaLog, "IP_FM9 = %s", config.memoryIp);
	log_info(dmaLog, "PUERTO_FM9 = %d", config.memoryPort);
	log_info(dmaLog, "TRANSFER_SIZE = %d", config.transferSize);
	log_info(dmaLog, "-----------------------");
}

bool checkIfFileIsInFS(char* filePath)
{
	int32_t nbytes = 0;
	int32_t isFileInFS = 0;

	//Send task and filePath to the FS
	if((nbytes = send_int(fileSystemServerSocket, VALIDAR_ARCHIVO)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (checkIfFileIsInFS) - Error al indicar al solicitar al FS que valide la existencia de un archivo");

		log_info(dmaLog, "Debido a la desconexion del FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(fileSystemServerSocket, filePath)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (checkIfFileIsInFS) - Error al enviar al FS el path del archivo a validar");

		log_info(dmaLog, "Debido a la desconexion del FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Receive validation result from the FS
	if((nbytes = receive_int(fileSystemServerSocket, &isFileInFS)) <= 0)
	{
		if(nbytes == 0)
			log_error(dmaLog, "DMAFunctions (checkIfFileIsInFS) - El FS fue desconectado al intentar recibir el resultado de la validacion de archivo");
		else
			log_error(dmaLog, "DMAFunctions (checkIfFileIsInFS) - Error al recibir el resultado de la validacion de archivo del FS");

		log_info(dmaLog, "Debido a un error en la comunicacion con el FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	if(isFileInFS == OPERACION_FAIL)	//File does not exist in the FS
		return false;
	else if (isFileInFS == OPERACION_OK)	//File exists in the FS
		return true;
	else
	{
		log_error(dmaLog, "DMAFunctions (checkIfFileIsInFS) - Se recibio una respuesta incorrecta al esperar el resultado de la validacion de un archivo del FS. Este proceso sera abortado");
		exit(EXIT_FAILURE);
	}
}

char* getFileFromFS(char* filePath)
{
	int32_t nbytes = 0;
	int32_t operationResult = 0;
	char* fileContents = NULL;

	//Send task, filePath, offset & size to the FS;
	//offset = 0; size = -1  -> means I want to get the whole file (from offset = 0), but I do not know the file size

	if((nbytes = send_int(fileSystemServerSocket, OBTENER_DATOS)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (getFileFromFS) - Error al solicitar al FS que obtenga datos de un archivo");

		log_info(dmaLog, "Debido a la desconexion del FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(fileSystemServerSocket, filePath)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (getFileFromFS) - Error al enviar al FS el path del archivo del cual obtener datos");

		log_info(dmaLog, "Debido a la desconexion del FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(fileSystemServerSocket, 0)) < 0)	//Offset = 0
	{
		log_error(dmaLog, "DMAFunctions (getFileFromFS) - Error al enviar al FS el offset del archivo del cual se obtienen datos");

		log_info(dmaLog, "Debido a la desconexion del FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(fileSystemServerSocket, UNKNOWN_FILE_SIZE)) < 0)	//Size = -1
	{
		log_error(dmaLog, "DMAFunctions (getFileFromFS) - Error al enviar al FS el tamaño de los datos a obtener de un archivo");

		log_info(dmaLog, "Debido a la desconexion del FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Receive operation result message from the FS
	if((nbytes = receive_int(fileSystemServerSocket, &operationResult)) <= 0)
	{
		if(nbytes == 0)
			log_error(dmaLog, "DMAFunctions (getFileFromFS) - El FS fue desconectado al intentar recibir el resultado de la apertura de un archivo");
		else
			log_error(dmaLog, "DMAFunctions (getFileFromFS) - Error al recibir el resultado de la apertura de un archivo del FS");

		log_info(dmaLog, "Debido a un error en la comunicacion con el FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Recv Error Handling
	}

	if(operationResult == OPERACION_FAIL)	//Error getting data from a file
	{
		return NULL;
	}
	else if (operationResult == OPERACION_OK)	//Data obtained from a file successfully
	{
		//Receive file data from the FS
		if((nbytes = receive_string(fileSystemServerSocket, &fileContents)) <= 0)
		{
			if(nbytes == 0)
				log_error(dmaLog, "DMAFunctions (getFileFromFS) - El FS fue desconectado al intentar recibir el contenido de un archivo");
			else
				log_error(dmaLog, "DMAFunctions (getFileFromFS) - Error al recibir el contenido de un archivo del FS");

			log_info(dmaLog, "Debido a un error en la comunicacion con el FS, este proceso se cerrara");
			exit(EXIT_FAILURE);
			//TODO (Optional) - Recv Error Handling
		}

		return fileContents;
	}
	else
	{
		log_error(dmaLog, "DMAFunctions (getFileFromFS) - Se recibio una respuesta incorrecta al esperar el resultado de la apertura de un archivo del FS. Este proceso sera abortado");
		exit(EXIT_FAILURE);
	}
}

void sendProcessErrorMessageToScheduler(uint32_t process)
{
	int32_t nbytes = 0;

	if((nbytes = send_int(schedulerServerSocket, PROCESS_ERROR)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (sendProcessErrorMessageToScheduler) - Error al indicar al planificador que ocurrio un error");

		log_info(dmaLog, "Debido a la desconexion del planificador, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(schedulerServerSocket, process)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (sendProcessErrorMessageToScheduler) - Error al enviar al planificador el id del proceso con error");

		log_info(dmaLog, "Debido a la desconexion del planificador, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
}

t_list* parseScriptFromFS(char* script)
{
	//Receives the script that the memory sends and transforms it to a list; the line/instruction 1 of the script is the element with index 0 of the list, and so on...

	t_list* parsedScript = NULL;
	parsedScript = string_split_to_list(script, "\n");

	return parsedScript;
}


//TODO - This is ok for segmentation but maybe not for pagination; this function may be needed to be changed to work well with pagination
t_list* parseScriptFromMemory(char* script, uint32_t scriptLines)
{
	char* buffer = NULL;
	uint32_t currentLineOffset = 0;
	uint32_t currentMemoryLineStart = 0;
	uint32_t lineLength = 0;
	t_list* parsedScript = NULL;
	parsedScript = list_create();

	for(uint32_t i = 0; i < scriptLines; i++)
	{
		currentMemoryLineStart = i * memoryLineSize; //The beginning of a memoryLine inside the script received from the memory
		currentLineOffset = currentMemoryLineStart;	 //That memoryLine contains an instruction line ending in '\n' (so I need to read until I find that char)


		//This is to see how many chars after
		while(script[currentLineOffset] != '\n')
			currentLineOffset++;


		lineLength = currentLineOffset - currentMemoryLineStart;  //I need to subtract them because the offset starts with the number of the
																  //currentLine (see above, at the beginning of the 'for' loop!)

		buffer = calloc(1, lineLength);	//This string has the size of the line calculated above
										//(the offset is how many chars after the start of the line there are until a '\n' char)

		//'curentLineOffset - 2' is the char before the '\n' (indexes start at 0, so 'currentLineOffset - 1' is the last character of the buffer)
		//At 'currentLineOffset - 1' there should be a '\0' character (it is a string!) to replace the '\n' char that marked the end of the line
		//(that '\n' is not needed here)
		memcpy(buffer, (script + currentMemoryLineStart), (lineLength - 2));

		list_add(parsedScript, buffer);
	}

	return parsedScript;
}

char* convertParsedFileToMemoryBuffer(t_list* parsedFile, uint32_t* bufferSize)
{
	uint32_t fileLines = list_size(parsedFile);
	size_t _bufferSize = (fileLines * sizeof(char) * memoryLineSize) + 1;	//The '\n' of each line is part of the size of a line
	char* memoryBuffer = calloc(1, _bufferSize);
	char* line = NULL;
	uint32_t lineSize = 0;
	uint32_t bufferOffset = 0;

	(*bufferSize) = _bufferSize;

	if(memoryBuffer == NULL)
	{
		log_error(dmaLog, "DMAFunctions (convertParsedFileToMemoryBuffer) - Error al intentar reservar espacio para el buffer a ser enviado a la memoria. Este proceso sera abortado por falta de espacio en MP");
		exit(EXIT_FAILURE);
	}

	for(uint32_t i = 0; i < fileLines; i++)
	{
		line = (char*) list_get(parsedFile, i);
		lineSize = string_length(line);

		if(lineSize > memoryLineSize)
		{
			return NULL;	//A file line's size cannot be more than a memory line's size (because that line would not fit into the memory) -> A line is like a byte for the memory
		}


		//Copy a file line only if the line is not a null string. If that line is a null string, it was a line (the last line of the script or not)
		//that contained only a '\n' char, and it wat deleted when parsing the file into a list.
		//That line should only contain a '\n' char, followed by as many '\0' chars as needed to complete that memoryLine
		if(string_length > 0)
		{
			memcpy(memoryBuffer + bufferOffset, line, lineSize);
			bufferOffset += memoryLineSize - 1;	//Move from the beginning of the last memoryLine (which contains a fileLine), to the end space for that memoryLine
		}

		//In memoryLineSize -1, which is the last space for a memoryLine, copy a '\n' char to end that line
		memcpy(memoryBuffer + bufferOffset, "\n", 1);
		bufferOffset++;	//Move the offset 1 space to begin copying the next line from that space (outside of the space of the last line)
	}


	return memoryBuffer;
}

bool sendFileToMemory(char* filePath, uint32_t lines, char* buffer, uint32_t bufferSize, uint32_t process)
{
	int32_t nbytes = 0;
	int32_t operationResult = 0;

	if((nbytes = send_int(memoryServerSocket, CARGAR_ARCHIVO)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (sendFileToMemory) - Error al indicar a la memoria que debe cargar un archivo");

		log_info(dmaLog, "Debido a la desconexion de la memoria, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(memoryServerSocket, process)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (sendFileToMemory) - Error al enviar a la memoria el id del proceso para el cual se carga un archivo");

		log_info(dmaLog, "Debido a la desconexion de la memoria, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(memoryServerSocket, filePath)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (sendFileToMemory) - Error al enviar a la memoria el path del archivo a cargar en ella");

		log_info(dmaLog, "Debido a la desconexion de la memoria, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(memoryServerSocket, lines)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (sendFileToMemory) - Error al enviar a la memoria la cantidad de lineas del archivo a cargar en ella");

		log_info(dmaLog, "Debido a la desconexion de la memoria, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send(memoryServerSocket, buffer, bufferSize, MSG_WAITALL)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (sendFileToMemory) - Error al enviar a la memoria el contenido del archivo a cargar en ella");

		log_info(dmaLog, "Debido a la desconexion de la memoria, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Receive operation result message from the memory
	if((nbytes = receive_int(memoryServerSocket, &operationResult)) <= 0)
	{
		if(nbytes == 0)
			log_error(dmaLog, "DMAFunctions (sendFileToMemory) - La memoria fue desconectada al intentar recibir el resultado de cargar los datos de un archivo en ella");
		else
			log_error(dmaLog, "DMAFunctions (sendFileToMemory) - Error al intentar recibir el resultado de cargar los datos de un archivo en la memoria");

		log_info(dmaLog, "Debido a un error en la comunicacion con el FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Recv Error Handling
	}

	if(operationResult == ESPACIO_INSUFICIENTE)	//Error putting file data into memory
	{
		return false;
	}
	else if (operationResult != OK)
	{
		log_error(dmaLog, "DMAFunctions (sendFileToMemory) - Se recibio una respuesta incorrecta al esperar el resultado del guardado de datos de un archivo en la memoria. Este proceso sera abortado");
		exit(EXIT_FAILURE);
	}

	return true;
}

char* convertParsedFileToFileSystemBuffer(t_list* parsedFile)
{
	//As each string in the list is a duplicate of the original string, and "strdup" supposedly copies characters from a string
	//until it reaches a '\0' character, there is no need to parse each string from the list to remove the additional '\0' characters

	//Because I do not know the size of all the lines of the list combined into a string, and to avoid calling "realloc" for each line
	//of the list, I will allocate a large memory address -> lines * memoryLineSize

	uint32_t fileLines = list_size(parsedFile);
	size_t _bufferSize = (fileLines * sizeof(char) * memoryLineSize) + 1; //The '\n' of each line is part of the line size
	char* filesystemBuffer = calloc(1, _bufferSize);
	char* line = NULL;
	uint32_t lineSize = 0;
	uint32_t bufferOffset = 0;
	uint32_t fileSize = 0;


	if(filesystemBuffer == NULL)
	{
		log_error(dmaLog, "DMAFunctions (convertParsedFileToFileSystemBuffer) - Error al intentar reservar espacio para el buffer a ser enviado al FS. Este proceso sera abortado por falta de espacio en MP");
		exit(EXIT_FAILURE);
	}

	for(uint32_t i = 0; i < fileLines; i++)
	{
		line = (char*) list_get(parsedFile, i);
		lineSize = string_length(line);

		memcpy(filesystemBuffer + bufferOffset, line, lineSize);
		bufferOffset += lineSize;	//Move from the beginning of the last line copied to the buffer, to the end of that line, if the line were a memory line (may leave a trail of empty chars)

		memcpy(filesystemBuffer + bufferOffset, "\n", 1);	//Adds a '\n' char after each line
		bufferOffset++;
	}

	fileSize = string_length(filesystemBuffer);
	realloc(filesystemBuffer, fileSize + 1);

	return filesystemBuffer;
}

char* getFileFromMemory(char* filePath, uint32_t* scriptLines)
{
	char* fileContents = NULL;
	int32_t nbytes = 0;
	int32_t operationResult = 0;
	uint32_t _scriptLines = 0;

	//Send task and filePath to the memory
	if((nbytes = send_int(memoryServerSocket, FLUSH)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (getFileFromMemory) - Error al solicitar a la memoria que obtenga datos de un archivo");

		log_info(dmaLog, "Debido a la desconexion de la memoria, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(memoryServerSocket, filePath)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (getFileFromMemory) - Error al enviar a la memoria el path del archivo del cual obtener datos");

		log_info(dmaLog, "Debido a la desconexion de la memoria, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Receive operation result message from the memory
	if((nbytes = receive_int(memoryServerSocket, &operationResult)) <= 0)
	{
		if(nbytes == 0)
			log_error(dmaLog, "DMAFunctions (getFileFromMemory) - La memoria fue desconectada al intentar recibir el resultado de obtener los datos de un archivo de ella");
		else
			log_error(dmaLog, "DMAFunctions (getFileFromMemory) - Error al recibir el resultado de obtener los datos de un archivo contenido en la memoria");

		log_info(dmaLog, "Debido a un error en la comunicacion con la memoria, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Recv Error Handling
	}

	if(operationResult == ARCHIVO_NO_ABIERTO)	//Error getting data from a file
	{
		return NULL;
	}
	else if (operationResult == OK)	//Data obtained from a file successfully
	{
		//Receive file data from the memory
		if((nbytes = receive_string(fileSystemServerSocket, &fileContents)) <= 0)
		{
			if(nbytes == 0)
				log_error(dmaLog, "DMAFunctions (getFileFromMemory) - La memoria fue desconectada al intentar recibir el contenido de un archivo");
			else
				log_error(dmaLog, "DMAFunctions (getFileFromMemory) - Error al recibir el contenido de un archivo de la memoria");

			log_info(dmaLog, "Debido a un error en la comunicacion con la memoria, este proceso se cerrara");
			exit(EXIT_FAILURE);
			//TODO (Optional) - Recv Error Handling
		}
		if((nbytes = receive_int(memoryServerSocket, &_scriptLines)) <= 0)
		{
			if(nbytes == 0)
				log_error(dmaLog, "DMAFunctions (getFileFromMemory) - La memoria fue desconectada al intentar recibir la cantidad de lineas del archivo recibido");
			else
				log_error(dmaLog, "DMAFunctions (getFileFromMemory) - Error al recibir la cantidad de lineas del archivo recibido de memoria");

			log_info(dmaLog, "Debido a un error en la comunicacion con la memoria, este proceso se cerrara");
			exit(EXIT_FAILURE);
			//TODO (Optional) - Recv Error Handling
		}

		return fileContents;
	}
	else
	{
		log_error(dmaLog, "DMAFunctions (getFileFromMemory) - Se recibio una respuesta incorrecta al esperar el resultado de la obtencion de datos de un archivo de la memoria. Este proceso sera abortado");
		exit(EXIT_FAILURE);
	}
}

int32_t sendFileToFileSystem(char* filePath, char* dataToReplace)
{
	int32_t nbytes = 0;
	int32_t operationResult = 0;

	//Send the task, filePath, offset, size and data of the file which needs to be flushed
	//Offset = 0; Size = 0 -> This means that the file is replaced from the beginning (offset = 0) and all data is overwritten (size = 0)
	if((nbytes = send_int(fileSystemServerSocket, GUARDAR_DATOS)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (sendFileToFileSystem) - Error al solicitar al FS que guarde los datos de un archivo");

		log_info(dmaLog, "Debido a la desconexion del FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(fileSystemServerSocket, filePath)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (sendFileToFileSystem) - Error al enviar al FS el path del archivo para el cual se deben guardar datos");

		log_info(dmaLog, "Debido a la desconexion del FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(fileSystemServerSocket, 0)) < 0)	//Offset = 0
	{
		log_error(dmaLog, "DMAFunctions (sendFileToFileSystem) - Error al enviar al FS el offset del archivo del cual se obtienen datos");

		log_info(dmaLog, "Debido a la desconexion del FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(fileSystemServerSocket, 0)) < 0)	//Size = 0
	{
		log_error(dmaLog, "DMAFunctions (sendFileToFileSystem) - Error al enviar al FS el tamaño de los datos a obtener de un archivo");

		log_info(dmaLog, "Debido a la desconexion del FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(fileSystemServerSocket, dataToReplace)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (sendFileToFileSystem) - Error al enviar al FS el contenido del archivo para el cual se deben guardar datos");

		log_info(dmaLog, "Debido a la desconexion del FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Receive operation result message from the FS
	if((nbytes = receive_int(fileSystemServerSocket, &operationResult)) <= 0)
	{
		if(nbytes == 0)
			log_error(dmaLog, "DMAFunctions (sendFileToFileSystem) - El FS fue desconectado al intentar recibir el resultado del guardado de un archivo");
		else
			log_error(dmaLog, "DMAFunctions (sendFileToFileSystem) - Error al recibir el resultado del guardado de un archivo del FS");

		log_info(dmaLog, "Debido a un error en la comunicacion con el FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Recv Error Handling
	}

	if ((operationResult != OPERACION_OK) || (operationResult != OPERACION_FAIL) || (operationResult != BLOQUES_INSUFICIENTES))
	{
		log_error(dmaLog, "DMAFunctions (sendFileToFileSystem) - Se recibio una respuesta incorrecta al esperar el resultado del guardado de un archivo en el FS. Este proceso sera abortado");
		exit(EXIT_FAILURE);
	}

	return operationResult;
}

void tellSchedulerToUnblockProcess(uint32_t process, char* filePath, bool fileIsScript)
{
	int32_t nbytes = 0;

	if((nbytes = send_int(schedulerServerSocket, UNLOCK_PROCESS)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (tellSchedulerToUnblockProcess) - Error al indicar al planificador que desbloquee un proceso");

		log_info(dmaLog, "Debido a la desconexion del planificador, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(schedulerServerSocket, process)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (tellSchedulerToUnblockProcess) - Error al enviar al planificador pid del proceso a desbloquear");

		log_info(dmaLog, "Debido a la desconexion del planificador, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(schedulerServerSocket, filePath)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (tellSchedulerToUnblockProcess) - Error al enviar al planificador el path del archivo solicitado por el proceso");

		log_info(dmaLog, "Debido a la desconexion del planificador, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(schedulerServerSocket, fileIsScript)) < 0)
	{
		log_error(dmaLog, "DMAFunctions (tellSchedulerToUnblockProcess) - Error al indicar al planificador si el archivo abierto es un script");

		log_info(dmaLog, "Debido a la desconexion del planificador, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
}
