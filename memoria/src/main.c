#include "main.h"

void levantar_config(char *ruta)
{
    logger_memoria = iniciar_logger("memoria.log", "MEMORIA:");

    config = iniciar_config(ruta);
    puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    ip_filesystem = config_get_string_value(config, "IP_FILESYSTEM");
    tam_memoria = config_get_string_value(config, "TAM_MEMORIA");
    puerto_filesystem = config_get_string_value(config, "PUERTO_FILESYSTEM");
    tam_pagina = config_get_string_value(config, "TAM_PAGINA");
    path_instrucciones = config_get_string_value(config, "PATH_INSTRUCCIONES");
    retardo_respuesta = config_get_string_value(config, "RETARDO_RESPUESTA");
    algoritmo_reemplazo = config_get_string_value(config, "ALGORITMO_REEMPLAZO");

    log_info(logger_memoria, "Config cargada");
}
bool leerYEnviarInstruccion(FILE *archivo, int conexion_memoria) {
    char linea[256];
    Instruccion instruccion;
    if (fgets(linea, sizeof(linea), archivo) != NULL) {
        char **palabras = string_split(linea, " ");

        if (palabras[0] != NULL) {
            strcpy(instruccion.opcode, palabras[0]);

            if (palabras[1] != NULL) {
                strcpy(instruccion.operando1, palabras[1]);

                if (palabras[2] != NULL) {
                    strcpy(instruccion.operando2, palabras[2]);
                } else {
                    instruccion.operando2[0] = '\0'; // Vaciar el operando2 si no hay tercer palabra
                }
            } else {
                instruccion.operando1[0] = '\0'; // Vaciar el operando1 si no hay segunda palabra
                instruccion.operando2[0] = '\0'; // Vaciar el operando2
            }
        } else {
            log_error(logger_memoria, "Error al cargar la instrucción");
        }

        /*if (!send_instruccion(conexion_memoria, instruccion)) {
            log_error(logger_memoria, "Error al enviar la instrucción");
        }*/
    }
}
int main(int argc, char *argv[])
{

    // CONFIG y logger
    levantar_config("memoria.config");
    // inicio servidor de escucha
    fd_memoria = iniciar_servidor(logger_memoria, NULL, puerto_escucha);

    // genero conexion a filesystem
    conexion_memoria_filesystem = crear_conexion(logger_memoria, "FILESYSTEM", ip_filesystem, puerto_filesystem);
    int cliente_memoria = esperar_cliente(logger_memoria, "MEMORIA ESCUCHA", fd_memoria);
    // espero clientes kernel,cpu y filesystem
    pcbDesalojado contexto;
    pcb proceso;
    int bytes_recibidos = 0;
    int pid;
    Proceso *tabla_proceso;
    t_marco* marcos;

    while(server_escuchar(fd_memoria));

    /*while (server_escuchar_memoria(logger_memoria, "MEMORIA", fd_memoria)) {
        int bytes = recv(cliente_memoria, (char *)&contexto + bytes_recibidos, sizeof(pcbDesalojado) - bytes_recibidos, 0);
        if (bytes > 0) {
            bytes_recibidos += bytes;        
            if (bytes_recibidos == sizeof(int)) {
                int num_pag;
                if (recv_int(cliente_memoria, &num_pag) < 0) {
                    log_error(logger_memoria, "Error al recibir el número de página.");
                } else {
                    int num_marco = calcularMarco(proceso.pid, marcos, num_pag);
                    if (send_int(fd_cpu_dispatch, num_marco) < 0) {
                        log_error(logger_memoria, "Error al enviar el número de marco a la CPU.");
                    }
                }
            } else if (bytes_recibidos == sizeof(pcbDesalojado)) {
                if (recv_pcbDesalojado(cliente_memoria, &contexto) < 0) {
                    log_error(logger_memoria, "Error al recibir el PCB Desalojado.");
                } else {
                    manejarConexion(contexto);
                    // Si es necesario devolver el PCB después de manejarlo
                }
            } else if (bytes_recibidos == sizeof(pcb)) {
                if (recv_pcb(cliente_memoria, &proceso) < 0) {
                    log_error(logger_memoria, "Error al recibir el PCB.");
                } else {
                    int pid_solicitado = proceso.pid;
                    Proceso *procesoactual = buscarProcesoPorPID(tabla_proceso, pid_solicitado);
                    if (procesoactual != NULL) {
                        FILE *archivo = fopen(procesoactual->rutaArchivo, "r");
                        if (archivo != NULL) {
                            if (leerYEnviarInstruccion(archivo, fd_cpu_dispatch)) {
                                fclose(archivo);
                            } else {
                                fclose(archivo);
                                log_error(logger_memoria, "Error al leer y enviar instrucciones para el PID: %d", pid_solicitado);
                            }
                        } else {
                            log_error(logger_memoria, "Error al abrir el archivo de instrucciones para el PID: %d", pid_solicitado);
                        }
                    } else {
                        log_error(logger_memoria, "PID: %d no encontrado en la tabla de procesos", pid_solicitado);
                    }
                }
            }
        } else {
            log_error(logger_memoria, "Error al recibir datos desde el cliente de memoria");
            // Manejo de error: No se pudieron recibir datos del cliente de memoria
            // Puedes agregar código adicional para manejar este error
        }
    }*/
   
    // CIERRO LOG Y CONFIG y libero conexion
    terminar_programa(logger_memoria, config);
    liberar_conexion(conexion_memoria_filesystem);
}

// ----------------------COMUNICACION----------------------------------

