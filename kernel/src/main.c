#include "main.h"

int contador_proceso = 0;

int main(int argc, char* argv[]) {

    // CONFIG y logger
    levantar_config("kernel.config");

    // conexiones a cpu LLAMAR CUANDO SE CREA O INTERRUMPE UN PROCESO, NO DESDE EL INICIO
    conexion_dispatch = crear_conexion(logger_kernel,"CPU_DISPATCH",ip_cpu,puerto_cpu_dispatch);
    conexion_interrupt = crear_conexion(logger_kernel,"CPU_INTERRUPT",ip_cpu,puerto_cpu_interrupt);

    //conexion a memoria
    conexion_memoria = crear_conexion(logger_kernel,"MEMORIA",ip_memoria,puerto_memoria);
    
    //conexion a FileSystem
    conexion_fileSystem = crear_conexion(logger_kernel,"FILESYSTEM",ip_filesystem,puerto_filesystem);

    //mando mensaje de prueba
    send_aprobar_operativos(conexion_fileSystem, 1, 14);

    while(true){
        leer_consola();
    }
    


    // libero conexiones, log y config
    terminar_programa(logger_kernel, config);
    liberar_conexion(conexion_dispatch);
    liberar_conexion(conexion_interrupt);
    liberar_conexion(conexion_memoria);
    liberar_conexion(conexion_fileSystem);
}

void levantar_config(char* ruta){
    logger_kernel = iniciar_logger("kernel.log", "KERNEL:");

    config = iniciar_config(ruta);
    ip_memoria = config_get_string_value(config,"IP_MEMORIA");
    ip_cpu = config_get_string_value(config,"IP_CPU");
    ip_filesystem = config_get_string_value(config,"IP_FILESYSTEM");
    puerto_filesystem= config_get_string_value(config,"PUERTO_FILESYSTEM");
    puerto_memoria= config_get_string_value(config,"PUERTO_MEMORIA");
    puerto_cpu_dispatch = config_get_string_value(config,"PUERTO_CPU_DISPATCH");
    puerto_cpu_interrupt = config_get_string_value(config,"PUERTO_CPU_INTERRUPT");
    algoritmo_planificacion = config_get_string_value(config,"ALGORITMO_PLANIFICACION");
    quantum = config_get_string_value(config,"QUANTUM");
    //es una lista VER
    recursos = config_get_string_value(config,"RECURSOS");
    //es una lista VER
    instancia_recursos = config_get_string_value(config,"INSTANCIAS_RECURSOS");
    grado_multiprogramacion = config_get_string_value(config,"GRADO_MULTIPROGRAMACION_INI");
    
    log_info(logger_kernel,"Config cargada");
}

void leer_consola()
{
	int leido;
	//LISTADO DE FUNCIONES
    printf("1-INICIAR_PROCESO [PATH] [SIZE] [PRIORIDAD] \n");
    printf("2-FINALIZAR_PROCESO [PID] \n");
    printf("3-DETENER_PLANIFICACION \n");
    printf("4-INICIAR_PLANIFICACION \n");
    printf("5-MULTIPROGRAMACION [NIVEL] \n");
    printf("6-PROCESO_ESTADO \n");
    scanf("%d", &leido);
    switch (leido)
    {        
    // INICIAR_PROCESO [PATH] [SIZE] [PRIORIDAD]
    case 1:
        //leo de la consola los parametros separados por espacio, y los mando a la funcion
        char * leido = readline("Ingrese PATH SIZE PRIORIDAD: ");
        //no se si usar free
        char * parametros[3];
        int i = 0;
        char *p = strtok (leido, " ");
        while (p)
            {
                parametros[i++] = p;
                p = strtok (NULL, " ");
            }
        iniciar_proceso(parametros[0], parametros[1], parametros[2]);
        free(leido);  
        free (p);
        break;
    //FINALIZAR_PROCESO [PID]
    case 2:
        char * pid = readline("Ingrese PID: ");
        finalizar_proceso(pid);
        free (pid);
        break;
    //DETENER_PLANIFICACION
    case 3:
        detener_planificacion();
        break;
    //INICIAR_PLANIFICACION
    case 4:
        iniciar_planificacion();
        break;
    //MULTIPROGRAMACION [VALOR]
    case 5:
        multiprogramacion(/*valor*/);
        break;
    //PROCESO_ESTADO
    case 6:
        proceso_estado();
        break;
    default:
        printf("Comando incorrecto \n");
        break;
    }
}

void iniciar_proceso(char * path, char* size, char* prioridad)
{
    printf("entre a iniciar proceso \n");
    int isize = *size - '0';
    int iprioridad = *prioridad - '0';
    
    //generar estructura PCB
    pcb* pcb_proceso = malloc(sizeof(pcb));

    pcb_proceso->pid = contador_proceso;
    pcb_proceso->tamanio = isize;
    pcb_proceso->pc = 0;//arranca desde la instruccion 0
    pcb_proceso->prioridad = iprioridad;
    pcb_proceso->estado = 1;//arranca en NEW
    //pcb_proceso->registros =
    //pcb_proceso->archivos =



    //hacer log de crear proceso
    //le sumo uno al contador que funciona como id de proceso
    contador_proceso++;

}
void finalizar_proceso(char * pid)
{
    printf("entre a finalizar proceso\n");
    // hacer log de fin de proceso
}
void detener_planificacion()
{
    printf("detengo planificacion \n");
}
void iniciar_planificacion()
{
    printf("inicio planificacion \n");
}
void multiprogramacion(/*char* grado_multiprogramacion*/)
{
    printf("multiprogramacion \n");
}
void proceso_estado()
{
    printf("proceso_estado \n");
}
