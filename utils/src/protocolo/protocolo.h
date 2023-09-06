#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include <utils/utils.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>
#include<pthread.h>

typedef enum {
    APROBAR_OPERATIVOS,
    MIRAR_NETFLIX,
    PRUEBA = 69,
} op_code;

///

bool send_aprobar_operativos(int fd, uint8_t  nota1, uint8_t  nota2);
bool recv_aprobar_operativos(int fd, uint8_t* nota1, uint8_t* nota2);

bool send_mirar_netflix(int fd, char*  peli, uint8_t  cant_pochoclos);
bool recv_mirar_netflix(int fd, char** peli, uint8_t* cant_pochoclos);

bool send_debug(int fd);


#endif 