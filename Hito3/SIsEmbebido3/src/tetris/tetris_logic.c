#include "tetris_logic.h"

/* Tetrominós base codificados en matriz 4x4 dentro de 16 bits */
static const Tetromino TETROMINOS[NUM_TETROMINOS] = {
    /* T */
    { { 0x4E00, 0x4640, 0x0E40, 0x4C40 } },

    /* O */
    { { 0x6600, 0x6600, 0x6600, 0x6600 } },

    /* I */
    { { 0x0F00, 0x2222, 0x00F0, 0x4444 } },

    /* L */
    { { 0x8E00, 0x6440, 0x0E20, 0x44C0 } },

    /* J */
    { { 0x2E00, 0x4460, 0x0E80, 0xC440 } },

    /* S */
    { { 0x6C00, 0x4620, 0x06C0, 0x8C40 } },

    /* Z */
    { { 0xC600, 0x2640, 0x0C60, 0x4C80 } }
};

static void limpiarMatriz( uint8_t matriz[ALTO_TABLERO][ANCHO_TABLERO] );
static void dibujarTableroFijo( const EstadoJuego *juego,
                                uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] );
static void dibujarPiezaActiva( const EstadoJuego *juego,
                                uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] );
static bool filaCompleta( const EstadoJuego *juego, uint8_t fila );
static void bajarFilasSuperiores( EstadoJuego *juego, uint8_t filaEliminada );
static void actualizarPuntajePorPiezas( EstadoJuego *juego );

void
tetris_inicializarJuego( EstadoJuego *juego )
{
    tetris_reiniciarTablero( juego );
    juego->puntaje = 0;
    juego->lineasCompletas = 0;
    juego->piezasColocadas = 0;
    juego->gameOver = false;
    tetris_generarPiezaInicial( juego );
}

void
tetris_reiniciarTablero( EstadoJuego *juego )
{
    uint8_t fila;
    uint8_t col;

    for( fila = 0; fila < ALTO_TABLERO; fila++ ) {
        for( col = 0; col < ANCHO_TABLERO; col++ ) {
            juego->tableroFijo.celdas[fila][col] = 0;
        }
    }
}

void
tetris_generarPiezaInicial( EstadoJuego *juego )
{
    juego->piezaActiva.tipo = 0;
    juego->piezaActiva.definicion = &TETROMINOS[0];
    juego->piezaActiva.x = 2;
    juego->piezaActiva.y = 0;
    juego->piezaActiva.rotacion = 0;
}

bool
tetris_celdaOcupadaEnRotacion(
    const Tetromino *pieza,
    uint8_t rot,
    uint8_t fila,
    uint8_t col )
{
    uint8_t indiceBit = (uint8_t)( fila * 4 + col );
    uint16_t mascara = (uint16_t)( 1U << ( 15 - indiceBit ) );

    return ( pieza->rotaciones[rot] & mascara ) ? true : false;
}

bool
tetris_puedeUbicarse(
    const EstadoJuego *juego,
    const Tetromino *pieza,
    int8_t x,
    int8_t y,
    uint8_t rot )
{
    uint8_t fila;
    uint8_t col;

    for( fila = 0; fila < 4; fila++ ) {
        for( col = 0; col < 4; col++ ) {
            int8_t xTablero;
            int8_t yTablero;

            if( !tetris_celdaOcupadaEnRotacion( pieza, rot, fila, col ) ) {
                continue;
            }

            xTablero = (int8_t)( x + col );
            yTablero = (int8_t)( y + fila );

            if( xTablero < 0 || xTablero >= ANCHO_TABLERO ) {
                return false;
            }

            if( yTablero < 0 || yTablero >= ALTO_TABLERO ) {
                return false;
            }

            if( juego->tableroFijo.celdas[(uint8_t)yTablero][(uint8_t)xTablero] ) {
                return false;
            }
        }
    }

    return true;
}

bool
tetris_intentarMover( EstadoJuego *juego, int8_t dx, int8_t dy )
{
    int8_t nuevoX = (int8_t)( juego->piezaActiva.x + dx );
    int8_t nuevoY = (int8_t)( juego->piezaActiva.y + dy );

    if( !tetris_puedeUbicarse( juego,
                               juego->piezaActiva.definicion,
                               nuevoX,
                               nuevoY,
                               juego->piezaActiva.rotacion ) ) {
        return false;
    }

    juego->piezaActiva.x = nuevoX;
    juego->piezaActiva.y = nuevoY;

    return true;
}

bool
tetris_intentarRotar( EstadoJuego *juego )
{
    uint8_t nuevaRotacion =
        (uint8_t)( ( juego->piezaActiva.rotacion + 1U ) & 0x03U );

    if( !tetris_puedeUbicarse( juego,
                               juego->piezaActiva.definicion,
                               juego->piezaActiva.x,
                               juego->piezaActiva.y,
                               nuevaRotacion ) ) {
        return false;
    }

    juego->piezaActiva.rotacion = nuevaRotacion;

    return true;
}

void
tetris_fijarPiezaActiva( EstadoJuego *juego )
{
    uint8_t fila;
    uint8_t col;

    for( fila = 0; fila < 4; fila++ ) {
        for( col = 0; col < 4; col++ ) {
            int8_t xTablero;
            int8_t yTablero;

            if( !tetris_celdaOcupadaEnRotacion( juego->piezaActiva.definicion,
                                                juego->piezaActiva.rotacion,
                                                fila,
                                                col ) ) {
                continue;
            }

            xTablero = (int8_t)( juego->piezaActiva.x + col );
            yTablero = (int8_t)( juego->piezaActiva.y + fila );

            if( xTablero >= 0 && xTablero < ANCHO_TABLERO &&
                yTablero >= 0 && yTablero < ALTO_TABLERO ) {
                juego->tableroFijo.celdas[(uint8_t)yTablero][(uint8_t)xTablero] = 1;
            }
        }
    }

    juego->piezasColocadas++;
    actualizarPuntajePorPiezas( juego );
}

