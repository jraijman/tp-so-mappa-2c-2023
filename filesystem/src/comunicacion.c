#include "comunicacion.h"

static void procesar_conexion(void* void_args) {
    t_procesar_conexion_args* args = (t_procesar_conexion_args*) void_args;
    t_log* logger = args->log;
    int cliente_socket = args->fd;
    char* server_name = args->server_name;
    bool bitmapSwap[cant_bloques_swap];
    bool bitmapBloques[cant_bloques_total-cant_bloques_swap];
    for(int i=0; i<cant_bloques_swap; i++){
        bitmapSwap[i]=args->bitmapSwap[i];
    }
    for(int i=0; i<cant_bloques_total-cant_bloques_swap; i++){
    bitmapBloques[i]=args->bitmapBloques[i];
    }
    free(args);
    op_code cop;
    while(cliente_socket != -1){
        if (recv(cliente_socket, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
    		log_info(logger, ANSI_COLOR_BLUE"El cliente se desconecto de %s server", server_name);
			return;
    	}
        switch (cop) {
            case MENSAJE:
                char* msj = recibir_mensaje_fs(cliente_socket);
                if(strcmp(msj,"CONECTATE")==0){
                     sleep(2);
                    conexion_filesystem_memoria = crear_conexion(logger,"MEMORIA",ip_memoria,puerto_memoria);
                    enviar_mensaje("Hola soy FILESYSTEM", conexion_filesystem_memoria);
                }else{
                    log_info(logger, ANSI_COLOR_YELLOW"Me llegó el mensaje: %s", msj);
                }
                free(msj); // Liberar la memoria del mensaje recibido
                break;
		    case PAQUETE:
                t_list *paquete_recibido = recibir_paquete(cliente_socket);
                log_info(logger, ANSI_COLOR_YELLOW "Recibí un paquete con los siguientes valores: ");
                //list_iterate(paquete_recibido, (void*) iterator);
                break;
            case ABRIR_ARCHIVO:{
                t_list *paqueteRecibido=recibir_paquete(cliente_socket);
                char* nombre = list_get(paqueteRecibido, 0);
                int tamano=abrir_archivo(nombre);
                if(tamano!=-1){
                    t_paquete* paqueteEnviar=crear_paquete(ABRIR_ARCHIVO);
                    agregar_a_paquete(paqueteEnviar,&tamano,sizeof(int));
                    enviar_paquete(paqueteEnviar,cliente_socket);
                }else{
                    enviar_mensaje("El archivo solicitado no existe",cliente_socket);
                    t_paquete* paqueteEnviar=crear_paquete(ARCHIVO_NO_EXISTE);
                    enviar_paquete(paqueteEnviar,cliente_socket);
                }
                break;
            }
            case INICIALIZAR_PROCESO:{
                t_list* paquete=recibir_paquete(cliente_socket);
                int cantidad_bloques=*(int*)list_get(paquete, 0);
                list_destroy(paquete);
                int bloques_reservados[cantidad_bloques];
                if(reservar_bloquesSWAP(cantidad_bloques,bloques_reservados,bitmapSwap)){
                    t_paquete* paqueteReserva=crear_paquete(INICIALIZAR_PROCESO);
                    for(int i=0; i<cantidad_bloques;i++)
                    {
                        int bloque = bloques_reservados[i];
                        agregar_a_paquete(paqueteReserva, &bloque, sizeof(int));
                    }
                    enviar_paquete(paqueteReserva, cliente_socket);
                    eliminar_paquete(paqueteReserva);
                }else{
                    enviar_mensaje("Error al reservar los bloques SWAP",cliente_socket);
                    log_error(logger,"Error al reservar los bloques SWAP");
                }
                break;
            }
            case FINALIZAR_PROCESO:{   
                t_list* paquete=recibir_paquete(cliente_socket);
                int cantidad_bloques=*(int*)list_get(paquete,0);
                int bloques_a_liberar [cantidad_bloques];
                for(int i=1; i<cantidad_bloques; i++){
                    bloques_a_liberar[i-1]=*(int*)list_get(paquete,i);
                }
                if(liberar_bloquesSWAP(bloques_a_liberar,cantidad_bloques,bitmapSwap)){
                    enviar_mensaje("SWAP LIBERADO",cliente_socket);
                }else{
                    log_error(logger, "Error al liberar los bloques SWAP");
                    enviar_mensaje("ERROR AL LIBERAR SWAP",cliente_socket);
                }
                break;
            }
            case CREAR_ARCHIVO:{   
                t_list* paquete=recibir_paquete(cliente_socket);
                char* nombre=list_get(paquete,0);
                crear_archivo(nombre);
                enviar_mensaje("OK crear archivo",cliente_socket);
                t_paquete* paqueteEnviar=crear_paquete(ARCHIVO_CREADO);
                enviar_paquete(paqueteEnviar,cliente_socket);
                eliminar_paquete(paqueteEnviar);
                list_destroy(paquete);
                break;
            }
            case TRUNCAR_ARCHIVO:{
                t_list* paquete=recibir_paquete(cliente_socket);
                char* nombre=list_get(paquete,0);
                int tamano=*(int*)list_get(paquete,1);
                list_destroy(paquete);
                truncarArchivo(nombre,tamano,bitmapBloques);
                enviar_mensaje("ARCHIVO TRUNCADO", cliente_socket);
                break;
            }
            case PEDIDO_SWAP:{
                t_list* paquete=recibir_paquete(cliente_socket);
                int num_bloque=*(int*)list_get(paquete,0);
                list_destroy(paquete);
                char* info_leida = leer_bloque(num_bloque);
                send_leido_swap(cliente_socket,info_leida,tam_bloque);
                free(info_leida);
                break;
            }
           
            default:
                log_error(logger, "MESNAJE DESCONOCIDO OPCODE: %d", cop);
                break;
        }
    }

    log_warning(logger, "El cliente se desconecto de %s server", server_name);
    return;
}

int server_escuchar_filesystem(t_log* logger,char* server_name,int server_socket,bool* bitmapBloques, bool* bitmapSwap){
    int cliente_socket = esperar_cliente(logger, server_name, server_socket);
    if (cliente_socket != -1) {
        pthread_t hilo;
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        args->log = logger;
        args->fd = cliente_socket;
        args->server_name = server_name;
        args->bitmapBloques=bitmapBloques;
        args->bitmapSwap=bitmapSwap;
        pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) args);
        pthread_detach(hilo);
        return 1;
    }
    return 0;
}