#include "src/tetris/tetris_logic.h"
#include "src/drivers/driver_avr.h"
#include "src/app/app_game.h"

/* -------------------------- Estado global -------------------------- */
static EstadoJuego juego;
static DebounceBotones debounce;
static BotonesEstado botones;
static uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO];

void
setup( void )
{
    driver_avr_inicializarHardware( );
    app_game_init( &juego, &debounce, &botones, framebuffer );
}

void
loop( void )
{
    app_game_update( &juego, &debounce, &botones, framebuffer );
    driver_avr_refrescarDisplay( framebuffer );
}