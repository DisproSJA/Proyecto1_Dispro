#ifndef TETRIS_LOGIC_H
#define TETRIS_LOGIC_H

#include "tetris_types.h"

#ifdef __cplusplus
extern "C" {
#endif

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

    void tetris_dibujarEstadoEnFramebuffer(
        const EstadoJuego *juego,
        uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] );

#ifdef __cplusplus
}
#endif

#endif