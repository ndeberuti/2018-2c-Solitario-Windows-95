#include "DMA.h"


void server()
{
	fd_set read_fds; //Temporal FD set used in the select function
	struct sockaddr_in remoteaddr; //Client address
	uint32_t fdmax = 0; //Number of the maximum FD
	uint32_t newfd = 0; //New connection socket
	int32_t command = 0; //Client command
	uint32_t nbytes = 0;
	uint32_t addrlen = 0;
	FD_ZERO(&master); //Set to zero all sockets in the master and read FD sets
	FD_ZERO(&read_fds);

	//Get listener socket
	uint32_t servidor = build_server(config.port, dmaLog);

	//Add listener to the master FD set
	FD_SET(servidor, &master);
	//Keep track of the maximum GD
	fdmax = servidor;

	//Main loop
	while (true)
	{
		read_fds = master;
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
		{
			log_error(dmaLog, "Error de select!. El proceso sera abortado...");
			exit(EXIT_FAILURE);
		}
		//Check existing connections and wait for data to read fom any of them
		for (uint32_t _socket = 0; _socket <= fdmax; _socket++)
			if (FD_ISSET(_socket, &read_fds))
			{ //Data received
				if (_socket == servidor)
				{
					//Manage new connections
					addrlen = sizeof(remoteaddr);
					if ((newfd = accept(servidor, (struct sockaddr *) &remoteaddr, &addrlen)) == -1)
						log_error(dmaLog, "Error de accept!");
					else
					{
						FD_SET(newfd, &master); //Add the FD to the master set
						if (newfd > fdmax) //Update the maximum fd
						{
							fdmax = newfd;
						}
					}
				}
				else
					//Manage client data
					if ((nbytes = receive_int(_socket, &command)) <= 0)
					{
						//An error occurred or a connection was closed
						if (nbytes == 0)
						{
							if(_socket == schedulerServerSocket)
							{
								log_error(dmaLog, "El planificador fue desconectado. Se abortara el proceso...");
								exit(EXIT_FAILURE);
							}
							else if(_socket == fileSystemServerSocket)
							{
								log_error(dmaLog, "El file system fue desconectado. Se abortara el proceso...");
								exit(EXIT_FAILURE);
							}
							else if(_socket == memoryServerSocket)
							{
								log_error(dmaLog, "La memoria fue desconectada. Se abortara el proceso...");
								exit(EXIT_FAILURE);
							}
							else
								log_error(dmaLog, "Se desconecto una CPU al intentar recibir un tarea...");
						}
						else
							log_error(dmaLog, "Error en recv (comando)");

						close(_socket);
						FD_CLR(_socket, &master); //Delete the socket from the master FdSet
					}
					else
					{
						//Receive data from a client
						commandHandler(command, _socket);
					}
			} // if (FD_ISSET(i, &read_fds))
	} // while (true)
}

void commandHandler(uint32_t command, uint32_t _socket)
{
	switch (command)
	{
		case NEW_CPU_CONNECTION:
			log_info(dmaLog, "Nueva conexion de CPU\n");
		break;

		case OPEN_FILE:
			openFile(_socket, true);
		break;

		case OPEN_SCRIPT:
			openFile(_socket, false);
		break;

		case FLUSH_FILE:
			flushFile(_socket);
		break;

		case CREATE_FILE:
			createFile(_socket);
		break;

		case DELETE_FILE:
			deleteFile(_socket);
		break;

		default:
			log_warning(dmaLog, "%d: Comando recibido incorrecto", command);
		break;
	}
}


