#include "main.h"

int main(int argc, char* argv[]) {

    
    // CONFIG y logger
    levantar_config("kernel.config");
    iniciar_listas();
    iniciar_semaforos();

    // Conecto kernel con cpu, memoria y filesystem
	fd_cpu_dispatch = -1,fd_cpu_interrupt = -1, fd_memoria = -1, fd_filesystem = -1;
	if (!generar_conexiones()) {
		log_error(logger_kernel, "Alguna conexion falló");
		// libero conexiones, log y config
        terminar_programa(logger_kernel, config);
        //liberar_conexion(fd_cpu_dispatch);
        //liberar_conexion(fd_cpu_interrupt);
        liberar_conexion(fd_memoria);
        //liberar_conexion(fd_filesystem);
		exit(1);
	}
    
    //mensajes de prueba
    //enviar_mensaje("Hola, Soy Kernel!", fd_filesystem);
	enviar_mensaje("Hola, Soy Kernel!", fd_memoria);
	//enviar_mensaje("Hola, Soy Kernel!", fd_cpu);
    

    // inicio hilos
    iniciar_hilos();
    while ((1))
    {
        /* code */
    }
   
    // libero conexiones, log y config
    terminar_programa(logger_kernel, config);
    //liberar_conexion(fd_cpu_dispatch);
    //liberar_conexion(fd_cpu_interrupt);
    liberar_conexion(fd_memoria);
    //liberar_conexion(fd_filesystem);
}

bool generar_conexiones() {
	pthread_t conexion_filesystem;
	pthread_t conexion_cpu_dispatch;
    pthread_t conexion_cpu_interrupt;

	fd_filesystem = crear_conexion(logger_kernel,"FILESYSTEM",ip_filesystem,puerto_filesystem);
	//pthread_create(&conexion_filesystem, NULL, (void*) procesar_conexion_fs, (void*) &fd_filesystem);
	//pthread_detach(conexion_filesystem);


	fd_cpu_dispatch = crear_conexion(logger_kernel,"CPU_DISPATCH",ip_cpu,puerto_cpu_dispatch);
    //pthread_create(&conexion_cpu_dispatch, NULL, (void*) procesar_conexion_dispatch, (void*) &fd_cpu_dispatch);
	//pthread_detach(conexion_cpu_dispatch);
    fd_cpu_interrupt = crear_conexion(logger_kernel,"CPU_INTERRUPT",ip_cpu,puerto_cpu_interrupt);
	//pthread_create(&conexion_cpu_interrupt, NULL, (void*) procesar_conexion_interrupt, (void*) &fd_cpu_interrupt);
	//pthread_detach(conexion_cpu_interrupt);
    
    
    fd_memoria = crear_conexion(logger_kernel,"MEMORIA",ip_memoria,puerto_memoria);

	return fd_filesystem != -1 && fd_cpu_dispatch != -1 && fd_cpu_interrupt != -1 && fd_memoria != -1;
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
    recursos =  config_get_array_value(config, "RECURSOS");
    char** instancias = string_array_new();
	instancias = config_get_array_value(config, "INSTANCIAS_RECURSOS");
	instancia_recursos = string_to_int_array(instancias);
	string_array_destroy(instancias);
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
            if (parametros[0] != NULL || parametros[1] != NULL || parametros[2] != NULL){
                 iniciar_proceso(parametros[0], parametros[1], parametros[2]);
            }else printf("error en algun parametro \n");
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
    pthread_create(&hilo_plan_largo, NULL, planif_largo_plazo, NULL);

    //creo hilo para la planificacion a corto plazo
    pthread_create(&hilo_plan_corto, NULL, planif_corto_plazo, NULL);

    //creo hilo que espera mensaje de CPU de finalizar proceso
    pthread_create(&hilo_cpu_exit, NULL, finalizar_proceso_cpu, NULL);

    pthread_detach(hilo_consola);
    pthread_detach(hilo_plan_largo);
    pthread_detach(hilo_plan_corto);
    pthread_detach(hilo_cpu_exit);

}

void iniciar_listas(){
	cola_new = queue_create();
	cola_ready = queue_create();
	cola_exec = queue_create(); 
	cola_block = queue_create(); 
	cola_exit = queue_create();

    lista_recursos = inicializar_recursos();
}

