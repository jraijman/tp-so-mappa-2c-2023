#include "./protocolo.h"

void* serializar_paquete(t_paquete* paquete, int bytes) {
    void* magic = malloc(bytes);
    int desplazamiento = 0;

    // Copia el código de operación al búfer
    memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
    desplazamiento += sizeof(int);

    // Copia el tamaño del buffer al búfer
    memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
    desplazamiento += sizeof(int);

    // Copia los datos del buffer al búfer
    memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
    desplazamiento += paquete->buffer->size;

    return magic;
}

void eliminar_paquete(t_paquete* paquete) {
    // Liberar memoria de los datos en el paquete
    free(paquete->buffer->stream);

    // Liberar memoria del buffer
    free(paquete->buffer);

    // Liberar memoria del paquete
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
        log_info(logger, "Me llegó el mensaje: %s", buffer);
        free(buffer); // Liberar la memoria del mensaje recibido
    }
}

t_list* recibir_paquete(int socket_cliente){
	int size;
	int desplazamiento = 0;
	void* buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
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
        t_archivos* archivo = list_get(lista_archivos, i);

        // Agregar la ruta del archivo al paquete
        agregar_a_paquete(paquete_archivos, archivo->path, strlen(archivo->path) + 1);

        // Agregar el puntero al paquete
        agregar_a_paquete(paquete_archivos, &(archivo->puntero), sizeof(int));
    }
}

t_list* desempaquetar_archivos(t_list* paquete, int* comienzo) {
    t_list* lista_archivos = list_create();
    int* cantidad_archivos = list_get(paquete, *comienzo);
    int i = *comienzo + 1;

    while (i - *comienzo - 1 < (*cantidad_archivos * 2)) {
        t_archivos* archivo = malloc(sizeof(t_archivos));

        // Desempaquetar la ruta del archivo
        char* path = (char*)list_get(paquete, i);
        archivo->path = strdup(path);
        free(path);
        i++;

        // Desempaquetar el puntero
        int* puntero = list_get(paquete, i);
        archivo->puntero = puntero;
        free(puntero);
        i++;

        list_add(lista_archivos, archivo);
    }
	comienzo*=i;
    free(cantidad_archivos);
    return lista_archivos;
}

void send_archivos(int fd_modulo, t_list* lista_archivos) {
    t_paquete* paquete_archivos = crear_paquete(ENVIO_LISTA_ARCHIVOS);
    empaquetar_archivos(paquete_archivos, lista_archivos);
    enviar_paquete(paquete_archivos, fd_modulo);
    eliminar_paquete(paquete_archivos);
}

t_list* recv_archivos(t_log* logger, int fd_modulo) {
    t_list* paquete = recibir_paquete(fd_modulo);
    t_list* lista_archivos = desempaquetar_archivos(paquete, 0);
    list_destroy(paquete);
    log_info(logger, "Se recibió una lista de archivos.");
    return lista_archivos;
}

void archivo_destroyer(t_archivos* archivo) {
    free(archivo->path);
    free(archivo);
    archivo = NULL;
}

void lista_archivos_destroy(t_list* lista) {
    while (!list_is_empty(lista)) {
        t_archivos* archivo = list_remove(lista, 0);
        archivo_destroyer(archivo);
    }
    list_destroy(lista);
}

void empaquetar_registros(t_paquete* paquete, t_registros* registro){
	agregar_a_paquete(paquete,&(registro->ax), sizeof(int));
	agregar_a_paquete(paquete,&(registro->bx), sizeof(int));
	agregar_a_paquete(paquete,&(registro->cx), sizeof(int));
	agregar_a_paquete(paquete,&(registro->dx), sizeof(int));
}

