#include <comunicacion.h>

static void procesar_conexion(void* void_args) {
    t_procesar_conexion_args* args = (t_procesar_conexion_args*) void_args;
    t_log* logger = args->log;
    int cliente_socket_dispatch = args->fd_dispatch;
    int cliente_socket_interrupt = args->fd_interrupt;
    char* server_name = args->server_name;
    free(args);

    op_code cop;
    while(cliente_socket_dispatch != -1 && cliente_socket_interrupt != -1){
        //OP CODES
        // aca van todos los tipos de mensaje que puede recibir, indicando en cada caso que hace
        // ver como es con los dos servidores dispatch e interrupt


        /*
        if (recv(cliente_socket_dispatch, &cop, sizeof(cop), 0) != sizeof(cop)) {
    		log_info(logger, "DISCONNECT!");
    		return;
    	}

         switch (cop) {
            case PRUEBA:
                log_info(logger, "mw llwgo mensaje de prueba");
                break;

            case APROBAR_OPERATIVOS:
            {
                uint8_t nota1, nota2;

                if (!recv_aprobar_operativos(cliente_socket, &nota1, &nota2)) {
                    log_error(logger, "Fallo recibiendo APROBAR_OPERATIVOS");
                    break;
                }

                log_info(logger, "Aprobe operativos ");

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
        */

    }
    

    log_warning(logger, "El cliente se desconecto de %s server", server_name);
    return;
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