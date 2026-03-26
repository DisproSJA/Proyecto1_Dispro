/* Regla  1         2         3         4         5         6         7      */

/********************************** Encabezados *******************************/

/* -------------------- Inclusión de Librerías Estándar --------------------- */

#include <Arduino.h>

/* ---------------------- Inclusión de Librerías Propias -------------------- */



/**************************** Constantes Simbólicas ***************************/

/* ------------------- Tiempos de Refresco de la Pantalla ------------------- */
const uint16_t RETARDO_FILA_US = 200; /* refresco por fila [us] */

/* -------------------------------- Macros ---------------------------------- */
#define MI_LSBFIRST 0
#define MI_MSBFIRST 1

#define ACTIVAR_BIT( REG, BIT )   ( ( REG ) |=  ( 1U << ( BIT ) ) )
#define LIMPIAR_BIT( REG, BIT )   ( ( REG ) &= ~( 1U << ( BIT ) ) )

/* ------------------ Mapeo AVR de pines usados en Arduino UNO -------------- */
/*
    D6  -> PD6 -> SER   -> dataPin
    D7  -> PD7 -> MR    -> resetPin
    D8  -> PB0 -> SRCLK -> clockPin
    D9  -> PB1 -> RCLK  -> latchPin
    D10 -> PB2 -> OE    -> OEPin
*/

#define DATA_BIT   PD6
#define RESET_BIT  PD7
#define CLOCK_BIT  PB0
#define LATCH_BIT  PB1
#define OE_BIT     PB2



/*************************** Prototipos de Funciones **************************/

/* ---------------- Funciones definidas después de loop() ------------------- */
void inicializarHardware( void );
void miShiftOutRegistro( uint8_t ordenBits, uint8_t valor );
void enviarDatos( byte col1, byte fil1, byte col2, byte fil2 );
void refrescarDosFiguras( const byte figura1[8], const byte figura2[8] );
void apagarTodo( void );
void imprimirDosFigurasASCII( const byte figura1[8], const byte figura2[8] );
void imprimirSeparadorSerial( void );



/************** Definición e Inicialización de los Datos Globales *************/

/* ------------------ Datos globales del programa principal ----------------- */
/* Cada byte representa una fila.
   Bit en 1 = LED encendido en esa columna. */


/* CORAZON */
const byte CORAZON[8] = {
    0b00000000,
    0b01100110,
    0b11111111,
    0b11111111,
    0b11111111,
    0b01111110,
    0b00111100,
    0b00011000
}; 

/* SONRISA */
const byte SONRISA[8] = {
    0b00111100,
    0b01000010,
    0b10100101,
    0b10000001,
    0b10100101,
    0b10011001,
    0b01000010,
    0b00111100
}; 


/******************************* Función Principal ****************************/

/*FN***************************************************************************
*
*   void setup( void );
*
*   Propósito: Inicializar el hardware, habilitar el puerto serial y mostrar
*              en consola la misma figura estática que se visualiza en la
*              matriz LED para cumplir con el Hito 1.
*
*   Plan:
*           Parte 1: Inicializar los pines del 74HC595 usando registros AVR
*           Parte 2: Iniciar la comunicación serial
*           Parte 3: Imprimir la figura en formato ASCII en consola
*
*   Retorna: Nada
*
*   Registro de Revisiones:
*
*   FECHA      RESPONSABLE              COMENTARIO
*   -----------------------------------------------------------------------
*   MAR 20/26  Juan Andres Sanchez      Funciones básicas
*   MAR 22/26  Felipe Trujillo          Ajuste de estructura y comentarios
*   MAR 23/26  Sofia Vega               Documentación y validación
*
*******************************************************************************/

void
setup( void )
{
/* Parte 1: Inicializar los pines del 74HC595 usando registros AVR */
    inicializarHardware();

/* Parte 2: Iniciar la comunicación serial */
    Serial.begin(9600);
    delay(300);

/* Parte 3: Imprimir la figura en formato ASCII en consola */
    imprimirSeparadorSerial();
    Serial.println( "HITO 1 - Proyecto Dispro" );
    Serial.println( "Matriz 1: CORAZON | Matriz 2: SONRISA" );
    Serial.println( "Representacion ASCII de las Figuras:" );
    Serial.println();
    imprimirDosFigurasASCII( CORAZON, SONRISA );
    imprimirSeparadorSerial();

} /* setup */



