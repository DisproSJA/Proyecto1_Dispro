#include "src/tetris/tetris_logic.h"
#include "src/drivers/driver_avr.h"

/* -------------------------- Estado global -------------------------- */
static EstadoJuego juego;
static DebounceBotones debounce;
static BotonesEstado botones;

static uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO];

/* Para detectar flanco de pulsación */
static bool btnIzqAnterior = false;
static bool btnDerAnterior = false;
static bool btnRotAnterior = false;
static bool btnBajAnterior = false;

/* Temporización de caída automática */

static unsigned long ultimoDescensoMs = 0;
static const unsigned long INTERVALO_CAIDA_MS = 3000UL;

/* ---------------------- Prototipos privados ------------------------ */
static void procesarEntradas( void );
static void actualizarFramebuffer( void );
static void procesarCaidaAutomatica( void );

void
setup( void )
{
    driver_avr_inicializarHardware( );

    debounce.estableActual = 0;
    debounce.ultimaLectura = 0;
    debounce.ultimoCambioMs = 0;

    botones.izq = false;
    botones.der = false;
    botones.rot = false;
    botones.baj = false;

    tetris_inicializarJuego( &juego );
    actualizarFramebuffer( );

    ultimoDescensoMs = millis( );
}

void
loop( void )
{
    driver_avr_actualizarBotones( &debounce, &botones, millis( ) );

    procesarEntradas( );
    procesarCaidaAutomatica( );
    actualizarFramebuffer( );

    driver_avr_refrescarDisplay( framebuffer );
}

static void
procesarEntradas( void )
{
    if( juego.gameOver ) {
        btnIzqAnterior = botones.izq;
        btnDerAnterior = botones.der;
        btnRotAnterior = botones.rot;
        btnBajAnterior = botones.baj;
        return;
    }

    if( botones.izq && !btnIzqAnterior ) {
        tetris_intentarMover( &juego, -1, 0 );
        actualizarFramebuffer( );
    }

    if( botones.der && !btnDerAnterior ) {
        tetris_intentarMover( &juego, 1, 0 );
        actualizarFramebuffer( );
    }

    if( botones.rot && !btnRotAnterior ) {
        tetris_intentarRotar( &juego );
        actualizarFramebuffer( );
    }

    if( botones.baj && !btnBajAnterior ) {
        tetris_bajarOFijar( &juego );
        actualizarFramebuffer( );
        ultimoDescensoMs = millis( );
    }

    btnIzqAnterior = botones.izq;
    btnDerAnterior = botones.der;
    btnRotAnterior = botones.rot;
    btnBajAnterior = botones.baj;
}

static void
procesarCaidaAutomatica( void )
{
    unsigned long tiempoActual = millis( );

    if( juego.gameOver ) {
        return;
    }

    if( ( tiempoActual - ultimoDescensoMs ) >= INTERVALO_CAIDA_MS ) {
        tetris_bajarOFijar( &juego );
        ultimoDescensoMs = tiempoActual;
    }
}

static void
actualizarFramebuffer( void )
{
    tetris_dibujarEstadoEnFramebuffer( &juego, framebuffer );
}