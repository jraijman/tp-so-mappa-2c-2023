#include "./protocolo.h"

// -------------------------------------OPERACIONES DE ENVIO DE PCB----------------------------------------


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
    void* stream = malloc(sizeof(op_code) + sizeof(pcb));

    op_code cop = ENVIO_PCB;
    memcpy(stream, &cop, sizeof(op_code));
    memcpy(stream+sizeof(op_code), &proceso, sizeof(pcb));
    return stream;
}

static void deserializar_pcb(void* stream, pcb* proceso) {
    memcpy(proceso, stream, sizeof(pcb));
}
bool send_pcb(int fd,pcb* proceso){
    size_t size = sizeof(op_code) + sizeof(pcb);
    void* stream = serializar_pcb(*proceso);
    if (send(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}

bool recv_pcb(int fd,pcb* proceso) {
    size_t size = sizeof(pcb);
    void* stream = malloc(size);

    if (recv(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }

    deserializar_pcb(stream, proceso);

    free(stream);
    return true;
}
//--------------------------------------ENVIO PCB DESALOJADO POR CPU-------------------------

static void* serializar_pcbDesalojado(pcbDesalojado proceso) {
    size_t instruccion_len = strlen(proceso.instruccion) + 1;
    size_t extra_len = strlen(proceso.extra) + 1;
    size_t stream_size = sizeof(op_code) + sizeof(int) + sizeof(pcb) + instruccion_len + extra_len;
    
    void* stream = malloc(stream_size);

    op_code cop = ENVIO_PCB_DESALOJADO;
    int instruccion_len_int = (int)instruccion_len;
    int extra_len_int = (int)extra_len;

    memcpy(stream, &cop, sizeof(op_code));
    memcpy(stream + sizeof(op_code), &instruccion_len_int, sizeof(int));
    memcpy(stream + sizeof(op_code) + sizeof(int), &extra_len_int, sizeof(int));
    memcpy(stream + sizeof(op_code) + 2 * sizeof(int), &proceso.contexto, sizeof(pcb));
    memcpy(stream + sizeof(op_code) + 2 * sizeof(int) + sizeof(pcb), proceso.instruccion, instruccion_len);
    memcpy(stream + sizeof(op_code) + 2 * sizeof(int) + sizeof(pcb) + instruccion_len, proceso.extra, extra_len);

    return stream;
}

static void deserializar_pcbDesalojado(void* stream, pcbDesalojado* proceso) {
    int instruccion_len_int, extra_len_int;
    
    memcpy(&instruccion_len_int, stream + sizeof(op_code), sizeof(int));
    memcpy(&extra_len_int, stream + sizeof(op_code) + sizeof(int), sizeof(int));
    
    size_t instruccion_len = (size_t)instruccion_len_int;
    size_t extra_len = (size_t)extra_len_int;

    memcpy(&proceso->contexto, stream + sizeof(op_code) + 2 * sizeof(int), sizeof(pcb));

    proceso->instruccion = (char*)malloc(instruccion_len);
    proceso->extra = (char*)malloc(extra_len);

    memcpy(proceso->instruccion, stream + sizeof(op_code) + 2 * sizeof(int) + sizeof(pcb), instruccion_len);
    memcpy(proceso->extra, stream + sizeof(op_code) + 2 * sizeof(int) + sizeof(pcb) + instruccion_len, extra_len);
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
    void* stream = malloc(sizeof(op_code) + sizeof(Instruccion));

    op_code cop = ENVIO_INSTRUCCION;
    memcpy(stream, &cop, sizeof(op_code));
    memcpy(stream + sizeof(op_code), &instruccion, sizeof(Instruccion));
    return stream;
}

static void deserializar_instruccion(void* stream, Instruccion* instruccion) {
    memcpy(instruccion, stream + sizeof(op_code), sizeof(Instruccion));
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

/*static void* serializar_aprobar_operativos(uint8_t nota1, uint8_t nota2) {
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