#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>

#define ANSI_COLOR_GREEN   "\x1b[32m" // Código de escape para color verde
#define ANSI_COLOR_BLUE    "\x1b[34m" // Código de escape para color azul
#define ANSI_COLOR_YELLOW  "\x1b[33m" // Código de escape para color amarillo
#define ANSI_COLOR_RESET   "\x1b[0m"  // Código de escape para resetear el color


t_config* iniciar_config(char* ruta);
t_log* iniciar_logger(char* file, char *process_name);
void terminar_programa(t_log* logger, t_config* config);
char *list_to_string(t_list *list);
#endif 