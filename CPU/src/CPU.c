/*
 * CPU.c
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */
#include "CPU.h"

int main(void) {
	system("clear");
	puts("PROCESO CPU\n");

	log_cpu = init_log(PATH_LOG, "Proceso CPU", true, LOG_LEVEL_INFO);
	log_info(log_cpu, "Inicio del proceso");

	config = load_config();

	safa = connect_server(config.IP_SAFA, config.PUERTO_SAFA, log_cpu);
	diego = connect_server(config.IP_DIEGO, config.PUERTO_DIEGO, log_cpu);
	fm9 = connect_server(config.IP_FM9, config.PUERTO_FM9, log_cpu);

	exit(EXIT_SUCCESS);
}

config_t load_config() {
	t_config *config = config_create(PATH_CONFIG);

	config_t miConfig;
	miConfig.IP_SAFA = strdup(config_get_string_value(config, "IP_SAFA"));
	miConfig.PUERTO_SAFA = config_get_int_value(config, "PUERTO_SAFA");
	miConfig.IP_DIEGO = strdup(config_get_string_value(config, "IP_DIEGO"));
	miConfig.PUERTO_DIEGO = config_get_int_value(config, "PUERTO_DIEGO");
	miConfig.IP_FM9 = strdup(config_get_string_value(config, "IP_FM9"));
	miConfig.PUERTO_FM9 = config_get_int_value(config, "PUERTO_FM9");
	miConfig.RETARDO = config_get_int_value(config, "RETARDO");

	log_info(log_cpu, "---- Configuracion ----");
	log_info(log_cpu, "IP_SAFA = %s", miConfig.IP_SAFA);
	log_info(log_cpu, "PUERTO_SAFA = %d", miConfig.PUERTO_SAFA);
	log_info(log_cpu, "IP_DIEGO = %s", miConfig.IP_DIEGO);
	log_info(log_cpu, "PUERTO_DIEGO = %d", miConfig.PUERTO_DIEGO);
	log_info(log_cpu, "IP_FM9 = %s", miConfig.IP_FM9);
	log_info(log_cpu, "PUERTO_FM9 = %d", miConfig.PUERTO_FM9);
	log_info(log_cpu, "RETARDO = %d milisegundos", miConfig.RETARDO);
	log_info(log_cpu, "-----------------------");

	config_destroy(config);
	return miConfig;
}
