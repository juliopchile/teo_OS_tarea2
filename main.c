/**
 * @file tarea2.c
 * @brief Sincronización entre Procesos - Teoria de Sistemas Operativos - ELO321
 *
 * @details
 * El objetivo de este código es proporcionar una solución a la problemática de sincronización de procesos.
 * La situación modelada consiste en un puente de dos sentidos, que actúa como un recurso compartido, y dos
 * colas en cada extremo del puente, llenas de personas que quieren atravesarlo.
 *
 * Los individuos se trasladan en grupos de tamaño variable, adaptándose al tamaño de las colas en cada lado
 * del puente. La tasa de llegada de nuevos transeúntes es aleatoria y tiene un promedio diferente para cada
 * lado, lo que hace necesaria la capacidad de ajustar el tamaño de los grupos (ventana).
 *
 * En este código, se hace uso de enteros atómicos para ciertos contadores. Dado que el compilador GCC 4.8.5
 * en Aragorn no contiene la librería <stdatomic.h>, se utilizaron macros para implementarlos.
 *
 * Las funciones principales del código se encuentran en los hilos recorrerEstacionamiento y newVehiculo, mientras
 * que las funciones auxiliares están descritas en "funciones.h". Las funciones de los hilos son de diseño propio.
 * Para algunas funciones auxiliares, como las de impresión o el manejo de tiempos de nanosegundos, se recurrió a
 * información y código encontrado en Internet y a la asistencia de ChatGPT.
 *
 * @date 11 de junio de 2024
 * @author Julio López
 */

#include <time.h>
#include "funciones.h"

//Constantes
#define BUFFER_SIZE         20
#define PARKING_SIZE        10
#define WINDOW_SIZE         3
#define PARKING_SPEED       50000

// Esta variable global almacenará el tiempo de inicio
struct timespec start_time;

/* Colas de espera */
CircularBuffer *leftBuffer, *rightBuffer, *parkingBuffer;

/* Semáforos */
sem_t leftSemaphore, rightSemaphore, parkingSemaphore, windowSem;

/* Mutex para printear */
pthread_mutex_t printMutex;

/* Contadores*/
ATOMIC_INT contador_in = 0;
ATOMIC_INT contador_out = 0;                                                                                                                                                       ATOMIC_INT window = WINDOW_SIZE;
int dir;

void* recorrerEstacionamiento(void* arg) {
    int value;
    int half_time = (int)(PARKING_SPEED / 2);
    my_sleep(3*PARKING_SPEED);
    while(ATOMIC_LOAD(&contador_out) <= 150) {
        sem_wait(&parkingSemaphore);   // ! Esperar a que el estacionamiento esté disponible.

        //? ENTRADA AL ESTACIONAMIENTO *//
        for (int i = 0; i < ATOMIC_LOAD(&window); i++) {
            //* LADO IZQUIERDO *//
            if((dir == 1)) {
                sem_wait(&leftSemaphore);               // ? Se pausa leftSemaphore
                my_sleep(half_time);                    // Saliendo de la cola
                value = removeFromBuffer(leftBuffer);   // Sale de la cola.
                printState('a', value);                 // TODO reemplazar con Ncurses
                sem_post(&leftSemaphore);               // ? Se libera leftSemaphore
                my_sleep(half_time);                    // Entrando al estacionamiento
                addToBuffer(parkingBuffer, value);      // Entra al estacionamiento.
                parkingBuffer->tail = (parkingBuffer->tail + 1) % (parkingBuffer->size);
                printState('A', value);                 // TODO reemplazar con Ncurses
            }

            //* LADO DERECHO *//
            else if((dir == 2)) {
                sem_wait(&rightSemaphore);              // ? Se pausa rightSemaphore
                my_sleep(half_time);                    // Saliendo de la cola
                value = removeFromBuffer(rightBuffer);  // Sale alguien de la cola.
                printState('b', value);                 // TODO reemplazar con Ncurses
                sem_post(&rightSemaphore);              // ? Se libera rightSemaphore
                my_sleep(half_time);
                addToBuffer(parkingBuffer, value);      // Entra al estacionamiento.
                parkingBuffer->tail = (parkingBuffer->tail + 1) % (parkingBuffer->size);
                printState('B', value);                 // TODO reemplazar con Ncurses
            }
        }

        //? AVANZAR EN EL ESTACIONAMIENTO *//
        // Vaciar el buffer del estacionamiento.
        while(!isBufferEmpty(parkingBuffer)) {
            my_sleep(PARKING_SPEED);                    // Vehiculo moviéndose/saliendo del estacionamiento.
            value = removeFromBuffer(parkingBuffer);
            printState('O', value);                     // TODO reemplazar con Ncurses
        }

        // Restablece las posiciones del estacionamiento para mostrarlo bien.                                                                                                                      parkingBuffer->head = 0;
        parkingBuffer->tail = 0;

        //? CAMBIAR SENTIDO DEL TRAFICO *//
        dir = (dir == 1) ? 2 : 1;       // 1 es izquierda, 2 es derecha
        updateWindowSize(leftBuffer, rightBuffer);

        sem_post(&parkingSemaphore);    // ! Liberar el estacionamiento.
    }
    ATOMIC_STORE(&contador_in, 1);
    return NULL;
}

