/********************************** Encabezados *******************************/

/* -------------------- Inclusión de Librerías Estándar --------------------- */

#include <Arduino.h>

/* ---------------------- Inclusión de Librerías Propias -------------------- */



/**************************** Constantes Simbólicas ***************************/

/* ------------------- Tiempos de Refresco de la Pantalla ------------------- */
const uint16_t RETARDO_FILA_US = 200; /* refresco por fila [us] */
const uint16_t DEBOUNCE_MS     = 40;  /* antirrebote [ms]       */

/* ---------------------- Dimensiones del Tablero Lógico -------------------- */
#define ANCHO_TABLERO  8
#define ALTO_TABLERO  16

/* -------------------------------- Macros ---------------------------------- */
#define MI_LSBFIRST 0
#define MI_MSBFIRST 1

#define ACTIVAR_BIT( REG, BIT )   ( ( REG ) |=  ( 1U << ( BIT ) ) )
#define LIMPIAR_BIT( REG, BIT )   ( ( REG ) &= ~( 1U << ( BIT ) ) )

/* ------------------ Mapeo AVR de pines usados en Arduino UNO -------------- */
/*
    Botones:
    D3 -> PD3 -> IZQUIERDA
    D5 -> PD5 -> DERECHA
    D2 -> PD2 -> ROTAR
    D4 -> PD4 -> BAJAR

    Registros 74HC595:
    D6  -> PD6 -> SER   -> dataPin
    D7  -> PD7 -> MR    -> resetPin
    D8  -> PB0 -> SRCLK -> clockPin
    D9  -> PB1 -> RCLK  -> latchPin
    D10 -> PB2 -> OE    -> OEPin
*/

#define BTN_IZQ_BIT   PD3
#define BTN_DER_BIT   PD5
#define BTN_ROT_BIT   PD2
#define BTN_BAJ_BIT   PD4

#define DATA_BIT      PD6
#define RESET_BIT     PD7
#define CLOCK_BIT     PB0
#define LATCH_BIT     PB1
#define OE_BIT        PB2

/* ---------------------- Máscaras lógicas de botones ----------------------- */
#define BTN_IZQ_MASK  0x01
#define BTN_DER_MASK  0x02
#define BTN_ROT_MASK  0x04
#define BTN_BAJ_MASK  0x08



/******************************* Tipos de Datos *******************************/

/* Tetrominó con 4 rotaciones codificadas en 16 bits */
typedef struct {
    uint16_t rotaciones[4];
} Tetromino;

/* Pieza activa dentro del tablero */
typedef struct {
    const Tetromino *definicion;
    int8_t x;
    int8_t y;
    uint8_t rotacion;
} PiezaActiva;



/*************************** Prototipos de Funciones **************************/

/* ---------------- Funciones definidas después de loop() ------------------- */
void inicializarHardware( void );
void miShiftOutRegistro( uint8_t ordenBits, uint8_t valor );
void enviarDatos( byte col1, byte fil1, byte col2, byte fil2 );
void refrescarFramebuffer( void );
void apagarTodo( void );

uint8_t leerBotonesCrudo( void );
void actualizarBotones( void );
void procesarBotones( void );
void reportarEstado( uint8_t estado );

void inicializarJuego( void );
void reiniciarPiezaActiva( void );

bool celdaOcupadaEnRotacion( const Tetromino *pieza, uint8_t rot,
                             uint8_t fila, uint8_t col );
bool puedeUbicarse( const Tetromino *pieza, int8_t x, int8_t y, uint8_t rot );
bool intentarMover( int8_t dx, int8_t dy );
bool intentarRotar( void );

void limpiarFramebuffer( void );
void dibujarPiezaActivaEnFramebuffer( void );
void reconstruirFramebuffer( void );
void obtenerBytesDeFila( uint8_t filaFisica, byte *matriz1, byte *matriz2 );

void imprimirSeparadorSerial( void );
void imprimirFramebufferASCII( void );



