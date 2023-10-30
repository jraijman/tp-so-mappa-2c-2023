#include "comunicacion.h"

static void procesar_conexion(void* void_args) {
    t_procesar_conexion_args* args = (t_procesar_conexion_args*) void_args;
    t_log* logger = args->log;
    int cliente_socket = args->fd;
    char* server_name = args->server_name;
    free(args);

    op_code cop;
    while(cliente_socket != -1){
        //OP CODES
        // aca van todos los tipos de mensaje que puede recibir, indicando en cada caso que hace

        if (recv(cliente_socket, &cop, sizeof(cop), 0) != sizeof(cop)) {
    		log_info(logger, "DISCONNECT!");
    		return;
    	}

         switch (cop) {
           case ENVIO_PCB:
            {
                pcb proceso;

                if (!recv_pcb(cliente_socket, &proceso)) {
                    log_error(logger, "Fallo recibiendo ENVIO_PCB");
                    break;
                }

                log_info(logger, "recibi pcb id: %d, prioridad: %d", proceso.pid, proceso.prioridad);

                break;
            }
                        
            // Errores
            case -1:
                log_error(logger, "Cliente desconectado de %s...", server_name);
                return;
            default:
                log_error(logger, "Algo anduvo mal en el server de %s", server_name);
                return;
        }

    }

    log_warning(logger, "El cliente se desconecto de %s server", server_name);
    return;
}

void serializar_instruccion(const Instruccion *instruccion, void *buffer, size_t buffer_size) {
    if (buffer_size >= sizeof(Instruccion)) {
        memcpy(buffer, instruccion, sizeof(Instruccion));
    }
}
int server_escuchar_memoria(t_log* logger,char* server_name,int server_socket){
     int cliente_socket = esperar_cliente(logger, server_name, server_socket);

    if (cliente_socket != -1) {
        pthread_t hilo;
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        args->log = logger;
        args->fd = cliente_socket;
        args->server_name = server_name;

        pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) args);
        pthread_detach(hilo);
        return 1;
    }
    return 0;
}

void insertarProcesoOrdenado(Proceso* lista, int pid, int estado, const char* rutaArchivo) {
    Proceso* nuevoProceso = (Proceso*)malloc(sizeof(Proceso));
    char* nombreArchivo[25];
    nuevoProceso->pid = pid;
    nuevoProceso->estado = estado;
    sprintf(nombreArchivo, '%d', pid);
    strcat(rutaArchivo, nombreArchivo);
    strcpy(nuevoProceso->rutaArchivo, rutaArchivo);
    nuevoProceso->siguiente = NULL;
   
    if (lista == NULL || pid < lista->pid) {
        nuevoProceso->siguiente = lista;
        lista = nuevoProceso;
    }

 
    Proceso* aux = lista;
    while (aux->siguiente != NULL && aux->siguiente->pid < pid) {
        aux = aux->siguiente;
    }

    nuevoProceso->siguiente = aux->siguiente;
    aux->siguiente = nuevoProceso;

}

Proceso* buscarProcesoPorPID(Proceso* lista, int pid) {
    Proceso* aux = lista;
    while (aux != NULL) {
        if (aux->pid == pid) {
            return aux; 
        }
        aux = aux->siguiente;
    }
    return NULL; 
}


bool notificar_liberacion_swap(int socket_fd, int pid, int cantidad_bloques, int* bloques){
    //retorno provisorio para que funcione
    return true;
}

void manejarConexion(pcbDesalojado contexto){
    char* instruccion = contexto.instruccion;
    if(strcmp(instruccion, "INICIALIZACION")==0)
    {
        printf("Inizializando estructura en memoria para un proceso de pid:%d y tamanio:%d", contexto.contexto.pid,contexto.contexto.size);      
        inicializar_estructura_proceso(contexto.contexto.pid);
        notificar_reserva_swap(fd_filesystem, contexto.contexto.pid, contexto.contexto.size); 
    }
    else if (strcmp(instruccion, "FINALIZACION")==0)
    {
        printf("Memoria recibio una peticion de kernel de finalizar un proceso");
        eliminar_proceso_memoria(contexto.contexto.pid);
        int arreglo[] = {1, 2, 3, 4};
        int *paginas = arreglo;
        int cantidad_paginas = obtenerCantidadPaginasAsignadas(contexto.contexto.pid);
                                                                                        //numero de paginas a liberar
        notificar_liberacion_swap(fd_filesystem, contexto.contexto.pid, cantidad_paginas, paginas);
    }
    else{
        printf("No reconocido");
    }
}