t_registros* desempaquetar_registros(t_list * paquete,int posicion){
	t_registros *registro = malloc(sizeof(t_registros));

	int* ax = list_get(paquete,posicion);
	registro->ax = ax;
	free(ax);

	int* bx = list_get(paquete,posicion+1);
	registro->bx = bx;
	free(bx);

	int* cx = list_get(paquete,posicion+2);
    registro->cx = cx;
	free(cx);

	int* dx = list_get(paquete,posicion+3);
	registro->dx = dx;
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

pcb* desempaquetar_pcb(t_list* paquete, int* counter){
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
	contexto->path =  malloc(strlen(path));
	strcpy(contexto->path, path);
	free(path);

	estado_proceso* estado =  list_get(paquete, 6);
	contexto->estado = *estado;
	free(estado);

	int comienzo_registros = 7;
	t_registros* registro_contexto = desempaquetar_registros(paquete, comienzo_registros);
	contexto->registros = registro_contexto;

	counter = comienzo_registros + 4;
	t_list* archivos = desempaquetar_archivos(paquete, counter);
	contexto->archivos = archivos;

    
    return contexto;
}



void send_pcb(pcb* contexto, int fd_modulo){
	printf("Enviando pcb");
    t_paquete* paquete = crear_paquete(ENVIO_PCB);
	empaquetar_pcb(paquete, contexto);
	enviar_paquete(paquete, fd_modulo);
	pcb_destroyer(contexto);
	eliminar_paquete(paquete);
}

pcb* recv_pcb(int fd_modulo){
    printf("Recibiendo pcb");
	t_list* paquete = recibir_paquete(fd_modulo);
	pcb* contexto_recibido = desempaquetar_pcb(paquete);
	list_destroy(paquete);
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
	lista_archivos_destroy(contexto->archivos);
	//list_destroy_and_destroy_elements(contexto->tabla_de_segmentos, (void*) segmento_destroy);
	registros_destroy(contexto->registros);
	free(contexto);
	contexto = NULL;
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

void send_inicializar_proceso(pcb *contexto, int fd_modulo){
    t_paquete* paquete = crear_paquete(INICIALIZAR_PROCESO);
	empaquetar_pcb(paquete, contexto);
	enviar_paquete(paquete, fd_modulo);
	//pcb_destroyer(contexto); no se   porq lo destruye
	eliminar_paquete(paquete);
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
	list_destroy(paquete);
	return *pid;
}
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
    printf("Enviando instruccion");
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

void send_fetch_instruccion(int pid, int pc, int fd_modulo) {
    t_paquete* paquete = crear_paquete(ENVIO_INSTRUCCION);

    // Agregar el PID al paquete
    agregar_a_paquete(paquete, &pid, sizeof(int));

    // Agregar el PC al paquete
    agregar_a_paquete(paquete, &pc, sizeof(int));

    enviar_paquete(paquete, fd_modulo);
    eliminar_paquete(paquete);
}

int recv_fetch_instruccion(int fd_modulo, int* pid, int* pc) {
    t_list* paquete = recibir_paquete(fd_modulo);

    // Obtener el PID del paquete
    int* pid_recv = list_get(paquete, 0);
    *pid = *pid_recv;
    free(pid_recv);

    // Obtener el PC del paquete
    int* pc_recv = list_get(paquete, 1);
    *pc = *pc_recv;
    free(pc_recv);

    list_destroy(paquete);
    return 0; // Puedes devolver el valor necesario en tu implementación.
}

void send_pcbDesalojado(pcb* contexto, char* instruccion, char* extra, int fd, t_log* logger){
	t_paquete* paquete;
	if(strcmp(instruccion,"SIGNAL")==0){
		paquete=crear_paquete(PCB_SIGNAL);
		empaquetar_pcb(paquete, contexto);
		agregar_a_paquete(paquete, extra);
	}else if(strcmp(instruccion,"WAIT")==0){
		paquete=crear_paquete(PCB_WAIT);
		empaquetar_pcb(paquete, contexto);
		agregar_a_paquete(paquete, extra);
	}else if(strcmp(instruccion,"EXIT")==0){
		paquete=crear_paquete(PCB_EXIT);
		empaquetar_pcb(paquete, contexto);
		agregar_a_paquete(paquete, extra);
	}else if(strcmp(instruccion,"SLEEP")==0){
		paquete=crear_paquete(PCB_SLEEP);
		empaquetar_pcb(paquete, contexto);
		agregar_a_paquete(paquete, extra);
	}else if(strcmp(instruccion,"INTERRUPCION")==0){
		paquete=crear_paquete(PCB_INTERRUPCION);
		empaquetar_pcb(paquete, contexto);
		agregar_a_paquete(paquete, extra);
	}else{log_error(logger,"NO SE RECONOCE EL MOTIVO DE DESALOJO")
	return;}
	enviar_paquete(paquete, fd);
}
void recv_pcbDesalojado(int fd,pcb* contexto, char* extra){
	t_list* paquete=recibir_paquete(fd);
	int counter;
	contexto=desempaquetar_pcb(paquete,&counter);
	extra=(char*)list_get(paquete, counter);
}