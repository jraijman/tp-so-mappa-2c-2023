#include "main.h"

int main(int argc, char* argv[]) {

    
    // CONFIG y logger
    levantar_config("kernel.config");
    iniciar_listas();
    iniciar_semaforos();



    // conexiones a cpu LLAMAR CUANDO SE CREA O INTERRUMPE UN PROCESO, NO DESDE EL INICIO
    conexion_dispatch = crear_conexion(logger_kernel,"CPU_DISPATCH",ip_cpu,puerto_cpu_dispatch);
    conexion_interrupt = crear_conexion(logger_kernel,"CPU_INTERRUPT",ip_cpu,puerto_cpu_interrupt);

    //conexion a memoria
    conexion_memoria = crear_conexion(logger_kernel,"MEMORIA",ip_memoria,puerto_memoria);
    
    //conexion a FileSystem
    conexion_fileSystem = crear_conexion(logger_kernel,"FILESYSTEM",ip_filesystem,puerto_filesystem);

    // inicio hilos
    iniciar_hilos();
    while ((1))
    {
        /* code */
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
    recursos = config_list_to_t_list(config, "RECURSOS"); //es una t_list
    instancia_recursos = config_list_to_t_list(config, "INSTANCIAS_RECURSOS"); //es una t_list
    grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION_INI");
    
    log_info(logger_kernel,"Config cargada");
}

void * leer_consola(void * arg)
{
    while (true){
        int leido;
        //LISTADO DE FUNCIONES
        printf("\n\n1-INICIAR_PROCESO \n");
        printf("2-FINALIZAR_PROCESO\n");
        printf("3-DETENER_PLANIFICACION \n");
        printf("4-INICIAR_PLANIFICACION \n");
        printf("5-MULTIPROGRAMACION\n");
        printf("6-PROCESO_ESTADO \n\n");
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
}

void iniciar_hilos(){
    //creo hilo para la consola
    pthread_create(&hilo_consola, NULL, leer_consola, NULL);

    //creo hilo para pasar a cola de ready
    pthread_create(&hilo_new_ready, NULL, pasar_new_a_ready, NULL);

    //creo hilo para la planificacion a corto plazo
    pthread_create(&hilo_plan_corto, NULL, planif_corto_plazo, NULL);

    //creo hilo que espera mensaje de CPU de finalizar proceso
    pthread_create(&hilo_cpu_exit, NULL, finalizar_proceso_cpu, NULL);




    pthread_detach(hilo_consola);
    pthread_detach(hilo_new_ready);
    pthread_detach(hilo_plan_corto);
    pthread_detach(hilo_cpu_exit);

}

void iniciar_listas(){

//ver si no hay que usar colas
	cola_new = queue_create();
	lista_ready = list_create();
	lista_exec = list_create(); //preguntar
	lista_block = list_create(); //preguntar
	lista_exit = list_create(); //preguntar
}

void iniciar_proceso(char * path, char* size, char* prioridad)
{
        
    //generar estructura PCB
    pcb* proceso = malloc(sizeof(pcb));

    proceso->pid = contador_proceso;
    proceso->size = atoi(size);
    proceso->pc = 0;//arranca desde la instruccion 0
    proceso->prioridad = atoi(prioridad);
    proceso->estado = 0;
    proceso->registros.ax =0;
    proceso->registros.bx =0;
    proceso->registros.cx =0;
    proceso->registros.dx =0;
    //proceso->archivos =

    agregar_a_new(proceso);

    //MANDAR MENSAJE A MEMORIA- CREACION DE PROCESO--------------------------------
    //send_creo_proceso(conexion_memoria, proceso);

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

//-----------------------------------OPERACIONES DE LISTAS/COLAS-------------------------------------------
void agregar_a_new(pcb* proceso) {
    int estado_anterior = proceso->estado;

    //uso un mutex por seccion critica
    pthread_mutex_lock(&mutex_new);
	queue_push(cola_new, proceso);
    pthread_mutex_unlock(&mutex_new);
    log_info(logger_kernel, "Se crea el proceso %d en NEW", proceso->pid);
    proceso->estado = NEW;
    log_info(logger_kernel, "PID: %d - Estado Anterior: %d - Estado Actual: %d",proceso->pid,estado_anterior,proceso->estado);
    sem_post(&cantidad_new);

}
pcb* sacar_de_new(){

    //me fijo que hay en new para sacar y uso un mutex por seccion critica
	sem_wait(&cantidad_new);
	pthread_mutex_lock(&mutex_new);
	pcb* proceso = queue_pop(cola_new);
	pthread_mutex_unlock(&mutex_new);
	return proceso;
}


void agregar_a_ready(pcb* proceso){
    int estado_anterior = proceso->estado;
	pthread_mutex_lock(&mutex_ready);
    proceso->estado = 2;
    log_info(logger_kernel, "PID: %d - Estado Anterior: %d - Estado Actual: %d",proceso->pid,estado_anterior,proceso->estado);
	list_add(lista_ready, proceso);
    // debe loguear los pids que hay en la cola
	log_info(logger_kernel, "Cola Ready %s: %s",algoritmo_planificacion,"NO ANDA LA LISTA");

	pthread_mutex_unlock(&mutex_ready);
	sem_post(&cantidad_ready);
}




//-----------------------------------------------------------------------

void* pasar_new_a_ready(void* args){
    while(1){
        //verifica que el grado de multiprogramacion permite varios procesos en ready
        sem_wait(&cantidad_multiprogramacion);
        pcb* proceso = sacar_de_new();
        agregar_a_ready(proceso);
        //ENVIAR MENSAJE A MEMORIA -----> ver si va aca
        send_pcb(conexion_memoria, proceso);
    }
}

void* planif_corto_plazo(void* args){
    while(1){
        pthread_mutex_lock(&mutex_exec);
        pcb* procesoAEjecutar = obtenerSiguienteFIFO();
    
        if(procesoAEjecutar != NULL) {
        printf("el pcb es %d", procesoAEjecutar->pid);
        int estado_anterior = procesoAEjecutar->estado;
        //procesoAEjecutar le cambiamos el estado a 3
        log_info(logger_kernel, "PID: %d - Estado Anterior: %d - Estado Actual: %d",procesoAEjecutar->pid,estado_anterior,procesoAEjecutar->estado);
        //mandar proceso a CPU
        //esperamos a bloqueo o a exit
        //si se bloquea, cambiamos el estado a 4 y lo metemos en bloqueo
        //si tira el exit, hacer un signal al sem_ready y correr finalizar proceso

        pthread_mutex_unlock(&mutex_exec);
        }
    }
}

pcb* obtenerSiguienteFIFO(){

	pcb* procesoPlanificado = NULL;

	pthread_mutex_lock(&mutex_ready);
    if (list_size(lista_ready) > 0){
	procesoPlanificado = list_remove(lista_ready, 0);
    }
    pthread_mutex_unlock(&mutex_ready);

	return procesoPlanificado;
}

//---------------------------------------------------------------
void * finalizar_proceso_cpu(void * args){

}


void iniciar_semaforos(){
    sem_init(&cantidad_multiprogramacion,0,grado_multiprogramacion);
    sem_init(&cantidad_new,0,0);
    sem_init(&cantidad_ready,0,0);

    //mutex de colas de planificacion
    pthread_mutex_init(&mutex_new, NULL);
	pthread_mutex_init(&mutex_ready, NULL);
	pthread_mutex_init(&mutex_exec, NULL);
    pthread_mutex_init(&mutex_block, NULL);
	pthread_mutex_init(&mutex_exit, NULL);
    

}

t_list* config_list_to_t_list(t_config* config, char* nombre){
    t_list* lista = list_create();
    char** array_auxiliar;
    char* string_auxiliar;
    int i = 0;
    array_auxiliar = config_get_array_value(config, nombre);
    while (array_auxiliar[i] != NULL) {
        string_auxiliar = array_auxiliar[i];
        list_add(lista, string_auxiliar);
        i++;
    }
    return lista;
}

//------------------------------
//NO ANDA
//imprimir pid de pćb lista de ready
/*char* pid_lista_ready (t_list* lista){
    char pids [200] = "";
	int size =  list_size(lista);
    strcpy(pids, "[ ");
	for(int i = 0; i < size; i++) {
        pcb * p= list_get(lista, i);
        sprintf( &pids[ strlen(pids) ],  "%d, ", p->pid );
    }
    strcat(pids, " ]");
    return pids;
    
}
*/