bool
tetris_generarNuevaPieza( EstadoJuego *juego )
{
    static uint8_t siguienteTipo = 1U;

    juego->piezaActiva.tipo = siguienteTipo;
    juego->piezaActiva.definicion = &TETROMINOS[siguienteTipo];
    juego->piezaActiva.x = 2;
    juego->piezaActiva.y = 0;
    juego->piezaActiva.rotacion = 0;

    siguienteTipo++;
    if( siguienteTipo >= NUM_TETROMINOS ) {
        siguienteTipo = 0;
    }

    if( !tetris_puedeUbicarse( juego,
                               juego->piezaActiva.definicion,
                               juego->piezaActiva.x,
                               juego->piezaActiva.y,
                               juego->piezaActiva.rotacion ) ) {
        juego->gameOver = true;
        return false;
    }

    return true;
}

bool
tetris_bajarOFijar( EstadoJuego *juego )
{
    uint8_t lineasEliminadas;

    if( juego->gameOver ) {
        return false;
    }

    if( tetris_intentarMover( juego, 0, 1 ) ) {
        return true;
    }

    tetris_fijarPiezaActiva( juego );

    if( tetris_hayBloquesEnFilaSuperior( juego ) ) {
        juego->gameOver = true;
        return false;
    }

    lineasEliminadas = tetris_eliminarLineasCompletas( juego );
    if( lineasEliminadas > 0U ) {
        juego->lineasCompletas =
            (uint16_t)( juego->lineasCompletas + lineasEliminadas );
    }

    return tetris_generarNuevaPieza( juego );
}

uint8_t
tetris_eliminarLineasCompletas( EstadoJuego *juego )
{
    uint8_t fila;
    uint8_t eliminadas = 0;

    for( fila = 0; fila < ALTO_TABLERO; fila++ ) {
        if( filaCompleta( juego, fila ) ) {
            bajarFilasSuperiores( juego, fila );
            eliminadas++;
            fila--;
        }
    }

    return eliminadas;
}

bool
tetris_hayBloquesEnFilaSuperior( const EstadoJuego *juego )
{
    uint8_t col;

    for( col = 0; col < ANCHO_TABLERO; col++ ) {
        if( juego->tableroFijo.celdas[0][col] != 0U ) {
            return true;
        }
    }

    return false;
}

void
tetris_dibujarEstadoEnFramebuffer(
    const EstadoJuego *juego,
    uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] )
{
    limpiarMatriz( framebuffer );
    dibujarTableroFijo( juego, framebuffer );
    dibujarPiezaActiva( juego, framebuffer );
}

static void
limpiarMatriz( uint8_t matriz[ALTO_TABLERO][ANCHO_TABLERO] )
{
    uint8_t fila;
    uint8_t col;

    for( fila = 0; fila < ALTO_TABLERO; fila++ ) {
        for( col = 0; col < ANCHO_TABLERO; col++ ) {
            matriz[fila][col] = 0;
        }
    }
}

static void
dibujarTableroFijo(
    const EstadoJuego *juego,
    uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] )
{
    uint8_t fila;
    uint8_t col;

    for( fila = 0; fila < ALTO_TABLERO; fila++ ) {
        for( col = 0; col < ANCHO_TABLERO; col++ ) {
            framebuffer[fila][col] = juego->tableroFijo.celdas[fila][col];
        }
    }
}

static void
dibujarPiezaActiva(
    const EstadoJuego *juego,
    uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] )
{
    uint8_t fila;
    uint8_t col;

    if( juego->gameOver ) {
        return;
    }

    for( fila = 0; fila < 4; fila++ ) {
        for( col = 0; col < 4; col++ ) {
            int8_t xTablero;
            int8_t yTablero;

            if( !tetris_celdaOcupadaEnRotacion( juego->piezaActiva.definicion,
                                                juego->piezaActiva.rotacion,
                                                fila,
                                                col ) ) {
                continue;
            }

            xTablero = (int8_t)( juego->piezaActiva.x + col );
            yTablero = (int8_t)( juego->piezaActiva.y + fila );

            if( xTablero >= 0 && xTablero < ANCHO_TABLERO &&
                yTablero >= 0 && yTablero < ALTO_TABLERO ) {
                framebuffer[(uint8_t)yTablero][(uint8_t)xTablero] = 1;
            }
        }
    }
}

static bool
filaCompleta( const EstadoJuego *juego, uint8_t fila )
{
    uint8_t col;

    for( col = 0; col < ANCHO_TABLERO; col++ ) {
        if( juego->tableroFijo.celdas[fila][col] == 0U ) {
            return false;
        }
    }

    return true;
}

static void
bajarFilasSuperiores( EstadoJuego *juego, uint8_t filaEliminada )
{
    int8_t fila;
    uint8_t col;

    for( fila = (int8_t)filaEliminada; fila > 0; fila-- ) {
        for( col = 0; col < ANCHO_TABLERO; col++ ) {
            juego->tableroFijo.celdas[(uint8_t)fila][col] =
                juego->tableroFijo.celdas[(uint8_t)( fila - 1 )][col];
        }
    }

    for( col = 0; col < ANCHO_TABLERO; col++ ) {
        juego->tableroFijo.celdas[0][col] = 0;
    }
}

static void
actualizarPuntajePorPiezas( EstadoJuego *juego )
{
    juego->puntaje = (uint16_t)( ( juego->piezasColocadas / 10U ) * 10U );
}