void iniciar_proceso(char * path, char* size, char* prioridad)
{
    //generar estructura PCB
    pcb* proceso = malloc(sizeof(pcb));
    proceso->pid = contador_proceso;
    proceso->size = atoi(size);
    proceso->pc = 0;//arranca desde la instruccion 0
    proceso->prioridad = atoi(prioridad);
    proceso->estado = NEW;
    proceso->registros = malloc(sizeof(t_registros));
    proceso->registros->ax = malloc(sizeof(int));
	proceso->registros->bx = malloc(sizeof(int));
	proceso->registros->cx = malloc(sizeof(int));
	proceso->registros->dx = malloc(sizeof(int));
    proceso->registros->ax =0;
    proceso->registros->bx =0;
    proceso->registros->cx =0;
    proceso->registros->dx =0;
    proceso->archivos = list_create();
    //memcpy(proceso->path, path, sizeof(path));
    //ver si falta el tiempo para manejar el quantum
    agregar_a_new(proceso);
    //MANDAR MENSAJE A MEMORIA- CREACION DE PROCESO-
    send_inicializar_proceso(proceso, fd_memoria);
    //le sumo uno al contador que funciona como id de proceso
    contador_proceso++;

}

void finalizar_proceso(char * pid)
{
    pid_a_eliminar = atoi(pid);
    pcb* procesoAEliminar = NULL;

    procesoAEliminar = buscar_pcb_cola(cola_new, pid_a_eliminar);
    if(procesoAEliminar == NULL){
        procesoAEliminar = buscar_pcb_cola(cola_ready, pid_a_eliminar);
    }
    else if(procesoAEliminar == NULL){
        procesoAEliminar = buscar_pcb_cola(cola_exec, pid_a_eliminar);
    }
    else if(procesoAEliminar == NULL){
        procesoAEliminar = buscar_pcb_cola(cola_block, pid_a_eliminar);
    }

    if(procesoAEliminar == NULL){
        printf("error al finalizar proceso \n");
    }
    else{
        // hacer log de fin de proceso
        log_info(logger_kernel, "Finaliza el proceso %d - Motivo: <SUCCESS - CONSOLA>", procesoAEliminar->pid);
        //borrar todo lo que corresponde a ese proceso
    }
    
    
    
    
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
    cambiar_estado(proceso, NEW);
    pthread_mutex_lock(&mutex_new);
	queue_push(cola_new, proceso);
    pthread_mutex_unlock(&mutex_new);
    log_info(logger_kernel, "Se crea el proceso %d en NEW", proceso->pid);
    sem_post(&cantidad_new);
}

pcb* sacar_de_new(){
	sem_wait(&cantidad_new);
	pthread_mutex_lock(&mutex_new);
	pcb *proceso = queue_pop(cola_new);
	pthread_mutex_unlock(&mutex_new);
	return proceso;
}


void agregar_a_ready(pcb* proceso){
    cambiar_estado(proceso, READY);
    pthread_mutex_lock(&mutex_ready);
    queue_push(cola_ready, proceso);
	pthread_mutex_unlock(&mutex_ready);
	log_info(logger_kernel, "Cola Ready %s: %s",algoritmo_planificacion, list_to_string(pid_lista_ready(cola_ready->elements)));
	sem_post(&cantidad_ready);
}

void agregar_a_exec(pcb* proceso){
    cambiar_estado(proceso, EXEC);
    pthread_mutex_lock(&mutex_exec);
    queue_push(cola_exec, proceso);
	pthread_mutex_unlock(&mutex_exec);
	sem_post(&cantidad_exec);
}

void agregar_a_exit(pcb* proceso){
    const char* estado_anterior = estado_proceso_a_char(proceso->estado);
	pthread_mutex_lock(&mutex_exit);
    proceso->estado = 5;
    log_info(logger_kernel, "PID: %d - Estado Anterior: %s - Estado Actual: %s",proceso->pid,estado_anterior,estado_proceso_a_char(proceso->estado));
	list_add(cola_exit->elements, proceso);
	pthread_mutex_unlock(&mutex_exit);
	sem_post(&cantidad_exit);
}


//-----------------------------------------------------------------------
//             PLANIFICADORES


