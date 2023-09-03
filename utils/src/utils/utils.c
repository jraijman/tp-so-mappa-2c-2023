#include "./utils.h"

 t_config* iniciar_config(char * ruta)
    {
    
        t_config* nuevo_config;
        if ((nuevo_config = config_create(ruta)) == NULL) {
            // ¡No se pudo crear el config! 
            exit(2);
        }
        return nuevo_config;
    }

t_log* iniciar_logger(char* file, char *process_name)
{
	t_log* logger;
	logger = log_create(file, process_name, 1, LOG_LEVEL_INFO);
	return logger;
}

void terminar_programa(t_log* logger, t_config* config)
{
	/* Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	  con las funciones de las commons y del TP mencionadas en el enunciado */
	if (config != NULL) {
		config_destroy(config);
	}
	if (logger != NULL) {
		log_destroy(logger);
	}

}