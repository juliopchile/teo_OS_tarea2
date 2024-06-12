/**
 * @file funciones.h
 * @brief Declaraciones para funciones útiles.
 *
 * @details
 * Este archivo contiene las declaraciones de las funciones utilizadas en las 
 * operaciones sobre el buffer circular y otras utilidades requeridas por el programa.
 *
 * @date 4 de junio de 2023 (creación)
 * @version 1.1
 * @date 11 de junio de 2024 (última actualización)
 * @authors
 * Julio López
 *
 * @history
 * Versión 1.0 - 4 de junio de 2023 - Creación del archivo.
 * Versión 1.1 - 11 de junio de 2024 - Actualización y modificación de funciones para implementar ncurses.
 */

#ifndef FUNCIONES_H
#define FUNCIONES_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define OCCUPIED_CHAR       '🚗'
#define EMPTY_CHAR          '_'

/* Esto se hace porque Aragorn no incluye <stdatomic.h>*/
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
    #include <stdatomic.h>
    #define ATOMIC_INT atomic_int
    #define ATOMIC_LOAD(ptr) atomic_load(ptr)
    #define ATOMIC_STORE(ptr, val) atomic_store(ptr, val)
    #define ATOMIC_ADD(ptr, val) atomic_fetch_add(ptr, val)
#else
    #define ATOMIC_INT volatile int
    #define ATOMIC_LOAD(ptr) __sync_fetch_and_add(ptr, 0)
    #define ATOMIC_STORE(ptr, val) __sync_bool_compare_and_swap(ptr, *ptr, val)
    #define ATOMIC_ADD(ptr, val) __sync_fetch_and_add(ptr, val)
#endif


/**
 * @brief Estructura para un buffer circular con un tamaño configurable.
 *
 * @param buffer Puntero a un arreglo de enteros donde almacenar los datos.
 * @param head Indice a la cabeza del buffer (donde se añaden nuevos datos).
 * @param tail Indice a la cola del buffer (donde se retiran los datos).
 * @param size Tamaño del buffer.
 */
typedef struct {
    int* buffer;  /**< Puntero a un arreglo de enteros donde almacenar los datos */
    int head;     /**< Indice a la cabeza del buffer (donde se añaden nuevos datos) */
    int tail;     /**< Indice a la cola del buffer (donde se retiran los datos) */
    int size;     /**< Tamaño del buffer */
} CircularBuffer;


/**
 * @brief Inicializa un nuevo Buffer Circular
 *
 * @param size Largo del buffer
 * @return CircularBuffer* Puntero al buffer circular.
 */
CircularBuffer* createBuffer(int size);


/**
 * @brief Si se encuentra un elemento distinto de 0, el buffer no está vacío (retorna 0).
 * Caso contrario está vacío (retorna 1).
 *
 * @param cb Buffer a revisar
 * @return El estado del buffer
 */
int isBufferEmpty(CircularBuffer *cb);


/**
 * @brief Cuenta cuantos espacios están ocupados en el buffer.
 *
 * @param cb
 * @return int
 */
int countBuffer(CircularBuffer *cb);


/**
 * @brief Borra un buffer al liberar su memoria.
 *
 * @param cb Buffer a liberar
 */
void destroyBuffer(CircularBuffer *cb);


/**
 * @brief Añade un valor al buffer circular.
 *
 * @param cb Un puntero al buffer circular
 * @param value El valor a añadir
 */
void addToBuffer(CircularBuffer *cb, int value);


/**
 * @brief Retira un valor del buffer circular.
 *
 * @param cb Un puntero al buffer circular
 * @return El valor retirado del buffer
 */
int removeFromBuffer(CircularBuffer *cb);


/**
 * @brief Imprime el contenido del buffer circular.
 *
 * @param cb Un puntero al buffer circular
 */
void printBuffer(CircularBuffer *cb);


/**
 * @brief Imprime el contenido del buffer circular en sentido opuesto.
 *
 * @param cb Un puntero al buffer circular
 */
void printBuffer2(CircularBuffer *cb);


/**
 * @brief Vacía la cola correspondiente.
 *
 * Esta función vacía la cola al permitirle a los transeúntes transitar por el puente.
 *
 */
void* recorrerEstacionamiento(void*);


/**
 * @brief Representa a la llegada de gente.
 *
 * Tras un periodo de tiempo se añade un nuevo transeúnte a la cola designada por el argumento de entrada "arg".
 *
 * @param arg Id del buffer, indica si es izquierda o derecha.
 * @return void*
 */
void* newVehiculo(void* arg);


/**
 * @brief Muestra en la consola el estado actual del puente y las colas
 */
void printState(char variable, int value);


/**
 * @brief Función auxiliar para mostrar en la consola
 */
void printBuffersAndDirection();


/**
 * @brief Crea una espera en micro segundos.
 *
 * @param microseconds
 */
void my_sleep(int microseconds);


/**
 * @brief Obtiene el tiempo actual.
 *
 * @return double
 */
double get_time();


/**
 * @brief Actualiza el tamaño de la ventana de acuerdo a los tamaños de los buffers.
 *
 * @param leftBuf El buffer izquierdo.
 * @param rightBuf El buffer derecho.
 * @param dir Dirección actual.
 */
void updateWindowSize(CircularBuffer *leftBuf, CircularBuffer *rightBuf);


#endif