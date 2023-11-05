#include "./protocolo.h"

void* serializar_paquete(t_paquete* paquete, int bytes){
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

void eliminar_paquete(t_paquete* paquete){
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

void recibir_mensaje(t_log* logger, int socket_cliente){
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje: %s", buffer);
	free(buffer);
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

void empaquetar_archivos(t_paquete* paquete_archivos, t_list* lista_archivos){
	int cantidad_archivos = list_size(lista_archivos);
	agregar_a_paquete(paquete_archivos, &(cantidad_archivos), sizeof(int));
	for(int i=0; i<cantidad_archivos; i++){
		t_archivos* archivo = list_get(lista_archivos, i);
		agregar_a_paquete(paquete_archivos, archivo->path, strlen(archivo->path) + 1);
		agregar_a_paquete(paquete_archivos, archivo->puntero, sizeof(int));
	}
}
t_list* desempaquetar_archivos(t_list* paquete, int comienzo){
	t_list* lista_archivos = list_create();
	int* cantidad_archivos = list_get(paquete, comienzo);
	int i = comienzo + 1;

	while(i - comienzo - 1< (*cantidad_archivos* 4)){
		t_archivos* archivo = malloc(sizeof(t_archivos));

		char* path = list_get(paquete, i);
		archivo->path = malloc(strlen(path));
		strcpy(archivo->path, path);
		free(path);
		i++;

        int* puntero = list_get(paquete, i);
		archivo->puntero =  puntero;
		free(puntero);
		i++;


		list_add(lista_archivos, archivo);
	}
	free(cantidad_archivos);
	return lista_archivos;
}

void send_archivos(int fd_modulo,t_list* lista_archivos){
	t_paquete* paquete_archivos = crear_paquete(ENVIO_LISTA_ARCHIVOS);
	empaquetar_archivos(paquete_archivos, lista_archivos);
	enviar_paquete(paquete_archivos, fd_modulo);
	eliminar_paquete(paquete_archivos);
}

t_list* recv_archivos(t_log* logger, int fd_modulo){
	t_list* paquete = recibir_paquete(fd_modulo);
	t_list* lista_archivos = desempaquetar_archivos(paquete, 0);
	list_destroy(paquete);
	log_info(logger, "Se recibió una lista de archivos.");
	return lista_archivos;
}


void archivo_destroyer(t_archivos* archivo){
	free(archivo->path);
	free(archivo->puntero);
	free(archivo);
	archivo = NULL;
}

void lista_archivos_destroy(t_list* lista){
	while(!list_is_empty(lista)){
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
	agregar_a_paquete(paquete, &(contexto->estado), sizeof(estado_proceso));
	empaquetar_registros(paquete, contexto->registros);
	empaquetar_archivos(paquete, contexto->archivos);
    
}

pcb* desempaquetar_pcb(t_list* paquete){
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

	estado_proceso* estado =  list_get(paquete, 5);
	contexto->estado = *estado;
	free(estado);

	int comienzo_registros = 6;
	t_registros* registro_contexto = desempaquetar_registros(paquete, comienzo_registros);
	contexto->registros = registro_contexto;

	int comienzo_archivos = comienzo_registros + 4;
	t_list* archivos = desempaquetar_archivos(paquete, comienzo_archivos);
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
// -------------------------------------NO LAS USAMOS----------------------------------------

/*
static void* serializar_pid(int pid) {
   void* stream = malloc(sizeof(int));
    memcpy(stream, &pid, sizeof(int));
    return stream;
}

void deserializar_pid(void* stream, int* pid) {
    memcpy(pid, stream ,sizeof(int));
}
//enviar un entero, sirve para PID, pc, size
bool send_int(int fd,int pid){
	size_t size = sizeof(int);
	void* stream = serializar_pid(pid);
    if (send(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }

    free(stream);
    return true;
}

//recibir un entero, sirve para PID, pc, size
bool recv_int(int fd, int* pid) {
    size_t size = sizeof(int);
    void* stream = malloc(size);
    if (recv(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }
    deserializar_pid(stream, pid);
    free(stream);
    return true;
}

//----------------------------------------------
//envio de pcb

static void* serializar_pcb(pcb proceso) {
    void* stream = malloc(sizeof(op_code) + sizeof(int) + sizeof(int) + sizeof(int) +
                         sizeof(uint32_t) * 4 + sizeof(int) + strlen(proceso.path) + 1);
    
    op_code cop = ENVIO_PCB;
    memcpy(stream, &cop, sizeof(op_code));
    void* offset = stream + sizeof(op_code);
    
    memcpy(offset, &proceso.pid, sizeof(int));
    offset += sizeof(int);
    memcpy(offset, &proceso.pc, sizeof(int));
    offset += sizeof(int);
    memcpy(offset, &proceso.size, sizeof(int));
    offset += sizeof(int);
    memcpy(offset, &proceso.registros, sizeof(struct Reg));
    offset += sizeof(struct Reg);
    memcpy(offset, &proceso.prioridad, sizeof(int));
    offset += sizeof(int);
    size_t path_len = strlen(proceso.path) + 1;
    memcpy(offset, &path_len, sizeof(size_t));
    offset += sizeof(size_t);
    memcpy(offset, proceso.path, path_len);
    
    return stream;
}

static void deserializar_pcb(void* stream, pcb* proceso) {
    void* offset = stream + sizeof(op_code);
    
    memcpy(&proceso->pid, offset, sizeof(int));
    offset += sizeof(int);
    memcpy(&proceso->pc, offset, sizeof(int));
    offset += sizeof(int);
    memcpy(&proceso->size, offset, sizeof(int));
    offset += sizeof(int);
    memcpy(&proceso->registros, offset, sizeof(struct Reg));
    offset += sizeof(struct Reg);
    memcpy(&proceso->prioridad, offset, sizeof(int));
    offset += sizeof(int);
    size_t path_len;
    memcpy(&path_len, offset, sizeof(size_t));
    offset += sizeof(size_t);
    memcpy(proceso->path, offset, path_len);
}
bool send_pcb(int fd, pcb* proceso) {
    printf("Enviando PCB...\n");
    void* stream = serializar_pcb(*proceso);
    size_t size = sizeof(op_code) + sizeof(pcb);

    ssize_t bytes_enviados = send(fd, stream, size, 0);

    if (bytes_enviados == size) {
        printf("PCB enviado correctamente.\n");
        free(stream);
        return true;
    } else if (bytes_enviados == -1) {
        printf("Error al enviar el PCB");
        printf("Cerrando la conexión debido a un error.\n");
        free(stream);
        return false;
    } else {
        printf("Error inesperado al enviar el PCB.\n");
        printf("Cerrando la conexión debido a un error inesperado.\n");
        free(stream);
        return false;
    }
}

bool recv_pcb(int fd, pcb* proceso) {
    printf("Esperando recibir un PCB...");
    size_t size = sizeof(pcb);
    void* stream = malloc(size);

    ssize_t bytes_recibidos = recv(fd, stream, size, MSG_WAITALL);

    if (bytes_recibidos == size) {
        printf("PCB recibido correctamente.");
        deserializar_pcb(stream, proceso);
        free(stream);
        return true;
    } else if (bytes_recibidos == 0) {
        printf("La conexión fue cerrada por el otro extremo durante la recepción del PCB.");
        free(stream);
        return false;
    } else if (bytes_recibidos == -1) {
        printf("Cerrando la conexión debido a un error.");
        free(stream);
        return false;
    } else {
        free(stream);
        return false;
    }
}

//--------------------------------------ENVIO PCB DESALOJADO POR CPU-------------------------
static void* serializar_pcbDesalojado(pcbDesalojado proceso) {
    size_t instruccion_len = strlen(proceso.instruccion) + 1;
    size_t extra_len = strlen(proceso.extra) + 1;
    size_t stream_size = sizeof(op_code) + sizeof(int) + instruccion_len + extra_len + sizeof(pcb);
    
    void* stream = malloc(stream_size);

    op_code cop = ENVIO_PCB_DESALOJADO;
    int instruccion_len_int = (int)instruccion_len;
    
    memcpy(stream, &cop, sizeof(op_code));
    void* offset = stream + sizeof(op_code);
    memcpy(offset, &instruccion_len_int, sizeof(int));
    offset += sizeof(int);
    memcpy(offset, proceso.instruccion, instruccion_len);
    offset += instruccion_len;
    memcpy(offset, proceso.extra, extra_len);
    offset += extra_len;
    
    // Serializa la estructura pcb
    void* pcb_stream = serializar_pcb(proceso.contexto);
    memcpy(offset, pcb_stream, sizeof(pcb));
    free(pcb_stream);

    return stream;
}

static void deserializar_pcbDesalojado(void* stream, pcbDesalojado* proceso) {
    int instruccion_len_int;
    
    memcpy(&instruccion_len_int, stream + sizeof(op_code), sizeof(int));
    void* offset = stream + sizeof(op_code) + sizeof(int);
    
    size_t instruccion_len = (size_t)instruccion_len_int;
    
    proceso->instruccion = malloc(instruccion_len);
    memcpy(proceso->instruccion, offset, instruccion_len);
    offset += instruccion_len;
    
    size_t extra_len = strlen(offset) + 1;
    proceso->extra = malloc(extra_len);
    memcpy(proceso->extra, offset, extra_len);
    offset += extra_len;
    pcb base;
    // Deserializa la estructura pcb
    deserializar_pcb(offset, &proceso->contexto);
    proceso->contexto=base;
}


bool send_pcbDesalojado(pcbDesalojado proceso, int fd) {
    void* stream = serializar_pcbDesalojado(proceso);
    size_t size = sizeof(op_code) + 2 * sizeof(int) + sizeof(pcb) + strlen(proceso.instruccion) + 1 + strlen(proceso.extra) + 1;

    if (send(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }

    free(stream);
    return true;
}

bool recv_pcbDesalojado(int fd, pcbDesalojado* proceso) {
    size_t size = sizeof(op_code) + 2 * sizeof(int) + sizeof(pcb);
    void* stream = malloc(size);

    if (recv(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }

    deserializar_pcbDesalojado(stream, proceso);

    free(stream);
    return true;
}
//--------------------------------------Envio de instrucciones-------------------------------
static void* serializar_instruccion(Instruccion instruccion) {
    void* stream = malloc(sizeof(op_code) + sizeof(instruccion));
    
    op_code cop = ENVIO_INSTRUCCION;
    memcpy(stream, &cop, sizeof(op_code));
    void* offset = stream + sizeof(op_code);
    
    memcpy(offset, instruccion.opcode, sizeof(instruccion.opcode));
    offset += sizeof(instruccion.opcode);
    memcpy(offset, instruccion.operando1, sizeof(instruccion.operando1));
    offset += sizeof(instruccion.operando1);
    memcpy(offset, instruccion.operando2, sizeof(instruccion.operando2));
    
    return stream;
}

static void deserializar_instruccion(void* stream, Instruccion* instruccion) {
    void* offset = stream + sizeof(op_code);
    
    memcpy(instruccion->opcode, offset, sizeof(instruccion->opcode));
    offset += sizeof(instruccion->opcode);
    memcpy(instruccion->operando1, offset, sizeof(instruccion->operando1));
    offset += sizeof(instruccion->operando1);
    memcpy(instruccion->operando2, offset, sizeof(instruccion->operando2));
}

bool send_instruccion(int fd, Instruccion instruccion) {
    size_t size = sizeof(op_code) + sizeof(Instruccion);
    void* stream = serializar_instruccion(instruccion);
    if (send(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}

bool recv_instruccion(int fd, Instruccion* instruccion) {
    size_t size = sizeof(Instruccion);
    void* stream = malloc(size);

    if (recv(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }

    deserializar_instruccion(stream, instruccion);

    free(stream);
    return true;
}
//--------------------------------------Envio de direcciones---------------------------------
static void* serializar_direccion(Direccion direccion) {
    void* stream = malloc(sizeof(op_code) + sizeof(Direccion));

    op_code cop = ENVIO_DIRECCION;
    memcpy(stream, &cop, sizeof(op_code));
    memcpy(stream + sizeof(op_code), &direccion, sizeof(Direccion));
    return stream;
}

static void deserializar_direccion(void* stream, Direccion* direccion) {
    memcpy(direccion, stream + sizeof(op_code), sizeof(Direccion));
}

bool send_direccion(int fd, Direccion* direccion) {
    size_t size = sizeof(op_code) + sizeof(Direccion);
    void* stream = serializar_direccion(*direccion);
    if (send(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }
    free(stream);
    printf("Envío de dirección exitoso\n");
    return true;
}

bool recv_direccion(int fd, Direccion* direccion) {
    size_t size = sizeof(Direccion);
    void* stream = malloc(size);

    if (recv(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }

    deserializar_direccion(stream, direccion);

    free(stream);
    printf("Recepción de dirección exitosa\n");
    return true;
}
// -------------------------------------EJEMPLOS DE FUNCIONES--------------------------------

static void* serializar_aprobar_operativos(uint8_t nota1, uint8_t nota2) {
    void* stream = malloc(sizeof(op_code) + sizeof(uint8_t) * 2);

    op_code cop = APROBAR_OPERATIVOS;
    memcpy(stream, &cop, sizeof(op_code));
    memcpy(stream+sizeof(op_code), &nota1, sizeof(uint8_t));
    memcpy(stream+sizeof(op_code)+sizeof(uint8_t), &nota2, sizeof(uint8_t));
    return stream;
}

static void deserializar_aprobar_operativos(void* stream, uint8_t* nota1, uint8_t* nota2) {
    memcpy(nota1, stream, sizeof(uint8_t));
    memcpy(nota2, stream+sizeof(uint8_t), sizeof(uint8_t));
}

bool send_aprobar_operativos(int fd, uint8_t nota1, uint8_t nota2) {
    size_t size = sizeof(op_code) + sizeof(uint8_t) * 2;
    void* stream = serializar_aprobar_operativos(nota1, nota2);
    if (send(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}

bool recv_aprobar_operativos(int fd, uint8_t* nota1, uint8_t* nota2) {
    size_t size = sizeof(uint8_t) * 2;
    void* stream = malloc(size);

    if (recv(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }

    deserializar_aprobar_operativos(stream, nota1, nota2);

    free(stream);
    return true;
}

// MIRAR_NETFLIX
static void* serializar_mirar_netflix(size_t* size, char* peli, uint8_t cant_pochoclos) {
    size_t size_peli = strlen(peli) + 1;
    *size =
          sizeof(op_code)   // cop
        + sizeof(size_t)    // total
        + sizeof(size_t)    // size de char* peli
        + size_peli         // char* peli
        + sizeof(uint8_t);  // cant_pochoclos
    size_t size_payload = *size - sizeof(op_code) - sizeof(size_t);

    void* stream = malloc(*size);

    op_code cop = MIRAR_NETFLIX;
    memcpy(stream, &cop, sizeof(op_code));
    memcpy(stream+sizeof(op_code), &size_payload, sizeof(size_t));
    memcpy(stream+sizeof(op_code)+sizeof(size_t), &size_peli, sizeof(size_t));
    memcpy(stream+sizeof(op_code)+sizeof(size_t)*2, peli, size_peli);
    memcpy(stream+sizeof(op_code)+sizeof(size_t)*2+size_peli, &cant_pochoclos, sizeof(uint8_t));

    return stream;
}

static void deserializar_mirar_netflix(void* stream, char** peli, uint8_t* cant_pochoclos) {
    // Peli
    size_t size_peli;
    memcpy(&size_peli, stream, sizeof(size_t));

    char* r_peli = malloc(size_peli);
    memcpy(r_peli, stream+sizeof(size_t), size_peli);
    *peli = r_peli;

    // Pochoclos
    memcpy(cant_pochoclos, stream+sizeof(size_t)+size_peli, sizeof(uint8_t));
}

bool send_mirar_netflix(int fd, char* peli, uint8_t cant_pochoclos) {
    size_t size;
    void* stream = serializar_mirar_netflix(&size, peli, cant_pochoclos);
    if (send(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}

bool recv_mirar_netflix(int fd, char** peli, uint8_t* cant_pochoclos) {
    size_t size_payload;
    if (recv(fd, &size_payload, sizeof(size_t), 0) != sizeof(size_t))
        return false;

    void* stream = malloc(size_payload);
    if (recv(fd, stream, size_payload, 0) != size_payload) {
        free(stream);
        return false;
    }

    deserializar_mirar_netflix(stream, peli, cant_pochoclos);

    free(stream);
    return true;
}

// DEBUG
bool send_debug(int fd) {
    op_code cop = DEBUG;
    if (send(fd, &cop, sizeof(op_code), 0) != sizeof(op_code))
        return false;
    return true;
}*/