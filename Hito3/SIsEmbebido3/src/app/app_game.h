#ifndef APP_GAME_H
#define APP_GAME_H

#include <stdint.h>
#include "../tetris/tetris_types.h"
#include "../drivers/driver_avr.h"

#ifdef __cplusplus
extern "C" {
#endif

    void app_game_init( EstadoJuego *juego,
                        DebounceBotones *debounce,
                        BotonesEstado *botones,
                        uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] );

    void app_game_update( EstadoJuego *juego,
                          DebounceBotones *debounce,
                          BotonesEstado *botones,
                          uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] );

#ifdef __cplusplus
}
#endif

#endif