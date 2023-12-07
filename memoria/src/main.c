
#include "main.h"

pthread_mutex_t mx_memoria = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_bitarray_marcos_ocupados = PTHREAD_MUTEX_INITIALIZER;
uint8_t* bitarray_marcos_ocupados;
void* memoria;
void levantar_config(char *ruta)
{
    logger_memoria = iniciar_logger("memoria.log", "MEMORIA:");

    config = iniciar_config(ruta);
    puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    ip_filesystem = config_get_string_value(config, "IP_FILESYSTEM");
    tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    puerto_filesystem = config_get_string_value(config, "PUERTO_FILESYSTEM");
    tam_pagina = config_get_int_value(config, "TAM_PAGINA");
    path_instrucciones = config_get_string_value(config, "PATH_INSTRUCCIONES");
    RETARDO_REPUESTA = config_get_int_value(config, "RETARDO_RESPUESTA");
    algoritmo_reemplazo = config_get_string_value(config, "ALGORITMO_REEMPLAZO");
}

int main(int argc, char *argv[])
{

    if (argc < 2) {
        fprintf(stderr, "Se esperaba: %s [CONFIG_PATH]\n", argv[0]);
        exit(1);
    }
    
    // CONFIG y logger
    levantar_config(argv[1]);

    // genero conexion a filesystem
    conexion_memoria_filesystem = crear_conexion(logger_memoria, "FILESYSTEM", ip_filesystem, puerto_filesystem);
    enviar_mensaje("Hola soy MEMORIA", conexion_memoria_filesystem);

     // inicio servidor de escucha
    fd_memoria = iniciar_servidor(logger_memoria, NULL, puerto_escucha, "MEMORIA");
    //mando a fs para que se conecte
    t_paquete* paquete = crear_paquete(CONEXION_MEMORIA);
	enviar_paquete(paquete, conexion_memoria_filesystem);
	eliminar_paquete(paquete);
    
    // espero clientes kernel,cpu y filesystem
    pcb* proceso;
    int bytes_recibidos = 0;
    int pid;
    Proceso *tabla_proceso;
    t_marco* marcos;
    lista_tablas_de_procesos = list_create();
    sem_init(&swap_asignado, 0, 0);

    while(server_escuchar(fd_memoria));
   
    // CIERRO LOG Y CONFIG y libero conexion
    terminar_programa(logger_memoria, config);
    liberar_conexion(conexion_memoria_filesystem);
}

// ----------------------COMUNICACION----------------------------------

