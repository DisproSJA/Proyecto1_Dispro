/******************************************************************************
 *   Archivo:     tetris_logic.c
 *   Proyecto:    Tetris Multi-Plataforma - Hito Final
 *   Materia:     Diseno de Sistemas con Procesador (DISPRO)
 *
 *   Descripcion: Implementacion de toda la logica del Tetris.
 *                Incluye: definicion de los 7 tetrominos, deteccion de
 *                colisiones, movimiento, rotacion, fijado de piezas,
 *                eliminacion de lineas completas y sistema de puntuacion.
 *                Este archivo NO tiene dependencias de hardware.
 *
 *   Integrantes:
 *      - Sofia Vega
 *      - Juan Sanchez
 *      - Andres Trujillo
 *
 *   Fecha:       Abril 2026
 ******************************************************************************/

#include "tetris_logic.h"
#include <stdlib.h>

/******************************************************************************
 * DEFINICION DE LOS 7 TETROMINOS
 *
 * Cada pieza se codifica como una matriz 4x4 dentro de 16 bits.
 * Bit 15 = celda (0,0),  Bit 14 = celda (0,1),  ...  Bit 0 = celda (3,3).
 * Cada tetromino tiene sus 4 rotaciones precalculadas.
 ******************************************************************************/

static const Tetromino TETROMINOS[NUM_TETROMINOS] = {
    { { 0x4E00, 0x4640, 0x0E40, 0x4C40 } },
    { { 0x6600, 0x6600, 0x6600, 0x6600 } },
    { { 0x0F00, 0x2222, 0x00F0, 0x4444 } },
    { { 0x8E00, 0x6440, 0x0E20, 0x44C0 } },
    { { 0x2E00, 0x4460, 0x0E80, 0xC440 } },
    { { 0x6C00, 0x4620, 0x06C0, 0x8C40 } },
    { { 0xC600, 0x2640, 0x0C60, 0x4C80 } }
};

/******************************************************************************
 * PROTOTIPOS DE FUNCIONES INTERNAS (STATIC)
 ******************************************************************************/

static void limpiarMatriz( uint8_t matriz[ALTO_TABLERO][ANCHO_TABLERO] );

static void dibujarTableroFijo( const EstadoJuego *juego,
                                uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] );

static void dibujarPiezaActiva( const EstadoJuego *juego,
                                uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] );

static bool filaCompleta( const EstadoJuego *juego, uint8_t fila );

static void bajarFilasSuperiores( EstadoJuego *juego, uint8_t filaEliminada );

static void actualizarPuntajePorPiezas( EstadoJuego *juego );

static void actualizarPuntajePorLineas( EstadoJuego *juego, uint8_t lineas );

/******************************************************************************
 * IMPLEMENTACION DE FUNCIONES PUBLICAS
 ******************************************************************************/