void openFile(uint32_t _socket, bool addFileToFileTable)
{
	int32_t nbytes = 0;
	int32_t currentProcess = 0;
	char* currentFilePath = NULL;		//Path of the file to open
	bool fileIsInFS = false;
	bool couldSendDataToMemory = false;

	if((nbytes = receive_int(_socket, &currentProcess)) <= 0)
	{
		if(nbytes == 0)
			log_error(dmaLog, "ServerThread (openFile) - La CPU fue desconectada al intentar recibir un pid\n");
		if(nbytes < 0)
			log_error(dmaLog, "ServerThread (openFile) - Error al intentar recibir un pid de la CPU\n");

		close(_socket);
		FD_CLR(_socket, &master); //Delete the socket from the master FdSet
		return;
	}
	if((nbytes = receive_string(_socket, &currentFilePath)) <= 0)
	{
		if(nbytes == 0)
			log_error(dmaLog, "ServerThread (openFile) - La CPU fue desconectada al intentar recibir un path de script\n");
		if(nbytes < 0)
			log_error(dmaLog, "ServerThread (openFile) - Error al intentar recibir un path de script de la CPU\n");

		close(_socket);
		FD_CLR(_socket, &master); //Delete the socket from the master FdSet
		free(currentFilePath);
		return;
	}

	fileIsInFS = checkIfFileIsInFS(currentFilePath);

	if(fileIsInFS)
	{
		t_list* parsedFile = NULL;
		uint32_t fileLines = 0;
		uint32_t bufferSize = 0;
		char* fileContents = getFileFromFS(currentFilePath);
		char* fileBuffer = NULL; //Contains a buffer with all the lines of a file, each line contained in a sub-buffer that has the size of a memory line;
								 //each line is separated by a '\n' character

		if(fileContents == NULL)	//There was an error
		{
			sendProcessErrorMessageToScheduler(currentProcess);
			free(currentFilePath);
			return;
		}

		log_info(dmaLog, "Eeeeeeh voy a abrir laaaaa eeeel archivo \"%s\" para mmmmm eeeel proceso %d", currentFilePath, currentProcess);

		parsedFile = parseScriptFromFS(fileContents);	//Creates a list, with each element containing a line of the file (in order of appearance)
 		free(fileContents);

		fileLines = list_size(parsedFile);
		fileBuffer = convertParsedFileToMemoryBuffer(parsedFile, &bufferSize);

		if(fileBuffer == NULL)	//Error, a file line cannot fit into a memory line -> segmentation fault
		{
			sendProcessErrorMessageToScheduler(currentProcess);
			log_error(dmaLog, "Se intento abrir el archivo \"%s\", pero este contiene lineas de tamaño mayor a una linea de memoria");

			free(currentFilePath);
			list_destroy_and_destroy_elements(parsedFile, free);
			free(fileBuffer);

			return;
		}

		couldSendDataToMemory = sendFileToMemory(currentFilePath, fileLines, fileBuffer, bufferSize, currentProcess);

		if(couldSendDataToMemory)
		{
			log_info(dmaLog, "El archivo \"%s\" fue abierto para el proceso %d de forma exitosa", currentFilePath, currentProcess);

			tellSchedulerToUnblockProcess(currentProcess, currentFilePath, addFileToFileTable, "abrir");
		}
		else
		{
			sendProcessErrorMessageToScheduler(currentProcess);
			log_error(dmaLog, "Se intento guardar el contenido del archivo \"%s\" en memoria, para el proceso %d, pero la memoria no tiene espacio suficiente", currentFilePath, currentProcess);
		}

		list_destroy_and_destroy_elements(parsedFile, free);
		free(fileBuffer);
	}
	else
	{
		sendProcessErrorMessageToScheduler(currentProcess);	//File is not in the FS
		log_error(dmaLog, "Se intento abrir el archivo \"%s\", pero el mismo no se encuentra en la ruta especificada o no existe", currentFilePath);
	}

	free(currentFilePath);
}

