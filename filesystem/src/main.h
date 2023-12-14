#ifndef MAIN_H_
#define MAIN_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include "../../utils/src/utils/utils.h"
#include "../../utils/src/sockets/sockets.h"
#include "comunicacion.h"

typedef struct {
    char* nombre_archivo;
    int tamanio_archivo;
    int bloque_inicial;
} FCB;
typedef struct {
    uint32_t entrada_FAT;
} FAT;
typedef struct {
    char* info;
} BLOQUE;

bool liberar_bloquesSWAP(int bloques[],int cantidad, bool* bitmap);
bool reservar_bloquesSWAP(int cant_bloques, int bloques_reservados[],bool* bitmap);
int abrir_archivo(char* ruta);
bool crear_archivo(char* nombre);
int asignarBloque(char* nombre, bool* bitmap);
bool truncarArchivo(char* nombre, int tamano, bool* bitmap);
void achicarArchivo(int bloqueInicio, int tamanoActual, int reduccion, bool* bitmap);
void liberarBloque(FILE* f, uint32_t bloqueLib, bool* bitmap);
void ampliarArchivo(int bloqueInicio,int tamanoActual, int ampliacion,bool* bitmap);
void actualizarFAT(FILE* f, uint32_t ultimoBloque, uint32_t nuevoBloque);
void reservarBloque(FILE* f, uint32_t bloque, bool* bitmap);
char* escribir_bloque(int num_bloque, char* info);
void escribir_bloque_void(int num_bloque, void* info);
int obtener_bloqueInicial(char* nombre);
int obtener_bloque(int bloqueInicial,int nroBloque);
uint32_t buscarBloqueLibre(bool* bitmap);
uint32_t buscarUltimoBloque(FILE* fat, int inicio);
uint32_t buscarUltimoBloqueAchicar(FILE* fat, int inicio, uint32_t* anteultimo, uint32_t* antepenultimo);
void actualizarFcb(char* nombre, int tamano, int bloque);
char* leer_bloque(int num_bloque);


#endif