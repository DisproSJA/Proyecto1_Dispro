#include "ui_display.h"

static const uint8_t DIGITOS_3X5[10][5] = {
    { 0b111, 0b101, 0b101, 0b101, 0b111 },
    { 0b010, 0b110, 0b010, 0b010, 0b111 },
    { 0b111, 0b001, 0b111, 0b100, 0b111 },
    { 0b111, 0b001, 0b111, 0b001, 0b111 },
    { 0b101, 0b101, 0b111, 0b001, 0b001 },
    { 0b111, 0b100, 0b111, 0b001, 0b111 },
    { 0b111, 0b100, 0b111, 0b101, 0b111 },
    { 0b111, 0b001, 0b001, 0b001, 0b001 },
    { 0b111, 0b101, 0b111, 0b101, 0b111 },
    { 0b111, 0b101, 0b111, 0b001, 0b111 }
};

static void dibujarNumeroDosDigitosEnMatriz(
    uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO],
    uint8_t filaInicio,
    uint8_t valor );

static void dibujarDigito3x5(
    uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO],
    uint8_t filaBase,
    uint8_t colBase,
    uint8_t digito );

static void escribirPixelEspejado(
    uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO],
    uint8_t fila,
    uint8_t col );

void
ui_limpiarFramebuffer(
    uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] )
{
    uint8_t fila;
    uint8_t col;

    for( fila = 0; fila < ALTO_TABLERO; fila++ ) {
        for( col = 0; col < ANCHO_TABLERO; col++ ) {
            framebuffer[fila][col] = 0;
        }
    }
}

void
ui_dibujarPantallaGameOver(
    const EstadoJuego *juego,
    uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] )
{
    ui_limpiarFramebuffer( framebuffer );

    dibujarNumeroDosDigitosEnMatriz(
        framebuffer,
        0,
        (uint8_t)( juego->piezasColocadas % 100U ) );

    dibujarNumeroDosDigitosEnMatriz(
        framebuffer,
        8,
        (uint8_t)( juego->puntaje % 100U ) );
}

static void
dibujarNumeroDosDigitosEnMatriz(
    uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO],
    uint8_t filaInicio,
    uint8_t valor )
{
    uint8_t decenas = (uint8_t)( valor / 10U );
    uint8_t unidades = (uint8_t)( valor % 10U );

    dibujarDigito3x5( framebuffer, filaInicio + 1U, 1U, decenas );
    dibujarDigito3x5( framebuffer, filaInicio + 1U, 5U, unidades );
}

static void
dibujarDigito3x5(
    uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO],
    uint8_t filaBase,
    uint8_t colBase,
    uint8_t digito )
{
    uint8_t fila;
    uint8_t col;

    for( fila = 0; fila < 5; fila++ ) {
        for( col = 0; col < 3; col++ ) {
            if( DIGITOS_3X5[digito][fila] & ( 1U << ( 2U - col ) ) ) {
                escribirPixelEspejado(
                    framebuffer,
                    (uint8_t)( filaBase + fila ),
                    (uint8_t)( colBase + col ) );
            }
        }
    }
}

static void
escribirPixelEspejado(
    uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO],
    uint8_t fila,
    uint8_t col )
{
    uint8_t colEspejada = (uint8_t)( 7U - col );
    framebuffer[fila][colEspejada] = 1;
}