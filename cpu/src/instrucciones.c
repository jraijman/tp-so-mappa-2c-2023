#include "instrucciones.h"
#include <sys/socket.h>

int traducir(int direccion_logica,int fd) {
    Direccion direccion;
    direccion.direccionLogica=direccion_logica;
    direccion.desplazamiento=-1;
    direccion.direccionFisica=-1;
    direccion.numeroPagina=-1;
    direccion.marco=-1;
    direccion.pageFault=false;
    direccion.tamano_pagina=-1;
    send_direccion(fd, &direccion);
    recv_direccion(fd, &direccion);//RECIBO LA DIRECCIÓN ACTUALIZADA CON EL TAMANO DE PAGINA SI SE PUEDE CONSEGUIR ANTES MEJOR AHORRO PASOS.
    direccion.numeroPagina=direccion.direccionLogica/direccion.tamano_pagina;
    direccion.desplazamiento=direccion.direccionLogica-(direccion.numeroPagina*direccion.tamano_pagina);
    send_direccion(fd,&direccion);
    recv_direccion(fd,&direccion);//RECIBO LA DIRECCIÓN ACTUALIZADA CON LA DIRECCIÓN FÍSICA DEL MARCO;
    direccion.direccionFisica=direccion.marco+direccion.desplazamiento;
    return direccion.direccionFisica;
}

void setInstruccion(pcb* contexto, Instruccion instruccion, t_log* logger) {
    log_info(logger, "EJECUTANDO INSTRUCCION SET");
    char* registro = instruccion.operando1;
    char* valor = instruccion.operando2;

    int* registro_destino = obtener_registro(contexto, registro);

    if (registro_destino != NULL) {
        *registro_destino = atoi(valor);
    } else {
        log_error(logger, "Registro no reconocido en la instrucción SET");
    }
}

void sumInstruccion(pcb* contexto, Instruccion instruccion, t_log* logger) {
    log_info(logger, "EJECUTANDO INSTRUCCION SUM");
    char* destino = instruccion.operando1;
    char* origen = instruccion.operando2;

    int* registro_destino = obtener_registro(contexto, destino);
    int* registro_origen = obtener_registro(contexto, origen);

    if (registro_destino != NULL && registro_origen != NULL) {
        *registro_destino = *registro_destino + *registro_origen;
    } else {
        log_error(logger, "Registro no reconocido en la instrucción SUM");
    }
}

void subInstruccion(pcb* contexto, Instruccion instruccion, t_log* logger) {
    log_info(logger, "EJECUTANDO INSTRUCCION SUB");
    char* destino = instruccion.operando1;
    char* origen = instruccion.operando2;

    int* registro_destino = obtener_registro(contexto, destino);
    int* registro_origen = obtener_registro(contexto, origen);

    if (registro_destino != NULL && registro_origen != NULL) {
        *registro_destino = *registro_destino - *registro_origen;
    } else {
        log_error(logger, "Registro no reconocido en la instrucción SUB");
    }
}

void jnzInstruccion(pcb* contexto, Instruccion instruccion, t_log* logger) {
    log_info(logger,"EJECUTANDO INSTRUCCION JNZ");
    // JNZ (Registro, Instrucción): Si el valor del registro es distinto de cero, actualiza el program counter al número de instrucción pasado por parámetro.
    char* registro = instruccion.operando1;
    char* nueva_instruccion = instruccion.operando2;

    int* valor_registro = obtener_registro(contexto, registro);

    if (*valor_registro != 0 && valor_registro!=NULL) {
        contexto->pc = atoi(nueva_instruccion);
    }else if (valor_registro==NULL){
        log_error(logger, "Registro no reconocido en la instrucción JNZ");
    }
}

void sleepInstruccion(pcb* contexto, Instruccion instruccion, t_log* logger) {
    log_info(logger,"EJECUTANDO INSTRUCCION SLEEP");
    // SLEEP (Tiempo): Esta instrucción representa una syscall bloqueante. Se deberá devolver el Contexto de Ejecución actualizado al Kernel junto a la cantidad de segundos que va a bloquearse el proceso.
    char* tiempo = instruccion.operando1;
    send_pcbDesalojado(contexto, "SLEEP", tiempo, fd_cpu_dispatch, logger);    
}