void flushFile(uint32_t _socket)
{
	int32_t nbytes = 0;
	int32_t currentProcess = 0;
	char* currentFilePath = NULL;		//Path of the file to open
	bool fileIsInFS = false;

	if((nbytes = receive_int(_socket, &currentProcess)) <= 0)
	{
		if(nbytes == 0)
			log_error(dmaLog, "ServerThread (flushFile) - La CPU fue desconectada al intentar recibir un pid\n");
		if(nbytes < 0)
			log_error(dmaLog, "ServerThread (flushFile) - Error al intentar recibir un pid de la CPU\n");

		close(_socket);
		FD_CLR(_socket, &master); //Delete the socket from the master FdSet
		return;
	}
	if((nbytes = receive_string(_socket, &currentFilePath)) <= 0)
	{
		if(nbytes == 0)
			log_error(dmaLog, "ServerThread (flushFile) - La CPU fue desconectada al intentar recibir un path de script\n");
		if(nbytes < 0)
			log_error(dmaLog, "ServerThread (flushFile) - Error al intentar recibir un path de script de la CPU\n");

		close(_socket);
		FD_CLR(_socket, &master); //Delete the socket from the master FdSet
		free(currentFilePath);
		return;
	}

	fileIsInFS = checkIfFileIsInFS(currentFilePath);

	if(fileIsInFS)
	{
		t_list* parsedFile = NULL;
		uint32_t scriptLines = 0;
		char* fileContents = getFileFromMemory(currentFilePath, &scriptLines);
		char* fileBuffer = NULL; //Contains a buffer with all the lines of a file, each line contained in a sub-buffer that has the size of a memory line;
								 //each line is separated by a '\n' character

		if(fileContents == NULL)	//There was an error getting the file from the memory (the file is not in the memory)
		{
			sendProcessErrorMessageToScheduler(currentProcess);
			log_error(dmaLog, "Se intentaron obtener los datos del archivo del path \"%s\", el cual deberia encontrarse en memoria, pero no fue hallado en ella", currentFilePath);

			free(currentFilePath);
			return;
		}

		log_info(dmaLog, "Se guardara el archivo \"%s\" en el FS para el proceso %d", currentFilePath, currentProcess);

		parsedFile = parseScriptFromMemory(fileContents, scriptLines);	//Creates a list, with each element containing a line of the file (in order of appearance)
		free(fileContents);

		fileBuffer = convertParsedFileToFileSystemBuffer(parsedFile);

		int32_t operationResult = sendFileToFileSystem(currentFilePath, fileBuffer);

		if(operationResult == OPERACION_FAIL)
		{
			sendProcessErrorMessageToScheduler(currentProcess);
			log_error(dmaLog, "Se intentaron guardar los datos del archivo del path \"%s\", pero ocurrio un error al realizar la operacion", currentFilePath);

			free(currentFilePath);
			list_destroy_and_destroy_elements(parsedFile, free);
			free(fileBuffer);

			return;
		}
		else if(operationResult == BLOQUES_INSUFICIENTES)
		{
			sendProcessErrorMessageToScheduler(currentProcess);
			log_error(dmaLog, "Se intentaron guardar los datos del archivo del path \"%s\", pero el FS no posee bloques suficientes para guardar los cambios del archivo", currentFilePath);

			free(currentFilePath);
			list_destroy_and_destroy_elements(parsedFile, free);
			free(fileBuffer);

			return;
		}

		log_info(dmaLog, "El archivo \"%s\" fue guardado en el FS para el proceso %d de forma exitosa", currentFilePath, currentProcess);

		tellSchedulerToUnblockProcess(currentProcess, currentFilePath, false, "guardar");

		list_destroy_and_destroy_elements(parsedFile, free);
		free(fileBuffer);
	}
	else
	{
		sendProcessErrorMessageToScheduler(currentProcess);	//File is not in the FS
		log_error(dmaLog, "Se intento guardar el archivo \"%s\" en el FS, pero el mismo no se encuentra en la ruta especificada o no existe", currentFilePath);
	}

	free(currentFilePath);
}