/************** Definición e Inicialización de los Datos Globales *************/

/* ------------------ Datos globales del programa principal ----------------- */

/* Pieza T codificada en una matriz lógica 4x4 para sus 4 rotaciones */
const Tetromino PIEZA_T = {
    {
        0x4E00,
        0x4640,
        0x0E40,
        0x4C40
    }
};

/* Estado actual de la pieza activa */
PiezaActiva piezaActiva;

/* Framebuffer lógico del tablero 8x16 */
uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO];

/* Variables para antirrebote y detección de eventos en botones */
uint8_t botonesCrudoAnterior = 0;
uint8_t botonesEstable       = 0;
uint8_t botonesEstablePrevio = 0xFF;
unsigned long tiempoCambio   = 0;



/******************************* Función Principal ****************************/

/*FN***************************************************************************
*
*   void setup( void );
*
*   Propósito: Inicializar el hardware, habilitar el puerto serial e iniciar
*              la prueba del Hito 2 con una sola pieza de Tetris en un tablero
*              vertical de 8x16.
*
*   Plan:
*           Parte 1: Inicializar los pines del sistema usando registros AVR
*           Parte 2: Iniciar la comunicación serial
*           Parte 3: Inicializar la lógica del juego
*           Parte 4: Construir la primera imagen del framebuffer
*           Parte 5: Mostrar el estado inicial en consola
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
/* Parte 1: Inicializar los pines del sistema usando registros AVR */
    inicializarHardware();

/* Parte 2: Iniciar la comunicación serial */
    Serial.begin( 9600 );
    delay( 300 );

/* Parte 3: Inicializar la lógica del juego */
    inicializarJuego();

/* Parte 4: Construir la primera imagen del framebuffer */
    reconstruirFramebuffer();

/* Parte 5: Mostrar el estado inicial en consola */
    imprimirSeparadorSerial();
    Serial.println( "HITO 2 - PRUEBA PIEZA T" );
    Serial.println( "Tablero vertical 8x16" );
    Serial.println( "Matriz 1 arriba | Matriz 2 abajo" );
    imprimirSeparadorSerial();
    imprimirFramebufferASCII();

} /* setup */



/*FN***************************************************************************
*
*   void loop( void );
*
*   Propósito: Leer continuamente los botones, procesar el movimiento de la
*              pieza activa, reconstruir el framebuffer y refrescar la imagen
*              en ambas matrices LED.
*
*   Plan:
*           Parte 1: Actualizar el estado estable de los botones
*           Parte 2: Procesar la acción correspondiente
*           Parte 3: Reconstruir el framebuffer lógico
*           Parte 4: Refrescar la visualización en las matrices
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
/* Parte 1: Actualizar el estado estable de los botones */
    actualizarBotones();

/* Parte 2: Procesar la acción correspondiente */
    procesarBotones();

/* Parte 3: Reconstruir el framebuffer lógico */
    reconstruirFramebuffer();

/* Parte 4: Refrescar la visualización en las matrices */
    refrescarFramebuffer();
} /* loop */



/*************************** Definición de Funciones **************************/

/*FN***************************************************************************
*
*   void inicializarHardware( void );
*
*   Propósito: Configurar a nivel de registros AVR los pines conectados a los
*              botones y a los registros 74HC595, estableciendo estados
*              iniciales seguros.
*
*   Plan:
*           Parte 1: Configurar como salidas los bits usados en PORTB y PORTD
*           Parte 2: Configurar como entradas los pines de botones
*           Parte 3: Desactivar pull-up internos en los botones
*           Parte 4: Establecer estados iniciales seguros
*           Parte 5: Habilitar operación normal del display
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

/* Parte 2: Configurar como entradas los pines de botones */
    LIMPIAR_BIT( DDRD, BTN_IZQ_BIT );
    LIMPIAR_BIT( DDRD, BTN_DER_BIT );
    LIMPIAR_BIT( DDRD, BTN_ROT_BIT );
    LIMPIAR_BIT( DDRD, BTN_BAJ_BIT );

