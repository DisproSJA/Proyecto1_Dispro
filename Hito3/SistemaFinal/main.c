/******************************************************************************
 *   Archivo:     main.c
 *   Proyecto:    Tetris Multi-Plataforma - Hito Final
 *   Materia:     Diseno de Sistemas con Procesador (DISPRO)
 *
 *   Descripcion: Punto de entrada del programa. Contiene el flujo principal
 *                del juego: inicializacion de hardware, bucle de juego con
 *                procesamiento de entrada, caida automatica con velocidad
 *                progresiva segun el nivel, y dibujado de pantalla.
 *
 *   Compilar en Windows con MinGW:
 *       gcc -o tetris.exe main.c tetris_logic.c driver_pc.c
 *
 *   Integrantes:
 *      - Sofia Vega
 *      - Juan Sanchez
 *      - Andres Trujillo
 *
 *   Fecha:       Abril 2026
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "tetris_logic.h"
#include "hal_display.h"

/******************************************************************************
 * CONSTANTES DE CONFIGURACION DEL BUCLE DE JUEGO
 ******************************************************************************/

#define RETARDO_CICLO_MS     15     /* Pausa entre iteraciones del bucle     */
#define CAIDA_MS_INICIAL     800    /* Intervalo de caida al inicio (ms)     */
#define REDUCCION_NIVEL_MS   50     /* Reduccion de intervalo por nivel (ms) */
#define CAIDA_MS_MINIMA      100    /* Velocidad maxima de caida (ms)        */

/******************************************************************************
 * VARIABLES GLOBALES DEL PROGRAMA
 ******************************************************************************/

static EstadoJuego g_juego;
static uint8_t     g_framebuffer[ALTO_TABLERO][ANCHO_TABLERO];
static int         g_juegoActivo      = 1;
static long        g_ultimoDescensoMs = 0;

/******************************************************************************
 * PROTOTIPOS DE FUNCIONES INTERNAS
 ******************************************************************************/

static void salidaPrograma( void );
static long calcularIntervaloCaida( void );
static void procesarEntrada( void );
static void actualizarCaidaAutomatica( void );

/******************************************************************************
 * IMPLEMENTACION
 ******************************************************************************/

/*FN****************************************************************************
*
*   int main( void )
*
*   Que hace:  Punto de entrada del programa. Configura la consola, inicia
*              el motor de Tetris con una pieza aleatoria y entra al bucle
*              principal donde se procesan las entradas del teclado, se
*              actualiza la caida automatica y se redibuja la pantalla
*              hasta que el usuario presione Q para salir.
*
*   Retorna:   0 si el programa termino sin errores, 1 si hubo un error
*              al inicializar el hardware
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Sofia Vega
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
int main( void )
{
    if( hal_inicializarHardware() != 0 ) {
        printf( "Error al inicializar el hardware.\n" );
        return 1;
    }

    atexit( salidaPrograma );

    srand( (unsigned int)hal_obtenerTiempoMs() );

    tetris_inicializarJuego( &g_juego );
    g_ultimoDescensoMs = hal_obtenerTiempoMs();

    while( g_juegoActivo ) {
        procesarEntrada();
        actualizarCaidaAutomatica();

#ifdef PLATFORM_AVR
        /* En AVR, si es game over se muestran los digitos en las matrices */
        if( g_juego.gameOver ) {
            tetris_dibujarPantallaGameOver( &g_juego, g_framebuffer );
        } else {
            tetris_dibujarEstadoEnFramebuffer( &g_juego, g_framebuffer );
        }
#else
        /* En PC, el panel lateral ya muestra la info de game over */
        tetris_dibujarEstadoEnFramebuffer( &g_juego, g_framebuffer );
#endif

        hal_dibujarJuego( &g_juego, g_framebuffer );
        hal_retardo( RETARDO_CICLO_MS );
    }

    return 0;
}

/*FN****************************************************************************
*
*   static void salidaPrograma( void )
*
*   Que hace:  Se llama automaticamente al terminar el programa (registrada
*              con atexit) para restaurar el estado de la consola.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*******************************************************************************/
static void salidaPrograma( void )
{
    hal_finalizarHardware();
}

/*FN****************************************************************************
*
*   static long calcularIntervaloCaida( void )
*
*   Que hace:  Calcula cada cuantos milisegundos debe bajar la pieza
*              automaticamente segun el nivel actual. El nivel sube
*              cada 5 lineas eliminadas. Cada nivel reduce el intervalo
*              en 50 ms, con un minimo de 100 ms para que el juego
*              no sea imposible.
*
*   Retorna:   El intervalo de caida en milisegundos
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*******************************************************************************/
static long calcularIntervaloCaida( void )
{
    int nivel;
    long intervalo;

    nivel = (int)( g_juego.lineasCompletas / LINEAS_POR_NIVEL );
    intervalo = CAIDA_MS_INICIAL - ( nivel * REDUCCION_NIVEL_MS );

    if( intervalo < CAIDA_MS_MINIMA ) {
        intervalo = CAIDA_MS_MINIMA;
    }

    return intervalo;
}

/*FN****************************************************************************
*
*   static void procesarEntrada( void )
*
*   Que hace:  Lee la entrada del usuario a traves del HAL y ejecuta la
*              accion correspondiente: mover la pieza, rotarla, bajarla,
*              salir del juego o reiniciar despues de un game over.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*******************************************************************************/
static void procesarEntrada( void )
{
    int entrada;

    entrada = hal_leerEntrada();

    /* Si el juego termino, solo se permite reiniciar o salir */
    if( g_juego.gameOver ) {
        if( entrada == ENTRADA_REINICIAR ) {
            srand( (unsigned int)hal_obtenerTiempoMs() );
            tetris_inicializarJuego( &g_juego );
            g_ultimoDescensoMs = hal_obtenerTiempoMs();
        }

        if( entrada == ENTRADA_SALIR ) {
            g_juegoActivo = 0;
        }

        return;
    }

    /* Acciones durante el juego normal */
    switch( entrada ) {
        case ENTRADA_IZQUIERDA:
            tetris_intentarMover( &g_juego, -1, 0 );
            break;

        case ENTRADA_DERECHA:
            tetris_intentarMover( &g_juego, 1, 0 );
            break;

        case ENTRADA_ROTAR:
            tetris_intentarRotar( &g_juego );
            break;

        case ENTRADA_BAJAR:
            tetris_bajarOFijar( &g_juego );
            g_ultimoDescensoMs = hal_obtenerTiempoMs();
            break;

        case ENTRADA_SALIR:
            g_juegoActivo = 0;
            break;

        default:
            break;
    }
}

/*FN****************************************************************************
*
*   static void actualizarCaidaAutomatica( void )
*
*   Que hace:  Revisa si ya paso suficiente tiempo desde la ultima vez que
*              la pieza bajo sola. Si el intervalo se cumplio, la baja una
*              fila. La velocidad depende del nivel actual del juego.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*******************************************************************************/
static void actualizarCaidaAutomatica( void )
{
    long tiempoActual;
    long intervalo;

    if( g_juego.gameOver ) {
        return;
    }

    tiempoActual = hal_obtenerTiempoMs();
    intervalo    = calcularIntervaloCaida();

    if( ( tiempoActual - g_ultimoDescensoMs ) >= intervalo ) {
        tetris_bajarOFijar( &g_juego );
        g_ultimoDescensoMs = tiempoActual;
    }
}
