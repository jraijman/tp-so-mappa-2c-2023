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
int enviar_marco_a_cpu(int conexion_cpu_memoria,int numero_pagina,t_marco* l_marco, int numero_marcos){

    int numero_de_marco = -1;
    //Tenemos que hacer un for para buscar el numero marco en el numero de pagina
    for(int i = 0; i < numero_marcos ; i++){
        if(l_marco[i].ocupado && l_marco[i].pid == numero_pagina){
            numero_de_marco = l_marco[i].num_marco;
        }
    }
    if(numero_de_marco = -1){
        perror("No se encontro numero de marco");
        return -1;
    }
    if(send(conexion_cpu_memoria,&numero_de_marco,sizeof(int),0)< 0){
        perror("Error al enviar el numero de marco a la CPU");
        return -1;
    }
    return numero_de_marco;
}