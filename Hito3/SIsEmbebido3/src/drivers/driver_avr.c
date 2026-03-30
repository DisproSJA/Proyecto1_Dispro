#include "driver_avr.h"
#include <Arduino.h>

#define DEBOUNCE_MS     40U
#define RETARDO_FILA_US 200U

static void shiftOutRegistro( uint8_t valor );
static void enviarDatos( uint8_t col1,
                         uint8_t fil1,
                         uint8_t col2,
                         uint8_t fil2 );
static void obtenerBytesDeFila( uint8_t filaFisica,
                                const uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO],
                                uint8_t *matriz1,
                                uint8_t *matriz2 );

void
driver_avr_inicializarHardware( void )
{
    DDRD |= ( 1U << DATA_BIT ) | ( 1U << RESET595_BIT );
    DDRB |= ( 1U << CLOCK_BIT ) | ( 1U << LATCH_BIT ) | ( 1U << OE_BIT );

    DDRD &= ~( ( 1U << BTN_ROT_BIT ) |
               ( 1U << BTN_IZQ_BIT ) |
               ( 1U << BTN_BAJ_BIT ) |
               ( 1U << BTN_DER_BIT ) );

    /* Igual que en tu Hito 2: sin pull-up internos */
    PORTD &= ~( ( 1U << BTN_ROT_BIT ) |
                ( 1U << BTN_IZQ_BIT ) |
                ( 1U << BTN_BAJ_BIT ) |
                ( 1U << BTN_DER_BIT ) );

    PORTD &= ~( 1U << DATA_BIT );
    PORTB &= ~( 1U << CLOCK_BIT );
    PORTB &= ~( 1U << LATCH_BIT );

    /* OE activo en alto = apagado temporal */
    PORTB |= ( 1U << OE_BIT );

    /* Reset inactivo */
    PORTD |= ( 1U << RESET595_BIT );

    driver_avr_apagarTodo( );

    /* Habilitar salidas */
    PORTB &= ~( 1U << OE_BIT );
}

void
driver_avr_apagarTodo( void )
{
    /* columnas LOW = apagadas, filas HIGH = apagadas */
    enviarDatos( 0x00, 0xFF, 0x00, 0xFF );
}

void
driver_avr_refrescarDisplay(
    const uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] )
{
    uint8_t filaFisica;

    for( filaFisica = 0; filaFisica < 8; filaFisica++ ) {
        uint8_t columnasMatriz1 = 0x00;
        uint8_t columnasMatriz2 = 0x00;
        uint8_t fil1;
        uint8_t fil2;

        obtenerBytesDeFila( filaFisica,
                            framebuffer,
                            &columnasMatriz1,
                            &columnasMatriz2 );

        /* filas activas en bajo */
        fil1 = (uint8_t) ~( 1U << filaFisica );
        fil2 = (uint8_t) ~( 1U << filaFisica );

        enviarDatos( columnasMatriz1, fil1, columnasMatriz2, fil2 );
        delayMicroseconds( RETARDO_FILA_US );
    }
}

uint8_t
driver_avr_leerBotonesCrudo( void )
{
    uint8_t lectura = 0;

    if( PIND & ( 1U << BTN_IZQ_BIT ) ) {
        lectura |= ( 1U << 0 );
    }

    if( PIND & ( 1U << BTN_DER_BIT ) ) {
        lectura |= ( 1U << 1 );
    }

    if( PIND & ( 1U << BTN_ROT_BIT ) ) {
        lectura |= ( 1U << 2 );
    }

    if( PIND & ( 1U << BTN_BAJ_BIT ) ) {
        lectura |= ( 1U << 3 );
    }

    return lectura;
}

void
driver_avr_actualizarBotones( DebounceBotones *debounce,
                              BotonesEstado *botones,
                              unsigned long tiempoActualMs )
{
    uint8_t lecturaActual = driver_avr_leerBotonesCrudo( );

    if( lecturaActual != debounce->ultimaLectura ) {
        debounce->ultimaLectura = lecturaActual;
        debounce->ultimoCambioMs = tiempoActualMs;
    }

    if( ( tiempoActualMs - debounce->ultimoCambioMs ) >= DEBOUNCE_MS ) {
        debounce->estableActual = debounce->ultimaLectura;
    }

    botones->izq = ( debounce->estableActual & ( 1U << 0 ) ) ? true : false;
    botones->der = ( debounce->estableActual & ( 1U << 1 ) ) ? true : false;
    botones->rot = ( debounce->estableActual & ( 1U << 2 ) ) ? true : false;
    botones->baj = ( debounce->estableActual & ( 1U << 3 ) ) ? true : false;
}

static void
shiftOutRegistro( uint8_t valor )
{
    uint8_t i;

    for( i = 0; i < 8; i++ ) {
        uint8_t bitActual = ( valor & ( 1U << ( 7 - i ) ) ) ? 1U : 0U;

        if( bitActual ) {
            PORTD |= ( 1U << DATA_BIT );
        } else {
            PORTD &= ~( 1U << DATA_BIT );
        }

        PORTB |= ( 1U << CLOCK_BIT );
        PORTB &= ~( 1U << CLOCK_BIT );
    }
}

static void
enviarDatos( uint8_t col1,
             uint8_t fil1,
             uint8_t col2,
             uint8_t fil2 )
{
    /* deshabilitar salidas mientras se actualiza */
    PORTB |= ( 1U << OE_BIT );
    PORTB &= ~( 1U << LATCH_BIT );

    /* mismo orden que en tu Hito 2 */
    shiftOutRegistro( col2 );
    shiftOutRegistro( fil2 );
    shiftOutRegistro( col1 );
    shiftOutRegistro( fil1 );

    PORTB |= ( 1U << LATCH_BIT );
    PORTB &= ~( 1U << LATCH_BIT );
    PORTB &= ~( 1U << OE_BIT );
}

static void
obtenerBytesDeFila( uint8_t filaFisica,
                    const uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO],
                    uint8_t *matriz1,
                    uint8_t *matriz2 )
{
    uint8_t col;

    *matriz1 = 0x00;
    *matriz2 = 0x00;

    /* matriz superior: filas 0..7 */
    for( col = 0; col < ANCHO_TABLERO; col++ ) {
        if( framebuffer[filaFisica][col] ) {
            *matriz1 |= (uint8_t)( 1U << ( 7 - col ) );
        }
    }

    /* matriz inferior: filas 8..15 */
    for( col = 0; col < ANCHO_TABLERO; col++ ) {
        if( framebuffer[filaFisica + 8U][col] ) {
            *matriz2 |= (uint8_t)( 1U << ( 7 - col ) );
        }
    }
}