void* planif_largo_plazo(void* args){
    //falta agregar lo de finalizar cuando recibe un exit
    while(1){
        sem_wait(&cantidad_multiprogramacion);
        pcb* proceso = sacar_de_new();
        agregar_a_ready(proceso);
    }
}
void* planif_corto_plazo(void* args){
    while(1){
        //semaforo para que no haya mas de un proceso en exec, cuando se bloquea o termina el proceso, hacer signal
        sem_wait(&puedo_ejecutar_proceso);
        sem_wait(&cantidad_ready);
        
        if(strcmp(algoritmo_planificacion,"FIFO")==0){
            pcb* procesoAEjecutar = obtenerSiguienteFIFO();
            if(procesoAEjecutar != NULL) {
                agregar_a_exec(procesoAEjecutar);
                //mandar proceso a CPU
                send_pcb(procesoAEjecutar, fd_cpu_dispatch);
                //espero a  pcb actualizado               
                }
            }
        else if(strcmp(algoritmo_planificacion,"PRIORIDADES")==0){
            //TIENE DESALOJO
            pcb *procesoAEjecutar = obtenerSiguientePRIORIDADES();
            if(procesoAEjecutar != NULL) {
                agregar_a_exec(procesoAEjecutar);
                //mandar proceso a CPU
                send_pcb(procesoAEjecutar, fd_cpu_dispatch);
                 //espero a  pcb actualizado   
                 // si en ready hay uno con mayor prioridad, cambiarlo

                }

            }
        else if(strcmp(algoritmo_planificacion,"RR")==0){
            //procesoPlanificado = obtenerSiguienteRR();
            }
    }
}


pcb* obtenerSiguienteFIFO(){
	pcb* procesoPlanificado = NULL;
	pthread_mutex_lock(&mutex_ready);
	procesoPlanificado = queue_pop(cola_ready);
    pthread_mutex_unlock(&mutex_ready);
	return procesoPlanificado;
}

pcb* obtenerSiguienteRR(){
	
}

bool cmp(void *a, void *b) { 
    pcb* pa = a;
    pcb* pb = b;
    int intA = pa->prioridad; int intB = pb->prioridad; 
    return intA < intB; 
}

pcb* obtenerSiguientePRIORIDADES(){
	pcb* procesoPlanificado = NULL;
    if (list_size(cola_ready->elements) > 0){
    pthread_mutex_lock(&mutex_ready);
    t_list* lista_ordenada = list_sorted(cola_ready->elements,cmp);
	procesoPlanificado = list_remove(lista_ordenada, 0);
    list_remove_element(cola_ready->elements,procesoPlanificado);
    pthread_mutex_unlock(&mutex_ready);
    }
	return procesoPlanificado;
}

//---------------------------------------------------------------
//              MANEJO DE RECURSOS

void manejar_wait(pcb* pcb, char* recurso){
	t_recurso* recursobuscado= buscar_recurso(recurso);
	if(recursobuscado->encontrado == -1){
		log_error(logger_kernel, "No existe el recurso solicitado: %s", recurso);
		//pcb->contexto_de_ejecucion->motivo_exit = RECURSO_INEXISTENTE;
		//safe_pcb_add(cola_exit,pcb, &mutex_cola_exit);
		//sem_post(&sem_exit);
		//sem_post(&sem_exec);
	}else{
		recursobuscado->instancias --;
		log_info(logger_kernel,"PID: %d - Wait: %s - Instancias: %d", pcb->pid,recurso,recursobuscado->instancias);
		if(recursobuscado->instancias < 0){
			cambiar_estado(pcb, BLOCK);
			log_info(logger_kernel,"PID: %d - Bloqueado por: %s", pcb->pid,recurso);
			//safe_pcb_add(recursobuscado->cola_block_asignada, pcb, &recursobuscado->mutex_asignado);
			//sem_post(&sem_exec);
		}else{
			//safe_pcb_add(cola_exec, pcb, &mutex_cola_exec);
			//send_contexto_ejecucion(pcb->contexto_de_ejecucion, fd_cpu);
		}
	}
}
t_recurso* buscar_recurso(char* recurso){
	int largo = list_size(lista_recursos);
	t_recurso* recursobuscado;
	for(int i = 0; i < largo; i++){
		recursobuscado = list_get(lista_recursos, i);
		if (strcmp(recursobuscado->recurso, recurso) == 0){
			return recursobuscado;
		}
	}
	recursobuscado->encontrado=-1;
	return recursobuscado;
}

