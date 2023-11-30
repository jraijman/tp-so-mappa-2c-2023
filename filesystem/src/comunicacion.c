#include "comunicacion.h"

static void procesar_conexion(void* void_args) {
    t_procesar_conexion_args* args = (t_procesar_conexion_args*) void_args;
    t_log* logger = args->log;
    int cliente_socket = args->fd;
    char* server_name = args->server_name;
    free(args);
    op_code cop;
    while(cliente_socket != -1){
        if (recv(cliente_socket, &cop, sizeof(cop), 0) != sizeof(cop)) {
    		log_info(logger, ANSI_COLOR_BLUE"El cliente se desconecto de %s server", server_name);
			return;
    	}
        switch (cop) {
            case CONEXION_MEMORIA:
                sleep(2);
                conexion_filesystem_memoria = crear_conexion(logger,"MEMORIA",ip_memoria,puerto_memoria);
                enviar_mensaje("Hola soy FILESYSTEM", conexion_filesystem_memoria);
                break;
            case MENSAJE:
                recibir_mensaje(logger, cliente_socket);
                break;
		    case PAQUETE:
                t_list *paquete_recibido = recibir_paquete(cliente_socket);
                log_info(logger, ANSI_COLOR_YELLOW "Recibí un paquete con los siguientes valores: ");
                //list_iterate(paquete_recibido, (void*) iterator);
                break;
            case RESERVA_SWAP:
            {
                t_list* paquete=recibir_paquete(cliente_socket);
                int cantidad_bloques=list_get(paquete, 0);
                int bloques_reservados[cantidad_bloques];
                for(int i=0; i<)
                reservar_bloquesSWAP(cantidad_bloques,&bloques_reservados);
                //send_bloquesReservados;
                break;
            }
            default:
                log_error(logger, "Algo anduvo mal en el server de %s", server_name);
                return;
        }
    }

    log_warning(logger, "El cliente se desconecto de %s server", server_name);
    return;
}

int server_escuchar_filesystem(t_log* logger,char* server_name,int server_socket){
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