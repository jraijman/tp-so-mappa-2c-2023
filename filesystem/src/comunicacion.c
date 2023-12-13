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
            case INICIALIZAR_PROCESO:{
                t_list* paquete=recibir_paquete(cliente_socket);
                int* puntero=list_get(paquete, 0);
                int cantidad_bloques = *puntero;
                free(puntero);//memory leak
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
            case F_WRITE:{
                t_list* paquete=recibir_paquete(cliente_socket);
                int dirFisica=*(int*)list_get(paquete, 0);
                int puntero=*(int*)list_get(paquete,1);

                //falta nombre archivo para saber desde donde leer
                t_paquete* peticionMemoria=crear_paquete(F_WRITE);
                agregar_a_paquete(peticionMemoria, &dirFisica,sizeof(uint32_t));
                t_list* infoEscribir=recibir_paquete(conexion_filesystem_memoria);
                char* info=list_get(infoEscribir,0);
                int bloque=puntero/tam_bloque;
                free(info);
                escribir_bloque(bloque,info);
                list_destroy(paquete);
                eliminar_paquete(peticionMemoria);
                list_destroy(infoEscribir);
                break;
            }
            case F_READ:{
                t_list* paquete=recibir_paquete(cliente_socket);
                int dirFisica=*(int*)list_get(paquete, 0);
                int puntero=*(int*)list_get(paquete,1);

                //falta nombre archivo para saber desde donde leer
                char* info=leer_bloque(puntero/tam_bloque);
                t_paquete* escribir=crear_paquete(F_READ);
                agregar_a_paquete(escribir,info,tam_bloque);
                enviar_paquete(escribir,conexion_filesystem_memoria);
                t_list* confirmacion=recibir_paquete(conexion_filesystem_memoria);
                int confirma=*(int*)list_get(confirmacion,0);
                if(confirma==1){
                    enviar_mensaje("Lectura realizada correctamente",cliente_socket);
                }
                else{
                    enviar_mensaje("La lectura falló",cliente_socket);
                }
                free(info);
                list_destroy(paquete);
                list_destroy(confirmacion);
                eliminar_paquete(escribir);
                break;
            }
            case FINALIZAR_PROCESO:{   
                t_list* paquete=recibir_paquete(cliente_socket);
                int*puntero = list_get(paquete,0);
                int cantidad_bloques=*puntero;
                free(puntero);
               
                int bloques_a_liberar [cantidad_bloques];
                for(int i=1; i<cantidad_bloques+1; i++){
                    int*puntero2 = list_get(paquete,i);
                    bloques_a_liberar[i-1]=* puntero2;
                    free(puntero2);//memory leak
                }
                if(liberar_bloquesSWAP(bloques_a_liberar,cantidad_bloques,bitmapSwap)){
                    //enviar_mensaje("SWAP LIBERADO",cliente_socket);
                    log_info(logger, "Se liberaron los bloques SWAP");
                }else{
                    log_error(logger, "Error al liberar los bloques SWAP");
                    //enviar_mensaje("ERROR AL LIBERAR SWAP",cliente_socket);
                }
                list_destroy(paquete);
                break;
            }
            case ABRIR_ARCHIVO:{
                t_list *paqueteRecibido = recibir_paquete(cliente_socket);
                char* nombre = list_get(paqueteRecibido, 0);
                int tamano = abrir_archivo(nombre);
                list_destroy(paqueteRecibido);
                t_paquete* paqueteEnviar;
                if(tamano!=-1){
                    paqueteEnviar = crear_paquete(ARCHIVO_EXISTE);
                }else{
                    paqueteEnviar = crear_paquete(ARCHIVO_NO_EXISTE);
                }
                agregar_a_paquete(paqueteEnviar,&tamano,sizeof(int));
                enviar_paquete(paqueteEnviar,cliente_socket);
                eliminar_paquete(paqueteEnviar);
                break;
            }
            case CREAR_ARCHIVO:{   
                t_list* paquete = recibir_paquete(cliente_socket);
                char* nombre = list_get(paquete,0);
                list_destroy(paquete);
                crear_archivo(nombre);
                enviar_mensaje("OK crear archivo",cliente_socket);
                break;
            }
            case TRUNCAR_ARCHIVO:{
                t_list* paquete=recibir_paquete(cliente_socket);
                char* nombre=list_get(paquete,0);
                int* puntero = (int*)list_get(paquete,1);
                int tamanio=*puntero;
                free(puntero);
                list_destroy(paquete);
                truncarArchivo(nombre,tamanio,bitmapBloques);
                enviar_mensaje("ARCHIVO TRUNCADO", cliente_socket);
                break;
            }
            case PEDIDO_SWAP:{
                t_list* paquete=recibir_paquete(cliente_socket);
                int*puntero = list_get(paquete,0);
                int num_bloque=*puntero;
                free(puntero);
                list_destroy(paquete);
                pthread_t hiloSwap;
                t_pedido_swap_args* argsSwap = malloc(sizeof(t_pedido_swap_args));
                argsSwap->fd = cliente_socket;
                argsSwap->num_bloque = num_bloque;

                pthread_create(&hiloSwap, NULL, (void*) manejar_pedido_swap, (void*) argsSwap);
                pthread_detach(hiloSwap);
                break;
            }
            case ESCRIBIR_SWAP:{
                t_list* paquete=recibir_paquete(cliente_socket);
                int * puntero=list_get(paquete,0);
                int num_bloque=*puntero;
                free(puntero);
                char* info_a_escribir=list_get(paquete,1);
                list_destroy(paquete);
                pthread_t hiloEscribir;
                t_pedido_escribir_swap_args* argsSwap = malloc(sizeof(t_pedido_escribir_swap_args));
                argsSwap->fd = cliente_socket;
                argsSwap->num_bloque = num_bloque;
                argsSwap->info_a_escribir=info_a_escribir;
                pthread_create(&hiloEscribir, NULL, (void*) manejar_escribir_swap, (void*) argsSwap);
                pthread_detach(hiloEscribir);
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

void* manejar_pedido_swap(void* arg){
    t_pedido_swap_args* args = (t_pedido_swap_args*) arg;
    int num_bloque = args->num_bloque;
    int cliente = args->fd;

    char* info_leida = leer_bloque(num_bloque);
    send_leido_swap(cliente,info_leida,tam_bloque);
    free(info_leida);
    free(args); 
}

void* manejar_escribir_swap(void* arg){
    t_pedido_escribir_swap_args* args = (t_pedido_escribir_swap_args*) arg;
    int num_bloque = args->num_bloque;
    int cliente = args->fd;
    char* info_a_escribir = args->info_a_escribir;

    escribir_bloque(num_bloque,info_a_escribir);
    free(info_a_escribir);
    free(args); 
}