/*FN****************************************************************************
*
*   void tetris_inicializarJuego( EstadoJuego *juego )
*
*   Que hace:  Pone todo el estado del juego en cero (tablero vacio,
*              puntaje en cero) y genera la primera pieza
*              aleatoria para que empiece a caer.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
void tetris_inicializarJuego( EstadoJuego *juego )
{
    tetris_reiniciarTablero( juego );

    juego->puntaje         = 0;
    juego->lineasCompletas = 0;
    juego->piezasColocadas = 0;
    juego->gameOver        = false;

    tetris_generarPiezaInicial( juego );
}

/*FN****************************************************************************
*
*   void tetris_reiniciarTablero( EstadoJuego *juego )
*
*   Que hace:  Limpia todas las celdas del tablero poniendolas en cero
*              para empezar una partida nueva.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
void tetris_reiniciarTablero( EstadoJuego *juego )
{
    uint8_t fila;
    uint8_t col;

    for( fila = 0; fila < ALTO_TABLERO; fila++ ) {
        for( col = 0; col < ANCHO_TABLERO; col++ ) {
            juego->tableroFijo.celdas[fila][col] = 0;
        }
    }
}

/*FN****************************************************************************
*
*   void tetris_generarPiezaInicial( EstadoJuego *juego )
*
*   Que hace:  Genera la primera pieza del juego de forma aleatoria y la
*              coloca en la parte superior central del tablero.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
void tetris_generarPiezaInicial( EstadoJuego *juego )
{
    uint8_t tipo = (uint8_t)( rand() % NUM_TETROMINOS );

    juego->piezaActiva.tipo       = tipo;
    juego->piezaActiva.definicion = &TETROMINOS[tipo];
    juego->piezaActiva.x          = 2;
    juego->piezaActiva.y          = 0;
    juego->piezaActiva.rotacion   = 0;
}

/*FN****************************************************************************
*
*   bool tetris_celdaOcupadaEnRotacion( pieza, rot, fila, col )
*
*   Que hace:  Revisa si una celda especifica de un tetromino esta ocupada
*              en una rotacion dada. Usa la codificacion de 16 bits donde
*              cada bit representa una celda de la matriz 4x4.
*
*   Retorna:   true si la celda esta ocupada, false si esta vacia
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
bool tetris_celdaOcupadaEnRotacion( const Tetromino *pieza,
                               uint8_t rot,
                               uint8_t fila,
                               uint8_t col )
{
    uint8_t indiceBit = (uint8_t)( fila * 4 + col );
    uint16_t mascara  = (uint16_t)( 1U << ( 15 - indiceBit ) );

    return ( pieza->rotaciones[rot] & mascara ) ? true : false;
}

/*FN****************************************************************************
*
*   bool tetris_puedeUbicarse( juego, pieza, x, y, rot )
*
*   Que hace:  Verifica si un tetromino puede colocarse en una posicion
*              dada sin salirse del tablero y sin chocar con bloques fijos.
*
*   Retorna:   true si la ubicacion es valida, false si no
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
bool tetris_puedeUbicarse( const EstadoJuego *juego,
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

/*FN****************************************************************************
*
*   bool tetris_intentarMover( juego, dx, dy )
*
*   Que hace:  Intenta mover la pieza activa en la direccion indicada
*              (dx para horizontal, dy para vertical). Solo mueve si
*              la nueva posicion es valida.
*
*   Retorna:   true si se pudo mover, false si no
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
bool tetris_intentarMover( EstadoJuego *juego, int8_t dx, int8_t dy )
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

/*FN****************************************************************************
*
*   bool tetris_intentarRotar( juego )
*
*   Que hace:  Intenta rotar la pieza activa 90 grados en sentido horario.
*              Solo rota si la nueva orientacion es valida (no choca ni
*              se sale del tablero).
*
*   Retorna:   true si se pudo rotar, false si no
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
bool tetris_intentarRotar( EstadoJuego *juego )
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

/*FN****************************************************************************
*
*   void tetris_fijarPiezaActiva( juego )
*
*   Que hace:  Copia los bloques de la pieza activa al tablero fijo para
*              que queden permanentes. Tambien suma puntos por pieza
*              colocada e incrementa el contador de piezas.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
void tetris_fijarPiezaActiva( EstadoJuego *juego )
{
    uint8_t fila;
    uint8_t col;

    for( fila = 0; fila < 4; fila++ ) {
        for( col = 0; col < 4; col++ ) {
            int8_t xTablero;
            int8_t yTablero;

            if( !tetris_celdaOcupadaEnRotacion(
                    juego->piezaActiva.definicion,
                    juego->piezaActiva.rotacion,
                    fila, col ) ) {
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

/*FN****************************************************************************
*
*   bool tetris_generarNuevaPieza( juego )
*
*   Que hace:  Genera un nuevo tetromino aleatorio y lo coloca arriba del
*              tablero. Si la pieza nueva no cabe porque hay bloques en el
*              camino, se activa el game over.
*
*   Retorna:   true si la pieza se genero bien, false si es game over
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
bool tetris_generarNuevaPieza( EstadoJuego *juego )
{
    uint8_t tipo = (uint8_t)( rand() % NUM_TETROMINOS );

    juego->piezaActiva.tipo       = tipo;
    juego->piezaActiva.definicion = &TETROMINOS[tipo];
    juego->piezaActiva.x          = 2;
    juego->piezaActiva.y          = 0;
    juego->piezaActiva.rotacion   = 0;

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

/*FN****************************************************************************
*
*   bool tetris_bajarOFijar( juego )
*
*   Que hace:  Intenta bajar la pieza una fila. Si no puede bajar mas,
*              la fija al tablero, revisa si hay lineas completas para
*              eliminarlas, actualiza el puntaje y genera una pieza nueva.
*              Si la fila superior tiene bloques, se activa game over.
*
*   Retorna:   true si la pieza bajo, false si se fijo o es game over
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
bool tetris_bajarOFijar( EstadoJuego *juego )
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
        actualizarPuntajePorLineas( juego, lineasEliminadas );
    }

    return tetris_generarNuevaPieza( juego );
}

/*FN****************************************************************************
*
*   uint8_t tetris_eliminarLineasCompletas( juego )
*
*   Que hace:  Recorre todo el tablero buscando filas completamente llenas.
*              Cuando encuentra una, la elimina y baja todas las filas de
*              arriba una posicion.
*
*   Retorna:   Cantidad de lineas que se eliminaron
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
uint8_t tetris_eliminarLineasCompletas( EstadoJuego *juego )
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

/*FN****************************************************************************
*
*   bool tetris_hayBloquesEnFilaSuperior( juego )
*
*   Que hace:  Revisa si la fila mas alta del tablero (fila 0) tiene algun
*              bloque. Esto se usa para detectar game over despues de fijar
*              una pieza.
*
*   Retorna:   true si hay bloques en la fila 0, false si esta vacia
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
bool tetris_hayBloquesEnFilaSuperior( const EstadoJuego *juego )
{
    uint8_t col;

    for( col = 0; col < ANCHO_TABLERO; col++ ) {
        if( juego->tableroFijo.celdas[0][col] != 0U ) {
            return true;
        }
    }

    return false;
}

/*FN****************************************************************************
*
*   void tetris_dibujarEstadoEnFramebuffer( juego, framebuffer )
*
*   Que hace:  Construye la imagen completa del juego en el framebuffer.
*              Primero limpia la matriz, luego dibuja los bloques fijos
*              del tablero y por ultimo superpone la pieza activa.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
void tetris_dibujarEstadoEnFramebuffer( const EstadoJuego *juego,
                                   uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] )
{
    limpiarMatriz( framebuffer );
    dibujarTableroFijo( juego, framebuffer );
    dibujarPiezaActiva( juego, framebuffer );
}

/******************************************************************************
 * IMPLEMENTACION DE FUNCIONES INTERNAS (STATIC)
 ******************************************************************************/

