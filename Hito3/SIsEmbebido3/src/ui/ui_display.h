#ifndef UI_DISPLAY_H
#define UI_DISPLAY_H

#include <stdint.h>
#include "../tetris/tetris_types.h"

#ifdef __cplusplus
extern "C" {
#endif

    void ui_limpiarFramebuffer(
        uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] );

    void ui_dibujarPantallaGameOver(
        const EstadoJuego *juego,
        uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] );

#ifdef __cplusplus
}
#endif

#endif