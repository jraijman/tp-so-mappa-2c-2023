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
bool leerYEnviarInstruccion(FILE *archivo, pcb proceso, int conexion_memoria) {
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

        if (!send_instruccion(conexion_memoria, instruccion)) {
            log_error(logger_memoria, "Error al enviar la instrucción");
        }
    }
}
int main(int argc, char *argv[])
{

    // CONFIG y logger
    levantar_config("memoria.config");
    // inicio servidor de escucha
    fd_memoria = iniciar_servidor(logger_memoria, "MEMORIA", NULL, puerto_escucha);

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
    while (server_escuchar_memoria(logger_memoria, "MEMORIA", fd_memoria)) {
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
                    pcb *procesoactual = buscarProcesoPorPID(tabla_proceso, pid_solicitado);
                    if (procesoactual != NULL) {
                        FILE *archivo = fopen(procesoactual->path, "r");
                        if (archivo != NULL) {
                            if (leerYEnviarInstruccion(archivo,procesoactual, fd_cpu_dispatch)) {
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
    }
   
    // CIERRO LOG Y CONFIG y libero conexion
    terminar_programa(logger_memoria, config);
    liberar_conexion(conexion_memoria_filesystem);
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
void eliminar_proceso_memoria(int pid) {
    
    t_list* marcos_asignados = obtenerMarcosAsignados(pid);

    
    for (int i = 0; i < list_size(marcos_asignados); i++) {
        t_marco* marco = list_get(marcos_asignados, i);
        liberar_marco(marco);
    }
    list_destroy(marcos_asignados);
}
t_list* obtenerMarcosAsignados(pid){
    t_list* marcos_asignados = list_filter(l_marco, (void*)marco_asignado_a_este_proceso);
    return marcos_asignados;
}

bool marco_asignado_a_este_proceso(int pid,t_marco * marco) {
    return marco->pid == pid;
  }
/*
TODO:
INSTRUCCIONES SE ENVIA 1 SOLA, LA PETICION ES ITERATIVA DEL LADO CPU POR LO CUAL VAN A IR LLEGANDO PETICIONES DE INSTRUCCION (INDEXAS CON EL PROGRAM COUNTER PARA BAJAR LA LINEA QUE NECESITO)
*/
