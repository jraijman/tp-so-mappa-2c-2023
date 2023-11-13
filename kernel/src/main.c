#include "main.h"

int main(int argc, char* argv[]) {

    
    // CONFIG y logger
    levantar_config("kernel.config");
    iniciar_listas();
    iniciar_semaforos();

    // Conecto kernel con cpu, memoria y filesystem
	fd_cpu_dispatch = 0,fd_cpu_interrupt = 0, fd_memoria = 0, fd_filesystem = 0;
	if (!generar_conexiones()) {
		log_error(logger_kernel, "Alguna conexion falló");
		// libero conexiones, log y config
        terminar_programa(logger_kernel, config);
        liberar_conexion(fd_cpu_dispatch);
        liberar_conexion(fd_cpu_interrupt);
        liberar_conexion(fd_memoria);
        liberar_conexion(fd_filesystem);
		exit(1);
	}
    
    //mensajes de prueba
    enviar_mensaje("soy Kernel", fd_filesystem);
	enviar_mensaje("soy Kernel", fd_memoria);
    enviar_mensaje("soy Kernel", fd_cpu_dispatch);


    // inicio hilos
    iniciar_hilos();
    while ((1))
    {
        /* code */
    }
   
    // libero conexiones, log y config
    terminar_programa(logger_kernel, config);
    liberar_conexion(fd_cpu_dispatch);
    liberar_conexion(fd_cpu_interrupt);
    liberar_conexion(fd_memoria);
    liberar_conexion(fd_filesystem);
}

bool generar_conexiones() {
	fd_filesystem = crear_conexion(logger_kernel,"FILESYSTEM",ip_filesystem,puerto_filesystem);
	fd_cpu_dispatch = crear_conexion(logger_kernel,"CPU_DISPATCH",ip_cpu,puerto_cpu_dispatch);
    fd_cpu_interrupt = crear_conexion(logger_kernel,"CPU_INTERRUPT",ip_cpu,puerto_cpu_interrupt);
    fd_memoria = crear_conexion(logger_kernel,"MEMORIA",ip_memoria,puerto_memoria);
	return fd_filesystem != 0 && fd_cpu_dispatch != 0 && fd_cpu_interrupt != 0 && fd_memoria != 0;
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
    quantum = config_get_int_value(config,"QUANTUM");
    recursos =  config_get_array_value(config, "RECURSOS");
    char** instancias = string_array_new();
	instancias = config_get_array_value(config, "INSTANCIAS_RECURSOS");
	instancia_recursos = string_to_int_array(instancias);
	string_array_destroy(instancias);
    grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION_INI");
}

void * leer_consola(void * arg)
{
    while (1) {
        // Leer el comando ingresado por el usuario
        char* comando = readline("Ingrese un comando: ");

        // Verificar si el usuario ingresó algo
        if (comando == NULL || strlen(comando) == 0) {
            continue;
        }

        // Separar el comando y los argumentos
        char* token = strtok(comando, " ");
        char* arg1 = strtok(NULL, " ");
        char* arg2 = strtok(NULL, " ");
        char* arg3 = strtok(NULL, " ");

        // Ejecutar la función correspondiente según el comando ingresado
        if (strcmp(token, "INICIAR_PROCESO") == 0) {
            if (arg1 != NULL && arg2 != NULL && arg3 != NULL) {
                iniciar_proceso(arg1, arg2, arg3);
            } else {
                printf("Error en algun parametro\n");
            }
        } else if (strcmp(token, "FINALIZAR_PROCESO") == 0) {
            if (arg1 != NULL) {
                finalizar_proceso(arg1);
            } else {
                printf("Error en algun parametro\n");
            }
        } else if (strcmp(token, "DETENER_PLANIFICACION") == 0) {
            detener_planificacion();
        } else if (strcmp(token, "INICIAR_PLANIFICACION") == 0) {
            iniciar_planificacion();
        } else if (strcmp(token, "MULTIPROGRAMACION") == 0) {
            if (arg1 != NULL) {
                cambiar_multiprogramacion(arg1);
            } else {
                printf("Error en algun parametro\n");
            }
        } else if (strcmp(token, "PROCESO_ESTADO") == 0) {
            proceso_estado();
        } else {
            printf("Comando no reconocido\n");
        }
        // Liberar la memoria utilizada por el comando ingresado
        free(comando);
    }

    return 0;
}

