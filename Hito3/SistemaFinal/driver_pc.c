/******************************************************************************
 *   Archivo:     driver_pc.c
 *   Proyecto:    Tetris Multi-Plataforma - Hito Final
 *   Materia:     Diseno de Sistemas con Procesador (DISPRO)
 *
 *   Descripcion: Implementacion del driver para consola de PC (Windows).
 *                Este archivo implementa todas las funciones declaradas en
 *                hal_display.h usando stdio.h para la salida y la API de
 *                Windows para el manejo de la consola.
 *
 *                Se compila solo cuando PLATFORM_PC esta definido.
 *
 *   Integrantes:
 *      - Sofia Vega
 *      - Juan Sanchez
 *      - Andres Trujillo
 *
 *   Fecha:       Abril 2026
 ******************************************************************************/

#include "hal_display.h"

#ifdef PLATFORM_PC

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>

/******************************************************************************
 * VARIABLES GLOBALES DEL DRIVER
 ******************************************************************************/

static HANDLE g_hConsola;
static CONSOLE_CURSOR_INFO g_cursorOriginal;

/* Nombres de los integrantes del grupo */
static const char *g_integrantes[] = {
    "Sofia Vega",
    "Juan Sanchez",
    "Andres Trujillo"
};

/* Nombres de los 7 tetrominos en el mismo orden que TETROMINOS[] */
static const char *g_nombresPiezas[] = {
    "T", "O", "I", "L", "J", "S", "Z"
};

/******************************************************************************
 * PROTOTIPOS DE FUNCIONES INTERNAS (STATIC)
 ******************************************************************************/

static const char *obtenerNombrePieza( uint8_t tipo );

static void limpiarPantalla( void );

static void imprimirPanelJuego( const EstadoJuego *juego, int fila );

static void imprimirPanelGameOver( const EstadoJuego *juego, int fila );

/******************************************************************************
 * IMPLEMENTACION DE FUNCIONES HAL (PUBLICAS)
 ******************************************************************************/

/*FN****************************************************************************
*
*   int hal_inicializarHardware( void )
*
*   Que hace:  Obtiene el handle de la consola de Windows y esconde el
*              cursor para que la animacion del juego se vea sin parpadeo.
*
*   Retorna:   0 si todo salio bien, -1 si hubo error
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*******************************************************************************/
int hal_inicializarHardware( void )
{
    CONSOLE_CURSOR_INFO cursorInfo;

    g_hConsola = GetStdHandle( STD_OUTPUT_HANDLE );

    if( g_hConsola == INVALID_HANDLE_VALUE ) {
        return -1;
    }

    if( !GetConsoleCursorInfo( g_hConsola, &g_cursorOriginal ) ) {
        return -1;
    }

    cursorInfo = g_cursorOriginal;
    cursorInfo.bVisible = FALSE;

    if( !SetConsoleCursorInfo( g_hConsola, &cursorInfo ) ) {
        return -1;
    }

    return 0;
}

/*FN****************************************************************************
*
*   void hal_finalizarHardware( void )
*
*   Que hace:  Restaura el cursor de la consola a su estado original
*              para que la terminal quede como estaba antes del juego.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*******************************************************************************/
void hal_finalizarHardware( void )
{
    SetConsoleCursorInfo( g_hConsola, &g_cursorOriginal );
}

/*FN****************************************************************************
*
*   int hal_leerEntrada( void )
*
*   Que hace:  Revisa si el usuario presiono una tecla. Soporta tanto
*              las teclas WASD como las flechas del teclado, ademas de
*              Q para salir y R para reiniciar.
*
*   Retorna:   Uno de los codigos ENTRADA_xxx definidos en hal_display.h,
*              o ENTRADA_NINGUNA si no se presiono nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*******************************************************************************/
int hal_leerEntrada( void )
{
    int tecla;

    if( !_kbhit() ) {
        return ENTRADA_NINGUNA;
    }

    tecla = _getch();

    /* Las flechas generan dos bytes: primero 0 o 224, luego el codigo */
    if( tecla == 0 || tecla == 224 ) {
        tecla = _getch();

        switch( tecla ) {
            case 75:  return ENTRADA_IZQUIERDA;   /* Flecha izquierda */
            case 77:  return ENTRADA_DERECHA;     /* Flecha derecha   */
            case 72:  return ENTRADA_ROTAR;       /* Flecha arriba    */
            case 80:  return ENTRADA_BAJAR;       /* Flecha abajo     */
            default:  return ENTRADA_NINGUNA;
        }
    }

    switch( tecla ) {
        case 'a':  case 'A':  return ENTRADA_IZQUIERDA;
        case 'd':  case 'D':  return ENTRADA_DERECHA;
        case 'w':  case 'W':  return ENTRADA_ROTAR;
        case 's':  case 'S':  return ENTRADA_BAJAR;
        case 'q':  case 'Q':  return ENTRADA_SALIR;
        case 'r':  case 'R':  return ENTRADA_REINICIAR;
        default:              return ENTRADA_NINGUNA;
    }
}

