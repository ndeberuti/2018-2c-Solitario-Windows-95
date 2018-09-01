#include "funciones.h"

t_log *init_log(char *file, char *program_name, bool is_active_console, t_log_level level) {
	mkdir("/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Logs", 0755);
	t_log *archivoLog = log_create(file, program_name, is_active_console, level);
	return archivoLog;
}