static void procesar_conexion(void *void_args) {
	int *args = (int*) void_args;
	int cliente_socket = *args;

	op_code cop;
    t_list* parametros_escritura_fs;
    int* pid_escritura_fs;
    int* tam_esc_fs;
	while (cliente_socket != -1) {
		if (recv(cliente_socket, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
			log_info(logger_memoria, ANSI_COLOR_BLUE"El cliente se desconecto de %s server", server_name);
			return;
		}
		switch (cop) {
		case MENSAJE:
			recibir_mensaje(logger_memoria, cliente_socket);
			break;
		case PAQUETE:
			t_list *paquete_recibido = recibir_paquete(cliente_socket);
			log_info(logger_memoria, ANSI_COLOR_YELLOW "Recibí un paquete con los siguientes valores: ");
			//list_iterate(paquete_recibido, (void*) iterator);
			break;
		case INICIALIZAR_PROCESO:
			pcb* proceso = recv_pcb(cliente_socket); //void* memoriaFisica = malloc();
            log_info(logger_memoria, "Creación de Proceso PID: %d, path: %s", proceso->pid, proceso->path);
            enviar_mensaje("OK inicio proceso", cliente_socket);
            int cant_paginas_necesarias = paginas_necesarias(proceso);
            printf("cant paginas necesarias: %d\n", cant_paginas_necesarias);
            //mandar aca a fs para recibir pos en swap 
            send_reserva_swap(conexion_memoria_filesystem, cant_paginas_necesarias);
            //bloquear con recv hasta recibir la lista de swap 
            printf("bloqueado esperando a swap\n");
            op_code cop = recibir_operacion(conexion_memoria_filesystem);
            t_list* bloques_reservados  = recv_reserva_swap(conexion_memoria_filesystem);
            t_list* tabla_paginas = inicializar_proceso(proceso);
            list_add(lista_tablas_de_procesos, tabla_paginas);
            //SEMAFORO PARA MANDAR INTRUCCIONES A CPU
            sem_post(&swap_asignado);
            pcb_destroyer(proceso);
            break;
		case FINALIZAR_PROCESO:
			int pid_fin = recv_terminar_proceso(cliente_socket);
            enviar_mensaje("OK fin proceso", cliente_socket);
			log_info(logger_memoria, "Eliminación de Proceso PID: %d", pid_fin);
			//terminar_proceso(pid_fin);
			break;
        case PEDIDO_MARCO:
            t_list* paquete=recibir_paquete(cliente_socket);
            int direccionFisica=list_get(paquete,0);
            list_destroy(paquete);
            uint32_t numeroMarco;
            //Buscar el numero de marco;
            if(numeroMarco>0){
                t_paquete* paqueteMarco=crear_paquete(ENVIO_MARCO);
                agregar_a_paquete(paqueteMarco,numeroMarco,sizeof(uint32_t));
                enviar_paquete(paqueteMarco,cliente_socket);
                eliminar_paquete(paqueteMarco);
            }else{
                t_paquete* paquetePageFault=crear_paquete(PCB_PAGEFAULT);
                enviar_paquete(paquetePageFault,cliente_socket);
            }
            break;
        case PEDIDO_LECTURA_FS:
         /*
            valor_fs = malloc(*tamano_fs);

            // Simula un retardo en el acceso a memoria según la configuración.
            usleep(RETARDO_RESPUESTA * 1000);

            // Copia el valor desde el espacio de usuario a la variable valor_fs.
            memcpy(valor_fs, espacio_usuario + *posicion_fs, *tamano_fs);

            // Registra información sobre la lectura en el logger.
            log_info(logger_obligatorio, "PID: %d - Acción: LEER - Dirección física: %d - Tamaño: %d - Origen: FS", *pid_fs, *posicion_fs, *tamano_fs);

            // Registra el valor leído en el logger y lo envía de vuelta al módulo cliente (FS).
            log_valor_espacio_usuario_y_enviar(valor_fs, *tamano_fs, cliente_socket);
            break;
        */
        case PEDIDO_ESCRITURA_FS:
            // Recibe los parámetros de escritura del espacio de usuario desde el módulo cliente (FS).
            /*parametros_escritura_fs = recibir_paquete(cliente_socket);
            
            char* valor_a_escribir_fs = list_get(parametros_escritura_fs, 0);
            int* posicion_escritura_fs = list_get(parametros_escritura_fs, 1);
            tam_esc_fs = list_get(parametros_escritura_fs, 2);
            pid_escritura_fs = list_get(parametros_escritura_fs, 3);

            // Registra información sobre el tamaño del valor a escribir en el logger.
            log_info(logger_memoria, "el tamaño del valor a escribir es: %d", *tam_esc_fs);

            // Simula un retardo en el acceso a memoria según la configuración.
            usleep(RETARDO_REPUESTA);

            // Escribe el valor en el espacio de usuario en la posición especificada.
            memcpy(espacio_usuario + *posicion_escritura_fs, valor_a_escribir_fs, *tam_esc_fs);

            // Registra información sobre la escritura en el logger y envía un mensaje indicando el fin de la operación de escritura al módulo cliente (FS).
            log_info_y_enviar_fin_escritura(logger_obligatorio, *pid_escritura_fs, *posicion_escritura_fs, *tam_esc_fs, valor_a_escribir_fs, cliente_socket);*/

        break;
        case ENVIO_INSTRUCCION:
            char* path;
            int* pc;
            sleep(RETARDO_REPUESTA/1000);
            recv_fetch_instruccion(cliente_socket, &path,&pc);
            sem_wait(&swap_asignado); 
            leer_instruccion_por_pc_y_enviar(path,*pc, cliente_socket);
            sem_post(&swap_asignado);
            free(path); // Liberar memoria de la variable path
            free(pc); // Liberar memoria de la variable pc
            break;
        default:
            printf("Error al recibir mensaje con OPCODE %d \n", cop);
            break;

	return;
        }
    }
}

int server_escuchar(int fd_memoria) {
	server_name = "Memoria";
	int cliente_socket = esperar_cliente(logger_memoria, server_name, fd_memoria);

	if (cliente_socket != -1) {
		pthread_t hilo;
		pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) &cliente_socket);
		pthread_detach(hilo);
		return 1;
	}

	return 0;
}


