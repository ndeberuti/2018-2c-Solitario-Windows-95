#include "DMA.h"


void connectToServers()
{
	int32_t nbytes = 0;
	int32_t _lineSize = 0;

	if ((schedulerServerSocket = connect_server(config.schedulerIp, config.schedulerPort, NEW_DMA_CONNECTION, dmaLog)) == 0)
	{
		log_error(dmaLog, "Error al conectar con S-AFA");
		exit(EXIT_FAILURE);
	}

	log_info(dmaLog, "Conexion con S-AFA exitosa");

	if ((fileSystemServerSocket = connect_server(config.fileSystemIp, config.fileSystemPort, NEW_DMA_CONNECTION, dmaLog)) == 0)
	{
		log_error(dmaLog, "Error al conectar con MDJ");
		exit(EXIT_FAILURE);
	}

	log_info(dmaLog, "Conexion con MDJ exitosa");

	if ((memoryServerSocket = connect_server(config.memoryIp, config.memoryPort, NEW_DMA_CONNECTION, dmaLog)) == 0)
	{
		log_error(dmaLog, "Error al conectar con FM9");
		exit(EXIT_FAILURE);
	}

	if((nbytes = receive_int(memoryServerSocket, &_lineSize)) <= 0)
	{
		if(nbytes == 0)
			log_error(dmaLog, "La memoria fue desconectada al intentar recibir su tamaño de linea. El proceso sera abortado...");
		else
			log_error(dmaLog, "Error al recibir el tamaño de linea de la memoria. El proceso sera abortado...");

		exit(EXIT_FAILURE);
	}

	log_info(dmaLog, "Conexion con FM9 exitosa");

	memoryLineSize = _lineSize;
}

void initializeVariables()
{
	dmaLog = init_log("../../Logs/DMA.log", "El Diego", true, LOG_LEVEL_INFO);
	log_info(dmaLog, "Inicio del proceso\n");

	memoryLineSize = 0;

	int32_t result = getConfigs();
	showConfigs();

	if(result < 0)
	{
		if(result == MALLOC_ERROR)
		{
			log_error(dmaLog, "Se aborta el proceso por un error de malloc al intentar obtener las configuraciones...");
		}
		else if (result == CONFIG_PROPERTY_NOT_FOUND)
		{
			log_error(dmaLog, "Se aborta el proceso debido a que no se encontro una propiedad requerida en el archivo de configuracion...");
		}

		log_error(dmaLog, "Ocurrio un error al intentar obtener los datos del archivo de configuracion. El proceso sera abortado...");
		exit(CONFIG_LOAD_ERROR);
	}

	connectToServers();
}

int main(void)
{
	system("clear");
	puts("PROCESO EL DIEGO\n");

	initializeVariables();

	server();

	exit(EXIT_SUCCESS);
}
