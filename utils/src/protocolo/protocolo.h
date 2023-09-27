#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include "../utils/utils.h"
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>
#include<pthread.h>

typedef enum {
    /*APROBAR_OPERATIVOS,
    MIRAR_NETFLIX,
    PRUEBA = 69,
    */
    // ver si faltan (seguramente)

    //OP CODES DE KERNEL
    INICIAR_ESTRUCTURA, // enviando un mensaje al módulo Memoria para que inicialice sus estructuras necesarias
    FINALIZAR_PROCESO, //Cuando se reciba un mensaje de CPU con motivo de finalizar el proceso, dar aviso al módulo Memoria para que éste libere sus estructuras.
    CONTEXTO_EJECUCION, // se enviará su Contexto de Ejecución al CPU a través del puerto de dispatch
    INTERRUPCION, //se enviará una interrupción a través de la conexión de interrupt para forzar el desalojo del mismo
    CREAR_PROCESO, //informarle a la memoria que debe crear un proceso
    LIBERAR_ESTUCTURA, //solicitar a la memoria que libere todas las estructuras asociadas al proceso
    PAGE_FAULT, //el módulo CPU devuelva un PCB desalojado por Page Fault
    CARGAR_PAGINA, //Solicitar al módulo memoria que se cargue en memoria principal la página correspondiente
    




} op_code;

///

bool send_int(int fd,int pid);
bool recv_int(int fd, int* pid);

#endif 