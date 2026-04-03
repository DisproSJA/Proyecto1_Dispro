/******************************************************************************
 *   Archivo:     hal_display.h
 *   Proyecto:    Tetris Multi-Plataforma - Hito Final
 *   Materia:     Diseno de Sistemas con Procesador (DISPRO)
 *
 *   Descripcion: Capa de Abstraccion de Hardware (HAL). Define los prototipos
 *                de funciones de dibujo, entrada y tiempo que deben ser
 *                implementados por cada driver de plataforma (driver_pc.c
 *                para PC o driver_avr.c para microcontrolador).
 *
 *                Para cambiar de plataforma, modificar la directiva de
 *                compilacion al inicio de este archivo.
 *
 *   Integrantes:
 *      - Sofia Vega
 *      - Juan Sanchez
 *      - Andres Trujillo
 *
 *   Fecha:       Abril 2026
 ******************************************************************************/

#ifndef HAL_DISPLAY_H
#define HAL_DISPLAY_H

/******************************************************************************
 * SELECCION DE PLATAFORMA
 *
 * Descomentar UNA de las dos lineas siguientes segun la plataforma destino.
 * Solo una debe estar activa a la vez.
 ******************************************************************************/
//#define PLATFORM_PC
#define PLATFORM_AVR

/******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include "tetris_logic.h"

/******************************************************************************
 * CODIGOS DE ENTRADA DEL USUARIO
 *
 * Estos codigos son devueltos por hal_leerEntrada() sin importar si la
 * entrada viene de un teclado de PC o de botones fisicos en un micro.
 ******************************************************************************/

#define ENTRADA_NINGUNA    0
#define ENTRADA_IZQUIERDA  1
#define ENTRADA_DERECHA    2
#define ENTRADA_ROTAR      3
#define ENTRADA_BAJAR      4
#define ENTRADA_SALIR      5
#define ENTRADA_REINICIAR  6

/******************************************************************************
 * PROTOTIPOS DE FUNCIONES HAL
 *
 * Cada plataforma (driver_pc.c o driver_avr.c) debe implementar todas
 * estas funciones para que el juego sea portable.
 ******************************************************************************/

/*  Inicializa el hardware de la plataforma (consola o perifericos).
 *  Retorna 0 si todo salio bien, -1 si hubo error.                          */
int hal_inicializarHardware( void );

/*  Libera recursos y restaura el estado original del hardware.               */
void hal_finalizarHardware( void );

/*  Lee la entrada del usuario y devuelve uno de los codigos ENTRADA_xxx.     */
int hal_leerEntrada( void );

/*  Devuelve el tiempo actual en milisegundos desde el inicio del sistema.    */
long hal_obtenerTiempoMs( void );

/*  Espera la cantidad indicada de milisegundos.                              */
void hal_retardo( int milisegundos );

/*  Dibuja el estado completo del juego en la pantalla o display.
 *  Recibe el estado del juego (para informacion del panel) y el
 *  framebuffer ya construido con el tablero y la pieza activa.               */
void hal_dibujarJuego( const EstadoJuego *juego,
                       const uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] );

#endif /* HAL_DISPLAY_H */
