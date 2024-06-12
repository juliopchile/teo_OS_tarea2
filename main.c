/**
 * @file tarea2.c
 * @brief Sincronización entre Procesos - Teoria de Sistemas Operativos - ELO321
 *
 * @details
 * El objetivo de este código es proporcionar una solución a la problemática de sincronización de procesos para un
 * estacionamiento con una sola calle de entrada y salida. La situación modelada consiste en vehículos que desean
 * ingresar o salir del estacionamiento, utilizando la misma calle de un solo sentido, lo que requiere una gestión
 * adecuada para evitar conflictos y asegurar un tránsito fluido.
 *
 * Los vehículos se trasladan en un solo sentido a la vez, ya sea entrando o saliendo del estacionamiento.
 * La llegada y salida de vehículos es aleatoria, lo cual puede causar la formación de colas en ambos sentidos.
 * La solución implementada asegura que no se producirá un bloqueo indefinido (Deadlock) ni inanición (Starvation)
 * de vehículos en espera. Esto se logra al adaptar el flujo de vehiculos dinámicamente.
 *
 * Para manejar las secciones críticas y la sincronización entre procesos, se hace uso de técnicas como semáforos
 * y variables de condición. El retardo en el paso de los vehículos por la calle de entrada/salida se simula para
 * representar el tiempo necesario para completar la operación. Además, se garantiza que los vehículos en el sentido
 * opuesto no esperen indefinidamente.
 *
 * La visualización del estado del estacionamiento se realiza utilizando la biblioteca "Ncurses". Esta biblioteca
 * permite mostrar en tiempo real la cantidad de vehículos en espera de entrar o salir, así como el número de
 * vehículos que han ingresado o salido del estacionamiento. El propósito es brindar una representación clara
 * del estado del sistema y el avance de los vehículos.
 *
 * Las funciones principales del código incluyen la gestión de colas de espera, la sincronización del paso de vehículos
 * y la actualización de la visualización en pantalla. Las funciones auxiliares están agrupadas en un archivo separado
 * para mantener la estructura del código clara y modular.
 *
 * @date 11 de junio de 2024
 * @authors
 * Julio López
 * Hector Cepeda
 */

#include <time.h>
#include "funciones.h"
#include <curses.h>

//Constantes
#define BUFFER_SIZE         20
#define PARKING_SIZE        10
#define WINDOW_SIZE         3
#define PARKING_SPEED       500000


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
    while(ATOMIC_LOAD(&contador_out) <= 50) {
        sem_wait(&parkingSemaphore);   // ! Esperar a que el estacionamiento esté disponible.

        //? ENTRADA AL ESTACIONAMIENTO *//
        for (int i = 0; i < ATOMIC_LOAD(&window); i++) {
            //* LADO IZQUIERDO *//
            if((dir == 1)) {
                sem_wait(&leftSemaphore);               // ? Se pausa leftSemaphore
                my_sleep(half_time);                    // Saliendo de la cola
                value = removeFromBuffer(leftBuffer);   // Sale de la cola.
                printState('a', value);
                sem_post(&leftSemaphore);               // ? Se libera leftSemaphore
                my_sleep(half_time);                    // Entrando al estacionamiento
                addToBuffer(parkingBuffer, value);      // Entra al estacionamiento.
                parkingBuffer->tail = (parkingBuffer->tail + 1) % (parkingBuffer->size);
                printState('A', value);
            }

            //* LADO DERECHO *//
            else if((dir == 2)) {
                sem_wait(&rightSemaphore);              // ? Se pausa rightSemaphore
                my_sleep(half_time);                    // Saliendo de la cola
                value = removeFromBuffer(rightBuffer);  // Sale alguien de la cola.
                printState('b', value);
                sem_post(&rightSemaphore);              // ? Se libera rightSemaphore
                my_sleep(half_time);
                addToBuffer(parkingBuffer, value);      // Entra al estacionamiento.
                parkingBuffer->tail = (parkingBuffer->tail + 1) % (parkingBuffer->size);
                printState('B', value);
            }
        }

        //? AVANZAR EN EL ESTACIONAMIENTO *//
        // Vaciar el buffer del estacionamiento.
        while(!isBufferEmpty(parkingBuffer)) {
            my_sleep(PARKING_SPEED);                    // Vehiculo moviéndose/saliendo del estacionamiento.
            value = removeFromBuffer(parkingBuffer);
            printState('O', value);
        }

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
            printState('l', -1);
            sem_post(&leftSemaphore);       // Se libera el semáforo. (V)
        }

        //* LADO DERECHO *//
        else if(bufferId == 2) {
            waitingTime = ((2*PARKING_SPEED) + rand() % (5*PARKING_SPEED));
            my_sleep(waitingTime);
            sem_wait(&rightSemaphore);      // Se pausa el semáforo. (P)
            addToBuffer(rightBuffer, 1);    // Se añade vehiculo al buffer.
            printState('r', -1);
            sem_post(&rightSemaphore);      // Se libera el semáforo. (V)
        }
    }
    return NULL;
}

int main() {
    // Inicializar ncurses
    initscr();
    cbreak();
    noecho();
    curs_set(FALSE);

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

    // Terminar ncurses
    endwin();

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

void printState(char variable, int value) {
    static const char* messages[] = {
        ['a'] = "   Sale alguien de la cola izquierda.\n",
        ['A'] = "   Entra alguien al puente desde la cola izquierda.\n",
        ['b'] = "   Sale alguien de la cola derecha.\n",
        ['B'] = "   Entra alguien al puente desde la cola derecha.\n",
        ['O'] = "   Un vehículo cruzó.\n",
        ['l'] = "   Una nuevo vehículo espera en la cola izquierda\n",
        ['r'] = "   Una nuevo vehículo espera en la cola derecha\n",
        ['*'] = "   Nadie nuevo sale de las colas.\n",
        ['+'] = "   Nadie nuevo entra al puente.\n"
    };
    pthread_mutex_lock(&printMutex);  // Adquirir el mutex antes de imprimir

    // Limpiar la pantalla
    clear();

    // Imprimir buffers y dirección
    printBuffersAndDirection();

    // Imprimir el estado actual
    if (variable == 'O' && value == 1) {
        ATOMIC_ADD(&contador_out, 1); // contador_out++
    }
    const char* message = messages[(int)variable];

    // Mover el cursor a la fila 10, columna 0
    move(10, 0);
    if (value)
        printw("%s", message);
    else
        printw("%s", (variable == 'a' || variable == 'b') ? messages['*'] : messages['+']);

    // Refrescar la pantalla para mostrar los cambios
    refresh();

    pthread_mutex_unlock(&printMutex);  // Liberar el mutex después de imprimir
}

void printBuffersAndDirection() {
    // Mover el cursor a la posición inicial (fila 0, columna 0)
    move(0, 0);

    // Imprimir información general
    printw("T:%8.4fs  Dir:%d  Window:%2d  Cruzando:%2d  Done:%3d",
            (get_time() * 10), dir, ATOMIC_LOAD(&window), countBuffer(parkingBuffer), ATOMIC_LOAD(&contador_out));
    
    // Imprimir la cola izquierda
    printw("   Wait:%2d ", countBuffer(leftBuffer));
    printBuffer2(leftBuffer);
    
    // Imprimir el buffer del estacionamiento
    printw("   ");
    (dir == 1) ? printBuffer2(parkingBuffer) : printBuffer(parkingBuffer);
    
    // Imprimir la cola derecha
    printw("   ");
    printBuffer(rightBuffer);
    printw(" Wait:%2d\n", countBuffer(rightBuffer));
}