void iniciar_hilos(){
    //creo hilo para la consola
    pthread_create(&hilo_consola, NULL, leer_consola, NULL);

     //creo hilo para pasar a cola de ready
    pthread_create(&hilo_plan_largo, NULL, planif_largo_plazo, NULL);

    //creo hilo para la planificacion a corto plazo
    pthread_create(&hilo_plan_corto, NULL, planif_corto_plazo, NULL);

    //creo hilo que espera mensaje de CPU de finalizar proceso
    //pthread_create(&hilo_cpu_exit, NULL, finalizar_proceso_cpu, NULL);

    pthread_detach(hilo_consola);
    pthread_detach(hilo_plan_largo);
    pthread_detach(hilo_plan_corto);
    //pthread_detach(hilo_cpu_exit);

}

void iniciar_listas(){
	cola_new = queue_create();
	cola_ready = queue_create();
	cola_exec = queue_create(); 
	cola_block = queue_create(); 
	cola_exit = queue_create();

    lista_recursos = inicializar_recursos();
}

//-------------------------------OPERACIONES DE CONSOLA-------------------------------------------

void iniciar_proceso(char * path, char* size, char* prioridad){   
    // Código para crear un nuevo proceso en base a un archivo dentro del file system de linux y dejar el mismo en el estado NEW
    pcb* proceso = crear_pcb(path, size, prioridad);
    agregar_a_new(proceso);
    send_inicializar_proceso(proceso, fd_memoria);
    manejar_recibir(fd_memoria);
}