/*FN****************************************************************************
*
*   static void limpiarMatriz( matriz )
*
*   Que hace:  Pone en cero todas las celdas de una matriz del tamano
*              del tablero. Se usa para preparar el framebuffer antes de
*              dibujar.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
static void limpiarMatriz( uint8_t matriz[ALTO_TABLERO][ANCHO_TABLERO] )
{
    uint8_t fila;
    uint8_t col;

    for( fila = 0; fila < ALTO_TABLERO; fila++ ) {
        for( col = 0; col < ANCHO_TABLERO; col++ ) {
            matriz[fila][col] = 0;
        }
    }
}

/*FN****************************************************************************
*
*   static void dibujarTableroFijo( juego, framebuffer )
*
*   Que hace:  Copia las celdas del tablero fijo (bloques ya colocados)
*              al framebuffer para que se vean en pantalla.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
static void dibujarTableroFijo( const EstadoJuego *juego,
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

/*FN****************************************************************************
*
*   static void dibujarPiezaActiva( juego, framebuffer )
*
*   Que hace:  Superpone la pieza que esta cayendo sobre el framebuffer.
*              Si el juego termino (game over), no dibuja nada.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
static void dibujarPiezaActiva( const EstadoJuego *juego,
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

            if( !tetris_celdaOcupadaEnRotacion(
                    juego->piezaActiva.definicion,
                    juego->piezaActiva.rotacion,
                    fila, col ) ) {
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

/*FN****************************************************************************
*
*   static bool filaCompleta( juego, fila )
*
*   Que hace:  Revisa si una fila del tablero esta completamente llena
*              (todas las celdas tienen un bloque).
*
*   Retorna:   true si la fila esta llena, false si tiene algun hueco
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
static bool filaCompleta( const EstadoJuego *juego, uint8_t fila )
{
    uint8_t col;

    for( col = 0; col < ANCHO_TABLERO; col++ ) {
        if( juego->tableroFijo.celdas[fila][col] == 0U ) {
            return false;
        }
    }

    return true;
}

/*FN****************************************************************************
*
*   static void bajarFilasSuperiores( juego, filaEliminada )
*
*   Que hace:  Cuando se elimina una linea completa, baja todas las filas
*              que estan por encima una posicion para llenar el hueco.
*              La fila superior (fila 0) queda vacia.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
static void bajarFilasSuperiores( EstadoJuego *juego, uint8_t filaEliminada )
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

/*FN****************************************************************************
*
*   static void actualizarPuntajePorPiezas( juego )
*
*   Que hace:  Suma 10 puntos al puntaje cada vez que se coloca una pieza
*              en el tablero.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
static void actualizarPuntajePorPiezas( EstadoJuego *juego )
{
    juego->puntaje = (uint16_t)( juego->puntaje + 10U );
}

/*FN****************************************************************************
*
*   static void actualizarPuntajePorLineas( juego, lineas )
*
*   Que hace:  Suma puntos de bonificacion segun cuantas lineas se
*              eliminaron de una vez. El sistema clasico de Tetris da:
*              1 linea = 100 pts, 2 = 300 pts, 3 = 500 pts, 4 = 800 pts.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo   
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
static void actualizarPuntajePorLineas( EstadoJuego *juego, uint8_t lineas )
{
    static const uint16_t bonusPorLineas[] = { 0, 100, 300, 500, 800 };

    if( lineas >= 1 && lineas <= 4 ) {
        juego->puntaje =
            (uint16_t)( juego->puntaje + bonusPorLineas[lineas] );
    }
}
