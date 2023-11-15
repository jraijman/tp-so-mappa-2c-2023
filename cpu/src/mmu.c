#include "mmu.h"
#include <sys/socket.h>

void traducir(Instruccion *instruccion, Direccion *direccion,pcb* contexto) {
    if(strcmp(instruccion->opcode,"MOV_OUT"))
    {
        direccion->direccionLogica=atoi(instruccion->operando1);
            send_direccion(conexion_cpu_memoria, direccion);
            recv_direccion(conexion_cpu_memoria, direccion);
    }else{
        direccion->direccionLogica=atoi(instruccion->operando2);
            send_direccion(conexion_cpu_memoria, direccion);
            recv_direccion(conexion_cpu_memoria, direccion);
    }
    if(direccion->pageFault){
        send_pcbDesalojado(contexto,"PAGEFAULT",string_itoa(direccion->numeroMarco),fd_cpu_dispatch,logger_cpu);
    }
    else{
        direccion->numeroMarco=direccion->direccionLogica/direccion->tamano_pagina;
        direccion->desplazamiento=direccion->direccionLogica-(direccion->numeroMarco*direccion->tamano_pagina);
    }
}