void waitInstruccion(pcb* contexto, Instruccion instruccion, t_log* logger) {
    log_info(logger,"EJECUTANDO INSTRUCCION WAIT");
    // WAIT (Recurso): Esta instrucción solicita al Kernel que se asigne una instancia del recurso indicado por parámetro.
    char* recurso = instruccion.operando1;
    send_pcbDesalojado(contexto, "WAIT", recurso, fd_cpu_dispatch, logger);
}

void signalInstruccion(pcb* contexto, Instruccion instruccion, t_log* logger) {
    log_info(logger,"EJECUTANDO INSTRUCCION SIGNAL");
    // SIGNAL (Recurso): Esta instrucción solicita al Kernel que libere una instancia del recurso indicado por parámetro.
    char* recurso = instruccion.operando1;
    send_pcbDesalojado(contexto, "SIGNAL", recurso, fd_cpu_dispatch, logger);
}

void exitInstruccion(pcb* contexto, Instruccion instruccion, t_log* logger) {
    log_info(logger,"EJECUTANDO INSTRUCCION EXIT");
    // EXIT: Esta instrucción representa la syscall de finalización del proceso. Se deberá devolver el Contexto de Ejecución actualizado al Kernel para su finalización.
    send_pcbDesalojado(contexto, "EXIT", "", fd_cpu_dispatch, logger);
}

void movInInstruccion(pcb* contexto, Instruccion instruccion, t_log* logger) {
    log_info(logger,"EJECUTANDO INSTRUCCION MOV_IN");
    // MOV_IN (Registro, Dirección Lógica): Lee el valor de memoria correspondiente a la Dirección Lógica y lo almacena en el Registro.
    char* registro = instruccion.operando1;
    int direccion_logica = atoi(instruccion.operando2);
    int* registro_destino = obtener_registro(contexto, registro);
    if(registro_destino!=NULL){
    *registro_destino = direccion_logica;
    }else{
        log_error(logger, "Registro no reconocido en instruccion MOV_IN");
    }
}

void movOutInstruccion(pcb* contexto, Instruccion instruccion,int fd_memoria, t_log* logger) {
    log_info(logger,"EJECUTANDO INSTRUCCION MOV_OUT");
    // MOV_OUT (Dirección Lógica, Registro): Lee el valor del Registro y lo escribe en 
    //la dirección física de memoria obtenida a partir de la Dirección Lógica.
    int direccion_logica = atoi(instruccion.operando1);
    char* registro = instruccion.operando2;
    int* registro_origen = obtener_registro(contexto, registro);
    if(registro_origen != NULL){
    int direccion_física=traducir(direccion_logica, fd_memoria);
    //escribir_memoria(direccion_logica, *registro_origen);
    }else{
        log_error(logger,"Registro no reconocido en instruccion MOV_OUT");
    }
}

void fOpenInstruccion(pcb* contexto, Instruccion instruccion,int fd_cpu_dispatch, t_log* logger) {
    log_info(logger,"EJECUTANDO INSTRUCCION F_OPEN");
    // F_OPEN (Nombre Archivo, Modo Apertura): Esta instrucción solicita al kernel que abra el archivo pasado por parámetro con el modo de apertura indicado.
    char* nombre_archivo = instruccion.operando1;
    char* modo_apertura = instruccion.operando2;
    t_paquete* paquete = crear_paquete(F_OPEN);
    agregar_a_paquete(paquete,&nombre_archivo,(sizeof(char)*string_length(nombre_archivo)));
    agregar_a_paquete(paquete,&modo_apertura,(sizeof(char)*string_length(modo_apertura)));
    enviar_paquete(paquete, fd_cpu_dispatch);
    eliminar_paquete(paquete);
    //VER QUE HACER ACA (DEBO ESPERAR A QUE DEVUELVA UN MENSAJE DE CONFIRMACIÓN O ALGO DEL ESTILO?)    
}

void fCloseInstruccion(pcb* contexto, Instruccion instruccion,int fd_cpu_dispatch, t_log* logger) {
    log_info(logger,"EJECUTANDO INSTRUCCION FCLOSE");
    // F_CLOSE (Nombre Archivo): Esta instrucción solicita al kernel que cierre el archivo pasado por parámetro.
    char* nombre_archivo = instruccion.operando1;
    t_paquete* paquete = crear_paquete(F_CLOSE);
    agregar_a_paquete(paquete,&nombre_archivo,(sizeof(char)*string_length(nombre_archivo)));
    enviar_paquete(paquete, fd_cpu_dispatch);
    eliminar_paquete(paquete);
    //LO MISMO, TENGO QUE ESPERAR ALGO O SIGO NORMAL?
}