/* Parte 3: Desactivar pull-up internos en los botones */
    LIMPIAR_BIT( PORTD, BTN_IZQ_BIT );
    LIMPIAR_BIT( PORTD, BTN_DER_BIT );
    LIMPIAR_BIT( PORTD, BTN_ROT_BIT );
    LIMPIAR_BIT( PORTD, BTN_BAJ_BIT );

/* Parte 4: Establecer estados iniciales seguros */
    LIMPIAR_BIT( PORTD, DATA_BIT );
    LIMPIAR_BIT( PORTB, CLOCK_BIT );
    LIMPIAR_BIT( PORTB, LATCH_BIT );
    ACTIVAR_BIT( PORTB, OE_BIT );
    ACTIVAR_BIT( PORTD, RESET_BIT );

/* Parte 5: Habilitar operación normal del display */
    apagarTodo();
    LIMPIAR_BIT( PORTB, OE_BIT );

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
*   Propósito: Enviar los 4 bytes requeridos por los registros de
*              desplazamiento en cascada para las dos matrices LED.
*
*   Plan:
*           Parte 1: Deshabilitar temporalmente las salidas con OE
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
/* Parte 1: Deshabilitar temporalmente las salidas con OE */
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
*   void refrescarFramebuffer( void );
*
*   Propósito: Refrescar una vez las ocho filas físicas de las matrices LED a
*              partir del framebuffer lógico de un tablero vertical 8x16.
*
*   Plan:
*           Parte 1: Recorrer cada fila física del display
*           Parte 2: Obtener los bytes correspondientes a la matriz superior
*                    e inferior
*           Parte 3: Calcular las filas activas
*           Parte 4: Enviar los datos y mantener el tiempo de refresco
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
refrescarFramebuffer( void )
{
/* Parte 1: Recorrer cada fila física del display */
    for( uint8_t filaFisica = 0; filaFisica < 8; filaFisica++ ) {

        byte columnasMatriz1 = 0x00;
        byte columnasMatriz2 = 0x00;
        byte fil1;
        byte fil2;

/* Parte 2: Obtener los bytes correspondientes a la matriz superior e inferior */
        obtenerBytesDeFila( filaFisica, &columnasMatriz1, &columnasMatriz2 );

/* Parte 3: Calcular las filas activas */
        fil1 = (byte) ~( 1 << filaFisica );
        fil2 = (byte) ~( 1 << filaFisica );

/* Parte 4: Enviar los datos y mantener el tiempo de refresco */
        enviarDatos( columnasMatriz1, fil1, columnasMatriz2, fil2 );
        delayMicroseconds( RETARDO_FILA_US );
    }

} /* refrescarFramebuffer */



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
*   uint8_t leerBotonesCrudo( void );
*
*   Propósito: Leer directamente el estado instantáneo de los botones desde
*              el registro PIND y construir una máscara lógica.
*
*   Plan:
*           Parte 1: Inicializar la variable de lectura
*           Parte 2: Evaluar cada botón por separado
*           Parte 3: Retornar la máscara resultante
*
*   Retorna:
*           Máscara de botones activos
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

uint8_t
leerBotonesCrudo( void )
{
    uint8_t lectura = 0;

/* Parte 1 y Parte 2: Evaluar cada botón por separado */
    if( PIND & ( 1 << BTN_IZQ_BIT ) )
        lectura |= BTN_IZQ_MASK;

    if( PIND & ( 1 << BTN_DER_BIT ) )
        lectura |= BTN_DER_MASK;

    if( PIND & ( 1 << BTN_ROT_BIT ) )
        lectura |= BTN_ROT_MASK;

    if( PIND & ( 1 << BTN_BAJ_BIT ) )
        lectura |= BTN_BAJ_MASK;

/* Parte 3: Retornar la máscara resultante */
    return lectura;

} /* leerBotonesCrudo */



