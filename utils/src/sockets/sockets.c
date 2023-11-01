#include "./sockets.h"

int iniciar_servidor(t_log* logger, const char* name, char* ip, char* puerto)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, puerto, &hints, &servinfo);
    bool conecto = false;

    for (struct addrinfo *p = servinfo; p != NULL; p = p->ai_next) {
        socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socket_servidor == -1) // fallo de crear socket
            continue;

        if (setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
            herror("setsockopt(SO_REUSEADDR) failed");

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            // Si entra aca fallo el bind
            close(socket_servidor);
            continue;
        }
        // Ni bien conecta uno nos vamos del for
        conecto = true;
        break;
    }

    if(!conecto) {
        free(servinfo);
        return 0;
    }

    listen(socket_servidor, SOMAXCONN); // Escuchando (hasta SOMAXCONN conexiones simultaneas)

    // Aviso al logger
    log_info(logger, "Escuchando en :%s (%s)\n", puerto, name);

    freeaddrinfo(servinfo);

    return socket_servidor;
}

int esperar_cliente(t_log* logger, const char* name, int socket_servidor)
{
    struct sockaddr_in dir_cliente;
    socklen_t tam_direccion = sizeof(struct sockaddr_in);

    int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

    log_info(logger, "Cliente se conecto a (%s)\n", name);
    printf("Llego el cliente");
    return socket_cliente;

}

int crear_conexion(t_log* logger,const char* server_name, char* ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family,
            server_info->ai_socktype,
            server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
    // Fallo en crear el socket
    if(socket_cliente == -1) {
        log_error(logger, "Error creando el socket para %s:%s", ip, puerto);
        return 0;
    }

    // Error conectando
    if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1) {
        log_error(logger, "Error al conectarme (a %s)\n", server_name);
        freeaddrinfo(server_info);
        return 0;
    } else
        log_info(logger, "Me conecte en %s:%s (a %s)\n", ip, puerto, server_name);

    freeaddrinfo(server_info);

    return socket_cliente;
}
//NO SE SI ES CON PUNTERO O NO
void liberar_conexion(int socket_cliente) {
    close(socket_cliente);
	socket_cliente = -1;
}