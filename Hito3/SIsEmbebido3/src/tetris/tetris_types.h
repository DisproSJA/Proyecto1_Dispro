#ifndef TETRIS_TYPES_H
#define TETRIS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

typedef struct {
    uint16_t rotaciones[NUM_ROTACIONES];
} Tetromino;

typedef struct {
    const Tetromino *definicion;
    int8_t x;
    int8_t y;
    uint8_t rotacion;
    uint8_t tipo;
} PiezaActiva;

typedef struct {
    uint8_t celdas[ALTO_TABLERO][ANCHO_TABLERO];
} Tablero;

typedef struct {
    Tablero tableroFijo;
    PiezaActiva piezaActiva;
    uint16_t puntaje;
    uint16_t lineasCompletas;
    uint16_t piezasColocadas;
    bool gameOver;
} EstadoJuego;

#endif