void finalizar_proceso(char * pid){
    // Código para finalizar un proceso que se encuentre dentro del sistema
    // y liberar recursos, archivos y memoria
    int pid_int = atoi(pid);
    pcb* procesoAEliminar = NULL;

    procesoAEliminar = buscar_y_remover_pcb_cola(cola_new, pid_int, cantidad_new, mutex_new);
    if(procesoAEliminar == NULL){
        procesoAEliminar = buscar_y_remover_pcb_cola(cola_ready, pid_int, cantidad_ready, mutex_ready);
    }
    if(procesoAEliminar == NULL){
        procesoAEliminar = buscar_y_remover_pcb_cola(cola_exec, pid_int, cantidad_exec, mutex_exec);
    }
    if(procesoAEliminar == NULL){
        procesoAEliminar = buscar_y_remover_pcb_cola(cola_block, pid_int, cantidad_block, mutex_block);
    }

    if(procesoAEliminar == NULL){
         printf("No se encontró el proceso con PID %d\n", pid_int);
    }
    else{
        // hacer log de fin de proceso
        log_info(logger_kernel, "Finaliza el proceso %d - Motivo: CONSOLA", procesoAEliminar->pid);
        // Incrementar el semáforo cantidad_multiprogramacion para indicar que hay un lugar libre en el grado de multiprogramación
        if (procesoAEliminar->estado != NEW) {
            sem_post(&cantidad_multiprogramacion);
        }
        //Incrementar el semáforo puedo_ejecutar_proceso para indicar que hay un lugar libre en la cola de EXEC
        if (procesoAEliminar->estado == EXEC) {
            sem_post(&puedo_ejecutar_proceso);
        }
        //borrar todo lo que corresponde a ese proceso
        // Mandar a la cola de EXIT;
        agregar_a_exit(procesoAEliminar);
        //mandar mensaje a memoria para que libere
        send_terminar_proceso(procesoAEliminar->pid,fd_memoria);
        //recivo mensaje de fin
        manejar_recibir(fd_memoria);
        }
 
}
void detener_planificacion(){
    // Código para pausar la planificación de corto y largo plazo
    // y pausar el manejo de los motivos de desalojo de los procesos
    printf("detengo planificacion \n");
}
void iniciar_planificacion(){
    // Código para retomar la planificación de corto y largo plazo
    // en caso de que se encuentre pausada
    printf("inicio planificacion \n");
}
void cambiar_multiprogramacion(char* nuevo_grado_multiprogramacion){
    // Código para actualizar el grado de multiprogramación configurado inicialmente
    // por archivo de configuración y desalojar o finalizar los procesos si es necesario
    grado_multiprogramacion = atoi(nuevo_grado_multiprogramacion);
    printf("Se cambió el grado de multiprogramación a %d\n", grado_multiprogramacion);
}
void proceso_estado(){
    // Código para mostrar por consola el listado de los estados con los procesos que se encuentran dentro de cada uno de ellos
    printf("Procesos por estado:\n");
    printf("Procesos NEW: %s\n",list_to_string(pid_lista_ready(cola_new->elements)));
    printf("Procesos READY: %s\n",list_to_string(pid_lista_ready(cola_ready->elements)));
    printf("Procesos EXEC: %s\n",list_to_string(pid_lista_ready(cola_exec->elements)));
    printf("Procesos BLOCKED: %s\n",list_to_string(pid_lista_ready(cola_block->elements)));
    printf("Procesos EXIT: %s\n",list_to_string(pid_lista_ready(cola_exit->elements)));
    printf("\n");
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
pcb* sacar_de_exec(){
    sem_wait(&cantidad_exec);
    pthread_mutex_lock(&mutex_exec);
    pcb *proceso = queue_pop(cola_exec);
    pthread_mutex_unlock(&mutex_exec);
    return proceso;
}

void agregar_a_exit(pcb* proceso){
    cambiar_estado(proceso, EXIT_ESTADO);
    pthread_mutex_lock(&mutex_exit);
    queue_push(cola_exit, proceso);
	pthread_mutex_unlock(&mutex_exit);
	sem_post(&cantidad_exit);
}

void agregar_a_block(pcb* proceso){
    cambiar_estado(proceso, BLOCK);
    pthread_mutex_lock(&mutex_block);
    queue_push(cola_block, proceso);
    pthread_mutex_unlock(&mutex_block);
    sem_post(&cantidad_block);
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
    pthread_t hilo_interrupciones;
    while(1){
        //semaforo para que no haya mas de un proceso en exec, cuando se bloquea o termina el proceso, hacer signal
        sem_wait(&puedo_ejecutar_proceso);
        if(strcmp(algoritmo_planificacion,"FIFO")==0){
            pcb* procesoAEjecutar = obtenerSiguienteFIFO();
            if(procesoAEjecutar != NULL) {
                agregar_a_exec(procesoAEjecutar);
                send_pcb(procesoAEjecutar, fd_cpu_dispatch);
                manejar_recibir(fd_cpu_dispatch);
                }
            }
        else if(strcmp(algoritmo_planificacion,"PRIORIDADES")==0){
            //TIENE DESALOJO
            pcb *procesoAEjecutar = obtenerSiguientePRIORIDADES();
            if(procesoAEjecutar != NULL) {
                //hilo que controla si hay que mandar interrupcion
                pthread_create(&hilo_interrupciones, NULL, (void*) controlar_interrupcion_prioridades, NULL);
                agregar_a_exec(procesoAEjecutar);
                send_pcb(procesoAEjecutar, fd_cpu_dispatch);
                manejar_recibir(fd_cpu_dispatch);
                }

            }
        else if(strcmp(algoritmo_planificacion,"RR")==0){
            //TIENE DESALOJO
            pcb *procesoAEjecutar = obtenerSiguienteRR();
            if(procesoAEjecutar != NULL) {
                //hilo que controla si hay que mandar interrupcion
                pthread_create(&hilo_interrupciones, NULL, (void*) controlar_interrupcion_rr, NULL);
                agregar_a_exec(procesoAEjecutar);
                send_pcb(procesoAEjecutar, fd_cpu_dispatch);
                manejar_recibir(fd_cpu_dispatch);
                }
            }
    }
}

void controlar_interrupcion_rr(){
    while(1){
        sem_wait(&control_interrupciones_rr);
        cpu_disponible=false;
        printf(" \n controlo interrupcion Round Robin \n");
        usleep(quantum*1000);
		if(!cpu_disponible){
			printf(" \n deberia mandar interrupt \n ");
			hay_interrupcion=true;
		}
    }

}
void controlar_interrupcion_prioridades(){
    while(1){
        sem_wait(&control_interrupciones_prioridades);
        cpu_disponible=false;
        printf(" \n controlo interrupcion PRIORIDADES \n");
		//controlar si en ready hay un proceso con mayor prioridad
    }
}

pcb* obtenerSiguienteFIFO(){
    sem_wait(&cantidad_ready);
	pcb* procesoPlanificado = NULL;
	pthread_mutex_lock(&mutex_ready);
	procesoPlanificado = queue_pop(cola_ready);
    pthread_mutex_unlock(&mutex_ready);
	return procesoPlanificado;
}

pcb* obtenerSiguienteRR(){
	sem_wait(&cantidad_ready);
	pcb* procesoPlanificado = NULL;
	pthread_mutex_lock(&mutex_ready);
	procesoPlanificado = queue_pop(cola_ready);
    pthread_mutex_unlock(&mutex_ready);
    sem_post(&control_interrupciones_rr);
	return procesoPlanificado;
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
    sem_post(&control_interrupciones_prioridades);
	return procesoPlanificado;
}

//---------------------------------------------------------------
//              MANEJO DE RECURSOS

void manejar_wait(pcb* pcb, char* recurso){
	t_recurso* recursobuscado= buscar_recurso(recurso);
    sacar_de_exec();
	if(recursobuscado->encontrado == -1){
        // El recurso no existe, enviar proceso a EXIT
		log_info(logger_kernel, "No existe el recurso solicitado: %s", recurso);
        agregar_a_exit(pcb);
        sem_post(&puedo_ejecutar_proceso);
        //mando a memoria que finalizo proceso?
        //send_terminar_proceso(pcb->pid,fd_memoria);  
	}
    else{
        pthread_mutex_lock(&recursobuscado->mutex);
		recursobuscado->instancias --;
        pthread_mutex_unlock(&recursobuscado->mutex);
		log_info(logger_kernel,"PID: %d - Wait: %s - Instancias: %d", pcb->pid,recurso,recursobuscado->instancias);
		if(recursobuscado->instancias < 0){
            // No hay instancias disponibles, bloquear proceso
            //dudas con esta parte
			log_info(logger_kernel,"PID: %d - Bloqueado por: %s", pcb->pid,recurso);
            //agregar a la cola de bloqueados del recurso
            pthread_mutex_lock(&recursobuscado->mutex);
            queue_push(recursobuscado->bloqueados, pcb);
            pthread_mutex_unlock(&recursobuscado->mutex);
            //agregar a la cola de block
            agregar_a_block(pcb);
            sem_post(&puedo_ejecutar_proceso);
		}
        else{
            // Hay instancias disponibles, continuar ejecución
            list_add(recursobuscado->procesos, &(pcb->pid));
			agregar_a_exec(pcb);
			send_pcb(pcb,fd_cpu_dispatch);
		}
	}
}

void manejar_signal(pcb* proceso, char* recurso){
    t_recurso* recursobuscado= buscar_recurso(recurso);
    sacar_de_exec();
    if(recursobuscado->encontrado == -1){
        // El recurso no existe, enviar proceso a EXIT
        log_info(logger_kernel, "No existe el recurso solicitado: %s", recurso);
        agregar_a_exit(proceso);
        sem_post(&puedo_ejecutar_proceso);
        //mando a memoria que finalizo proceso?
        //send_terminar_proceso(pcb->pid,fd_memoria);  
    }
    else{
        if(lista_contiene_id(recursobuscado->procesos, proceso->pid) == false){
            // proceso no tiene instancias del recusro
            agregar_a_exec(proceso);
            send_pcb(proceso,fd_cpu_dispatch);
        }
        else{
            pthread_mutex_lock(&recursobuscado->mutex);
            recursobuscado->instancias ++;
            pthread_mutex_unlock(&recursobuscado->mutex);
            log_info(logger_kernel,"PID: %d - Signal: %s - Instancias: %d", proceso->pid,recurso,recursobuscado->instancias);
            if(recursobuscado->instancias <= 0){
                //tengo q desbloquear los bloqueados por el recurso
                //saco de la cola de bloqueados del recurso
                pthread_mutex_lock(&recursobuscado->mutex);
                queue_pop(recursobuscado->bloqueados);
                pthread_mutex_unlock(&recursobuscado->mutex);
                //mover de block a ready
            }
            // Devuelvo pcb a cpu
            agregar_a_exec(proceso);
            send_pcb(proceso,fd_cpu_dispatch);
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

void manejar_recibir(int socket_fd){
    if(socket_fd == 0){
        printf("Error al recibir mensaje\n");
        return;
    }else{
        pcb* proceso = NULL;
        char * extra = NULL;
        int tipo_mensaje = recibir_operacion(socket_fd);
        if (tipo_mensaje == MENSAJE) {
            recibir_mensaje(logger_kernel, socket_fd);
        }if(tipo_mensaje == PCB_WAIT){
            recv_pcbDesalojado(socket_fd, &proceso, &extra);
            manejar_wait(proceso, extra);
        }
        if(tipo_mensaje == PCB_SIGNAL){
            recv_pcbDesalojado(socket_fd, &proceso, &extra);
            manejar_signal(proceso, extra);
        }
        if(tipo_mensaje == PCB_EXIT){
            recv_pcbDesalojado(socket_fd, &proceso, &extra);
        }
        if(tipo_mensaje == PCB_SLEEP){
            recv_pcbDesalojado(socket_fd, &proceso, &extra);
            
        }
        if(tipo_mensaje == PCB_INTERRUPCION){
            recv_pcbDesalojado(socket_fd, &proceso, &extra);
        }
    }
}


void iniciar_semaforos(){
    sem_init(&cantidad_multiprogramacion,0,grado_multiprogramacion);
    sem_init(&cantidad_new,0,0);
    sem_init(&cantidad_ready,0,0);
    sem_init(&cantidad_exit,0,0);
    sem_init(&cantidad_exec,0,0);
    sem_init(&cantidad_block,0,0);
    sem_init(&puedo_ejecutar_proceso,0,1);
    sem_init(&control_interrupciones_rr,0,0);
    sem_init(&control_interrupciones_prioridades,0,0);

    //mutex de colas de planificacion
    pthread_mutex_init(&mutex_new, NULL);
	pthread_mutex_init(&mutex_ready, NULL);
	pthread_mutex_init(&mutex_exec, NULL);
    pthread_mutex_init(&mutex_block, NULL);
	pthread_mutex_init(&mutex_exit, NULL);
}



//------------------AUXILIARES----------------------------------------------------------------------------

pcb* crear_pcb(char* nombre_archivo, char* size, char* prioridad) {
    // Crear un PCB y asignar los valores iniciales
    pcb* proceso = malloc(sizeof(pcb));
    proceso->pid = contador_pid;
    proceso->size = atoi(size);
    proceso->pc = 0;
    proceso->prioridad = atoi(prioridad);
    proceso->estado = NEW;
    proceso->registros = malloc(sizeof(t_registros));
    proceso->registros->ax = 0;
    proceso->registros->bx = 0;
    proceso->registros->cx = 0;
    proceso->registros->dx = 0;
    proceso->archivos = list_create();
    proceso->path = malloc(sizeof(char) * strlen(nombre_archivo) + 1);
    strcpy(proceso->path, nombre_archivo);
    contador_pid++;
    return proceso;
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

pcb* buscar_y_remover_pcb_cola(t_queue* cola, int id, sem_t semaforo, pthread_mutex_t mutex){
    // Función para verificar si un PCB tiene el PID especificado
    bool tiene_pid(pcb* proceso) {
        return proceso->pid == id;
    }
     // Buscar el PCB en la cola y revover si existe
     //hay que meter semaforo
    pcb * proceso = NULL;
    pthread_mutex_lock(&mutex);
    proceso = list_remove_by_condition(cola->elements, (void *) tiene_pid);
    pthread_mutex_unlock(&mutex);
    if(proceso != NULL){
        sem_wait(&semaforo);
        return proceso;
    }else{
        return NULL;
    }
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

bool lista_contiene_id(t_list* lista, int id) {
    bool contiene_id = false;
    for (int i = 0; i < list_size(lista); i++) {
        int* elemento = (int*) list_get(lista, i);
        if (*elemento == id) {
            contiene_id = true;
            break;
        }
    }
    return contiene_id;
}
