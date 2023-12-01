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
    retardo_respuesta = config_get_int_value(config, "RETARDO_RESPUESTA");
    algoritmo_reemplazo = config_get_string_value(config, "ALGORITMO_REEMPLAZO");
}
bool leerYEnviarInstruccion(FILE *archivo, int conexion_memoria) {
    char linea[256];
    Instruccion instruccion = {0};
    if (fgets(linea, sizeof(linea), archivo) != NULL) {
        char **palabras = string_split(linea, " ");

        if (palabras[0] != NULL) {
            strcpy(instruccion.opcode, palabras[0]);

            if (palabras[1] != NULL) {
                strcpy(instruccion.operando1, palabras[1]);

                if (palabras[2] != NULL) {
                    strcpy(instruccion.operando2, palabras[2]);
                } else {
                    instruccion.operando2[0] = '\0'; // Vaciar el operando2 si no hay tercer palabra
                }
            } else {
                instruccion.operando1[0] = '\0'; // Vaciar el operando1 si no hay segunda palabra
                instruccion.operando2[0] = '\0'; // Vaciar el operando2
            }
        } else {
            log_error(logger_memoria, "Error al cargar la instrucción");
        }

        /*if (!send_instruccion(conexion_memoria, instruccion)) {
            log_error(logger_memoria, "Error al enviar la instrucción");
        }*/
    }
}
int main(int argc, char *argv[])
{

    // CONFIG y logger
    levantar_config("memoria.config");

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
            //mandar aca a fs para recibir por en swap 
            //bloquear con recv hasta recibir la lista de swap 
            //t_list* tabla_paginas = inicializar_proceso(proceso);
            //list_add(lista_tablas_de_procesos, tabla_paginas);
            break;
		case FINALIZAR_PROCESO:
			int pid_fin = recv_terminar_proceso(cliente_socket);
            enviar_mensaje("OK fin proceso", cliente_socket);
			log_info(logger_memoria, "Eliminación de Proceso PID: %d", pid_fin);
			terminar_proceso(pid_fin);
			break;
        case ENVIO_INSTRUCCION:
            printf("Recibí pedido de instrucción\n");
            char* path;
            int pc;
            recv_fetch_instruccion(cliente_socket,path,pc);
            leer_instruccion_por_pc(path,pc);
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
		int *args = malloc(sizeof(int));
		args = &cliente_socket;
		pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) args);
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
    int entradas_por_tabla = proceso->size/tam_pagina;
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
        t_archivos* archivo = list_get(proceso->archivos, i);
        fclose((FILE*) archivo->path);
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


// ----------------------MEMORIA DE INSTRUCCIONES----------------------------

void leer_instruccion_por_pc(char *path_instrucciones,int pc) {
    FILE *archivo = fopen(path_instrucciones, "r");
    if (archivo == NULL) {
        perror("No se pudo abrir el archivo de instrucciones");
        return NULL;
    }

    char instruccion[256];
    int current_pc = 0;

    while (fgets(instruccion, sizeof(instruccion), archivo) != NULL) {
        if (current_pc == pc) {
            printf("Instrucción %d: %s", pc, instruccion);
            break;
        }
        current_pc++;
    }

    fclose(archivo);
}