/*FN***************************************************************************
*
*   void actualizarBotones( void );
*
*   Propósito: Aplicar antirrebote por software a la lectura de botones,
*              generando un estado estable de entrada.
*
*   Plan:
*           Parte 1: Leer el estado instantáneo actual
*           Parte 2: Detectar cambios respecto a la lectura anterior
*           Parte 3: Reiniciar el temporizador si hubo cambio
*           Parte 4: Actualizar el estado estable al cumplir el tiempo
*                    de antirrebote
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
actualizarBotones( void )
{
    uint8_t lecturaActual = leerBotonesCrudo();

/* Parte 1 y Parte 2: Detectar cambios respecto a la lectura anterior */
    if( lecturaActual != botonesCrudoAnterior ) {
        botonesCrudoAnterior = lecturaActual;
        tiempoCambio = millis();
    }

/* Parte 3 y Parte 4: Actualizar el estado estable */
    if( ( millis() - tiempoCambio ) >= DEBOUNCE_MS )
        botonesEstable = botonesCrudoAnterior;

} /* actualizarBotones */



/*FN***************************************************************************
*
*   void procesarBotones( void );
*
*   Propósito: Procesar una única acción cuando se detecta un nuevo estado
*              estable de botones, moviendo o rotando la pieza activa.
*
*   Plan:
*           Parte 1: Verificar si hubo cambio de estado estable
*           Parte 2: Actualizar el último estado procesado
*           Parte 3: Ejecutar la acción asociada al botón detectado
*           Parte 4: Reportar el estado y mostrar el tablero por serial
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
procesarBotones( void )
{
/* Parte 1: Verificar si hubo cambio de estado estable */
    if( botonesEstable == botonesEstablePrevio )
        return;

/* Parte 2: Actualizar el último estado procesado */
    botonesEstablePrevio = botonesEstable;

/* Parte 3 y Parte 4: Ejecutar la acción y reportar el estado */
    switch( botonesEstable ) {

        case 0:
            reportarEstado( botonesEstable );
            break;

        case BTN_IZQ_MASK:
            intentarMover( -1, 0 );
            reportarEstado( botonesEstable );
            reconstruirFramebuffer();
            imprimirFramebufferASCII();
            break;

        case BTN_DER_MASK:
            intentarMover( 1, 0 );
            reportarEstado( botonesEstable );
            reconstruirFramebuffer();
            imprimirFramebufferASCII();
            break;

        case BTN_ROT_MASK:
            intentarRotar();
            reportarEstado( botonesEstable );
            reconstruirFramebuffer();
            imprimirFramebufferASCII();
            break;

        case BTN_BAJ_MASK:
            intentarMover( 0, 1 );
            reportarEstado( botonesEstable );
            reconstruirFramebuffer();
            imprimirFramebufferASCII();
            break;

        default:
            reportarEstado( botonesEstable );
            break;
    }

} /* procesarBotones */



/*FN***************************************************************************
*
*   void reportarEstado( uint8_t estado );
*
*   Propósito: Informar por monitor serial el estado estable detectado en los
*              botones.
*
*   Plan:
*           Parte 1: Evaluar la máscara recibida
*           Parte 2: Imprimir el nombre del botón o estado correspondiente
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
reportarEstado(
    uint8_t estado )
{
/* Parte 1 y Parte 2: Imprimir el nombre del botón o estado correspondiente */
    switch( estado ) {
        case 0:
            Serial.println( "NINGUNO" );
            break;

        case BTN_IZQ_MASK:
            Serial.println( "IZQUIERDA" );
            break;

        case BTN_DER_MASK:
            Serial.println( "DERECHA" );
            break;

        case BTN_ROT_MASK:
            Serial.println( "ROTAR" );
            break;

        case BTN_BAJ_MASK:
            Serial.println( "BAJAR" );
            break;

        default:
            Serial.println( "MULTIPLE" );
            break;
    }

} /* reportarEstado */



