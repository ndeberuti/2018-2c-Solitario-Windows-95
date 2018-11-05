#include "Scheduler.h"


void console()
{
	char* line;
	char* token;
	console_t* console;

	while (true)
	{
		line = readline("S-AFA> ");

		if (strlen(line) > 0)
		{
			add_history(line);
			log_info(consoleLog, "Linea leida: %s", line);
			console = malloc(sizeof(console_t));

			if (console != NULL)
			{
				console->command = strdup(strtok(line, " "));
				console->paramsQty = 0;

				while (console->paramsQty < MAX_PARAMS && (token = strtok(NULL, " ")) != NULL)
					console->param[console->paramsQty++] = strdup(token);

				if (str_eq(console->command, "clear"))
					system("clear");

				else if (str_eq(console->command, "ejecutar"))
				{
					if (console->paramsQty < 1)
						print_c(consoleLog, "%s: falta la ruta del script que se desea ejecutar\n", console->command);
					else
					{
						executeScript(console->param[0]);
					}
				}

				else if (str_eq(console->command, "status"))
				{
					// TODO: status command
				}

				else if (str_eq(console->command, "finalizar"))
				{
					if (console->paramsQty < 1)
						print_c(consoleLog, "%s: falta el ID correspondiente a un DT Block\n", console->command);
					else
					{
						// TODO: finish process command
					}
				}

				else if (str_eq(console->command, "metricas"))
				{
					if (console->paramsQty == 1)
						getProcessMetrics(console->param[0]);

					else if (console->paramsQty == 0)
						getSystemMetrics();

					else
						log_info(consoleLog, "%s: numero incorrecto de argumentos. Recuerde que puede ingresar solo el id de proceso o ningun argumento\n", console->command);
				}

				else
					print_c(consoleLog, "%s: Comando incorrecto\n", console->command);

				free(console->command);
				for (uint32_t i = 0; i < console->paramsQty; i++)
					free(console->param[i]);
				free(console);
			}
		}

		free(line);
	}
}


void executeScript(char* script)
{
	PCB_t* process = createProcess(script);

	if(process == NULL)
	{
		log_error(schedulerLog, "Ocurrio un problema al intentar crear el PCB para el nuevo proceso\n");
		return;
	}

	pthread_mutex_lock(&newQueueMutex);
	addProcessToNewQueue(process);
	pthread_mutex_unlock(&newQueueMutex);
}

void getProcessMetrics()
{
	//TODO - Se obtienen metricas 1 y 4 del enunciado (las otras dice que son de sistema) y se imprimen
	//los procesos que van a exit por una instruccion es por terminacion anormal (errores, lo mataron, etc.. o sea, lo que escapa a una terminacion normal)
}

void getSystemMetrics()
{
	//TODO - Se obtienen metricas 2, 3 y 5 del enunciado y se imprimen
}