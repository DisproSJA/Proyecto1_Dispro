/******************************************************************************
 *   Archivo:     driver_avr.c
 *   Proyecto:    Tetris Multi-Plataforma - Hito Final
 *   Materia:     Diseno de Sistemas con Procesador (DISPRO)
 *
 *   Descripcion: Implementacion del driver para ATmega328P stand-alone.
 *                Este archivo implementa todas las funciones declaradas en
 *                hal_display.h usando los registros del micro (DDR, PORT,
 *                PIN, TCCRx) para controlar:
 *                  - 2 matrices LED 8x8 via 4 registros 74HC595 en cascada
 *                  - 4 botones fisicos con antirebote (debounce)
 *                  - Timer0 para base de tiempo en milisegundos
 *
 *                Se compila solo cuando PLATFORM_AVR esta definido.
 *
 *   Compilar:
 *       avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os
 *               -o tetris.elf main.c tetris_logic.c driver_avr.c
 *       avr-objcopy -O ihex tetris.elf tetris.hex
 *
 *   Integrantes:
 *      - Sofia Vega
 *      - Juan Sanchez
 *      - Andres Trujillo
 *
 *   Fecha:       Abril 2026
 ******************************************************************************/

#include "hal_display.h"

#ifdef PLATFORM_AVR

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

/******************************************************************************
 * DEFINICION DE PINES DEL HARDWARE
 *
 * Botones conectados al puerto D (entrada, sin pull-up interno):
 *   PD2 = Rotar,  PD3 = Izquierda,  PD4 = Bajar,  PD5 = Derecha
 *
 * Registros 74HC595 conectados a los puertos D y B:
 *   PD6 = DATA (serial in),  PD7 = RESET de los 595
 *   PB0 = CLOCK,  PB1 = LATCH,  PB2 = OE (output enable)
 ******************************************************************************/

#define BTN_ROT_BIT    PD2
#define BTN_IZQ_BIT    PD3
#define BTN_BAJ_BIT    PD4
#define BTN_DER_BIT    PD5

#define DATA_BIT       PD6
#define RESET595_BIT   PD7

#define CLOCK_BIT      PB0
#define LATCH_BIT      PB1
#define OE_BIT         PB2

/******************************************************************************
 * CONSTANTES DEL DRIVER
 ******************************************************************************/

#define DEBOUNCE_MS      40U    /* Tiempo de antirebote para botones    */
#define RETARDO_FILA_US  200U   /* Tiempo que cada fila del LED queda encendida */
#define REPETIR_BAJAR_MS 180U   /* Intervalo de auto-repeticion al mantener
                                   presionado el boton de bajar            */

/******************************************************************************
 * VARIABLES GLOBALES DEL DRIVER
 ******************************************************************************/

/* Contador de milisegundos incrementado por la ISR del Timer0 */
static volatile uint32_t g_millis = 0;

/* Estado del antirebote de los 4 botones */
static uint8_t  g_botonesEstable   = 0;
static uint8_t  g_botonesAnterior  = 0;
static uint8_t  g_ultimaLectura    = 0;
static uint32_t g_ultimoCambioMs   = 0;

/* Para repetir la accion de bajar mientras se mantiene el boton */
static uint32_t g_ultimoBajarMs    = 0;

/* Para detectar el combo de reiniciar (Rotar + Bajar) */
static uint8_t  g_comboReiniciar   = 0;

/******************************************************************************
 * PROTOTIPOS DE FUNCIONES INTERNAS (STATIC)
 ******************************************************************************/

static void     inicializarTimer0( void );
static uint32_t obtenerMillis( void );
static void     retardoMicrosegundos( uint16_t tiempoUs );
static uint8_t  leerBotonesCrudo( void );
static void     actualizarDebounce( uint32_t ahora );

static void shiftOutRegistro( uint8_t valor );

static void enviarDatos( uint8_t col1,
                         uint8_t fil1,
                         uint8_t col2,
                         uint8_t fil2 );

static void obtenerBytesDeFila(
    uint8_t filaFisica,
    const uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO],
    uint8_t *matriz1,
    uint8_t *matriz2 );

static void apagarMatrices( void );

/******************************************************************************
 * ISR DEL TIMER0 - Cuenta milisegundos
 ******************************************************************************/

ISR( TIMER0_COMPA_vect )
{
    g_millis++;
}

/******************************************************************************
 * IMPLEMENTACION DE FUNCIONES HAL (PUBLICAS)
 ******************************************************************************/