static void procesar_conexion(void *void_args) {
	int *args = (int*) void_args;
	int cliente_socket = *args;

	op_code cop;
	while (cliente_socket != -1) {
		if (recv(cliente_socket, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
			log_info(logger_memoria, "El cliente se desconecto de %s server", server_name);
			return;
		}
		switch (cop) {
		case MENSAJE:
			recibir_mensaje(logger_memoria, cliente_socket);
			break;
		case PAQUETE:
			t_list *paquete_recibido = recibir_paquete(cliente_socket);
			log_info(logger_memoria, "Recibí un paquete con los siguientes valores: ");
			//list_iterate(paquete_recibido, (void*) iterator);
			break;
		case INICIALIZAR_PROCESO:
			//int pid_init = recv_inicializar_proceso(cliente_socket);
			log_info(logger_memoria, "Creación de Proceso PID: %d", 1);
            //tabla de paginas
			//send_proceso_inicializado(tabla_segmentos_inicial, cliente_socket);
			break;
		case FINALIZAR_PROCESO:
			int pid_fin = recv_terminar_proceso(cliente_socket);
			log_info(logger_memoria, "Eliminación de Proceso PID: %d", pid_fin);
			//terminar_proceso(pid_fin);
			break;

	return;
        } 
    }
}

int server_escuchar() {
	server_name = "Memoria";
	int cliente_socket = esperar_cliente(logger_memoria, server_name, fd_memoria);

	if (cliente_socket != -1) {
		pthread_t hilo;
		int *args = malloc(sizeof(int));
		args = &cliente_socket;
		pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) args);
		pthread_detach(hilo);
		return 1;
	}

	return 0;
}


void liberar_marco(t_marco* marco){
    
    log_info(logger_memoria,"se libera el marco:%d, con el proceso:%d",marco->num_marco,marco->pid);
    marco->ocupado = false;
    marco->pid = -1;
}

t_marco* marco_create(uint32_t numero_marco, uint32_t pid,bool estado_marco){
    t_marco* marco = malloc(sizeof(t_marco));
    marco->pid = pid;
    marco->ocupado = estado_marco;
    marco->num_marco = numero_marco;
}
int reservar_primer_marco_libre(int pid){
    for(int i = 0; i < list_size(l_marco);i++){
        t_marco* marco = list_get(l_marco,i);
        if(!marco->ocupado){
            marco->ocupado = true;
            marco->pid = pid;
            return marco->num_marco;
        }
    }
    return -1;
}
int calcularMarco(int pid, t_marco* marcos, int num_marcos) {
    for (int i = 0; i < num_marcos; i++) {
        if (marcos[i].ocupado && marcos[i].pid == pid) {
            return marcos[i].num_marco;
        }
    }
    
    return -1;
}

bool marco_asignado_a_este_proceso(int pid,t_marco * marco) {
    return marco->pid == pid;
}

t_list* obtenerMarcosAsignados(pid){
    t_list* marcos_asignados = list_filter(l_marco, (void*)marco_asignado_a_este_proceso);
    return marcos_asignados;
}

void eliminar_proceso_memoria(int pid) {
    
    t_list* marcos_asignados = obtenerMarcosAsignados(pid);

    
    for (int i = 0; i < list_size(marcos_asignados); i++) {
        t_marco* marco = list_get(marcos_asignados, i);
        liberar_marco(marco);
    }
    list_destroy(marcos_asignados);
}

void inicializar_estructura_proceso(int pid) {
    
    Proceso* proceso = malloc(sizeof(Proceso));
    
    if (proceso == NULL) {
        perror("Error en la asignación de memoria para la estructura de proceso");
        return;
    }

    proceso->pid = pid;
    proceso->estado = NEW; 
    proceso->rutaArchivo = NULL; 
    proceso->siguiente = NULL; 

    insertarProcesoOrdenado(l_proceso, proceso->pid, proceso->estado, proceso->rutaArchivo);


}
bool notificar_reserva_swap(int fd, int pid, int cantidad_bloques) {
    
    SolicitudReservaSwap solicitud;
    solicitud.pid = pid;
    solicitud.cantidad_bloques = cantidad_bloques;

    
    if (send(fd, &solicitud, sizeof(SolicitudReservaSwap), 0) != sizeof(SolicitudReservaSwap)) {
        perror("Error al enviar la solicitud de reserva de bloques de SWAP");
        return false;
    }

    return true;
}

// Función para notificar al módulo FS y liberar páginas en la partición de SWAP
bool notificarLiberacionSwap(int socket_fd, int pid, int cantidadPaginas, int* paginas) {
    SolicitudLiberacionSwap solicitud;
    solicitud.pid = pid;
    solicitud.cantidad_paginas = cantidadPaginas;

    solicitud.paginas = (int*)malloc(cantidadPaginas * sizeof(int));
    for (int i = 0; i < cantidadPaginas; i++) {
        solicitud.paginas[i] = paginas[i];
    }

    if (send(socket_fd, &solicitud, sizeof(SolicitudLiberacionSwap), 0) != sizeof(SolicitudLiberacionSwap)) {
        perror("Error al enviar la solicitud de liberación de páginas en SWAP al módulo FS");
        free(solicitud.paginas);
        return false;
    }

   
    free(solicitud.paginas);

    return true;
}
int obtenerCantidadPaginasAsignadas(int pid) {
    t_list* marcos_asignados = obtenerMarcosAsignados(pid);
    int cantidadDePaginasAsignadas = list_size(marcos_asignados);
    list_destroy(marcos_asignados);
    return cantidadDePaginasAsignadas;
}

/*
TODO:
INSTRUCCIONES SE ENVIA 1 SOLA, LA PETICION ES ITERATIVA DEL LADO CPU POR LO CUAL VAN A IR LLEGANDO PETICIONES DE INSTRUCCION (INDEXAS CON EL PROGRAM COUNTER PARA BAJAR LA LINEA QUE NECESITO)
*/
