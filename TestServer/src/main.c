#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


void server()
{
	fd_set read_fds; //Temporal FD set used in the select function
	struct sockaddr_in remoteaddr; //Client address
	uint32_t fdmax; //Number of the maximum FD
	uint32_t newfd; //New connection socket
	uint32_t command; //Client command
	uint32_t nbytes;
	uint32_t addrlen;
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
						command_handler(command, _socket);
					}
			} // if (FD_ISSET(i, &read_fds))
	} // while (true)
}