/*FN****************************************************************************
*
*   int hal_inicializarHardware( void )
*
*   Que hace:  Configura todos los perifericos del ATmega328P para el Tetris:
*              - Pines de datos y control de los 74HC595 como salida
*              - Pines de los 4 botones como entrada (sin pull-up)
*              - Timer0 en modo CTC para generar una interrupcion cada 1 ms
*              - Habilita interrupciones globales
*
*   Retorna:   0 siempre (en AVR no hay error de consola)
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Juan Andrés Sanchez
*******************************************************************************/
int hal_inicializarHardware( void )
{
    cli();

    /* Pines de los 74HC595 como salida */
    DDRD |= ( 1U << DATA_BIT ) | ( 1U << RESET595_BIT );
    DDRB |= ( 1U << CLOCK_BIT ) | ( 1U << LATCH_BIT ) | ( 1U << OE_BIT );

    /* Pines de botones como entrada, sin pull-up */
    DDRD &= ~( ( 1U << BTN_ROT_BIT ) |
               ( 1U << BTN_IZQ_BIT ) |
               ( 1U << BTN_BAJ_BIT ) |
               ( 1U << BTN_DER_BIT ) );

    PORTD &= ~( ( 1U << BTN_ROT_BIT ) |
                ( 1U << BTN_IZQ_BIT ) |
                ( 1U << BTN_BAJ_BIT ) |
                ( 1U << BTN_DER_BIT ) );

    /* Estado inicial de los pines de control */
    PORTD &= ~( 1U << DATA_BIT );
    PORTB &= ~( 1U << CLOCK_BIT );
    PORTB &= ~( 1U << LATCH_BIT );

    /* OE en alto (display apagado temporalmente) */
    PORTB |= ( 1U << OE_BIT );

    /* RESET de los 595 en alto (operacion normal) */
    PORTD |= ( 1U << RESET595_BIT );

    /* Arrancar Timer0 para contar milisegundos */
    inicializarTimer0();

    /* Apagar todas las matrices y habilitar la salida */
    apagarMatrices();
    PORTB &= ~( 1U << OE_BIT );

    sei();

    return 0;
}

/*FN****************************************************************************
*
*   void hal_finalizarHardware( void )
*
*   Que hace:  Apaga todas las matrices LED enviando ceros a los 74HC595.
*              En un micro esta funcion normalmente no se llama porque el
*              programa corre en un bucle infinito.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Juan Andrés Sanchez
*******************************************************************************/
void
hal_finalizarHardware( void )
{
    apagarMatrices();
}

/*FN****************************************************************************
*
*   int hal_leerEntrada( void )
*
*   Que hace:  Lee los 4 botones fisicos, aplica antirebote de 40 ms y
*              detecta flancos de subida (momento en que se presiona).
*              Tambien soporta:
*              - Repeticion automatica al mantener el boton de bajar
*              - Combo Rotar + Bajar para reiniciar despues de game over
*
*   Retorna:   Uno de los codigos ENTRADA_xxx, o ENTRADA_NINGUNA si no
*              se presiono nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Juan Andrés Sanchez
*******************************************************************************/
int
hal_leerEntrada( void )
{
    uint32_t ahora;
    uint8_t  flancoSubida;

    ahora = obtenerMillis();
    actualizarDebounce( ahora );

    /* Detectar flancos de subida (boton recien presionado) */
    flancoSubida = g_botonesEstable & ~g_botonesAnterior;
    g_botonesAnterior = g_botonesEstable;

    /* Combo de reinicio: Rotar + Bajar presionados al mismo tiempo */
    if( ( g_botonesEstable & ( 1U << 2 ) ) &&
        ( g_botonesEstable & ( 1U << 3 ) ) ) {
        if( !g_comboReiniciar ) {
            g_comboReiniciar = 1;
            return ENTRADA_REINICIAR;
        }
        return ENTRADA_NINGUNA;
    }
    g_comboReiniciar = 0;

    /* Flancos individuales de cada boton */
    if( flancoSubida & ( 1U << 0 ) ) {
        return ENTRADA_IZQUIERDA;
    }

    if( flancoSubida & ( 1U << 1 ) ) {
        return ENTRADA_DERECHA;
    }

    if( flancoSubida & ( 1U << 2 ) ) {
        return ENTRADA_ROTAR;
    }

    if( flancoSubida & ( 1U << 3 ) ) {
        g_ultimoBajarMs = ahora;
        return ENTRADA_BAJAR;
    }

    /* Repeticion al mantener presionado el boton de bajar */
    if( g_botonesEstable & ( 1U << 3 ) ) {
        if( ( ahora - g_ultimoBajarMs ) >= REPETIR_BAJAR_MS ) {
            g_ultimoBajarMs = ahora;
            return ENTRADA_BAJAR;
        }
    }

    return ENTRADA_NINGUNA;
}