//---------------------------------------------------------------

void * finalizar_proceso_cpu(void * args){
    while(1){
        /*pcbDesalojado* pcbDes;
        if (recv_pcbDesalojado(conexion_dispatch, pcbDes)){
            if (strcmp(pcbDes->instruccion, "EXIT") == 0){
                log_info(logger_kernel, "Instrucción EXIT - Proceso ha finalizado su ejecución");
                cambiar_estado_pcb(pcbDes->contexto, 5);
                agregar_a_exit(pcbDes->contexto); 
                //no entiendo de donde sale el proceso
                pthread_mutex_lock(&mutex_exec);
                list_remove(lista_exec,0);
                pthread_mutex_unlock(&mutex_exec);
                sem_wait(&cantidad_exec)
                //aviso a memoria que liubere

                //libero recursos del pcb
                

            }
            return true;
        } else {
            log_error(logger_kernel, "Error al recibir la instrucción de finalizar proceso de CPU");
            return false;
        }*/
    }
}


void iniciar_semaforos(){
    sem_init(&cantidad_multiprogramacion,0,grado_multiprogramacion);
    sem_init(&cantidad_new,0,0);
    sem_init(&cantidad_ready,0,0);
    sem_init(&cantidad_exit,0,0);
    sem_init(&cantidad_exec,0,0);
    sem_init(&puedo_ejecutar_proceso,0,1);



    //mutex de colas de planificacion
    pthread_mutex_init(&mutex_new, NULL);
	pthread_mutex_init(&mutex_ready, NULL);
	pthread_mutex_init(&mutex_exec, NULL);
    pthread_mutex_init(&mutex_block, NULL);
	pthread_mutex_init(&mutex_exit, NULL);
    

}



//------------------AUXILIARES----------------------------------------------------------------------------

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

t_list* inicializar_recursos(){
	t_list* lista = list_create();
	int cantidad_recursos = string_array_size(recursos);
	for(int i = 0; i < cantidad_recursos; i++){
		char* string = recursos[i];
		t_recurso* recurso = malloc(sizeof(t_recurso));
		recurso->recurso = malloc(sizeof(char) * strlen(string) + 1);
		strcpy(recurso->recurso, string);
		//t_list* cola_block = list_create();
		recurso->encontrado = 0;
		recurso->instancias = instancia_recursos[i];
		//recurso->cola_block_asignada = cola_block;
		pthread_mutex_init(&recurso->mutex, NULL);
		list_add(lista, recurso);
	}
	return lista;
}

bool remover(pcb* element) {
    return (element->pid == pid_a_eliminar);
} 

pcb* buscar_pcb_cola(t_queue* cola, int id){
    t_list* lista = cola->elements;

      
    return  list_remove_by_condition(lista, (void *)remover);
}


t_list *pid_lista_ready(t_list *list){
	t_list* lista_de_pids = list_create();
    for (int i = 0; i < list_size(list); i++){
        pcb* pcb = list_get(list, i);
        list_add(lista_de_pids, &(pcb->pid));
    }
    return lista_de_pids;
}

char *estado_proceso_a_char(estado_proceso estado) {
    switch (estado) {
        case NEW:
            return "NEW";
        case READY:
            return "READY";
        case EXEC:
            return "EXEC";
        case BLOCK:
            return "BLOCK";
        case EXIT_ESTADO:
            return "EXIT";
        default:
            return "NO HAY ESTADO";
    }
}

void cambiar_estado(pcb *pcb, estado_proceso nuevo_estado) {
	if(pcb->estado != nuevo_estado){
		char *nuevo_estado_string =  strdup(estado_proceso_a_char(nuevo_estado));
		char *estado_anterior_string =  strdup(estado_proceso_a_char(pcb->estado));
		pcb->estado = nuevo_estado;
		log_info(logger_kernel, "PID: %d - Estado Anterior: %s - Estado Actual: %s", pcb->pid, estado_anterior_string, nuevo_estado_string);
		free(estado_anterior_string);
		free(nuevo_estado_string);
	}
}

int* string_to_int_array(char** array_de_strings){
	int count = string_array_size(array_de_strings);
	int *numbers = malloc(sizeof(int) * count);
	for(int i = 0; i < count; i++){
		int num = atoi(array_de_strings[i]);
		numbers[i] = num;
	}
	return numbers;
}