/*FN***************************************************************************
*
*   void loop( void );
*
*   Propósito: Refrescar continuamente la imagen estática en ambas matrices
*              LED sin alternancia ni animación.
*
*   Plan:
*           Parte 1: Refrescar fila por fila la figura fija de ambas matrices
*
*   Retorna: Nada
*
*   Registro de Revisiones:
*
*   FECHA      RESPONSABLE              COMENTARIO
*   -----------------------------------------------------------------------
*   MAR 20/26  Juan Andres Sanchez      Funciones básicas iniciales
*   MAR 22/26  Felipe Trujillo          Ajuste de estructura y comentarios
*   MAR 23/26  Sofia Vega               Documentación y validación
*
*******************************************************************************/

void
loop( void )
{
/* Parte 1: Refrescar fila por fila la figura fija de ambas matrices */
    refrescarDosFiguras( CORAZON, SONRISA );
} /* loop */



/*************************** Definición de Funciones **************************/

/*FN***************************************************************************
*
*   void inicializarHardware( void );
*
*   Propósito: Configurar a nivel de registros AVR los pines conectados a los
*              74HC595 y establecer sus niveles lógicos iniciales.
*
*   Plan:
*           Parte 1: Configurar como salidas los bits usados en PORTB y PORTD
*           Parte 2: Establecer estados iniciales seguros
*           Parte 3: Habilitar operación normal de los registros
*
*   Retorna: Nada
*
*   Registro de Revisiones:
*
*   FECHA      RESPONSABLE              COMENTARIO
*   -----------------------------------------------------------------------
*   MAR 20/26  Juan Andres Sanchez      Funciones básicas
*   MAR 22/26  Felipe Trujillo          Ajuste de estructura y comentarios
*   MAR 23/26  Sofia Vega               Documentación y validación
*
*******************************************************************************/

void
inicializarHardware( void )
{
/* Parte 1: Configurar como salidas los bits usados en PORTB y PORTD */
    ACTIVAR_BIT( DDRD, DATA_BIT );
    ACTIVAR_BIT( DDRD, RESET_BIT );
    ACTIVAR_BIT( DDRB, CLOCK_BIT );
    ACTIVAR_BIT( DDRB, LATCH_BIT );
    ACTIVAR_BIT( DDRB, OE_BIT );

/* Parte 2: Establecer estados iniciales seguros */
    LIMPIAR_BIT( PORTD, DATA_BIT );
    LIMPIAR_BIT( PORTB, CLOCK_BIT );
    LIMPIAR_BIT( PORTB, LATCH_BIT );
    ACTIVAR_BIT( PORTB, OE_BIT );     /* salidas deshabilitadas al inicio */
    ACTIVAR_BIT( PORTD, RESET_BIT );  /* MR activo en alto: operación normal */

/* Parte 3: Habilitar operación normal de los registros */
    apagarTodo();
    LIMPIAR_BIT( PORTB, OE_BIT );     /* habilitar salidas */

} /* inicializarHardware */



/*FN***************************************************************************
*
*   void miShiftOutRegistro( uint8_t ordenBits, uint8_t valor );
*
*   Propósito: Enviar un byte en serie al 74HC595 manipulando directamente los
*              registros del AVR, sin usar shiftOut() ni digitalWrite().
*
*   Plan:
*           Parte 1: Evaluar cada bit según el orden seleccionado
*           Parte 2: Colocar el dato sobre la línea SER
*           Parte 3: Generar un pulso en SRCLK
*
*   Retorna: Nada
*
*   Registro de Revisiones:
*
*   FECHA      RESPONSABLE              COMENTARIO
*   -----------------------------------------------------------------------
*   MAR 20/26  Juan Andres Sanchez      Funciones básicas
*   MAR 22/26  Felipe Trujillo          Ajuste de estructura y comentarios
*   MAR 23/26  Sofia Vega               Documentación y validación
*
*******************************************************************************/