void* newVehiculo(void* arg) {
    int bufferId = *((int*)arg);
    int waitingTime;
    while(1) {
        if(ATOMIC_LOAD(&contador_in) == 1) {break;}
        //* LADO IZQUIERDO *//
        if(bufferId == 1) {
            waitingTime = ((2*PARKING_SPEED)+ rand() % (7*PARKING_SPEED));
            my_sleep(waitingTime);
            sem_wait(&leftSemaphore);       // Se pausa el semáforo. (P)
            addToBuffer(leftBuffer, 1);     // Se añade vehiculo al buffer.      
            printState('l', -1);            // TODO reemplazar con Ncurses                                                                                    printState('l', -1);
            sem_post(&leftSemaphore);       // Se libera el semáforo. (V)
        }

        //* LADO DERECHO *//
        else if(bufferId == 2) {
            waitingTime = ((2*PARKING_SPEED) + rand() % (5*PARKING_SPEED));
            my_sleep(waitingTime);
            sem_wait(&rightSemaphore);      // Se pausa el semáforo. (P)
            addToBuffer(rightBuffer, 1);    // Se añade vehiculo al buffer.
            printState('r', -1);            // TODO reemplazar con Ncurses
            sem_post(&rightSemaphore);      // Se libera el semáforo. (V)
        }
    }
    return NULL;
}

int main() {
    // Inicializar variables globales
    dir = 0;
    pthread_t leftIn, rightIn, puente;
    int left_id  = 1;
    int right_id  = 2;

    // Inicializar buffers
    leftBuffer = createBuffer(BUFFER_SIZE);         // Cola de espera izquierda
    rightBuffer = createBuffer(BUFFER_SIZE);        // Cola de espera derecha
    parkingBuffer = createBuffer(PARKING_SIZE);     // Capacidad del estacionamiento

    // Llenado inicial de las colas
    int initial_left_amount = 4 + rand() % 3;
    for (int i = 0; i < initial_left_amount; i++)
        addToBuffer(leftBuffer, 1);
    int initial_right_amount = 4 + rand() % 3;
    for (int i = 0; i < initial_right_amount; i++)
        addToBuffer(rightBuffer, 1);
    int initial_parking_amount = 4 + rand() % 3;
    for (int i = 0; i < initial_parking_amount; i++)
        addToBuffer(parkingBuffer, 1);

    // Inicializar semáforos y mutex
    sem_init(&leftSemaphore, 0, 1);
    sem_init(&rightSemaphore, 0, 1);
    sem_init(&parkingSemaphore, 0, 1);
    pthread_mutex_init(&printMutex, NULL);

    // Obtener y guardar el tiempo al inicio del programa
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Crear hilos
    pthread_create(&puente, NULL, recorrerEstacionamiento, NULL);
    pthread_create(&leftIn, NULL, newVehiculo, &left_id );
    pthread_create(&rightIn, NULL, newVehiculo, &right_id );

    // Esperar a que terminen los hilos
    pthread_join(leftIn, NULL);
    pthread_join(rightIn, NULL);
    pthread_join(puente, NULL);

    // Destruir semáforos y mutex
    sem_destroy(&leftSemaphore);
    sem_destroy(&rightSemaphore);
    sem_destroy(&parkingSemaphore);
    pthread_mutex_destroy(&printMutex);

    return 0;
}

