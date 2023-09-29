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
void leerYEnviarInstrucciones(FILE *archivo, int pid, int conexion_memoria)
{
    char linea[256];
    Instruccion structinstruccion;
    while (fgets(linea, sizeof(linea), archivo) != NULL)
    {
        char *instruccion[] = string_split(linea, " ");
        strcpy(structinstruccion.opcode,instruccion[0]);
        strcpy(structinstruccion.operando1,instruccion[1]);
        strcpy(structinstruccion.operando2,instruccion[2]);
        if (!send_instruccion(conexion_memoria, pid, linea))
        {
            // Manejar error de envío a memoria
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
    pcb contexto;
    int bytes_recibidos = 0;
    int pid;
    Proceso *tabla_proceso;
    while (server_escuchar_memoria(logger_memoria, "MEMORIA", fd_memoria))
    {
        int bytes_recibidos = 0;
        int bytes_esperados = sizeof(int); // Tamaño esperado de los datos

        if (bytes_esperados > 0)
        {
            while (bytes_recibidos < bytes_esperados)
            {
                int bytes = recv(cliente_memoria, (char *)&contexto + bytes_recibidos, bytes_esperados - bytes_recibidos, 0);

                if (bytes <= 0)
                {
                    // Manejo de error o desconexión del cliente
                    break;
                }

                bytes_recibidos += bytes;
            }
        }
        if (bytes_recibidos == sizeof(pcb))
        {
        // Si recibimos un PCB, cargamos el proceso en la tabla
        insertarProcesoOrdenado(tabla_proceso, contexto.pid, contexto.estado, "PATH_INSTRUCCIONES");
        }
        else if (bytes_recibidos == sizeof(int)) {
        // Si recibimos un PID, lo buscamos en la tabla y leemos las instrucciones
        int pid_solicitado = *(int*)&contexto;
        Proceso *procesoactual = buscarProcesoPorPID(tabla_proceso, pid_solicitado);
        if (procesoactual != NULL) {
            FILE *archivo = fopen(procesoactual->rutaArchivo, "r");
            if (archivo != NULL) {
                leerYEnviarInstrucciones(archivo, pid_solicitado, cliente_memoria);
                fclose(archivo);
            } else {
            // Manejar error de apertura de archivo
        }
        } else {
        // Manejar error: PID no encontrado en la tabla de procesos
        }
}
    }

    // CIERRO LOG Y CONFIG y libero conexion
    terminar_programa(logger_memoria, config);
    liberar_conexion(conexion_memoria_filesystem);
}
