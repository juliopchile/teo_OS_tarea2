/**
 * @file funciones.c
 * @brief Definiciones de funciones para un buffer circular.
 * @date 4 de junio de 2023
 * @authors Julio López
 */

#include "funciones.h"

CircularBuffer* createBuffer(int size) {
    CircularBuffer* cb = malloc(sizeof(CircularBuffer));
    cb->buffer = calloc(size, sizeof(int));
    cb->size = size;
    cb->head = 0;
    cb->tail = 0;
    return cb;
}

int isBufferEmpty(CircularBuffer *cb) {
    for (int i = 0; i < cb->size; i++) {
        if (cb->buffer[i] != 0) {
            return 0;
        }
    }
    return 1;
}

int countBuffer(CircularBuffer *cb) {
    int count = 0;
    for (int i = 0; i < cb->size; i++) {
        if (cb->buffer[i] != 0) {
            count++;
        }
    }
    return count;
}

void destroyBuffer(CircularBuffer *cb) {
    if(cb != NULL) {
        free(cb->buffer);
        free(cb);
    }
}

void addToBuffer(CircularBuffer *cb, int value) {
    cb->buffer[cb->head] = value;
    cb->head = (cb->head + 1) % (cb->size);
}

int removeFromBuffer(CircularBuffer *cb) {
    int value = cb->buffer[cb->tail];
    cb->buffer[cb->tail] = 0;
    cb->tail = (cb->tail + 1) % (cb->size);
    return value;
}

void printBuffer(CircularBuffer *cb) {
    for(int i = 0; i < cb->size; i++) {
        if(cb->buffer[(cb->tail + i) % (cb->size)])
            printf("*");
        else
            printf("_");
    }
}

void printBuffer2(CircularBuffer *cb) {
    for(int i = (cb->size)-1; i >= 0; i--) {
        if(cb->buffer[(cb->tail + i) % (cb->size)])
            printf("*");
        else
            printf("_");
    }
}