void createFile(uint32_t _socket)
{
	//Because I do not know the size the lines of the file will have, I suppose the file's max size will be -> lines * memoryLineSize

	int32_t nbytes = 0;
	int32_t operationResult = 0;
	int32_t currentProcess = 0;
	char* currentFilePath = NULL;		//Path of the file to open
	int32_t linesInFileToCreate = 0;
	uint32_t fileSize = 0;

	//Receive from the CPU the pid of the requesting process, the path where the file will be created, and the number of lines the file should have
	if((nbytes = receive_int(_socket, &currentProcess)) <= 0)
	{
		if(nbytes == 0)
			log_error(dmaLog, "ServerThread (createFile) - La CPU fue desconectada al intentar recibir un pid\n");
		if(nbytes < 0)
			log_error(dmaLog, "ServerThread (createFile) - Error al intentar recibir un pid de la CPU\n");

		close(_socket);
		FD_CLR(_socket, &master); //Delete the socket from the master FdSet
		return;
	}
	if((nbytes = receive_string(_socket, &currentFilePath)) <= 0)
	{
		if(nbytes == 0)
			log_error(dmaLog, "ServerThread (createFile) - La CPU fue desconectada al intentar recibir el path del archivo a crear\n");
		if(nbytes < 0)
			log_error(dmaLog, "ServerThread (createFile) - Error al intentar recibir el path del archivo a crear de la CPU\n");

		close(_socket);
		FD_CLR(_socket, &master); //Delete the socket from the master FdSet
		return;
	}
	if((nbytes = receive_int(_socket, &linesInFileToCreate)) <= 0)
	{
		if(nbytes == 0)
			log_error(dmaLog, "ServerThread (createFile) - La CPU fue desconectada al intentar recibir la cantidad de lineas del archivo a crear\n");
		if(nbytes < 0)
			log_error(dmaLog, "ServerThread (createFile) - Error al intentar recibir la cantidad de lineas del archivo a crear de la CPU\n");

		close(_socket);
		FD_CLR(_socket, &master); //Delete the socket from the master FdSet
		return;
	}

	//The file of the size will be the maximum size of each line (defined by the memoryLineSize) plus a char for
	//each line (to put the '\n' character at the end of each line)
	fileSize = (linesInFileToCreate * memoryLineSize * sizeof(char));


	//Send the task, the filePath and the size (in bytes) of the file to create to the FS
	if((nbytes = send_int(fileSystemServerSocket, CREAR_ARCHIVO)) < 0)
	{
		log_error(dmaLog, "ServerThread (createFile) - Error al solicitar al FS que cree un archivo");

		log_info(dmaLog, "Debido a la desconexion del FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(fileSystemServerSocket, currentFilePath)) < 0)
	{
		log_error(dmaLog, "ServerThread (createFile) - Error al enviar al FS el path del archivo que debe ser creado");

		log_info(dmaLog, "Debido a la desconexion del FS, este proceso se cerrara");
		free(currentFilePath);
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(fileSystemServerSocket, fileSize)) < 0)
	{
		log_error(dmaLog, "ServerThread (createFile) - Error al enviar al FS el tamaño del archivo que debe ser creado");

		log_info(dmaLog, "Debido a la desconexion del FS, este proceso se cerrara");
		free(currentFilePath);
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Receive operation result message from the FS
	if((nbytes = receive_int(fileSystemServerSocket, &operationResult)) <= 0)
	{
		if(nbytes == 0)
			log_error(dmaLog, "ServerThread (createFile) - El FS fue desconectado al intentar recibir el resultado de la creacion de un archivo");
		else
			log_error(dmaLog, "ServerThread (createFile) - Error al recibir el resultado de la creacion de un archivo del FS");

		log_info(dmaLog, "Debido a un error en la comunicacion con el FS, este proceso se cerrara");
		free(currentFilePath);
		exit(EXIT_FAILURE);
		//TODO (Optional) - Recv Error Handling
	}

	if(operationResult == OPERACION_OK)
	{
		log_info(dmaLog, "Se creo el archivo \"%s\" para el proceso %d de forma exitosa", currentFilePath, currentProcess);
		tellSchedulerToUnblockProcess(currentProcess, currentFilePath, false, "crear");
	}
	else if(operationResult == BLOQUES_INSUFICIENTES)
	{
		log_info(dmaLog, "Se intento crear el archivo \"%s\" para el proceso %d, pero no existen bloques suficientes en el FS", currentFilePath, currentProcess);
		sendProcessErrorMessageToScheduler(currentProcess);
	}
	else
	{
		log_error(dmaLog, "ServerThread (createFile) - Se recibio una respuesta incorrecta al esperar el resultado de la creacion de un archivo en el FS. Este proceso sera abortado");
		free(currentFilePath);
		exit(EXIT_FAILURE);
	}

	free(currentFilePath);
}

void deleteFile(uint32_t _socket)
{
	int32_t nbytes = 0;
	int32_t operationResult = 0;
	int32_t currentProcess = 0;
	char* currentFilePath = NULL;		//Path of the file to open

	//Receive from the CPU the pid of the requesting process, the path where the file will be created, and the number of lines the file should have
	if((nbytes = receive_int(_socket, &currentProcess)) <= 0)
	{
		if(nbytes == 0)
			log_error(dmaLog, "ServerThread (deleteFile) - La CPU fue desconectada al intentar recibir un pid\n");
		if(nbytes < 0)
			log_error(dmaLog, "ServerThread (deleteFile) - Error al intentar recibir un pid de la CPU\n");

		close(_socket);
		FD_CLR(_socket, &master); //Delete the socket from the master FdSet
		return;
	}
	if((nbytes = receive_string(_socket, &currentFilePath)) <= 0)
	{
		if(nbytes == 0)
			log_error(dmaLog, "ServerThread (deleteFile) - La CPU fue desconectada al intentar recibir el path del archivo a eliminar\n");
		if(nbytes < 0)
			log_error(dmaLog, "ServerThread (deleteFile) - Error al intentar recibir el path del archivo a eliminar de la CPU\n");

		close(_socket);
		FD_CLR(_socket, &master); //Delete the socket from the master FdSet
		return;
	}

	//Send the task and the filePath of the file to delete to the FS
	if((nbytes = send_int(fileSystemServerSocket, BORRAR_ARCHIVO)) < 0)
	{
		log_error(dmaLog, "ServerThread (deleteFile) - Error al solicitar al FS que elimine un archivo");

		log_info(dmaLog, "Debido a la desconexion del FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(fileSystemServerSocket, currentFilePath)) < 0)
	{
		log_error(dmaLog, "ServerThread (deleteFile) - Error al enviar al FS el path del archivo que debe ser eliminado");

		log_info(dmaLog, "Debido a la desconexion del FS, este proceso se cerrara");
		exit(EXIT_FAILURE);
		//TODO (Optional) - Send Error Handling
	}

	//Receive operation result message from the FS
	if((nbytes = receive_int(fileSystemServerSocket, &operationResult)) <= 0)
	{
		if(nbytes == 0)
			log_error(dmaLog, "ServerThread (deleteFile) - El FS fue desconectado al intentar recibir el resultado de la eliminacion de un archivo");
		else
			log_error(dmaLog, "ServerThread (deleteFile) - Error al recibir el resultado de la eliminacion de un archivo del FS");

		log_info(dmaLog, "Debido a un error en la comunicacion con el FS, este proceso se cerrara");
		free(currentFilePath);
		exit(EXIT_FAILURE);
		//TODO (Optional) - Recv Error Handling
	}

	if(operationResult == OPERACION_OK)
	{
		log_info(dmaLog, "Se elimino el archivo \"%s\" para el proceso %d de forma exitosa", currentFilePath, currentProcess);
		tellSchedulerToUnblockProcess(currentProcess, currentFilePath, false, "eliminar");
		free(currentFilePath);
	}
	else
	{
		log_error(dmaLog, "ServerThread (deleteFile) - Se recibio una respuesta incorrecta al esperar el resultado de la eliminacion de un archivo en el FS. Este proceso sera abortado");
		free(currentFilePath);
		exit(EXIT_FAILURE);
	}
}