/*FN***************************************************************************
*
*   void inicializarJuego( void );
*
*   Propósito: Inicializar la lógica básica del juego cargando la pieza T
*              en su posición inicial.
*
*   Plan:
*           Parte 1: Reiniciar la pieza activa
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
inicializarJuego( void )
{
/* Parte 1: Reiniciar la pieza activa */
    reiniciarPiezaActiva();

} /* inicializarJuego */



/*FN***************************************************************************
*
*   void reiniciarPiezaActiva( void );
*
*   Propósito: Colocar la pieza T en la posición y rotación iniciales dentro
*              del tablero lógico.
*
*   Plan:
*           Parte 1: Asociar la definición de la pieza T
*           Parte 2: Definir posición inicial
*           Parte 3: Definir rotación inicial
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
reiniciarPiezaActiva( void )
{
/* Parte 1: Asociar la definición de la pieza T */
    piezaActiva.definicion = &PIEZA_T;

/* Parte 2: Definir posición inicial */
    piezaActiva.x          = 2;
    piezaActiva.y          = 0;

/* Parte 3: Definir rotación inicial */
    piezaActiva.rotacion   = 0;

} /* reiniciarPiezaActiva */



/*FN***************************************************************************
*
*   bool celdaOcupadaEnRotacion( const Tetromino *pieza, uint8_t rot,
*                                uint8_t fila, uint8_t col );
*
*   Propósito: Determinar si una celda específica del bloque 4x4 de una pieza
*              se encuentra ocupada en una rotación dada.
*
*   Plan:
*           Parte 1: Calcular el índice del bit correspondiente
*           Parte 2: Construir la máscara del bit
*           Parte 3: Verificar si la celda está ocupada
*
*   Retorna:
*           true  -> la celda está ocupada
*           false -> la celda está vacía
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

bool
celdaOcupadaEnRotacion(
    const Tetromino *pieza,
    uint8_t rot,
    uint8_t fila,
    uint8_t col )
{
    uint8_t indiceBit = (uint8_t) ( fila * 4 + col );
    uint16_t mascara  = (uint16_t) ( 1U << ( 15 - indiceBit ) );

/* Parte 1, Parte 2 y Parte 3: Verificar si la celda está ocupada */
    return ( pieza->rotaciones[rot] & mascara ) ? true : false;

} /* celdaOcupadaEnRotacion */



/*FN***************************************************************************
*
*   bool puedeUbicarse( const Tetromino *pieza, int8_t x, int8_t y,
*                       uint8_t rot );
*
*   Propósito: Verificar si una pieza puede ubicarse dentro de los límites del
*              tablero lógico sin salirse de sus bordes.
*
*   Plan:
*           Parte 1: Recorrer la matriz 4x4 de la pieza
*           Parte 2: Evaluar únicamente las celdas ocupadas
*           Parte 3: Convertir coordenadas locales a coordenadas del tablero
*           Parte 4: Validar los límites del tablero
*
*   Retorna:
*           true  -> la ubicación es válida
*           false -> la ubicación es inválida
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

bool
puedeUbicarse(
    const Tetromino *pieza,
    int8_t x,
    int8_t y,
    uint8_t rot )
{
/* Parte 1: Recorrer la matriz 4x4 de la pieza */
    for( uint8_t fila = 0; fila < 4; fila++ ) {
        for( uint8_t col = 0; col < 4; col++ ) {

            int8_t xTablero;
            int8_t yTablero;

/* Parte 2: Evaluar únicamente las celdas ocupadas */
            if( !celdaOcupadaEnRotacion( pieza, rot, fila, col ) )
                continue;

/* Parte 3: Convertir coordenadas locales a coordenadas del tablero */
            xTablero = (int8_t) ( x + col );
            yTablero = (int8_t) ( y + fila );

/* Parte 4: Validar los límites del tablero */
            if( xTablero < 0 || xTablero >= ANCHO_TABLERO )
                return false;

            if( yTablero < 0 || yTablero >= ALTO_TABLERO )
                return false;
        }
    }

    return true;

} /* puedeUbicarse */



