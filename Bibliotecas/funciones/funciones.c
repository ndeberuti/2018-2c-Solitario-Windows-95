#include "funciones.h"

t_log *init_log(char *file, char *program_name, bool is_active_console, t_log_level level) {
	mkdir("/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Logs", 0755);
	t_log *archivoLog = log_create(file, program_name, is_active_console, level);
	return archivoLog;
}

bool str_eq(const char *s1, const char *s2) {
	uint32_t size = strlen(s2);
	return !strncmp(s1, s2, size) && strlen(s1) == size;
}

void print_c(t_log *log_file, char *message_template, ...) {
	va_list arguments;
	va_start(arguments, message_template);
	char *message = string_from_vformat(message_template, arguments);
	va_end(arguments);
	log_info(log_file, message);
	printf("%s\n", message);
	free(message);
}
