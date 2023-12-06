#include "main.h"

int main(int argc, char* argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Se esperaba: %s [CONFIG_PATH]\n", argv[0]);
        exit(1);
    }
    
    // CONFIG y logger
    levantar_config(argv[1]);

    iniciar_listas();
    iniciar_semaforos();

    // Conecto kernel con cpu, memoria y filesystem
	fd_cpu_dispatch = 0,fd_cpu_interrupt = 0, fd_memoria = 0, fd_filesystem = 0;
	if (!generar_conexiones()) {
		log_error(logger_kernel, "Alguna conexion falló");
		// libero conexiones, log y config
        /*terminar_programa(logger_kernel, config);
        liberar_conexion(fd_cpu_dispatch);
        liberar_conexion(fd_cpu_interrupt);
        liberar_conexion(fd_memoria);
        liberar_conexion(fd_filesystem);
		exit(1);*/
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
    grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION_INI");
    string_array_destroy(instancias);
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

    //creo hilo para recv de fds
    pthread_create(&hilo_respuestas_cpu, NULL, (void*)manejar_recibir_cpu, NULL);
    pthread_create(&hilo_respuestas_fs, NULL, (void*)manejar_recibir_fs, NULL);
    pthread_create(&hilo_respuestas_memoria, NULL, (void*)manejar_recibir_memoria, NULL);

    pthread_detach(hilo_consola);
    pthread_detach(hilo_plan_largo);
    pthread_detach(hilo_plan_corto);
    pthread_detach(hilo_respuestas_cpu);   
    pthread_detach(hilo_respuestas_fs);
    pthread_detach(hilo_respuestas_memoria);

}

void iniciar_listas(){
	cola_new = queue_create();
	cola_ready = queue_create();
	cola_exec = queue_create(); 
	cola_block = queue_create(); 
	cola_exit = queue_create();

    lista_recursos = inicializar_recursos();
    archivos_abiertos = list_create();
}

//-------------------------------OPERACIONES DE CONSOLA-------------------------------------------

void iniciar_proceso(char * path, char* size, char* prioridad){   
    // Código para crear un nuevo proceso en base a un archivo dentro del file system de linux y dejar el mismo en el estado NEW
    pcb* proceso = crear_pcb(path, size, prioridad);
    agregar_a_new(proceso);
    send_inicializar_proceso(proceso, fd_memoria);
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
        //TENGO QUE MANDAR UN INTERRUPT A CPU PARA FINALIZAR EL PROCESO 
        t_paquete* paquete = crear_paquete(INTERRUPCION_FINALIZAR);
        agregar_a_paquete(paquete, &pid, sizeof(int));
        enviar_paquete(paquete, fd_cpu_interrupt);
        eliminar_paquete(paquete);
    }
    if(procesoAEliminar == NULL){
        procesoAEliminar = buscar_y_remover_pcb_cola(cola_block, pid_int, cantidad_block, mutex_block);
    }

    if(procesoAEliminar == NULL){
         printf("No se encontró el proceso con PID %d\n", pid_int);
    }
    else{
        // hacer log de fin de proceso
        log_info(logger_kernel, ANSI_COLOR_PINK "Finaliza el proceso %d - Motivo: SUCCESS", procesoAEliminar->pid);
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
        liberar_recursos_proceso(procesoAEliminar);
        //liberar_archivos(procesoAEliminar);
        //mandar mensaje a memoria para que libere
        send_terminar_proceso(procesoAEliminar->pid,fd_memoria);
        }
 
}
void detener_planificacion(){
    // Código para pausar la planificación de corto y largo plazo
    planificacion_activa = false;
    pthread_mutex_lock(&mutex_plani_larga);
        sem_wait(&sem_plan_largo);
    pthread_mutex_unlock(&mutex_plani_larga);
    pthread_mutex_lock(&mutex_plani_corta); 
        sem_wait(&sem_plan_corto);
    pthread_mutex_unlock(&mutex_plani_corta);
    //send_interrupcion(0 ,fd_cpu_interrupt);
    log_info(logger_kernel, "PAUSA DE PLANIFICACIÓN");
}
void iniciar_planificacion(){
    // Código para retomar la planificación de corto y largo plazo
    if (planificacion_activa == false){
        sem_post(&sem_plan_largo);
        sem_post(&sem_plan_corto);
        log_info(logger_kernel, "INICIO DE PLANIFICACIÓN");
    }
    planificacion_activa = true;
}
void cambiar_multiprogramacion(char* nuevo_grado_multiprogramacion){
    // Código para actualizar el grado de multiprogramación configurado inicialmente
    // por archivo de configuración y desalojar o finalizar los procesos si es necesario
    log_info(logger_kernel, "Grado Anterior: %d - Grado Actual: %d", grado_multiprogramacion, atoi(nuevo_grado_multiprogramacion));
    grado_multiprogramacion = atoi(nuevo_grado_multiprogramacion);
    
}
void proceso_estado(){
    // Código para mostrar por consola el listado de los estados con los procesos que se encuentran dentro de cada uno de ellos
    printf("Procesos por estado:\n");
    log_info(logger_kernel, ANSI_COLOR_PINK "Estado NEW - PROCESOS: %s",list_to_string(pid_lista_ready(cola_new->elements)));
    log_info(logger_kernel, ANSI_COLOR_PINK "Estado READY - PROCESOS: %s", list_to_string(pid_lista_ready(cola_ready->elements)));
    log_info(logger_kernel, ANSI_COLOR_PINK "Estado EXEC - PROCESOS: %s", list_to_string(pid_lista_ready(cola_exec->elements)));
    log_info(logger_kernel, ANSI_COLOR_PINK "Estado BLOCKED - PROCESOS: %s", list_to_string(pid_lista_ready(cola_block->elements)));
    log_info(logger_kernel, ANSI_COLOR_PINK "Estado EXIT - PROCESOS: %s", list_to_string(pid_lista_ready(cola_exit->elements)));
    printf("\n");
}