/*FN***************************************************************************
*
*   bool intentarMover( int8_t dx, int8_t dy );
*
*   Propósito: Intentar mover la pieza activa una cantidad dada en X y Y,
*              aplicando el movimiento solo si es válido.
*
*   Plan:
*           Parte 1: Calcular la nueva posición candidata
*           Parte 2: Verificar si la pieza puede ubicarse en esa posición
*           Parte 3: Aplicar el movimiento si es posible
*
*   Retorna:
*           true  -> el movimiento fue aplicado
*           false -> el movimiento fue rechazado
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

bool
intentarMover(
    int8_t dx,
    int8_t dy )
{
    int8_t nuevoX = (int8_t) ( piezaActiva.x + dx );
    int8_t nuevoY = (int8_t) ( piezaActiva.y + dy );

/* Parte 1 y Parte 2: Verificar si la nueva posición es válida */
    if( !puedeUbicarse( piezaActiva.definicion,
                        nuevoX,
                        nuevoY,
                        piezaActiva.rotacion ) )
        return false;

/* Parte 3: Aplicar el movimiento si es posible */
    piezaActiva.x = nuevoX;
    piezaActiva.y = nuevoY;
    return true;

} /* intentarMover */



/*FN***************************************************************************
*
*   bool intentarRotar( void );
*
*   Propósito: Intentar rotar la pieza activa a su siguiente orientación,
*              aplicando el cambio solo si la nueva rotación es válida.
*
*   Plan:
*           Parte 1: Calcular la siguiente rotación
*           Parte 2: Verificar si la nueva rotación es válida
*           Parte 3: Aplicar la rotación si es posible
*
*   Retorna:
*           true  -> la rotación fue aplicada
*           false -> la rotación fue rechazada
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

bool
intentarRotar( void )
{
    uint8_t nuevaRotacion = (uint8_t) ( ( piezaActiva.rotacion + 1 ) & 0x03 );

/* Parte 1 y Parte 2: Verificar si la nueva rotación es válida */
    if( !puedeUbicarse( piezaActiva.definicion,
                        piezaActiva.x,
                        piezaActiva.y,
                        nuevaRotacion ) )
        return false;

/* Parte 3: Aplicar la rotación si es posible */
    piezaActiva.rotacion = nuevaRotacion;
    return true;

} /* intentarRotar */



/*FN***************************************************************************
*
*   void limpiarFramebuffer( void );
*
*   Propósito: Limpiar completamente el framebuffer lógico del tablero.
*
*   Plan:
*           Parte 1: Recorrer todas las filas y columnas del tablero
*           Parte 2: Colocar en cero cada celda
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
limpiarFramebuffer( void )
{
/* Parte 1 y Parte 2: Colocar en cero cada celda */
    for( uint8_t fila = 0; fila < ALTO_TABLERO; fila++ ) {
        for( uint8_t col = 0; col < ANCHO_TABLERO; col++ ) {
            framebuffer[fila][col] = 0;
        }
    }

} /* limpiarFramebuffer */



/*FN***************************************************************************
*
*   void dibujarPiezaActivaEnFramebuffer( void );
*
*   Propósito: Dibujar la pieza activa en el framebuffer lógico del tablero.
*
*   Plan:
*           Parte 1: Recorrer la matriz 4x4 de la pieza
*           Parte 2: Evaluar las celdas ocupadas
*           Parte 3: Convertir a coordenadas del tablero
*           Parte 4: Encender las celdas válidas en el framebuffer
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
dibujarPiezaActivaEnFramebuffer( void )
{
/* Parte 1: Recorrer la matriz 4x4 de la pieza */
    for( uint8_t fila = 0; fila < 4; fila++ ) {
        for( uint8_t col = 0; col < 4; col++ ) {

            int8_t xTablero;
            int8_t yTablero;

/* Parte 2: Evaluar las celdas ocupadas */
            if( !celdaOcupadaEnRotacion( piezaActiva.definicion,
                                         piezaActiva.rotacion,
                                         fila,
                                         col ) )
                continue;

/* Parte 3: Convertir a coordenadas del tablero */
            xTablero = (int8_t) ( piezaActiva.x + col );ión
            yTablero = (int8_t) ( piezaActiva.y + fila );

/* Parte 4: Encender las celdas válidas en el framebuffer */
            if( xTablero >= 0 && xTablero < ANCHO_TABLERO &&
                yTablero >= 0 && yTablero < ALTO_TABLERO )
                framebuffer[yTablero][xTablero] = 1;
        }
    }

} /* dibujarPiezaActivaEnFramebuffer */