/**************************************
 *      *FUNCIONES PARA EL TIEMPO
 *************************************/

double get_time() {
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);                                                                                                                                     double elapsed_time = current_time.tv_sec - start_time.tv_sec;
    elapsed_time += (current_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;
    return elapsed_time;
}

void my_sleep(int microseconds) {
    struct timespec ts;
    ts.tv_sec = microseconds / 1000000;             // Convertir microsegundos a segundos
    ts.tv_nsec = (microseconds % 1000000) * 1000;   // Convertir el residuo a nanosegundos
    nanosleep(&ts, NULL);
}

/**************************************
 *      *OTRAS FUNCIONES
 *************************************/
// Estas funciones están aquí y no en funciones.c porque acceden a variables globales del main

void updateWindowSize(CircularBuffer *leftBuf, CircularBuffer *rightBuf) {
    int leftSize = countBuffer(leftBuf);
    int rightSize = countBuffer(rightBuf);
    int avgSize;
    int newWindowSize;

    // Calcula el tamaño promedio de los buffers
    if (dir == 1) {                                                                                                                                                                        // Si la dirección es 1 (izquierda), se da más relevancia al tamaño del buffer izquierdo.
        avgSize = (4 * leftSize + rightSize) / 5;
        if (avgSize > leftSize) {
            avgSize = leftSize;
        }
    } else {
        // Si la dirección es 2 (derecha), se da más relevancia al tamaño del buffer derecho.
        avgSize = (leftSize + 4 * rightSize) / 5;
        if (avgSize > rightSize) {
            avgSize = rightSize;
        }
    }

    // Calcula el nuevo tamaño de la ventana
    newWindowSize = avgSize/1; // Ajustar este valor de acuerdo a lo necesario.
    if (newWindowSize > 0) {
        if (newWindowSize > PARKING_SIZE) {
            ATOMIC_STORE(&window, PARKING_SIZE);  // Valor máximo para la ventana.
        } else {
            ATOMIC_STORE(&window, newWindowSize);
        }
    }
    else {
        ATOMIC_STORE(&window, 1);   // Valor mínimo para la ventana.
    }
}

/**************************************
 *      *FUNCION PARA IMPRIMIR
 *************************************/
// TODO Cambiar por Ncurses

void printState(char variable, int value) {
    static const char* messages[] = {
        ['a'] = "   Sale alguien de la cola izquierda.\n",
        ['A'] = "   Entra alguien al puente desde la cola izquierda.\n",
        ['b'] = "   Sale alguien de la cola derecha.\n",
        ['B'] = "   Entra alguien al puente desde la cola derecha.\n",
        ['O'] = "   Una persona cruzó el puente.\n",
        ['l'] = "   Una nueva persona espera en la cola izquierda\n",
        ['r'] = "   Una nueva persona espera en la cola derecha\n",
        ['*'] = "   Nadie nuevo sale de las colas.\n",
        ['+'] = "   Nadie nuevo entra al puente.\n"
    };

    pthread_mutex_lock(&printMutex);  // Adquirir el mutex antes de imprimir

    printBuffersAndDirection();
    if(variable == 'O' && value == 1) {
        ATOMIC_ADD(&contador_out, 1); //contador_out++
    }

    const char* message = messages[(int)variable];
    if (value)
        printf("%s", message);
    else
        printf("%s", (variable == 'a' || variable == 'b') ? messages['*'] : messages['+']);

    pthread_mutex_unlock(&printMutex);  // Liberar el mutex después de imprimir
}

void printBuffersAndDirection() {
    printf("T:%8.4fs  Dir:%d  Window:%2d  Cruzando:%2d  Done:%3d", (get_time() * 10), dir, ATOMIC_LOAD(&window), countBuffer(parkingBuffer), ATOMIC_LOAD(&contador_out));
    printf("   Wait:%2d ", countBuffer(leftBuffer));
    printBuffer2(leftBuffer);
    printf("   ");
    (dir == 1) ? printBuffer2(parkingBuffer) : printBuffer(parkingBuffer);
    printf("   ");
    printBuffer(rightBuffer);
    printf(" Wait:%2d", countBuffer(rightBuffer));
}