void
miShiftOutRegistro(
    uint8_t ordenBits, /* Ent: Orden de bits a transmitir */
    uint8_t valor )    /* Ent: Byte a enviar              */
{
/* Parte 1: Evaluar cada bit según el orden seleccionado */
    for( uint8_t i = 0; i < 8; i++ ) {

        uint8_t bitActual;

        if( ordenBits == MI_MSBFIRST )
            bitActual = ( valor & ( 1 << ( 7 - i ) ) ) ? 1 : 0;
        else
            bitActual = ( valor & ( 1 << i ) ) ? 1 : 0;

/* Parte 2: Colocar el dato sobre la línea SER */
        if( bitActual )
            ACTIVAR_BIT( PORTD, DATA_BIT );
        else
            LIMPIAR_BIT( PORTD, DATA_BIT );

/* Parte 3: Generar un pulso en SRCLK */
        ACTIVAR_BIT( PORTB, CLOCK_BIT );
        LIMPIAR_BIT( PORTB, CLOCK_BIT );
    }

} /* miShiftOutRegistro */



/*FN***************************************************************************
*
*   void enviarDatos( byte col1, byte fil1, byte col2, byte fil2 );
*
*   Propósito: Enviar los 4 bytes requeridos por los 4 registros de
*              desplazamiento en cascada para las dos matrices.
*
*
*   Plan:
*           Parte 1: Deshabilitar temporalmente salidas con OE
*           Parte 2: Desplazar los 4 bytes en el orden requerido
*           Parte 3: Aplicar latch y volver a habilitar salidas
*
*   Retorna: Nada
*
*   Registro de Revisiones:
*
*   FECHA      RESPONSABLE              COMENTARIO
*   -----------------------------------------------------------------------
*   MAR 20/26  Juan Andres Sanchez      Funciones básicas
*   MAR 22/26  Felipe Trujillo          Ajuste de estructura y comentarios
*   MAR 23/26  Sofia Vega               Documentación y validación
*
*******************************************************************************/

void
enviarDatos(
    byte col1, /* Ent: Datos de columnas de la matriz 1 */
    byte fil1, /* Ent: Datos de filas de la matriz 1    */
    byte col2, /* Ent: Datos de columnas de la matriz 2 */
    byte fil2  /* Ent: Datos de filas de la matriz 2    */ )
{
/* Parte 1: Deshabilitar temporalmente salidas con OE */
    ACTIVAR_BIT( PORTB, OE_BIT );
    LIMPIAR_BIT( PORTB, LATCH_BIT );

/* Parte 2: Desplazar los 4 bytes en el orden requerido */
    miShiftOutRegistro( MI_MSBFIRST, col2 );
    miShiftOutRegistro( MI_MSBFIRST, fil2 );
    miShiftOutRegistro( MI_MSBFIRST, col1 );
    miShiftOutRegistro( MI_MSBFIRST, fil1 );

/* Parte 3: Aplicar latch y volver a habilitar salidas */
    ACTIVAR_BIT( PORTB, LATCH_BIT );
    LIMPIAR_BIT( PORTB, LATCH_BIT );
    LIMPIAR_BIT( PORTB, OE_BIT );

} /* enviarDatos */



/*FN***************************************************************************
*
*   void refrescarDosFiguras( const byte figura1[8], const byte figura2[8] );
*
*   Propósito: Refrescar una vez las ocho filas de las dos figuras estáticas.
*
*   Plan:
*           Parte 1: Recorrer cada fila de las figuras
*           Parte 2: Calcular bytes de columnas y filas activas
*           Parte 3: Enviar datos y mantener el tiempo de refresco
*
*   Retorna: Nada
*
*   Registro de Revisiones:
*
*   FECHA      RESPONSABLE              COMENTARIO
*   -----------------------------------------------------------------------
*   MAR 20/26  Juan Andres Sanchez      Funciones básicas
*   MAR 22/26  Felipe Trujillo          Ajuste de estructura y comentarios
*   MAR 23/26  Sofia Vega               Documentación y validación
*
*******************************************************************************/

void
refrescarDosFiguras(
    const byte figura1[8], /* Ent: Figura para la matriz 1 */
    const byte figura2[8]  /* Ent: Figura para la matriz 2 */ )
{
/* Parte 1: Recorrer cada fila de las figuras */
    for( uint8_t fila = 0; fila < 8; fila++ ) {

        byte columnas1 = figura1[fila];
        byte columnas2 = figura2[fila];

/* Parte 2: Calcular bytes de columnas y filas activas */
        /* En este montaje:
           columnas activas en HIGH
           filas activas en LOW */

        byte fil1 = (byte) ~( 1 << fila );
        byte fil2 = (byte) ~( 1 << fila );

/* Parte 3: Enviar datos y mantener el tiempo de refresco */
        enviarDatos( columnas1, fil1, columnas2, fil2 );
        delayMicroseconds( RETARDO_FILA_US );
    }

} /* refrescarDosFiguras */