/*FN***************************************************************************
*
*   void reconstruirFramebuffer( void );
*
*   Propósito: Reconstruir la imagen lógica del tablero a partir del estado
*              actual de la pieza activa.
*
*   Plan:
*           Parte 1: Limpiar el framebuffer
*           Parte 2: Dibujar la pieza activa
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
reconstruirFramebuffer( void )
{
/* Parte 1: Limpiar el framebuffer */
    limpiarFramebuffer();

/* Parte 2: Dibujar la pieza activa */
    dibujarPiezaActivaEnFramebuffer();

} /* reconstruirFramebuffer */



/*FN***************************************************************************
*
*   void obtenerBytesDeFila( uint8_t filaFisica, byte *matriz1, byte *matriz2 );
*
*   Propósito: Convertir una fila lógica del framebuffer en los bytes que deben
*              mostrarse en la matriz superior y en la matriz inferior.
*
*   Plan:
*           Parte 1: Inicializar ambos bytes en cero
*           Parte 2: Construir el byte de la matriz superior
*           Parte 3: Construir el byte de la matriz inferior
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
obtenerBytesDeFila(
    uint8_t filaFisica, /* Ent: Fila física a refrescar          */
    byte *matriz1,      /* Sal: Byte de columnas matriz superior */
    byte *matriz2 )     /* Sal: Byte de columnas matriz inferior */
{
/* Parte 1: Inicializar ambos bytes en cero */
    *matriz1 = 0x00;
    *matriz2 = 0x00;

/* Parte 2: Construir el byte de la matriz superior */
    for( uint8_t col = 0; col < ANCHO_TABLERO; col++ ) {
        if( framebuffer[filaFisica][col] ) {
            *matriz1 |= (byte) ( 1 << ( 7 - col ) );
        }
    }

/* Parte 3: Construir el byte de la matriz inferior */
    for( uint8_t col = 0; col < ANCHO_TABLERO; col++ ) {
        if( framebuffer[filaFisica + 8][col] ) {
            *matriz2 |= (byte) ( 1 << ( 7 - col ) );
        }
    }

} /* obtenerBytesDeFila */



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



/*FN***************************************************************************
*
*   void imprimirFramebufferASCII( void );
*
*   Propósito: Mostrar en consola serial el tablero lógico completo utilizando
*              caracteres ASCII para depuración visual.
*
*   Plan:
*           Parte 1: Imprimir un separador inicial
*           Parte 2: Recorrer todas las filas del framebuffer
*           Parte 3: Imprimir '#' para celdas activas y '.' para celdas vacías
*           Parte 4: Imprimir un separador final
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
imprimirFramebufferASCII( void )
{
/* Parte 1: Imprimir un separador inicial */
    imprimirSeparadorSerial();

/* Parte 2 y Parte 3: Imprimir el contenido del framebuffer */
    for( uint8_t fila = 0; fila < ALTO_TABLERO; fila++ ) {
        for( uint8_t col = 0; col < ANCHO_TABLERO; col++ ) {
            if( framebuffer[fila][col] )
                Serial.print( "#" );
            else
                Serial.print( "." );
        }
        Serial.println();
    }

/* Parte 4: Imprimir un separador final */
    imprimirSeparadorSerial();

} /* imprimirFramebufferASCII */
