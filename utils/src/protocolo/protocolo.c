#include "./protocolo.h"


void* serializar_paquete(t_paquete* paquete, int bytes) {
	int size_paquete = sizeof(int) + sizeof(int) + paquete->buffer->size;
    void* paquete_serializado = NULL;

	paquete_serializado = malloc(size_paquete); // TODO: need free (3)
	int offset = 0;
	memcpy(paquete_serializado + offset, &(paquete->codigo_operacion), sizeof(int));

	offset += sizeof(int);
	memcpy(paquete_serializado + offset, &(paquete->buffer->size), sizeof(int));

	offset += sizeof(int);
	memcpy(paquete_serializado + offset, paquete->buffer->stream, paquete->buffer->size);

	return paquete_serializado; 
}


void eliminar_paquete(t_paquete* paquete) {
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

void enviar_mensaje(char* mensaje, int socket_cliente){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

int recibir_operacion(int socket_cliente){
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void* recibir_buffer(int* size, int socket_cliente){
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(t_log* logger, int socket_cliente) {
    int size;
    char* buffer = (char*)recibir_buffer(&size, socket_cliente);
    if (buffer != NULL) {
        log_info(logger, ANSI_COLOR_YELLOW"Me llegó el mensaje: %s", buffer);
        free(buffer); // Liberar la memoria del mensaje recibido
    }
}
char* recibir_mensaje_fs(int socket_cliente) {
    int size;
    char* buffer = (char*)recibir_buffer(&size, socket_cliente);
    if (buffer != NULL) {
        return buffer;
    }
}

t_list* recibir_paquete(int socket_cliente){
	int size;
	int desplazamiento = 0;
	void* buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	//memory leaks
	if (buffer == NULL) {
        // Manejar el error de recepción del paquete
        list_destroy(valores);
        return NULL;
    }

	while(desplazamiento < size){
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}

void crear_buffer(t_paquete* paquete){
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(op_code codigo_op){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo_op;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio){
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente){
	int bytes = paquete->buffer->size + sizeof(int) + sizeof(op_code);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void empaquetar_archivos(t_paquete* paquete_archivos, t_list* lista_archivos) {
    int cantidad_archivos = list_size(lista_archivos);

    // Agregar la cantidad de archivos al paquete
    agregar_a_paquete(paquete_archivos, &cantidad_archivos, sizeof(int));

    for (int i = 0; i < cantidad_archivos; i++) {
        t_archivo_proceso* archivo = list_get(lista_archivos, i);

        agregar_a_paquete(paquete_archivos, archivo->nombre_archivo, strlen(archivo->nombre_archivo) + 1);
        agregar_a_paquete(paquete_archivos, &archivo->puntero, sizeof(int));
		agregar_a_paquete(paquete_archivos, archivo->modo_apertura, strlen(archivo->nombre_archivo) + 1);
    }
}
t_queue* desempaquetar_procesos_bloqueados(t_list* paquete, int* comienzo) {
		t_queue* procesos_bloqueados = queue_create();
		int* cantidad_procesos = list_get(paquete, *comienzo);
		int i = *comienzo + 1;

		while (i - *comienzo - 1 < *cantidad_procesos) {
			int* pid_bloqueado = malloc(sizeof(int));
			*pid_bloqueado = *(int*)list_get(paquete, i);
			queue_push(procesos_bloqueados, pid_bloqueado);
			i++;
		}

	*comienzo = i;
	free(cantidad_procesos);
	return procesos_bloqueados;
}

t_list* desempaquetar_archivos(t_list* paquete, int* comienzo) {
	t_list* lista_archivos = list_create();
	int* cantidad_archivos = list_get(paquete, *comienzo);
	int i = *comienzo + 1;

	while (i - *comienzo - 1 < (*cantidad_archivos * 2)) {
		t_archivo_proceso* archivo = malloc(sizeof(t_archivo));

		// Desempaquetar la ruta del archivo
		char* path = (char*)list_get(paquete, i);
		archivo->nombre_archivo = strdup(path);
		free(path);
		i++;

		// Desempaquetar el puntero
		int* puntero = list_get(paquete, i);
		archivo->puntero = *puntero;
		free(puntero);
		i++;

		// Desempaquetar el modo apertura
		char* modo = (char*)list_get(paquete, i);
		archivo->modo_apertura = strdup(modo);
		free(modo);
		i++;

		list_add(lista_archivos, archivo);
	}

	*comienzo = i;
	free(cantidad_archivos);
	return lista_archivos;
}

void send_archivos(int fd_modulo, t_list* lista_archivos) {
    t_paquete* paquete_archivos = crear_paquete(ENVIO_LISTA_ARCHIVOS);
    empaquetar_archivos(paquete_archivos, lista_archivos);
    enviar_paquete(paquete_archivos, fd_modulo);
    eliminar_paquete(paquete_archivos);
}

void send_pagina_cargada(int fd){
	enviar_mensaje("OK", fd);
}

char *recv_pagina_cargada(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	char* valor = list_get(paquete, 0);
	list_destroy(paquete);

	return valor;
}


t_list* recv_archivos(t_log* logger, int fd_modulo) {
    t_list* paquete = recibir_paquete(fd_modulo);
    t_list* lista_archivos = desempaquetar_archivos(paquete, 0);
    list_destroy(paquete);
    log_info(logger,ANSI_COLOR_YELLOW "Se recibió una lista de archivos.");
    return lista_archivos;
}

void archivo_destroyer(t_archivo* archivo) {
    //free(archivo->nombre_archivo);
    free(archivo);
}

void lista_archivos_destroy(t_list* lista) {
    while (!list_is_empty(lista)) {
        t_archivo* archivo = list_remove(lista, 0);
        archivo_destroyer(archivo);
    }
    list_destroy(lista);
}

void empaquetar_registros(t_paquete* paquete, t_registros* registro){
	agregar_a_paquete(paquete, registro->ax, sizeof(uint32_t));
	agregar_a_paquete(paquete, registro->bx, sizeof(uint32_t));
	agregar_a_paquete(paquete, registro->cx, sizeof(uint32_t));
	agregar_a_paquete(paquete, registro->dx, sizeof(uint32_t));
}

t_registros* desempaquetar_registros(t_list * paquete,int posicion){
	t_registros *registro = malloc(sizeof(t_registros));

	uint32_t* ax = list_get(paquete, posicion);
	registro->ax = malloc(sizeof(uint32_t));
	memcpy(registro->ax, ax, sizeof(uint32_t));
	free(ax);

	uint32_t* bx = list_get(paquete, posicion+1);
	registro->bx = malloc(sizeof(uint32_t));
	memcpy(registro->bx, bx, sizeof(uint32_t));
	free(bx);

	uint32_t* cx = list_get(paquete, posicion+2);
	registro->cx = malloc(sizeof(uint32_t));
    memcpy(registro->cx, cx, sizeof(uint32_t));
	free(cx);

	uint32_t* dx = list_get(paquete, posicion+3);
	registro->dx = malloc(sizeof(uint32_t));
	memcpy(registro->dx, dx, sizeof(uint32_t));
	free(dx);

	return registro;
}

void empaquetar_pcb(t_paquete* paquete, pcb* contexto){
    
	agregar_a_paquete(paquete, &(contexto->pid), sizeof(int));
	agregar_a_paquete(paquete, &(contexto->pc), sizeof(int));
	agregar_a_paquete(paquete, &(contexto->size), sizeof(int));
	agregar_a_paquete(paquete, &(contexto->prioridad), sizeof(int));
	agregar_a_paquete(paquete, &(contexto->tiempo_ejecucion), sizeof(int));
	agregar_a_paquete(paquete, (contexto->path), strlen(contexto->path) + 1);
	agregar_a_paquete(paquete, &(contexto->estado), sizeof(estado_proceso));
	empaquetar_registros(paquete, contexto->registros);
	empaquetar_archivos(paquete, contexto->archivos);
    
}

pcb* desempaquetar_pcb(t_list* paquete, int* counter) {
	pcb* contexto = malloc(sizeof(pcb));

	int* pid = list_get(paquete, 0);
	contexto->pid = *pid;
	free(pid);

	int* pc = list_get(paquete, 1);
	contexto->pc = *pc;
	free(pc);

	int* size = list_get(paquete, 2);
	contexto->size = *size;
	free(size);

	int* prioridad = list_get(paquete, 3);
	contexto->prioridad = *prioridad;
	free(prioridad);

	int* tiempo_ejecucion = list_get(paquete, 4);
	contexto->tiempo_ejecucion = *tiempo_ejecucion;
	free(tiempo_ejecucion);

	char* path = list_get(paquete, 5);
	contexto->path = malloc(strlen(path) + 1);
	strcpy(contexto->path, path);
	free(path);

	estado_proceso* estado = list_get(paquete, 6);
	contexto->estado = *estado;
	free(estado);

	int comienzo_registros = 7;
	t_registros* registro_contexto = desempaquetar_registros(paquete, comienzo_registros);
	contexto->registros = registro_contexto;

	*counter = comienzo_registros + 4;
	t_list* archivos = desempaquetar_archivos(paquete, counter);
	contexto->archivos = archivos;

	// memory leaks
    if (contexto->path == NULL || contexto->registros == NULL || contexto->archivos == NULL) {
        free(contexto->path);
        registros_destroy(contexto->registros);
        lista_archivos_destroy(contexto->archivos);
        free(contexto);
        return NULL;
    }

	return contexto;
}

void send_pcb(pcb* contexto, int fd_modulo){
	printf("\n ENVIANDO PCB PID: %d\n", contexto->pid);
    t_paquete* paquete = crear_paquete(ENVIO_PCB);
	empaquetar_pcb(paquete, contexto);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
	//pcb_destroyer(contexto);
}

pcb* recv_pcb(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	int counter = 0;
	pcb* contexto_recibido = desempaquetar_pcb(paquete, &counter);
	list_destroy(paquete);// memory leaks
	return contexto_recibido;
}

void registros_destroy(t_registros* registros){
	free(registros->ax);
	free(registros->bx);
	free(registros->cx);
	free(registros->dx);
	free(registros);
}

void pcb_destroyer(pcb* contexto){
	// Liberar la memoria de la lista de archivos
	lista_archivos_destroy(contexto->archivos);
	// Liberar la memoria de los registros
	registros_destroy(contexto->registros);
	// Liberar la memoria del path
	free(contexto->path);
	// Liberar la memoria del PCB
	free(contexto);
}

void send_cambiar_estado(estado_proceso estado, int fd_modulo){
	t_paquete* paquete = crear_paquete(CAMBIAR_ESTADO);
	agregar_a_paquete(paquete, &estado, sizeof(estado_proceso));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

estado_proceso recv_cambiar_estado(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	estado_proceso* estado = list_get(paquete, 0);
	estado_proceso ret = *estado;
	free(estado);
	list_destroy(paquete);
	return ret;
}

void send_tiempo_io(int tiempo_io, int fd_modulo){
	t_paquete* paquete = crear_paquete(MANEJAR_IO);
	agregar_a_paquete(paquete, &(tiempo_io), sizeof(int));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

void send_recurso_wait(char* recurso, int fd_modulo){
	t_paquete* paquete = crear_paquete(MANEJAR_WAIT);
	agregar_a_paquete(paquete, recurso, strlen(recurso) + 1);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

void send_recurso_signal(char* recurso, int fd_modulo){
	t_paquete* paquete = crear_paquete(MANEJAR_SIGNAL);
	agregar_a_paquete(paquete, recurso, strlen(recurso) + 1);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

int recv_tiempo_io(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	int* tiempo = list_get(paquete, 0);
	int ret = *tiempo;
	free(tiempo);
	list_destroy(paquete);
	return ret;
}

char* recv_recurso(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	char* recurso = list_get(paquete, 0);
	list_destroy(paquete);

	return recurso;
}

void send_reserva_swap(int fd, int cant_paginas_necesarias){
	t_paquete* paquete = crear_paquete(INICIALIZAR_PROCESO);
	agregar_a_paquete(paquete, &(cant_paginas_necesarias), sizeof(int));
	enviar_paquete(paquete, fd);
	eliminar_paquete(paquete);
}
t_list* recv_reserva_swap(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	t_list* lista_bloques = list_create();
	for(int i=0; i<list_size(paquete); i++){
		int* cant = list_get(paquete, i);
		//int ret = *cant;
		list_add(lista_bloques, cant);
		//free(cant);
	}
	list_destroy(paquete);
	return lista_bloques;
}

void send_inicializar_proceso(pcb *contexto, int fd_modulo){
    t_paquete* paquete = crear_paquete(INICIALIZAR_PROCESO);
	empaquetar_pcb(paquete, contexto);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}
void send_tam_pagina(int tam, int fd_modulo){
	t_paquete* paquete = crear_paquete(TAMANIO_PAGINA);
	agregar_a_paquete(paquete, &(tam), sizeof(int));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}
int recv_tam_pagina(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	int* tiempo = list_get(paquete, 0);
	int ret = *tiempo;
	free(tiempo);
	list_destroy(paquete);
	return ret;
}



void send_terminar_proceso(int pid, int fd_modulo){
	t_paquete* paquete = crear_paquete(FINALIZAR_PROCESO);
	agregar_a_paquete(paquete, &pid, sizeof(int));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

int recv_terminar_proceso(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	int* pid = list_get(paquete, 0);

	//memory leaks
	int pid_value = *pid; // Guardar el valor del PID antes de destruir el paquete
    list_destroy_and_destroy_elements(paquete, free); // Liberar memoria del paquete y sus elementos
	
	return pid_value;
}

/*int send_f_open(){

}*/

//--------------------------------------Instruccion------------------------------------------
void empaquetar_instruccion(t_paquete* paquete, Instruccion instruccion) {
    agregar_a_paquete(paquete, instruccion.opcode, strlen(instruccion.opcode) + 1);
    agregar_a_paquete(paquete, instruccion.operando1, strlen(instruccion.operando1) + 1);
    agregar_a_paquete(paquete, instruccion.operando2, strlen(instruccion.operando2) + 1);
}

Instruccion desempaquetar_instruccion(t_list* paquete) {
    Instruccion instruccion;
    instruccion.opcode = (char*)list_get(paquete, 0);
    instruccion.operando1 = (char*)list_get(paquete, 1);
    instruccion.operando2 = (char*)list_get(paquete, 2);
    return instruccion;
}

void send_instruccion(int socket_cliente, Instruccion instruccion) {
    //printf("Enviando instruccion, opcode :%s, oper1: %s\n",instruccion.opcode, instruccion.operando1);
    t_paquete* paquete = crear_paquete(ENVIO_INSTRUCCION);
    empaquetar_instruccion(paquete, instruccion);
    enviar_paquete(paquete, socket_cliente);
    eliminar_paquete(paquete);
}

Instruccion recv_instruccion(int socket_cliente) {
    printf("Recibiendo instruccion");
    t_list* paquete = recibir_paquete(socket_cliente);
    Instruccion instruccion = desempaquetar_instruccion(paquete);
    list_destroy(paquete);
    return instruccion;
}

void destroyInstruccion(Instruccion instruccion) {
    free(instruccion.opcode);
    free(instruccion.operando1);
    free(instruccion.operando2);
}

void send_fetch_instruccion(char * path, int pc, int fd_modulo) {
t_paquete* paquete = crear_paquete(ENVIO_INSTRUCCION);

// Agregar el path al paquete
agregar_a_paquete(paquete, path, strlen(path) + 1);

// Agregar el PC al paquete
agregar_a_paquete(paquete, &pc, sizeof(int));

enviar_paquete(paquete, fd_modulo);
eliminar_paquete(paquete);

}

int recv_fetch_instruccion(int fd_modulo, char** path, int** pc) {
    t_list* paquete = recibir_paquete(fd_modulo);

	// Obtener el path del paquete
	*path = (char*) list_get(paquete, 0);

    *pc = (int*) list_get(paquete, 1);


    list_destroy(paquete);
    return 0; 
}


void send_liberacion_swap(int fd, int cantidad,int bloques[]){
    t_paquete* paquete = crear_paquete(FINALIZAR_PROCESO);
	agregar_a_paquete(paquete, &cantidad, sizeof(int));
    for(int i=0; i<cantidad;i++){
		int bloque = bloques[i];
		agregar_a_paquete(paquete, &bloque, sizeof(int));
	}
    enviar_paquete(paquete, fd);
    eliminar_paquete(paquete);
}

int recv_liberacion_swap(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	int* pid = list_get(paquete, 0);
	list_destroy(paquete);
	return *pid;
}



void empaquetar_bloques(t_paquete* paquete_bloques, t_list* lista_bloques) {
    int cantidad_bloques = list_size(lista_bloques);

    // Agregar la cantidad de bloques al paquete
    agregar_a_paquete(paquete_bloques, &cantidad_bloques, sizeof(int));

    for (int i = 0; i < cantidad_bloques; i++) {
        int* bloque_asignado = list_get(lista_bloques, i);

        // Agregar el numero de bloque al paquete
        agregar_a_paquete(paquete_bloques, bloque_asignado, sizeof(int));
    }
}

t_list* desempaquetar_bloques(t_list* paquete, int* comienzo) {
	t_list* lista_bloques = list_create();
	int* cantidad_bloques = list_get(paquete, *comienzo);
	int i = *comienzo + 1;

	while (i - *comienzo - 1 < *cantidad_bloques) {
		int* bloque = malloc(sizeof(int));

		// Desempaquetar la ruta del archivo
		int* bloque_asignado = (int*)list_get(paquete, i);
		free(bloque_asignado);
		i++;

		list_add(lista_bloques, bloque);
	}

	*comienzo = i;
	free(cantidad_bloques);
	return lista_bloques;
}

void send_bloques_reservados(int fd, t_list* bloques_reservados) {
  	t_paquete* paquete_bloques = crear_paquete(BLOQUES_RESERVADOS);
    empaquetar_bloques(paquete_bloques, bloques_reservados);
    enviar_paquete(paquete_bloques, fd);
    eliminar_paquete(paquete_bloques);
}

t_list* recv_bloques_reservados(int fd_modulo) {
    t_list* paquete = recibir_paquete(fd_modulo);
    t_list* lista_bloques_reservados = desempaquetar_bloques(paquete, 0);
    list_destroy(paquete);
    return lista_bloques_reservados;
}



//----------------------------------PCBDESALOJADO-----------------------------------------
void send_pcbDesalojado(pcb* contexto, char* instruccion, char* extra, int fd, t_log* logger){
	t_paquete* paquete;
	if(strcmp(instruccion,"SIGNAL")==0){
		paquete=crear_paquete(PCB_SIGNAL);
		empaquetar_pcb(paquete, contexto);
		agregar_a_paquete(paquete, extra, strlen(extra) + 1);
	}else if(strcmp(instruccion,"WAIT")==0){
		paquete=crear_paquete(PCB_WAIT);
		empaquetar_pcb(paquete, contexto);
		agregar_a_paquete(paquete, extra, strlen(extra) + 1);
	}else if(strcmp(instruccion,"EXIT")==0){
		paquete=crear_paquete(PCB_EXIT);
		empaquetar_pcb(paquete, contexto);
		agregar_a_paquete(paquete, extra, strlen(extra) + 1);
	}else if(strcmp(instruccion,"SLEEP")==0){
		paquete=crear_paquete(PCB_SLEEP);
		empaquetar_pcb(paquete, contexto);
		agregar_a_paquete(paquete, extra, strlen(extra) + 1);
	}else if(strcmp(instruccion,"INTERRUPCION")==0){
		paquete=crear_paquete(PCB_INTERRUPCION);
		empaquetar_pcb(paquete, contexto);
		agregar_a_paquete(paquete, extra, strlen(extra) + 1);
	}else if(strcmp(instruccion,"PAGEFAULT")==0){
		paquete=crear_paquete(PCB_PAGEFAULT);
		empaquetar_pcb(paquete, contexto);
		agregar_a_paquete(paquete, extra, strlen(extra) + 1);
	}else{
		log_error(logger,"NO SE RECONOCE EL MOTIVO DE DESALOJO");
		return;
	}
	enviar_paquete(paquete, fd);
	eliminar_paquete(paquete);
}
void recv_pcbDesalojado(int fd, pcb** contexto, char** extra) {
    t_list* paquete = recibir_paquete(fd);
    int counter;
    *contexto = desempaquetar_pcb(paquete, &counter);
    *extra = (char*) list_get(paquete, counter);
	list_destroy(paquete);
}

void send_interrupcion(int pid, int fd_modulo){
	t_paquete* paquete = crear_paquete(INTERRUPCION);
	agregar_a_paquete(paquete, &pid, sizeof(int));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

int recv_interrupcion(int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	int* tiempo = list_get(paquete, 0);
	int ret = *tiempo;
	free(tiempo);
	list_destroy(paquete);
	return ret;
}


void send_valor_leido_fs(char* valor, int tamanio, int fd_modulo){
	t_paquete* paquete = crear_paquete(PEDIDO_LECTURA_FS);
	agregar_a_paquete(paquete, valor, tamanio);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}
void send_escribir_valor_fs(char* valor, int dir_fisica, int tamanio, int pid, int fd_modulo){
	t_paquete* paquete = crear_paquete(PEDIDO_ESCRITURA_FS);
	agregar_a_paquete(paquete, valor, tamanio);
	agregar_a_paquete(paquete, &(dir_fisica), sizeof(int));
	agregar_a_paquete(paquete, &(tamanio), sizeof(int));
	agregar_a_paquete(paquete, &(pid), sizeof(int));
	enviar_paquete(paquete, fd_modulo);
}
void send_fin_escritura(int fd_modulo){
	t_paquete* paquete = crear_paquete(FIN_ESCRITURA);
	op_code cop = FIN_ESCRITURA;
	agregar_a_paquete(paquete, &cop, sizeof(op_code));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

void recv_f_open(int fd,char** nombre_archivo, char ** modo_apertura, pcb**contexto){
	t_list* paquete = recibir_paquete(fd);
	int counter = 0;
    *contexto = desempaquetar_pcb(paquete, &counter);
	*nombre_archivo = (char*) list_get(paquete, counter);
	counter++;
	*modo_apertura = (char*) list_get(paquete, counter);
	list_destroy(paquete);
}

void recv_f_truncate(int fd,char** nombre_archivo, int* tam, pcb** contexto){
	t_list* paquete = recibir_paquete(fd);
	int counter = 0;
    *contexto = desempaquetar_pcb(paquete, &counter);
	*nombre_archivo = (char*) list_get(paquete, counter);
	counter++;
	int* puntero = list_get(paquete, counter);
	*tam = *puntero;
	free(puntero);	
	list_destroy(paquete);
}

// LA UTILIZA F_READ Y F_WRITE YA QUE RECIBEN LO MISMO
void recv_f_write(int fd,char** nombre_archivo, DireccionFisica* direccion, pcb** contexto){
	t_list* paquete = recibir_paquete(fd);
	int counter = 0;
    *contexto = desempaquetar_pcb(paquete, &counter);
	*nombre_archivo = (char*) list_get(paquete, counter);
	counter++;
	int* puntero = list_get(paquete, counter);
	direccion->marco = *puntero;
	puntero = list_get(paquete, counter);
	direccion->desplazamiento = *puntero;
	free(puntero);
	list_destroy(paquete);
}

void recv_f_close(int fd, char** nombre_archivo, pcb**contexto){
	t_list* paquete = recibir_paquete(fd);
	int counter = 0;
    *contexto = desempaquetar_pcb(paquete, &counter);
	*nombre_archivo = (char*) list_get(paquete, counter);
	list_destroy(paquete);
}

void send_abrir_archivo(char* nombre_archivo, int fd_modulo){
	t_paquete* paquete = crear_paquete(ABRIR_ARCHIVO);
	agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}
void send_truncar(char*nombre_archivo,int tamanio_archivo,int fd){
	t_paquete* paquete = crear_paquete(TRUNCAR_ARCHIVO);
	agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
	agregar_a_paquete(paquete, &tamanio_archivo, sizeof(int));
	enviar_paquete(paquete, fd);
	eliminar_paquete(paquete);
}

void send_crear_archivo(char* nombre_archivo, int fd_modulo){
	t_paquete* paquete = crear_paquete(CREAR_ARCHIVO);
	agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

void send_leer_archivo(int fd_modulo, char* nombre_archivo, DireccionFisica direccion, int puntero){
	t_paquete* paquete = crear_paquete(F_READ);
	agregar_a_paquete(paquete, &(direccion.marco), sizeof(int));
	agregar_a_paquete(paquete, &(direccion.desplazamiento), sizeof(int));
	agregar_a_paquete(paquete, &puntero, sizeof(int));
	agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

void send_escribir_archivo(int fd_modulo, char* nombre_archivo, DireccionFisica direccion, int puntero){
	t_paquete* paquete = crear_paquete(F_WRITE);
	agregar_a_paquete(paquete, &(direccion.marco), sizeof(int));
	agregar_a_paquete(paquete, &(direccion.desplazamiento), sizeof(int));
	agregar_a_paquete(paquete, &puntero, sizeof(int));
	agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}


//----------------------------------PAGE FAULT-----------------------------------------
void send_pedido_swap(int fd, int posicion_swap){
	//printf("Enviando pedido de swap\n");
	t_paquete* paquete = crear_paquete(PEDIDO_SWAP);
	agregar_a_paquete(paquete, &posicion_swap, sizeof(int));
	enviar_paquete(paquete, fd);
	eliminar_paquete(paquete);
}

char* recv_pedido_swap(int fd_modulo){
	//printf("Recibiendo pedido de swap\n");
	t_list* paquete = recibir_paquete(fd_modulo);
	char* valor = list_get(paquete, 0);
	list_destroy(paquete);
	return valor;
}

void send_leido_swap(int fd, char * leido, int tam_pagina){
	//printf("Enviando leido de swap\n");
	t_paquete* paquete = crear_paquete(PEDIDO_SWAP);
	agregar_a_paquete(paquete, leido, tam_pagina);
	enviar_paquete(paquete, fd);
	eliminar_paquete(paquete);
}

char * recv_leido_swap(int fd_modulo){
	//printf("Recibiendo leido de swap\n");
    op_code cop = recibir_operacion(fd_modulo);
	t_list* paquete = recibir_paquete(fd_modulo);
	char* leido = list_get(paquete, 0);
	list_destroy(paquete);
	return leido;
}

void send_pedido_marco(int fd_modulo, int pid, int pagina){
	//printf("Enviando pedido de marco\n");
	t_paquete* paquete = crear_paquete(PEDIDO_MARCO);
	agregar_a_paquete(paquete, &pid, sizeof(int));
	agregar_a_paquete(paquete, &pagina, sizeof(int));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

void recv_pedido_marco(int fd_modulo, int *pid, int *pagina){
	t_list* paquete = recibir_paquete(fd_modulo);
	int* puntero = list_get(paquete, 0);
	*pid = *puntero;
	free(puntero);	
	puntero = list_get(paquete, 1);
	*pagina = *puntero;
	free(puntero);//memory leaks
	list_destroy(paquete);
}

void send_marco(int fd_modulo, int memoria_fisica){
	//printf("Enviando marco\n");
	t_paquete* paquete = crear_paquete(ENVIO_MARCO);
	agregar_a_paquete(paquete, &memoria_fisica, sizeof(int));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

int recv_marco(int fd_modulo){
	//printf("Recibiendo marco\n");
	t_list* paquete = recibir_paquete(fd_modulo);
	int* tiempo = list_get(paquete, 0);
	int ret = *tiempo;
	free(tiempo);
	list_destroy(paquete);
	return ret;
}

void send_pcb_page_fault(int fd_modulo, pcb* contexto, int pagina){
	//printf("\n ENVIANDO POR PAGE FAULT PCB \n");
	t_paquete* paquete = crear_paquete(PCB_PAGEFAULT);
	empaquetar_pcb(paquete, contexto);
	agregar_a_paquete(paquete, &pagina, sizeof(int));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

void recv_pcb_page_fault(int fd_modulo, pcb** contexto, int *pagina){
	//printf("\n RECIBIENDO POR PAGE FAULT PCB \n");
	t_list* paquete = recibir_paquete(fd_modulo);
	int counter = 0;
	*contexto = desempaquetar_pcb(paquete, &counter);
	int* puntero = list_get(paquete, counter);
	*pagina = *puntero;
	free(puntero);	
	list_destroy(paquete);
}

void send_cargar_pagina(int fd_modulo, int pid, int pagina){
	//printf("\n ENVIANDO CARGAR PAGINA \n");
	t_paquete* paquete = crear_paquete(CARGAR_PAGINA);
	agregar_a_paquete(paquete, &pid, sizeof(int));
	agregar_a_paquete(paquete, &pagina, sizeof(int));
	enviar_paquete(paquete, fd_modulo);
	eliminar_paquete(paquete);
}

void recv_cargar_pagina(int fd_modulo, int *pid, int *pagina){
	//printf("\n RECIBIENDO CARGAR PAGINA \n");
	t_list* paquete = recibir_paquete(fd_modulo);
	int* puntero = list_get(paquete, 0);
	*pid = *puntero;//memory leaks
	free(puntero);
	puntero = list_get(paquete, 1);
	*pagina = *puntero;//memory leaks
	free(puntero);	
	list_destroy(paquete);
}


int recv_respuesta_abrir_archivo(int fd){
	op_code cop = recibir_operacion(fd);
	int tamanio;
	t_list* paquete = recibir_paquete(fd);
	int* respuesta = list_get(paquete, 0);
	tamanio = *respuesta;
	free(respuesta);
	list_destroy(paquete);
	return tamanio;
}