void liberar_marco(t_marco* marco){
    
    log_info(logger_memoria,"se libera el marco:%d, con el proceso:%d",marco->num_marco,marco->pid);
    marco->ocupado = false;
    marco->pid = -1;
}

int calcularMarco(int pid, t_marco* marcos, int num_marcos) {
    for (int i = 0; i < num_marcos; i++) {
        if (marcos[i].ocupado && marcos[i].pid == pid) {
            return marcos[i].num_marco;
        }
    }
    
    return -1;
}
// Devuelve el contenido de un marco que está en memoria.
void* obtener_marco(uint32_t nro_marco) {
  // Reserva memoria para un marco
  void* marco = malloc(tam_pagina);
  // Bloquea el acceso al array de memoria
  pthread_mutex_lock(&mx_memoria);

  memcpy(marco, memoria + nro_marco * tam_pagina, tam_pagina);

  // Desbloquea el acceso al array de memoria
  pthread_mutex_unlock(&mx_memoria);

  
  return marco;
}
void escribir_marco_en_memoria(uint32_t nro_marco, void* marco){
	char* tam_pagina_str = config_get_string_value(config, "TAM_PAGINA");
    uint32_t tam_pagina_int = atoi(tam_pagina_str);
    uint32_t marco_en_memoria = nro_marco * tam_pagina_int;
	pthread_mutex_lock(&mx_memoria);
	memcpy(memoria + marco_en_memoria, marco, tam_pagina);
	pthread_mutex_unlock(&mx_memoria);
}
int buscar_marco_libre(){
	pthread_mutex_lock(&mx_bitarray_marcos_ocupados);
    
	for (int i = 0; i < get_memory_and_page_size(); i++){
		if (bitarray_marcos_ocupados[i] == 0){
			pthread_mutex_unlock(&mx_bitarray_marcos_ocupados);
			return i;
		}
	}
	pthread_mutex_unlock(&mx_bitarray_marcos_ocupados);
	return -1;
}
// Devuelve la cantidad de marcos que requiere un proceso del tamanio especificado
uint32_t calcular_cant_marcos(uint16_t tamanio){
	uint32_t  cant_marcos = get_memory_and_page_size()-1;
    char* tam_pagina_str = config_get_string_value(config, "TAM_PAGINA");
    uint32_t tam_pagina_int = atoi(tam_pagina_str);
	if (tamanio % tam_pagina_int != 0)
		cant_marcos++;
	return cant_marcos;
}
t_list* obtenerMarcosAsignados(int pid){

}

void eliminar_proceso_memoria(int pid) {
    
    t_list* marcos_asignados = obtenerMarcosAsignados(pid);
    
    for (int i = 0; i < list_size(marcos_asignados); i++) {
        t_marco* marco = list_get(marcos_asignados, i);
        liberar_marco(marco);
    }
    list_destroy(marcos_asignados);
}


t_list* inicializar_proceso(pcb* proceso) {
    int entradas_por_tabla = paginas_necesarias(proceso);
    t_list* tabla_de_paginas = list_create();
	for (int i = 0; i < entradas_por_tabla ; i++){
		entrada_pagina* pagina = malloc(entradas_por_tabla * sizeof(pagina));
		pagina->num_marco=-1;
		pagina->modificado=0;
		pagina->en_memoria=0;
        pagina->pid= proceso->pid;
		list_add(tabla_de_paginas, pagina);
	}
    
	return tabla_de_paginas;
}

