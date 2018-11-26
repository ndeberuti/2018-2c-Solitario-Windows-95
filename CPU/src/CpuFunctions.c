#include "CPU.h"


int32_t getConfigs()
{
	char *configurationPath = calloc(35, sizeof(char));

	if(configurationPath == NULL)
		return MALLOC_ERROR;

	strcpy(configurationPath, "../../Configs/CPU.cfg"); //strcpy stops copying characters when it encounters a '\0', memcpy stops when it copies the defined amount

	t_config* configFile = config_create(configurationPath);
	config_t tempConfig;

	if(config_has_property(configFile, "IP_SAFA"))
	{
		tempConfig.schedulerIp = config_get_string_value(configFile, "IP_SAFA");
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
		tempConfig.schedulerIp = config_get_string_value(configFile, "IP_FM9");
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
		tempConfig.schedulerPort = config_get_int_value(configFile, "Puerto_FM9");
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
		tempConfig.schedulerIp = config_get_string_value(configFile, "IP_Diego");
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
		tempConfig.schedulerPort = config_get_int_value(configFile, "Puerto_Diego");
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
		tempConfig.schedulerIp = config_get_string_value(configFile, "IP_CPU");
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
		tempConfig.schedulerIp = config_get_string_value(configFile, "Puerto_CPU");
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
		tempConfig.executionDelay = config_get_string_value(configFile, "RetardoEjecucion");
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

int32_t handshakeProcess(uint32_t socket)
{
	int32_t nbytes;
	uint32_t messageReceived;

	if((nbytes = send_int(socket, NEW_CPU_CONNECTION)) < 0)
	{
		log_error(cpuLog, "Error al indicar al planificador que se esta conectando una CPU\n");
		return nbytes;
		//TODO (Optional) - Send Error Handling
	}

	if((nbytes = receive_int(socket, &messageReceived)) <= 0)
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

	log_info(cpuLog, "El planificador acepto la conexion de la CPU correctamente\n");

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

	if((nbytes = receive_int(socket, &messageReceived)) <= 0)
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
		if((result = handshakeProcess(socket)) < 0)		//Error receiving handshake messages (for example, a recv call took too long to receive messages)
		{
			log_error(cpuLog, "Error al conectar con S-AFA");
			exit(EXIT_FAILURE);
		}

		schedulerServerSocket = socket;

		log_info(cpuLog, "Conexion con S-AFA exitosa");
	}

	if ((socket = connect_server(config.dmaIp, config.dmaPort, -1, cpuLog)) == 0)
	{
		log_error(cpuLog, "Error al conectar con El Diego");
		exit(EXIT_FAILURE);
	}
	else
	{
		if((result = handshakeProcess(socket)) < 0)		//Error receiving handshake messages (for example, a recv call took too long to receive messages)
		{
			log_error(cpuLog, "Error al conectar con El Diego");
			exit(EXIT_FAILURE);
		}

		dmaServerSocket = socket;

		log_info(cpuLog, "Conexion con El Diego exitosa");
	}

	if ((socket = connect_server(config.memoryIp, config.memoryPort, -1, cpuLog)) == 0)
	{
		log_error(cpuLog, "Error al conectar con FM9");
		exit(EXIT_FAILURE);
	}
	else
	{
		if((result = handshakeProcess(socket)) < 0)		//Error receiving handshake messages (for example, a recv call took too long to receive messages)
		{
			log_error(cpuLog, "Error al conectar con FM9");
			exit(EXIT_FAILURE);
		}

			memoryServerSocket = socket;

			log_info(cpuLog, "Conexion con FM9 exitosa");
		}
}