/*FN****************************************************************************
*
*   long hal_obtenerTiempoMs( void )
*
*   Que hace:  Devuelve el contador de milisegundos del Timer0. Desactiva
*              las interrupciones brevemente para leer el valor de 32 bits
*              de forma segura (lectura atomica).
*
*   Retorna:   El tiempo actual en milisegundos desde el arranque
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Juan Andrés Sanchez
*******************************************************************************/
long
hal_obtenerTiempoMs( void )
{
    uint32_t copia;
    uint8_t sreg = SREG;

    cli();
    copia = g_millis;
    SREG = sreg;

    return (long)copia;
}

/*FN****************************************************************************
*
*   void hal_retardo( int milisegundos )
*
*   Que hace:  En AVR esta funcion no hace nada porque el ciclo de
*              multiplexacion de las matrices LED (hal_dibujarJuego)
*              ya proporciona el retardo natural de ~1.6 ms por ciclo.
*              Agregar un retardo adicional causaria parpadeo visible
*              en las matrices.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Juan Andrés Sanchez
*******************************************************************************/
void
hal_retardo( int milisegundos )
{
    (void)milisegundos;
}

/*FN****************************************************************************
*
*   void hal_dibujarJuego( juego, framebuffer )
*
*   Que hace:  Refresca las dos matrices LED de 8x8 haciendo un ciclo
*              completo de multiplexacion. Recorre las 8 filas fisicas,
*              convierte el framebuffer a bytes de columnas para cada
*              matriz y los envia a los 4 registros 74HC595 en cascada.
*              El parametro 'juego' se ignora porque las matrices solo
*              muestran el framebuffer (no hay panel lateral como en PC).
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Juan Andrés Sanchez
*******************************************************************************/
void
hal_dibujarJuego( const EstadoJuego *juego,
                  const uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] )
{
    uint8_t filaFisica;

    (void)juego;

    for( filaFisica = 0; filaFisica < 8; filaFisica++ ) {
        uint8_t columnasMatriz1 = 0x00;
        uint8_t columnasMatriz2 = 0x00;
        uint8_t fil1;
        uint8_t fil2;

        obtenerBytesDeFila( filaFisica,
                            framebuffer,
                            &columnasMatriz1,
                            &columnasMatriz2 );

        /* Activar solo la fila actual (activo bajo) */
        fil1 = (uint8_t) ~( 1U << filaFisica );
        fil2 = (uint8_t) ~( 1U << filaFisica );

        enviarDatos( columnasMatriz1, fil1, columnasMatriz2, fil2 );
        retardoMicrosegundos( RETARDO_FILA_US );
    }
}

/******************************************************************************
 * IMPLEMENTACION DE FUNCIONES INTERNAS (STATIC)
 ******************************************************************************/

/*FN****************************************************************************
*
*   static void inicializarTimer0( void )
*
*   Que hace:  Configura el Timer0 del ATmega328P en modo CTC para
*              generar una interrupcion cada 1 milisegundo.
*              Con F_CPU = 16 MHz y prescaler de 64:
*              16 MHz / 64 = 250 kHz -> 250 cuentas = 1 ms -> OCR0A = 249
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Juan Andrés Sanchez
*******************************************************************************/
static void
inicializarTimer0( void )
{
    TCCR0A = 0;
    TCCR0B = 0;
    TCNT0  = 0;

    /* Modo CTC (Clear Timer on Compare Match) */
    TCCR0A |= ( 1U << WGM01 );

    /* Comparar en 249 para obtener 1 ms */
    OCR0A = 249U;

    /* Habilitar interrupcion por comparacion */
    TIMSK0 |= ( 1U << OCIE0A );

    /* Prescaler de 64: CS01 + CS00 */
    TCCR0B |= ( 1U << CS01 ) | ( 1U << CS00 );
}

/*FN****************************************************************************
*
*   static uint32_t obtenerMillis( void )
*
*   Que hace:  Lee el contador de milisegundos de forma segura desactivando
*              interrupciones brevemente. Es la version interna que usan
*              las demas funciones del driver.
*
*   Retorna:   El valor actual del contador de milisegundos
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Juan Andrés Sanchez
*******************************************************************************/
static uint32_t
obtenerMillis( void )
{
    uint32_t copia;
    uint8_t sreg = SREG;

    cli();
    copia = g_millis;
    SREG = sreg;

    return copia;
}

/*FN****************************************************************************
*
*   static void retardoMicrosegundos( uint16_t tiempoUs )
*
*   Que hace:  Espera la cantidad indicada de microsegundos. Se usa para
*              mantener cada fila de la matriz LED encendida el tiempo
*              suficiente para que sea visible (persistencia de vision).
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Juan Andrés Sanchez
*******************************************************************************/
static void
retardoMicrosegundos( uint16_t tiempoUs )
{
    while( tiempoUs > 0U ) {
        _delay_us( 1 );
        tiempoUs--;
    }
}

