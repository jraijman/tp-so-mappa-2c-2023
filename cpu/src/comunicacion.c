#include <comunicacion.h>

static void procesar_conexion(void* void_args) {
    t_procesar_conexion_args* args = (t_procesar_conexion_args*) void_args;
    t_log* logger = args->log;
    int cliente_socket_dispatch = args->fd_dispatch;
    int cliente_socket_interrupt = args->fd_interrupt;
    char* server_name = args->server_name;
    free(args);

    while (cliente_socket_dispatch != -1 && cliente_socket_interrupt != -1) {
        // RECIBO LA ESTRUCTURA DE CONTEXTO DE EJECUCION
        struct ContextoEjecucion contexto_proceso;
        if (recv(cliente_socket_dispatch, &contexto_proceso, sizeof(struct ContextoEjecucion), 0) != sizeof(struct ContextoEjecucion)) {
            log_info(logger, "DISCONNECT!");
            return;
        }
        
        // PROCESO LA ESTRUCTURA RECIBIDA
        log_info(logger, "Se recibió un contexto de ejecución desde %s", server_name);
        log_info(logger, "PID: %u", contexto_proceso.pid);
        log_info(logger, "Program Counter: %u", contexto_proceso.program_counter);
        log_info(logger, "AX: %u", contexto_proceso.registros[0]);
    }

    log_warning(logger, "El cliente se desconectó de %s server", server_name);
    return;
}

static void* serializar_peticion(uint32_t pid, uint32_t program_counter) {
    void* stream = malloc(sizeof(PeticionMemoria));
    PeticionMemoria peticion;
    peticion.pid = pid;
    peticion.program_counter = program_counter;
    memcpy(stream, &peticion, sizeof(PeticionMemoria));
    return stream;
}

/*static void deserializar_peticion(void* stream, uint32_t* pid, uint32_t* program_counter) {
    PeticionMemoria peticion;
    memcpy(&peticion, stream, sizeof(PeticionMemoria));
    *pid = peticion.pid;
    *program_counter = peticion.program_counter;
}

static void* serializar_instruccion(const Instruccion* instruccion) {
    void* stream = malloc(sizeof(Instruccion));
    memcpy(stream, instruccion, sizeof(Instruccion));
    return stream;
}
*/
static void deserializar_instruccion(void* stream, Instruccion* instruccion) {
    memcpy(instruccion, stream, sizeof(Instruccion));
}
bool send_peticion(int fd, uint32_t pid, uint32_t program_counter) {
    size_t size = sizeof(PeticionMemoria);
    void* stream = serializar_peticion(pid, program_counter);
    if (send(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}
bool recv_instruccion(int fd, Instruccion *instruccion) {
    size_t size = sizeof(Instruccion);
    void* stream = malloc(size);

    if (recv(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }

    deserializar_instruccion(stream, &instruccion);

    free(stream);
    return true;
}


int server_escuchar_cpu(t_log* logger, char* server_name, int server_socket_dispatch,int server_socket_interupt){
     int cliente_socket_dispatch = esperar_cliente(logger, server_name, server_socket_dispatch);
     int cliente_socket_interrupt = esperar_cliente(logger, server_name, server_socket_interupt);

    if (cliente_socket_dispatch != -1 && cliente_socket_interrupt != -1) {
        pthread_t hilo;
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        args->log = logger;
        args->fd_dispatch= cliente_socket_dispatch;
        args->fd_interrupt= cliente_socket_interrupt;
        args->server_name = server_name;

        pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) args);
        pthread_detach(hilo);
        return 1;
    }
    return 0;
}