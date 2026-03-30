#include "app_game.h"
#include "../tetris/tetris_logic.h"
#include "../ui/ui_display.h"
#include <Arduino.h>

static bool btnIzqAnterior = false;
static bool btnDerAnterior = false;
static bool btnRotAnterior = false;
static bool btnBajAnterior = false;

static unsigned long ultimoDescensoMs = 0;
static const unsigned long INTERVALO_CAIDA_MS = 3500UL;

static unsigned long ultimoSoftDropMs = 0;
static const unsigned long INTERVALO_SOFT_DROP_MS = 250UL;

static void procesarEntradas( EstadoJuego *juego, const BotonesEstado *botones );
static void procesarCaidaAutomatica( EstadoJuego *juego );
static void actualizarFramebuffer( const EstadoJuego *juego,
                                   uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] );
static void intentarReiniciarJuego( EstadoJuego *juego, const BotonesEstado *botones );

void
app_game_init( EstadoJuego *juego,
               DebounceBotones *debounce,
               BotonesEstado *botones,
               uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] )
{
    debounce->estableActual = 0;
    debounce->ultimaLectura = 0;
    debounce->ultimoCambioMs = 0;

    botones->izq = false;
    botones->der = false;
    botones->rot = false;
    botones->baj = false;

    btnIzqAnterior = false;
    btnDerAnterior = false;
    btnRotAnterior = false;
    btnBajAnterior = false;

    tetris_inicializarJuego( juego );
    actualizarFramebuffer( juego, framebuffer );

    ultimoDescensoMs = millis( );
    ultimoSoftDropMs = millis( );
}

void
app_game_update( EstadoJuego *juego,
                 DebounceBotones *debounce,
                 BotonesEstado *botones,
                 uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] )
{
    driver_avr_actualizarBotones( debounce, botones, millis( ) );

    procesarEntradas( juego, botones );
    procesarCaidaAutomatica( juego );
    actualizarFramebuffer( juego, framebuffer );
}

static void
procesarEntradas( EstadoJuego *juego, const BotonesEstado *botones )
{
    unsigned long tiempoActual = millis( );

    if( juego->gameOver ) {
        intentarReiniciarJuego( juego, botones );

        btnIzqAnterior = botones->izq;
        btnDerAnterior = botones->der;
        btnRotAnterior = botones->rot;
        btnBajAnterior = botones->baj;
        return;
    }

    if( botones->izq && !btnIzqAnterior ) {
        tetris_intentarMover( juego, -1, 0 );
    }

    if( botones->der && !btnDerAnterior ) {
        tetris_intentarMover( juego, 1, 0 );
    }

    if( botones->rot && !btnRotAnterior ) {
        tetris_intentarRotar( juego );
    }

    if( botones->baj && !btnBajAnterior ) {
        tetris_bajarOFijar( juego );
        ultimoDescensoMs = tiempoActual;
        ultimoSoftDropMs = tiempoActual;
    }

    if( botones->baj && btnBajAnterior ) {
        if( ( tiempoActual - ultimoSoftDropMs ) >= INTERVALO_SOFT_DROP_MS ) {
            tetris_bajarOFijar( juego );
            ultimoSoftDropMs = tiempoActual;
            ultimoDescensoMs = tiempoActual;
        }
    }

    btnIzqAnterior = botones->izq;
    btnDerAnterior = botones->der;
    btnRotAnterior = botones->rot;
    btnBajAnterior = botones->baj;
}

static void
procesarCaidaAutomatica( EstadoJuego *juego )
{
    unsigned long tiempoActual = millis( );

    if( juego->gameOver ) {
        return;
    }

    if( ( tiempoActual - ultimoDescensoMs ) >= INTERVALO_CAIDA_MS ) {
        tetris_bajarOFijar( juego );
        ultimoDescensoMs = tiempoActual;
    }
}

static void
actualizarFramebuffer( const EstadoJuego *juego,
                       uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] )
{
    if( juego->gameOver ) {
        ui_dibujarPantallaGameOver( juego, framebuffer );
        return;
    }

    tetris_dibujarEstadoEnFramebuffer( juego, framebuffer );
}

static void
intentarReiniciarJuego( EstadoJuego *juego, const BotonesEstado *botones )
{
    if( botones->rot && botones->baj ) {
        tetris_inicializarJuego( juego );
        ultimoDescensoMs = millis( );
        ultimoSoftDropMs = millis( );
    }
}