/*FN****************************************************************************
*
*   static uint8_t leerBotonesCrudo( void )
*
*   Que hace:  Lee el estado actual de los 4 botones directamente del
*              registro PIND (sin antirebote). Cada boton se mapea a
*              un bit del byte de retorno:
*              Bit 0 = Izquierda,  Bit 1 = Derecha,
*              Bit 2 = Rotar,      Bit 3 = Bajar
*
*   Retorna:   Byte con el estado crudo de los 4 botones
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Juan Andrés Sanchez
*******************************************************************************/
static uint8_t
leerBotonesCrudo( void )
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

/*FN****************************************************************************
*
*   static void actualizarDebounce( uint32_t ahora )
*
*   Que hace:  Lee los botones y aplica el filtro de antirebote. Si la
*              lectura cambia, reinicia el temporizador. Solo cuando la
*              lectura se mantiene estable por 40 ms se acepta como
*              el estado real de los botones.
*
*   Retorna:   Nada (actualiza g_botonesEstable)
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Juan Andrés Sanchez
*******************************************************************************/
static void
actualizarDebounce( uint32_t ahora )
{
    uint8_t lecturaActual = leerBotonesCrudo();

    if( lecturaActual != g_ultimaLectura ) {
        g_ultimaLectura  = lecturaActual;
        g_ultimoCambioMs = ahora;
    }

    if( ( ahora - g_ultimoCambioMs ) >= DEBOUNCE_MS ) {
        g_botonesEstable = g_ultimaLectura;
    }
}

/*FN****************************************************************************
*
*   static void shiftOutRegistro( uint8_t valor )
*
*   Que hace:  Envia un byte al registro 74HC595 bit a bit por el pin
*              DATA, generando un pulso de CLOCK por cada bit. Los bits
*              se envian del mas significativo al menos significativo.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Juan Andrés Sanchez
*******************************************************************************/
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

/*FN****************************************************************************
*
*   static void enviarDatos( col1, fil1, col2, fil2 )
*
*   Que hace:  Envia 4 bytes a la cadena de 4 registros 74HC595 en cascada.
*              Desactiva la salida (OE alto) mientras carga los datos,
*              luego da un pulso de LATCH para transferir los datos a las
*              salidas y reactiva la salida (OE bajo).
*
*              Orden de la cadena: col2 -> fil2 -> col1 -> fil1
*              (el ultimo byte enviado llega al primer 595)
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Juan Andrés Sanchez
*******************************************************************************/
static void
enviarDatos( uint8_t col1,
             uint8_t fil1,
             uint8_t col2,
             uint8_t fil2 )
{
    PORTB |= ( 1U << OE_BIT );
    PORTB &= ~( 1U << LATCH_BIT );

    shiftOutRegistro( col2 );
    shiftOutRegistro( fil2 );
    shiftOutRegistro( col1 );
    shiftOutRegistro( fil1 );

    PORTB |= ( 1U << LATCH_BIT );
    PORTB &= ~( 1U << LATCH_BIT );
    PORTB &= ~( 1U << OE_BIT );
}

/*FN****************************************************************************
*
*   static void obtenerBytesDeFila( filaFisica, framebuffer, *m1, *m2 )
*
*   Que hace:  Convierte una fila del framebuffer (formato celda por celda)
*              a dos bytes de columnas (un bit por columna) para enviar a
*              los 74HC595. La matriz 1 muestra las filas 0-7 del tablero
*              y la matriz 2 las filas 8-15.
*
*   Retorna:   Nada (escribe en *matriz1 y *matriz2)
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Juan Andrés Sanchez
*******************************************************************************/
static void
obtenerBytesDeFila( uint8_t filaFisica,
                    const uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO],
                    uint8_t *matriz1,
                    uint8_t *matriz2 )
{
    uint8_t col;

    *matriz1 = 0x00;
    *matriz2 = 0x00;

    for( col = 0; col < ANCHO_TABLERO; col++ ) {
        if( framebuffer[filaFisica][col] ) {
            *matriz1 |= (uint8_t)( 1U << ( 7 - col ) );
        }
    }

    for( col = 0; col < ANCHO_TABLERO; col++ ) {
        if( framebuffer[filaFisica + 8U][col] ) {
            *matriz2 |= (uint8_t)( 1U << ( 7 - col ) );
        }
    }
}

/*FN****************************************************************************
*
*   static void apagarMatrices( void )
*
*   Que hace:  Envia ceros a las columnas y unos a las filas de ambas
*              matrices para que todos los LEDs queden apagados.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Juan Andrés Sanchez
*******************************************************************************/
static void
apagarMatrices( void )
{
    enviarDatos( 0x00, 0xFF, 0x00, 0xFF );
}

#endif /* PLATFORM_AVR */