/*FN***************************************************************************
*
*   void apagarTodo( void );
*
*   Propósito: Apagar ambas matrices LED.
*
*   Nota:
*           En este montaje:
*           columnas LOW  = apagadas
*           filas    HIGH = apagadas
*
*   Plan:
*           Parte 1: Enviar el patrón de apagado a ambas matrices
*
*   Retorna: Nada
*
*   Registro de Revisiones:
*
*   FECHA      RESPONSABLE              COMENTARIO
*   -----------------------------------------------------------------------
*   MAR 20/26  Juan Andres Sanchez      Funciones básicas
*   MAR 22/26  Felipe Trujillo          Ajuste de estructura y comentarios
*   MAR 23/26  Sofia Vega               Documentación y validación
*
*******************************************************************************/

void
apagarTodo( void )
{
/* Parte 1: Enviar el patrón de apagado a ambas matrices */
    enviarDatos( 0x00, 0xFF, 0x00, 0xFF );

} /* apagarTodo */


/*FN***************************************************************************
*
*   void imprimirDosFigurasASCII( const byte figura1[8], const byte figura2[8] );
*
*   Propósito: Mostrar en consola serial la misma figura de ambas matrices
*              utilizando caracteres ASCII.
*
*   Plan:
*           Parte 1: Recorrer las 8 filas de ambas figuras
*           Parte 2: Imprimir cada columna encendida con '#'
*           Parte 3: Separar visualmente la matriz 1 y la matriz 2
*
*   Retorna: Nada
*
*   Registro de Revisiones:
*
*   FECHA      RESPONSABLE              COMENTARIO
*   -----------------------------------------------------------------------
*   MAR 20/26  Juan Andres Sanchez      Funciones básicas
*   MAR 22/26  Felipe Trujillo          Ajuste de estructura y comentarios
*   MAR 23/26  Sofia Vega               Documentación y validación
*
*******************************************************************************/

void
imprimirDosFigurasASCII(
    const byte figura1[8], /* Ent: Figura para la matriz 1 */
    const byte figura2[8]  /* Ent: Figura para la matriz 2 */ )
{
/* Parte 1: Recorrer las 8 filas de ambas figuras */
    for( uint8_t fila = 0; fila < 8; fila++ ) {

/* Parte 2: Imprimir cada columna encendida con '#' */
        for( int8_t bit = 7; bit >= 0; bit-- ) {
            if( figura1[fila] & ( 1 << bit ) )
                Serial.print( "#" );
            else
                Serial.print( " " );
        }

/* Parte 3: Separar visualmente la matriz 1 y la matriz 2 */
        Serial.print( "  " );

        for( int8_t bit = 7; bit >= 0; bit-- ) {
            if( figura2[fila] & ( 1 << bit ) )
                Serial.print( "#" );
            else
                Serial.print( " " );
        }

        Serial.println();
    }

} /* imprimirDosFigurasASCII */


/*FN***************************************************************************
*
*   void imprimirSeparadorSerial( void );
*
*   Propósito: Imprimir una línea separadora en el monitor serial para mejorar
*              la legibilidad de la salida ASCII.
*
*   Plan:
*           Parte 1: Imprimir un separador horizontal
*
*   Retorna: Nada
*
*   Registro de Revisiones:
*
*   FECHA      RESPONSABLE              COMENTARIO
*   -----------------------------------------------------------------------
*   MAR 20/26  Juan Andres Sanchez      Funciones básicas
*   MAR 22/26  Felipe Trujillo          Ajuste de estructura y comentarios
*   MAR 23/26  Sofia Vega               Documentación y validación
*
*******************************************************************************/

void
imprimirSeparadorSerial( void )
{
/* Parte 1: Imprimir un separador horizontal */
    Serial.println( "----------------------------------------" );

} /* imprimirSeparadorSerial */