/*FN****************************************************************************
*
*   long hal_obtenerTiempoMs( void )
*
*   Que hace:  Devuelve el tiempo actual en milisegundos usando el
*              contador de Windows. Se usa para controlar la velocidad
*              de caida automatica de las piezas.
*
*   Retorna:   El tiempo actual en milisegundos
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*******************************************************************************/
long hal_obtenerTiempoMs( void )
{
    return (long) GetTickCount64();
}

/*FN****************************************************************************
*
*   void hal_retardo( int milisegundos )
*
*   Que hace:  Pausa la ejecucion la cantidad indicada de milisegundos.
*              Se usa en el bucle principal para no consumir toda la CPU.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*******************************************************************************/
void hal_retardo( int milisegundos )
{
    Sleep( (DWORD)milisegundos );
}

/*FN****************************************************************************
*
*   void hal_dibujarJuego( juego, framebuffer )
*
*   Que hace:  Dibuja toda la pantalla del Tetris en la consola: el
*              encabezado con el titulo, el tablero con la pieza cayendo,
*              el panel lateral con informacion del juego y la barra
*              inferior con instrucciones. Mueve el cursor al inicio
*              antes de dibujar para evitar parpadeo.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Sofia Vega
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
void hal_dibujarJuego( const EstadoJuego *juego,
                  const uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] )
{
    int f;
    int c;

    limpiarPantalla();

    /* Encabezado: titulo del juego y panel superior */
    printf( "\n" );
    printf( "<!========================!>   +--------------------+\n" );
    printf( "<!  T  E  T  R  I  S     !>   |  HITO FINAL - PC   |\n" );
    printf( "<!========================!>   +--------------------+\n" );

    /* Tablero + panel lateral (16 filas) */
    for( f = 0; f < ALTO_TABLERO; f++ ) {
        printf( "<!" );

        for( c = 0; c < ANCHO_TABLERO; c++ ) {
            printf( framebuffer[f][c] ? "[#]" : " . " );
        }

        printf( "!>   " );

        if( juego->gameOver ) {
            imprimirPanelGameOver( juego, f );
        } else {
            imprimirPanelJuego( juego, f );
        }
    }

    /* Borde inferior del tablero */
    printf( "<!========================!>   +--------------------+\n" );

    /* Instrucciones segun el estado del juego */
    if( juego->gameOver ) {
        printf( "\n  Presione R para reiniciar o Q para salir.\n" );
    } else {
        printf( "\n  Presione A, D, W, S o flechas. Q para salir.\n" );
    }

    fflush( stdout );
}

/******************************************************************************
 * IMPLEMENTACION DE FUNCIONES INTERNAS (STATIC)
 ******************************************************************************/

/*FN****************************************************************************
*
*   static const char *obtenerNombrePieza( uint8_t tipo )
*
*   Que hace:  Devuelve el nombre (letra) del tetromino segun su indice.
*              Por ejemplo: 0="T", 1="O", 2="I", etc.
*
*   Retorna:   Puntero a la cadena con el nombre de la pieza
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*******************************************************************************/
static const char * obtenerNombrePieza( uint8_t tipo )
{
    if( tipo < NUM_TETROMINOS ) {
        return g_nombresPiezas[tipo];
    }

    return "?";
}

/*FN****************************************************************************
*
*   static void limpiarPantalla( void )
*
*   Que hace:  Mueve el cursor de la consola a la posicion (0,0) para
*              redibujar encima del contenido anterior. Esto evita que
*              la pantalla parpadee al actualizar.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*******************************************************************************/
static void limpiarPantalla( void )
{
    COORD origen = { 0, 0 };
    SetConsoleCursorPosition( g_hConsola, origen );
}