void terminar_proceso(int pid){
    //borrar tabla paginas, mandar terminar a fs
    eliminar_tabla_paginas(pid);
    //liberar_recursos(proceso); Consultar con ayudante
    send_liberacion_swap(conexion_memoria_filesystem,pid);
}

int paginas_necesarias(pcb *proceso){
    return proceso->size / tam_pagina;
}

void eliminar_tabla_paginas(int pid){
for(int i = 0; i < list_size(lista_tablas_de_procesos); i++){
		t_list * tabla_pagina = list_get(lista_tablas_de_procesos, i);
        entrada_pagina * entrada = list_get(tabla_pagina, 0);//Siempre va a ser el mismo pid
		if(entrada->pid==pid){
            for(int j = 0; j < list_size(tabla_pagina); j++){
                list_remove(tabla_pagina, j);
                //ver si hace falta free
            }
            list_remove_element(lista_tablas_de_procesos, tabla_pagina);
		}
	}
	return;
}
void liberar_recursos(pcb* proceso) {

    if (proceso->registros) {
        free(proceso->registros);
    }
    for (int i = 0; i < list_size(proceso->archivos); i++) {
        t_archivo* archivo = list_get(proceso->archivos, i);
        fclose((FILE*) archivo->nombre_archivo);
        free(archivo);
    }
    list_destroy(proceso->archivos);
}

int obtenerCantidadPaginasAsignadas(int pid) {
    t_list* marcos_asignados = obtenerMarcosAsignados(pid);
    int cantidadDePaginasAsignadas = list_size(marcos_asignados);
    list_destroy(marcos_asignados);
    return cantidadDePaginasAsignadas;
}
uint32_t get_memory_and_page_size() {
  char* tam_memoria_str = config_get_string_value(config, "TAM_MEMORIA");
  char* tam_pagina_str = config_get_string_value(config, "TAM_PAGINA");

  uint32_t tam_memoria_int = atoi(tam_memoria_str);
  uint32_t tam_pagina_int = atoi(tam_pagina_str);

  return (tam_memoria_int / tam_pagina_int) + 1;
}
t_list* crear_tabla(int pid){
    t_list* tabla_de_pagina = list_create();
    uint32_t entrada_de_paginas = get_memory_and_page_size();
    for(int i=0; i<(entrada_de_paginas) + 1; i++){
        entrada_pagina* pagina = malloc(entrada_de_paginas* sizeof(entrada_pagina));
        pagina -> num_marco = -1;
        pagina -> modificado = 0;
        pagina -> en_memoria = 0;
        list_add(tabla_de_pagina,pagina);
    }
    return tabla_de_pagina;
}

