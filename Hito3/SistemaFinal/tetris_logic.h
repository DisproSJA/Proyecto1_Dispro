/******************************************************************************
 *   Archivo:     tetris_logic.h
 *   Proyecto:    Tetris Multi-Plataforma - Hito Final
 *   Materia:     Diseno de Sistemas con Procesador (DISPRO)
 *
 *   Descripcion: Definiciones de tipos de datos, constantes del tablero y
 *                prototipos de todas las funciones de logica pura del Tetris.
 *                Este archivo NO depende de ninguna plataforma de hardware.
 *
 *   Integrantes:
 *      - Sofia Vega
 *      - Juan Sanchez
 *      - Andres Trujillo
 *
 *   Fecha:       Abril 2026
 ******************************************************************************/

#ifndef TETRIS_LOGIC_H
#define TETRIS_LOGIC_H

#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * CONSTANTES DE CONFIGURACION DEL TABLERO Y PIEZAS
 ******************************************************************************/

#define ANCHO_TABLERO    8
#define ALTO_TABLERO     16

#define BLOQUE_PIEZA     4
#define NUM_ROTACIONES   4
#define NUM_TETROMINOS   7

#define LINEAS_POR_NIVEL 5

/******************************************************************************
 * TIPOS DE DATOS DEL JUEGO
 ******************************************************************************/

/* Cada tetromino tiene 4 rotaciones codificadas en 16 bits (matriz 4x4) */
typedef struct {
    uint16_t rotaciones[NUM_ROTACIONES];
} Tetromino;

/* Pieza que esta cayendo actualmente en el tablero */
typedef struct {
    const Tetromino *definicion;
    int8_t x;
    int8_t y;
    uint8_t rotacion;
    uint8_t tipo;
} PiezaActiva;

/* Tablero de juego con las celdas fijas (bloques ya colocados) */
typedef struct {
    uint8_t celdas[ALTO_TABLERO][ANCHO_TABLERO];
} Tablero;

/* Estado completo del juego en un momento dado */
typedef struct {
    Tablero tableroFijo;
    PiezaActiva piezaActiva;
    uint16_t puntaje;
    uint16_t lineasCompletas;
    uint16_t piezasColocadas;
    bool gameOver;
} EstadoJuego;

/******************************************************************************
 * PROTOTIPOS DE FUNCIONES DE LOGICA
 ******************************************************************************/

void tetris_inicializarJuego( EstadoJuego *juego );

void tetris_reiniciarTablero( EstadoJuego *juego );

void tetris_generarPiezaInicial( EstadoJuego *juego );

bool tetris_celdaOcupadaEnRotacion( const Tetromino *pieza,
                                    uint8_t rot,
                                    uint8_t fila,
                                    uint8_t col );

bool tetris_puedeUbicarse( const EstadoJuego *juego,
                           const Tetromino *pieza,
                           int8_t x,
                           int8_t y,
                           uint8_t rot );

bool tetris_intentarMover( EstadoJuego *juego, int8_t dx, int8_t dy );

bool tetris_intentarRotar( EstadoJuego *juego );

void tetris_fijarPiezaActiva( EstadoJuego *juego );

bool tetris_generarNuevaPieza( EstadoJuego *juego );

bool tetris_bajarOFijar( EstadoJuego *juego );

uint8_t tetris_eliminarLineasCompletas( EstadoJuego *juego );

bool tetris_hayBloquesEnFilaSuperior( const EstadoJuego *juego );

void tetris_dibujarEstadoEnFramebuffer(
    const EstadoJuego *juego,
    uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] );

void tetris_dibujarPantallaGameOver(
    const EstadoJuego *juego,
    uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] );

#endif