/*FN****************************************************************************
*
*   static void imprimirPanelJuego( juego, fila )
*
*   Que hace:  Imprime una fila del panel lateral derecho durante el juego
*              normal. Muestra la pieza actual, puntaje, lineas, nivel,
*              controles e integrantes del grupo.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Sofia Vega
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
static void imprimirPanelJuego( const EstadoJuego *juego, int fila )
{
    char buf[32];
    int nivel;

    nivel = (int)( juego->lineasCompletas / LINEAS_POR_NIVEL );

    /* Filas separadoras con bordes */
    if( fila == 4 || fila == 11 ) {
        printf( "+--------------------+\n" );
        return;
    }

    /* Filas con contenido dinamico o estatico */
    switch( fila ) {
        case 0:
            sprintf( buf, "PIEZA:  %s",
                     obtenerNombrePieza( juego->piezaActiva.tipo ) );
            break;
        case 1:
            sprintf( buf, "PUNTAJE: %04u",
                     (unsigned)juego->puntaje );
            break;
        case 2:
            sprintf( buf, "LINEAS:  %03u",
                     (unsigned)juego->lineasCompletas );
            break;
        case 3:
            sprintf( buf, "NIVEL:   %02d", nivel );
            break;
        case 5:
            sprintf( buf, "CONTROLES" );
            break;
        case 6:
            sprintf( buf, "A/< : IZQUIERDA" );
            break;
        case 7:
            sprintf( buf, "D/> : DERECHA" );
            break;
        case 8:
            sprintf( buf, "W/^ : ROTAR" );
            break;
        case 9:
            sprintf( buf, "S/v : BAJAR" );
            break;
        case 10:
            sprintf( buf, "Q   : SALIR" );
            break;
        case 12:
            sprintf( buf, "INTEGRANTES" );
            break;
        case 13:
            sprintf( buf, "%s", g_integrantes[0] );
            break;
        case 14:
            sprintf( buf, "%s", g_integrantes[1] );
            break;
        case 15:
            sprintf( buf, "%s", g_integrantes[2] );
            break;
        default:
            buf[0] = '\0';
            break;
    }

    printf( "|  %-18s|\n", buf );
}

/*FN****************************************************************************
*
*   static void imprimirPanelGameOver( juego, fila )
*
*   Que hace:  Imprime una fila del panel lateral derecho cuando el juego
*              ha terminado. Muestra el mensaje de game over, estadisticas
*              finales y las opciones de reiniciar o salir.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   ABR 02/26      Andres Felipe Trujillo
*   ABR 02/26      Sofia Vega
*   ABR 02/26      Juan Sanchez
*******************************************************************************/
static void imprimirPanelGameOver( const EstadoJuego *juego, int fila )
{
    char buf[32];
    int nivel;

    nivel = (int)( juego->lineasCompletas / LINEAS_POR_NIVEL );

    /* Filas separadoras con bordes */
    if( fila == 1 || fila == 6 || fila == 11 ) {
        printf( "+--------------------+\n" );
        return;
    }

    switch( fila ) {
        case 0:
            sprintf( buf, "JUEGO TERMINADO" );
            break;
        case 2:
            sprintf( buf, "PUNTAJE: %04u",
                     (unsigned)juego->puntaje );
            break;
        case 3:
            sprintf( buf, "LINEAS:  %03u",
                     (unsigned)juego->lineasCompletas );
            break;
        case 4:
            sprintf( buf, "PIEZAS:  %03u",
                     (unsigned)juego->piezasColocadas );
            break;
        case 5:
            sprintf( buf, "NIVEL:   %02d", nivel );
            break;
        case 8:
            sprintf( buf, "R : REINICIAR" );
            break;
        case 9:
            sprintf( buf, "Q : SALIR" );
            break;
        case 12:
            sprintf( buf, "INTEGRANTES" );
            break;
        case 13:
            sprintf( buf, "%s", g_integrantes[0] );
            break;
        case 14:
            sprintf( buf, "%s", g_integrantes[1] );
            break;
        case 15:
            sprintf( buf, "%s", g_integrantes[2] );
            break;
        default:
            buf[0] = '\0';
            break;
    }

    printf( "|  %-18s|\n", buf );
}

#endif /* PLATFORM_PC */