/* ----------------------PAGE FAULT----------------------------
// Función para tratar un fallo de página
uint32_t tratar_page_fault(uint32_t num_segmento, uint32_t num_pagina, uint16_t pid_actual) {
    // Crear y obtener listas para el proceso y la tabla de marcos
    t_list* tabla_de_proceso = list_create();
    tabla_de_proceso = list_get(lista_tablas_de_procesos, pid_actual);
    t_list* tabla_de_marcos = list_create();
    tabla_de_marcos = list_get(tabla_de_proceso, num_segmento);
    
    // Obtener información de la página que causó el fallo
    entrada_pagina* pagina = list_get(tabla_de_marcos, num_pagina);

    // Log: Page fault detectado
    log_info(logger, "[CPU][ACCESO A DISCO] PAGE FAULT!!!");

    // Obtener el número de marco en el swap
    //Lo obtenemos desde filesystem

    // Leer el contenido de la página desde el swap
    void* marco = leer_marco_en_swap(fd, nro_marco_en_swap, tam_pagina);

    int32_t nro_marco;

    // Si el proceso ya tiene todos sus marcos en memoria
    if (marcos_en_memoria(pid_actual) == marcos_por_proceso) {
        // Utilizar LRU O FIFO  para seleccionar un marco a reemplazar
        nro_marco = usar_algoritmo(pid_actual);

        // Log: Información sobre el reemplazo
        log_info(logger, "REEMPLAZO - PID: <%d> - Marco: <%d> - Page In: <SEGMENTO %d>|<PAGINA %d>",
                 pid_actual, nro_marco, num_segmento, num_pagina);
    } else {
        // Si el proceso aún tiene marcos disponibles, buscar un marco libre en memoria
        nro_marco = buscar_marco_libre();

        // Log: El proceso tiene marcos disponibles
        log_info(logger, "[CPU] El proceso tiene marcos disponibles :)");

        // Si no hay marcos libres, logear un error y terminar el programa
        if (nro_marco == -1) {
            log_error(logger, "ERROR!!!!! NO HAY MARCOS LIBRES EN MEMORIA!!!");
            exit(EXIT_FAILURE);
        }
    }

    // Marcar el nuevo marco como ocupado
    

    // Actualizar la información de la página
    pagina->nro_marco = nro_marco;
    pagina->en_memoria = 1;


    // Escribir el contenido del marco en memoria
    escribir_marco_en_memoria(pagina->nro_marco, marco);

    // Liberar la memoria utilizada para almacenar el contenido leído desde el swap
    free(marco);

    // Log: SWAP IN
    log_info(logger, "[CPU] LECTURA EN SWAP: SWAP IN - PID: <%d> - Marco: <%d> - Page In: <SEGMENTO %d>|<PÁGINA %d>",
             pid_actual, nro_marco, num_segmento, num_pagina);

    // Devolver el número de marco utilizado
    return nro_marco;
}
*/
// ----------------------MEMORIA DE INSTRUCCIONES----------------------------

char *armar_path_instruccion(char *path_consola) {
    char *path_completo = string_new();
    string_append(&path_completo, path_instrucciones);
    string_append(&path_completo, "/");
    string_append(&path_completo, path_consola);
    return path_completo;
}

void leer_instruccion_por_pc_y_enviar(char *path_consola, int pc, int fd) {
    char *path_completa_instruccion = armar_path_instruccion(path_consola);

    FILE *archivo = fopen(path_completa_instruccion, "r");
    if (archivo == NULL) {
        perror("No se pudo abrir el archivo de instrucciones");
        free(path_completa_instruccion);
        return;
    }

    char instruccion_leida[256];
    int current_pc = 0;

    while (fgets(instruccion_leida, sizeof(instruccion_leida), archivo) != NULL) {
        if (current_pc == pc) {
            printf("Instrucción %d: %s", pc, instruccion_leida);
            Instruccion instruccion = armar_estructura_instruccion(instruccion_leida);

            send_instruccion(fd, instruccion);

            // Liberar memoria asignada para la estructura Instruccion

            free(instruccion.opcode);
            free(instruccion.operando1);
            free(instruccion.operando2);
            
            break;
        }
        current_pc++;
    }
    fclose(archivo);
    free(path_completa_instruccion);
}

Instruccion armar_estructura_instruccion(char* instruccion_leida){
    char **palabras = string_split(instruccion_leida, " ");
    
    Instruccion instruccion;

    if (palabras[0] != NULL) {
        instruccion.opcode = malloc(sizeof(char) * (strlen(palabras[0]) + 1));
        strcpy(instruccion.opcode, palabras[0]);

        if (palabras[1] != NULL) {
            instruccion.operando1 = malloc(sizeof(char) * (strlen(palabras[1]) + 1));
            strcpy(instruccion.operando1, palabras[1]);

            if (instruccion.operando1[strlen(instruccion.operando1) - 1] == '\n') {
                instruccion.operando1[strlen(instruccion.operando1) - 1] = '\0';
            }

            if (palabras[2] != NULL) {
                instruccion.operando2 = malloc(sizeof(char) * (strlen(palabras[2]) + 1));
                strcpy(instruccion.operando2, palabras[2]);

                if (instruccion.operando2[strlen(instruccion.operando2) - 1] == '\n') {
                    instruccion.operando2[strlen(instruccion.operando2) - 1] = '\0';
                }
            } else {
                instruccion.operando2 = malloc(sizeof(char));
                instruccion.operando2[0] = '\0'; // Vaciar el operando2 si no hay tercer palabra
            }
        } else {
            instruccion.operando1 = malloc(sizeof(char));
            instruccion.operando2 = malloc(sizeof(char));
            instruccion.operando1[0] = '\0'; // Vaciar el operando1 si no hay segunda palabra
            instruccion.operando2[0] = '\0'; // Vaciar el operando2
        }
    } else {
        perror("Error al cargar la instrucción");

        // Liberar memoria en caso de error
        free(instruccion.opcode);
        free(instruccion.operando1);
        free(instruccion.operando2);
        instruccion.opcode = NULL;
        instruccion.operando1 = NULL;
        instruccion.operando2 = NULL;
    }

    // Liberar memoria asignada a palabras
    int i = 0;
    while (palabras[i] != NULL) {
        free(palabras[i]);
        i++;
    }
    free(palabras);

    return instruccion;
}