//-----------------------------------OPERACIONES DE LISTAS/COLAS-------------------------------------------
void agregar_a_new(pcb* proceso) {
    cambiar_estado(proceso, NEW);
    pthread_mutex_lock(&mutex_new);
	queue_push(cola_new, proceso);
    pthread_mutex_unlock(&mutex_new);
    log_info(logger_kernel,"Se crea el proceso %d en NEW", proceso->pid);
    sem_post(&cantidad_new);
}

pcb* sacar_de_new(){
	//sem_wait(&cantidad_new);
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
    t_list *lista_a_loguear = pid_lista_ready(cola_ready->elements);
	char *lista = list_to_string(lista_a_loguear);
	log_info(logger_kernel, "Cola Ready %s: %s",algoritmo_planificacion, lista);
	list_destroy(lista_a_loguear);
    free(lista);
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

pcb* sacar_de_block(){
    sem_wait(&cantidad_block);
    pthread_mutex_lock(&mutex_block);
    pcb *proceso = queue_pop(cola_block);
    pthread_mutex_unlock(&mutex_block);
    return proceso;
}


//-----------------------------------------------------------------------
//             PLANIFICADORES



void* planif_largo_plazo(void* args){
    //falta agregar lo de finalizar cuando recibe un exit
    while(1){
        sem_wait(&cantidad_new);
        sem_wait(&cantidad_multiprogramacion);
        pthread_mutex_lock(&mutex_plani_larga);
        sem_wait(&sem_plan_largo);
        //sacar de new y agregar a ready
        pcb* proceso = sacar_de_new();
        agregar_a_ready(proceso);
        sem_post(&sem_plan_largo);
        pthread_mutex_unlock(&mutex_plani_larga);
    }
}

void* planif_corto_plazo(void* args){
    pthread_t hilo_interrupciones;
    while(1){
        //semaforo para que no haya mas de un proceso en exec, cuando se bloquea o termina el proceso, hacer signal
        sem_wait(&puedo_ejecutar_proceso);
        sem_wait(&cantidad_ready);
        pthread_mutex_lock(&mutex_plani_corta);
        sem_wait(&sem_plan_corto);

        pcb *proceso_a_ejecutar = obtener_siguiente_proceso(algoritmo_planificacion);
        
        if (proceso_a_ejecutar != NULL) {
            agregar_a_exec(proceso_a_ejecutar);
            send_pcb(proceso_a_ejecutar, fd_cpu_dispatch);
        }

        sem_post(&sem_plan_corto);
        pthread_mutex_unlock(&mutex_plani_corta);

        crear_hilo_interrupcion(algoritmo_planificacion, hilo_interrupciones);
    }
}

pcb *obtener_siguiente_proceso(char* algoritmo_planificacion) {
    if (strcmp(algoritmo_planificacion, "FIFO") == 0) {
        return obtenerSiguienteFIFO();
    } else if (strcmp(algoritmo_planificacion, "RR") == 0) {
        return obtenerSiguienteRR();
    } else if (strcmp(algoritmo_planificacion, "PRIORIDADES") == 0) {
        return obtenerSiguientePRIORIDADES();
    } else {
        return NULL;
    }
}

void crear_hilo_interrupcion(char* algoritmo_planificacion, pthread_t hilo_interrupciones){
    if(strcmp(algoritmo_planificacion,"FIFO")==0){
        return;
    }
    else if(strcmp(algoritmo_planificacion,"PRIORIDADES")==0){
        pthread_create(&hilo_interrupciones, NULL, (void*) controlar_interrupcion_prioridades, NULL);
        return;
    }
    else if(strcmp(algoritmo_planificacion,"RR")==0){
        pthread_create(&hilo_interrupciones, NULL, (void*) controlar_interrupcion_rr, NULL);
        return;
    }
}

void controlar_interrupcion_rr(){
    while(1){
        sem_wait(&control_interrupciones_rr);
        cpu_disponible=false;
        usleep(quantum*1000);
		if(!cpu_disponible){
            if(list_size(cola_exec->elements) > 0 & list_size(cola_ready->elements) > 0){
                if(hay_exit == false){
                    pcb * proceso = queue_peek(cola_exec);
                    log_info(logger_kernel,"PID: %d - Desalojado por fin de Quantum", proceso->pid);
                    hay_interrupcion=true;
                    send_interrupcion(proceso->pid,fd_cpu_interrupt);
                    // VER TEMA DE CUANDO CORTAR LA EJECUCION
                }else{
                    hay_exit = false;
                }
            }
		}
    }

}
void controlar_interrupcion_prioridades(){
    while(1){
        sem_wait(&control_interrupciones_prioridades);
        cpu_disponible=false;
		//controlar si en ready hay un proceso con mayor prioridad
        if(list_size(cola_exec->elements) > 0 && list_size(cola_ready->elements) > 0){
            pthread_mutex_lock(&mutex_exec);
            pcb * proceso = queue_peek(cola_exec);
            pthread_mutex_unlock(&mutex_exec);
            t_list* lista_ordenada = list_sorted(cola_ready->elements,cmp);
            int prioridadAComparar = ((pcb*)list_get(lista_ordenada,0))->prioridad;
            if(hay_exit == false){
                if(prioridadAComparar < proceso->prioridad){
                    log_info(logger_kernel,"PID: %d - Desalojado por prioridad", proceso->pid);
                    hay_interrupcion=true;
                    send_interrupcion(proceso->pid,fd_cpu_interrupt);
                    // VER TEMA DE CUANDO CORTAR LA EJECUCION
                }
            }else{
                hay_exit = false;
            }
        }
    }
}

pcb* obtenerSiguienteFIFO(){
    //sem_wait(&cantidad_ready);
	pcb* procesoPlanificado = NULL;
	pthread_mutex_lock(&mutex_ready);
	procesoPlanificado = queue_pop(cola_ready);
    pthread_mutex_unlock(&mutex_ready);
	return procesoPlanificado;
}

pcb* obtenerSiguienteRR(){
	//sem_wait(&cantidad_ready);
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
    //sem_wait(&cantidad_ready);
	pcb* procesoPlanificado = NULL;
    pthread_mutex_lock(&mutex_ready);
    t_list* lista_ordenada = list_sorted(cola_ready->elements,cmp);
	procesoPlanificado = list_remove(lista_ordenada, 0);
    list_remove_element(cola_ready->elements,procesoPlanificado);
    pthread_mutex_unlock(&mutex_ready);
    sem_post(&control_interrupciones_prioridades);
	return procesoPlanificado;
}

//---------------------------------------------------------------
//              MANEJO DE RECURSOS

void manejar_wait(pcb* proceso, char* recurso){
	t_recurso* recursobuscado= buscar_recurso(recurso);
    pcb * pcb_a_borrar = sacar_de_exec();
    pcb_destroyer(pcb_a_borrar);
	if(recursobuscado->encontrado == -1){
        // El recurso no existe, enviar proceso a EXIT
		log_info(logger_kernel, ANSI_COLOR_PINK "Finaliza el proceso: %d - Motivo: INVALID_RESOURCE",proceso->pid);
        agregar_a_exit(proceso);
        sem_post(&puedo_ejecutar_proceso);
        //mando a memoria que finalizo proceso
        send_terminar_proceso(proceso->pid,fd_memoria);  
	}
    else{
        pthread_mutex_lock(&recursobuscado->mutex);
		recursobuscado->instancias --;
        pthread_mutex_unlock(&recursobuscado->mutex);
		log_info(logger_kernel,"PID: %d - Wait: %s - Instancias: %d", proceso->pid,recurso,recursobuscado->instancias);
		if(recursobuscado->instancias < 0){   
            // No hay instancias disponibles, bloquear proceso
			log_info(logger_kernel, ANSI_COLOR_CYAN "PID: %d - Bloqueado por: %s", proceso->pid,recurso);
            //agregar a la cola de bloqueados del recurso
            pthread_mutex_lock(&recursobuscado->mutex);
            queue_push(recursobuscado->bloqueados,proceso);
            pthread_mutex_unlock(&recursobuscado->mutex);
            //agregar a la cola de block
            agregar_a_block(proceso);
            detectar_deadlock_recurso();
            sem_post(&puedo_ejecutar_proceso);
		}
        else{
            // Hay instancias disponibles, continuar ejecución
            pthread_mutex_lock(&recursobuscado->mutex);
            queue_push((recursobuscado->procesos),proceso);
            pthread_mutex_unlock(&recursobuscado->mutex);
			agregar_a_exec(proceso);
			send_pcb(proceso,fd_cpu_dispatch);
		}
	}
}

void manejar_signal(pcb* proceso, char* recurso){
    t_recurso* recursobuscado = buscar_recurso(recurso);
    pcb * pcb_a_borrar = sacar_de_exec();
    pcb_destroyer(pcb_a_borrar);
    if(recursobuscado->encontrado == -1){
        // El recurso no existe, enviar proceso a EXIT
        log_info(logger_kernel, ANSI_COLOR_PINK "Finaliza el proceso: %d - Motivo: INVALID_RESOURCE",proceso->pid);
        agregar_a_exit(proceso);
        sem_post(&puedo_ejecutar_proceso);
        send_terminar_proceso(proceso->pid, fd_memoria);  
    }
    else{
        if(lista_contiene_id(recursobuscado->procesos->elements, proceso) == false){
            // proceso no tiene instancias del recusro
            log_info(logger_kernel, "No hay instancias del recurso: %s asignadas al proceso", recurso);
            agregar_a_exit(proceso);
            sem_post(&puedo_ejecutar_proceso);
            send_terminar_proceso(proceso->pid, fd_memoria);  
        }
        else{
            pthread_mutex_lock(&recursobuscado->mutex);
            recursobuscado->instancias ++;
            pthread_mutex_unlock(&recursobuscado->mutex);
            buscar_y_remover_pcb_cola(recursobuscado->procesos, proceso->pid, sem_no_usamos, recursobuscado->mutex);
            
            //pcb_destroyer(procesoQuitado);

            log_info(logger_kernel, ANSI_COLOR_CYAN "PID: %d - Signal: %s - Instancias: %d", proceso->pid,recurso,recursobuscado->instancias);
            if(recursobuscado->instancias <= 0){
                //tengo q desbloquear los bloqueados por el recurso
                //saco de la cola de bloqueados del recurso
                pthread_mutex_lock(&recursobuscado->mutex);
                pcb * procesoQuitadoBloqueados = queue_pop(recursobuscado->bloqueados);
                queue_push(recursobuscado->procesos, procesoQuitadoBloqueados);
                pthread_mutex_unlock(&recursobuscado->mutex);
                //sacar de block y mover a ready
                buscar_y_remover_pcb_cola(cola_block, procesoQuitadoBloqueados->pid, cantidad_block, mutex_block);
                agregar_a_ready(procesoQuitadoBloqueados);
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
            recursobuscado->encontrado = 0;
			return recursobuscado;
		}
	}
    recursobuscado->encontrado = -1;
	return recursobuscado;
}

//---------------------------------------------------------------
//              MANEJO DE FILESYSTEM

t_archivo* buscar_archivo_global(char* nombre_archivo){
	for(int i = 0; i < list_size(archivos_abiertos); i++){
		t_archivo* archivo = list_get(archivos_abiertos, i);
		if(strcmp(archivo->nombre_archivo, nombre_archivo) == 0){
			return archivo;
		}
	}
	return NULL;
}

t_archivo* buscar_archivo_en_pcb(char* nombre_archivo, pcb* pcb){
	for(int i = 0; i < list_size(pcb->archivos); i++){
		t_archivo* archivo = list_get(pcb->archivos, i);
		if(strcmp(archivo->nombre_archivo, nombre_archivo) == 0){
			return archivo;
		}
	}
	return NULL;
}

t_archivo* crear_archivo(char* nombre_archivo){
	t_archivo* archivo = malloc(sizeof(t_archivo));
	archivo->nombre_archivo = nombre_archivo;
	archivo->puntero = 0;
	archivo->bloqueados_archivo = queue_create();
    archivo->abierto_w = 0;
    archivo->cant_abierto_r = 0;
	//pthread_mutex_init(&archivo->mutex_w, NULL);
    //pthread_mutex_init(&archivo->mutex_r, NULL);
    //pthread_rwlock_init(&archivo->rwlock, NULL);
	return archivo;
}

t_archivo* quitar_archivo_de_proceso(char* nombre_archivo,pcb* pcb){
	t_archivo* archivo = buscar_archivo_en_pcb(nombre_archivo, pcb);
	list_remove_element(pcb->archivos, archivo);
	log_info(logger_kernel, ANSI_COLOR_YELLOW "al proceso %d le quedan %d archivos abiertos", pcb->pid, list_size(pcb->archivos));
	return archivo;
}

void ejecutar_f_open(char* nombre_archivo, char* modo_apertura, pcb* proceso){
    //depende de si esta en modo lectura o escritura
    //VER TEMA LOCKS
    t_archivo* archivo = buscar_archivo_global(nombre_archivo);

    if(strcmp(modo_apertura, "R") == 0){
        //validar si hay lock de escritura activo
        if(archivo->abierto_w == 1){
            //bloquea proceso y lo agrega a la cola de bloqueados del archivo
            agregar_a_block(proceso);
            queue_push(archivo->bloqueados_archivo, proceso->pid);
            sacar_de_exec();
            sem_post(&puedo_ejecutar_proceso);
        }else{
            if(archivo == NULL){
                //crear archivo y agregarlo a la lista de archivos abiertos
                archivo = crear_archivo(nombre_archivo);
                send_abrir_archivo(nombre_archivo,fd_filesystem);
                //necesito esperar a respuesta de fs si existe o no el archivo
                if(archivo_no_existe){
                    send_crear_archivo(nombre_archivo,fd_filesystem);   
                }
                sem_wait(&archivo_abierto);
                list_add(archivos_abiertos, archivo);
                list_add(proceso->archivos, archivo);   
            }else{
                list_add(proceso->archivos, archivo);  
            }
            send_pcb(proceso,fd_cpu_dispatch);
            archivo->cant_abierto_r ++;
        }
    }
    else if(strcmp(modo_apertura, "W") == 0){
        if(archivo->abierto_w == 1 || archivo->cant_abierto_r > 0){
            //bloquea proceso y lo agrega a la cola de bloqueados del archivo
            agregar_a_block(proceso);
            queue_push(archivo->bloqueados_archivo, proceso->pid);
            sacar_de_exec();
            sem_post(&puedo_ejecutar_proceso);
        }else{
            if(archivo == NULL){
                //crear archivo y agregarlo a la lista de archivos abiertos
                archivo = crear_archivo(nombre_archivo);
                send_abrir_archivo(nombre_archivo,fd_filesystem);
                //necesito esperar a respuesta de fs si existe o no el archivo
                if(archivo_no_existe){
                    send_crear_archivo(nombre_archivo,fd_filesystem);   
                }
                sem_wait(&archivo_abierto);
                list_add(archivos_abiertos, archivo);
                list_add(proceso->archivos, archivo);   
            }else{
                list_add(proceso->archivos, archivo);  
            }
            send_pcb(proceso,fd_cpu_dispatch);
            archivo->abierto_w = 1;
        }
    }
}

//--------------FUNCIONES DE RECIBIR MENSAJES-----------------

void manejar_recibir_cpu(){
    while(1){
        if(fd_cpu_dispatch == 0|| fd_cpu_dispatch == -1){
            printf("Error al recibir mensaje de cpu\n");
            break;
        }
        else{
            pcb* proceso = NULL;
            pcb* pcb_a_borrar = NULL;
            char * extra = NULL;
            char *nombre_archivo;
            op_code cop = recibir_operacion(fd_cpu_dispatch);
            switch (cop) {
                case MENSAJE:
                    recibir_mensaje(logger_kernel, fd_cpu_dispatch);
                    break;
                case PCB_WAIT:
                    //printf("\n manejo wait \n");
                    recv_pcbDesalojado(fd_cpu_dispatch, &proceso, &extra);
                    manejar_wait(proceso, extra);
                    break;
                case PCB_SIGNAL:
                    //printf("\n manejo signal \n");
                    recv_pcbDesalojado(fd_cpu_dispatch, &proceso, &extra);
                    manejar_signal(proceso, extra);
                    break;
                case PCB_EXIT:
                    hay_exit = true;
                    //printf("\n manejo exit \n");
                    recv_pcbDesalojado(fd_cpu_dispatch, &proceso, &extra);
                    //borrar todo lo que corresponde a ese proceso
                    pcb_a_borrar = sacar_de_exec();
                    pcb_destroyer(pcb_a_borrar);
                    // Mandar a la cola de EXIT;
                    agregar_a_exit(proceso);
                    sem_post(&cantidad_multiprogramacion);
                    sem_post(&puedo_ejecutar_proceso);
                    //Liberar recursos asignados del proceso
                    liberar_recursos_proceso(proceso);
                    //liberar_archivos(proceso );
                    //mandar mensaje a memoria para que libere
                    send_terminar_proceso(proceso->pid,fd_memoria);
                    break;
                case PCB_SLEEP:
                    //printf("\n manejo sleep \n");
                    recv_pcbDesalojado(fd_cpu_dispatch, &proceso, &extra);
                    pcb_a_borrar = sacar_de_exec();
                    //pcb_destroyer(pcb_a_borrar);
                    pthread_t hilo_sleep;
                    // Crear la estructura de argumentos
                    HiloArgs* args = malloc(sizeof(HiloArgs));
                    args->proceso = proceso;
                    args->extra = extra;
                    pthread_create(&hilo_sleep, NULL, manejar_sleep, args);
                    pthread_detach(hilo_consola);
                    break;
                case PCB_INTERRUPCION:
                    //printf("\n manejo interrupcion \n");
                    //ver bien coom hacer el interupt
                    recv_pcbDesalojado(fd_cpu_dispatch, &proceso, &extra);
                    log_info(logger_kernel,"RECIBI UN PCB DESALOJADO");
                    pcb_a_borrar = sacar_de_exec();
                    pcb_destroyer(pcb_a_borrar);
                    agregar_a_ready(proceso);
                    sem_post(&puedo_ejecutar_proceso);
                    break;
                case F_OPEN:
                    char* modo_apertura;
                    //NECESITO PCB PARA EL ARCIVO
                    recv_f_open(fd_cpu_dispatch, &nombre_archivo, &modo_apertura);
                    //NO SE SI SE PUEDE ASI??????
                    recv_pcbDesalojado(fd_cpu_dispatch, &proceso, &extra);
                    ejecutar_f_open(nombre_archivo,modo_apertura,proceso);
                    log_info(logger_kernel, "PID: %d - Abrir Archivo: %s", proceso->pid, nombre_archivo);
                    break;
                case F_CLOSE:
                    recv_f_close(fd_cpu_dispatch, &nombre_archivo);
                    
                    break;
                case F_SEEK:
                    
                    break;
                case F_TRUNCATE:
                    break;
                case F_READ:
                    break;
                case F_WRITE:
                    break;
                default:
                    printf("Error al recibir mensaje\n");
                    break;
            free(extra);
            free(nombre_archivo);
            }
        }   
    }    
}

void manejar_recibir_memoria(){
    while(1){
        if(fd_memoria == 0|| fd_memoria == -1){
            printf("Error al recibir mensaje de memoria\n");
            return;
        }
        else{
            op_code cop = recibir_operacion(fd_memoria);
            switch (cop) {
                case MENSAJE:
                    recibir_mensaje(logger_kernel, fd_memoria);
                    break;
                default:
                    printf("Error al recibir mensaje\n");
                    break;
            }
        }   
    }    
}

void manejar_recibir_fs(){
    while(1){
        if(fd_filesystem == 0 || fd_filesystem == -1){
            printf("Error al recibir mensaje de filesystem\n");
            return;
        }
        else{
            op_code cop = recibir_operacion(fd_filesystem);
            switch (cop) {
                case MENSAJE:
                    recibir_mensaje(logger_kernel, fd_filesystem);
                    break;
                case ABRIR_ARCHIVO:
                    //no se para que me sirve el tamanio
                    log_info(logger_kernel, ANSI_COLOR_YELLOW "SE ABRIO UN ARCHIVO");
                    sem_post(&archivo_abierto);
                    break;
                case ARCHIVO_NO_EXISTE:
                    //le tengo que mandar que cree el archivo
                    log_info(logger_kernel, ANSI_COLOR_YELLOW "NO EXISTE EL ARCHIVO");
                    archivo_no_existe = true;
                    break;
                case ARCHIVO_CREADO:
                    log_info(logger_kernel, ANSI_COLOR_YELLOW "SE CREO UN ARCHIVO");
                    sem_post(&archivo_abierto);
                    break;
                default:
                    printf("Error al recibir mensaje\n");
                    break;
            }
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
    sem_init(&sem_plan_largo, 0, 1);
    sem_init(&sem_plan_corto, 0, 1);
    sem_init(&archivo_abierto, 0, 0);
    sem_init(&sem_sleep, 0, 0);
    sem_init(&sem_no_usamos, 0, 500);
    

    //mutex de colas de planificacion
    pthread_mutex_init(&mutex_new, NULL);
	pthread_mutex_init(&mutex_ready, NULL);
	pthread_mutex_init(&mutex_exec, NULL);
    pthread_mutex_init(&mutex_block, NULL);
	pthread_mutex_init(&mutex_exit, NULL);

    //mutex para pausar planificacion
    pthread_mutex_init(&mutex_plani_corta, NULL);
    pthread_mutex_init(&mutex_plani_larga, NULL);
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
    proceso->registros->ax = malloc(sizeof(uint32_t));
    *(proceso->registros->ax) = 0;
    proceso->registros->bx = malloc(sizeof(uint32_t));
    *(proceso->registros->bx) = 0;
    proceso->registros->cx = malloc(sizeof(uint32_t));
    *(proceso->registros->cx) = 0;
    proceso->registros->dx = malloc(sizeof(uint32_t));
    *(proceso->registros->dx) = 0;
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
		recurso-> procesos = queue_create();
		recurso->encontrado = 0;
		recurso->instancias = instancia_recursos[i];
		recurso->bloqueados = queue_create();
		pthread_mutex_init(&recurso->mutex, NULL);
		list_add(lista, recurso);
	}
	return lista;
}

void buscar_proceso_asignado_recurso_y_eliminar(pcb* proceso, t_recurso* recurso_actual) {
    int largo_bloqueados = list_size(recurso_actual->bloqueados->elements);
    int largo_procesos = list_size(recurso_actual->procesos->elements);
	for(int i = 0; i < largo_bloqueados; i++){
        pcb* proceso_bloqueado = list_get(recurso_actual->bloqueados->elements, i);
        if (proceso_bloqueado->pid == proceso->pid) {
            list_remove(recurso_actual->bloqueados->elements, i);
            recurso_actual->instancias++;
            break;
        }
        //pcb_destroyer(proceso_bloqueado);
    }
    for(int i = 0; i < largo_procesos; i++){
        pcb* proceso_encontrado = list_get(recurso_actual->procesos->elements, i);
        if (proceso_encontrado->pid == proceso->pid) {
            if(recurso_actual->instancias <= 0){
                //tengo q desbloquear los bloqueados por el recurso
                //saco de la cola de bloqueados del recurso
                pthread_mutex_lock(&recurso_actual->mutex);
                pcb * procesoQuitadoBloqueados = queue_pop(recurso_actual->bloqueados);
                queue_push(recurso_actual->procesos, procesoQuitadoBloqueados);
                pthread_mutex_unlock(&recurso_actual->mutex);
                //sacar de block y mover a ready
                buscar_y_remover_pcb_cola(cola_block, procesoQuitadoBloqueados->pid, cantidad_block, mutex_block);
                agregar_a_ready(procesoQuitadoBloqueados);
            }
            list_remove(recurso_actual->procesos->elements, i);
            recurso_actual->instancias++;
            break;
        }
    }
    return;
}


void liberar_recursos_proceso(pcb* proceso) {
    int largo = list_size(lista_recursos);
	for(int i = 0; i < largo; i++){
        t_recurso* recurso_actual = list_get(lista_recursos, i);
        buscar_proceso_asignado_recurso_y_eliminar(proceso, recurso_actual);
    }
}


pcb* buscar_y_remover_pcb_cola(t_queue* cola, int id, sem_t semaforo, pthread_mutex_t mutex){
    // Función para verificar si un PCB tiene el PID especificado
    bool tiene_pid(pcb* proceso) {
        return proceso->pid == id;
    }
     // Buscar el PCB en la cola y devolver si existe
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

bool lista_contiene_id(t_list* lista, pcb * proceso) {
    int id = proceso->pid;
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

void* manejar_sleep(void * args){
    HiloArgs* hiloArgs = args;
    pcb* proceso = hiloArgs->proceso;
    char* extra = hiloArgs->extra;

    int tiempo = atoi(extra);
    log_info(logger_kernel, ANSI_COLOR_CYAN "PID: %d - Bloqueado por: SLEEP %d", proceso->pid,tiempo);
    agregar_a_block(proceso);

    sem_post(&puedo_ejecutar_proceso);
    sem_post(&sem_sleep);

    sleep(tiempo);
    proceso = buscar_y_remover_pcb_cola(cola_block, proceso->pid, cantidad_block, mutex_block);
    agregar_a_ready(proceso);

    // Libera la memoria de la estructura de argumentos
    free(hiloArgs);
    // Termina el hilo
    pthread_exit(NULL);
}

//-------------------DETECCION DE DEADLOCK---------------------------



void detectar_deadlock_recurso(){

    for(int i = 0; i < queue_size(cola_block); i++){
        pcb* un_proceso = queue_peek(cola_block, i);
        
        t_queue* cola_procesos = un_recurso->procesos;
        t_queue* cola_bloqueados = un_recurso->bloqueados;

        
        
        
    }
    
    
}

void deadlock_entre(pcb* otro_proceso,pcb* proceso){
    for (int i = 0; i < list_size(lista_recursos); i++) {
      t_recurso* rec= list_get(lista_recursos, i);
        if(lista_contiene_id(rec->bloqueados->elements, proceso)){
            if(lista_contiene_id(rec->procesos->elements, proceso)){
                return true;
            }
        }
        
    }
}