void fSeekInstruccion(pcb* contexto, Instruccion instruccion,int fd_cpu_dispatch, t_log* logger) {
    log_info(logger,"EJECUTANDO INSTRUCCION F_SEEK");
    // F_SEEK (Nombre Archivo, Posición): Esta instrucción solicita al kernel actualizar 
    //el puntero del archivo a la posición pasada por parámetro.
    char* nombre_archivo = instruccion.operando1;
    int posicion =atoi(instruccion.operando2);
    t_paquete* paquete = crear_paquete(F_SEEK);
    agregar_a_paquete(paquete,&nombre_archivo,(sizeof(char)*string_length(nombre_archivo)));
    agregar_a_paquete(paquete,&posicion,sizeof(int));
    enviar_paquete(paquete, fd_cpu_dispatch);
    eliminar_paquete(paquete);
}

void fReadInstruccion(pcb* contexto, Instruccion instruccion,int fd_cpu_dispatch, int fd_memoria, t_log* logger) {
    log_info(logger,"EJECUTANDO INSTRUCCION F_READ");
    // F_READ (Nombre Archivo, Dirección Lógica): Esta instrucción solicita al Kernel que se lea del archivo indicado y se escriba en la dirección física de Memoria la información leída.
    char* nombre_archivo = instruccion.operando1;
    int direccion_logica = atoi(instruccion.operando2);
    t_paquete* paquete = crear_paquete(F_READ);
    int direccion_física=traducir(direccion_logica, fd_memoria);
    agregar_a_paquete(paquete,&nombre_archivo,(sizeof(char)*string_length(nombre_archivo)));
    agregar_a_paquete(paquete,&direccion_logica,sizeof(int));
    agregar_a_paquete(paquete,&direccion_física,sizeof(int));
    enviar_paquete(paquete, fd_cpu_dispatch);
    eliminar_paquete(paquete);
}

void fWriteInstruccion(pcb* contexto, Instruccion instruccion,int fd_cpu_dispatch,int fd_memoria, t_log* logger) {
    log_info(logger,"EJECUTANDO INSTRUCCION F_WRITE");
    // F_WRITE (Nombre Archivo, Dirección Lógica): Esta instrucción solicita al Kernel que se escriba en el archivo indicado la información que es obtenida a partir de la dirección física de Memoria.
    char* nombre_archivo = instruccion.operando1;
    int direccion_logica = atoi(instruccion.operando2);
    t_paquete* paquete = crear_paquete(F_WRITE);
    int direccion_física=traducir(direccion_logica, fd_memoria);
    agregar_a_paquete(paquete,&nombre_archivo,(sizeof(char)*string_length(nombre_archivo)));
    agregar_a_paquete(paquete,&direccion_logica,sizeof(int));
    agregar_a_paquete(paquete,&direccion_física,sizeof(int));
    enviar_paquete(paquete, fd_cpu_dispatch);
    eliminar_paquete(paquete);
    }

void fTruncateInstruccion(pcb* contexto, Instruccion instruccion,int fd_cpu_dispatch, t_log* logger) {
    log_info(logger,"EJECUTANDO INSTRUCCION F_TRUNCATE");
    // F_TRUNCATE (Nombre Archivo, Tamaño): Esta instrucción solicita al Kernel que se modifique el 
    //tamaño del archivo al indicado por parámetro.
    char* nombre_archivo = instruccion.operando1;
    int tamano = atoi(instruccion.operando2);
    t_paquete* paquete = crear_paquete(F_TRUNCATE);
    agregar_a_paquete(paquete,&nombre_archivo,(sizeof(char)*string_length(nombre_archivo)));
    agregar_a_paquete(paquete,&tamano,sizeof(int));
    enviar_paquete(paquete, fd_cpu_dispatch);
    eliminar_paquete(paquete);
 }


int* obtener_registro(pcb* contexto, char* nombre_registro) {
    if (strcmp(nombre_registro, "AX") == 0) {
        return contexto->registros->ax;
    } else if (strcmp(nombre_registro, "BX") == 0) {
        return contexto->registros->bx;
    } else if (strcmp(nombre_registro, "CX") == 0) {
        return contexto->registros->cx;
    } else if (strcmp(nombre_registro, "DX") == 0) {
        return contexto->registros->dx;
    }else{
        return NULL;
    }
 }