// Función para loguear el valor leído y enviarlo de vuelta al módulo cliente (FS)
void log_valor_espacio_usuario_y_enviar(char* valor, int tamanio, int cliente_socket) {
    log_valor_espacio_usuario(valor, tamanio);
    send_valor_leido_fs(valor, tamanio, cliente_socket);
}

// Función para loguear información y enviar el mensaje de fin de escritura al módulo cliente (FS)
void log_info_y_enviar_fin_escritura(t_log* logger, int pid, int posicion, int tam, char* valor, int cliente_socket) {
    log_info(logger_memoria, "PID: %d - Acción: ESCRIBIR - Dirección física: %d - Tamaño: %d - Origen: FS", pid, posicion, tam);
    log_valor_espacio_usuario(valor, tam);
    send_fin_escritura(cliente_socket);
}
void log_valor_espacio_usuario(char* valor, int tamanio){
	char* valor_log = malloc(tamanio);
	memcpy(valor_log, valor, tamanio);
	memcpy(valor_log + tamanio, "\0", 1);
	int tamanio_valor = strlen(valor_log);
	log_info(logger_memoria, "se leyo/escribio %s de tamaño %d en el espacio de usuario", valor_log, tamanio_valor);
}

//LRU
// Función para reemplazar una página utilizando el algoritmo LRU
void algoritmo_lru(t_list* tabla_De_Paginas) {
    if (list_is_empty(tabla_De_Paginas)) {
        log_info(logger_memoria, "No hay páginas para reemplazar.");
        return;
    }

    // Filtrar y ordenar de más vieja a más nueva
    list_sort(tabla_De_Paginas, (void*)masVieja);
    
    // Obtener la página más antigua
    entrada_pagina* paginaReemplazo = list_get(tabla_De_Paginas, 0);
    log_info(logger_memoria, "Voy a reemplazar la página %d que estaba en el frame %d", paginaReemplazo->pid, paginaReemplazo->num_marco);

    // Guardar en memoria virtual si está modificada
    if (paginaReemplazo->modificado == 1) {
        //guardarMemoriaVirtual(paginaReemplazo);
    }

    // Desocupar el frame en el bitmap
    //desocuparFrameEnBitmap(paginaReemplazo->num_marco);

    // Actualizar el bit de presencia
    paginaReemplazo->en_memoria = 0;

    // Liberar memoria de la página reemplazada
    free(paginaReemplazo);

    // Actualizar el tiempo de uso de las demás páginas
    actualizarTiempoDeUso(tabla_De_Paginas);
}
bool masVieja(void *unaPag, void *otraPag) { 
    entrada_pagina* pa = unaPag;
    entrada_pagina* pb = otraPag;
    int intA = pa->tiempo_uso; int intB = pb->tiempo_uso; 
    return intA < intB; 
}
// Función para actualizar el tiempo de uso de todas las páginas
void actualizarTiempoDeUso(t_list* tabla_De_Paginas) {
    for (int i = 0; i < list_size(tabla_De_Paginas); ++i) {
        entrada_pagina* pagina = list_get(tabla_De_Paginas, i);
        pagina->tiempo_uso++;
    }
}