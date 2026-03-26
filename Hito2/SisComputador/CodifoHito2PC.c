#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>

/******************************************************************************
 * CONFIGURACION GENERAL
 ******************************************************************************/

#define FILAS             16
#define COLUMNAS           8
#define DIM_PIEZA         4
#define RETARDO_MS        15
#define CAIDA_MS          400

/******************************************************************************
 * VARIABLES GLOBALES
 ******************************************************************************/

static HANDLE g_hConsola;
static CONSOLE_CURSOR_INFO g_cursorOriginal;

static int g_juegoActivo = 1;
static int g_piezaFila = 0;
static int g_piezaColumna = 3;
static int g_rotacion = 0;
static long g_ultimoDescensoMs = 0;

/* Tablero base con algunos bloques fijos para probar colisiones */
static int g_tableroBase[FILAS][COLUMNAS] = {
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,1,0,0,0,0,0,0},
    {1,1,1,1,1,0,0,1},
    {1,1,0,1,1,1,1,1},
    {1,1,1,0,1,1,1,1}
};

/* Pieza T en sus cuatro rotaciones */
static const int g_piezaT[4][DIM_PIEZA][DIM_PIEZA] = {
    {
        {0,1,0,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    {
        {0,1,0,0},
        {0,1,1,0},
        {0,1,0,0},
        {0,0,0,0}
    },
    {
        {0,0,0,0},
        {1,1,1,0},
        {0,1,0,0},
        {0,0,0,0}
    },
    {
        {0,1,0,0},
        {1,1,0,0},
        {0,1,0,0},
        {0,0,0,0}
    }
};

/* Panel lateral */
static const char *g_integrantes[] = {
    "Sofia Vega",
    "Juan Sanchez",
    "Andres Trujillo"
};

/*FN****************************************************************************
*
*   long Tetris_TiempoActualMs( void )
*
*   Qué hace:  Devuelve el tiempo actual en milisegundos. Lo usamos para
*              saber cuándo le toca bajar sola a la pieza.
*
*   Retorna:   El tiempo actual en milisegundos
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
long Tetris_TiempoActualMs(void) {

    return (long) GetTickCount64();
}

/*FN****************************************************************************
*
*   int Tetris_ConfigurarTerminal( void )
*
*   Qué hace:  Prepara la consola de Windows y esconde el cursor para que
*              la animación se vea mejor y no parpadee.
*
*   Retorna:   0 si todo salió bien, -1 si hubo algún error
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
int Tetris_ConfigurarTerminal(void) {

    CONSOLE_CURSOR_INFO cursorInfo;

    g_hConsola = GetStdHandle(STD_OUTPUT_HANDLE);

    if (g_hConsola == INVALID_HANDLE_VALUE) {
        return -1;
    }

    if (!GetConsoleCursorInfo(g_hConsola, &g_cursorOriginal)) {
        return -1;
    }

    cursorInfo = g_cursorOriginal;
    cursorInfo.bVisible = FALSE;

    if (!SetConsoleCursorInfo(g_hConsola, &cursorInfo)) {
        return -1;
    }

    return 0;
}

/*FN****************************************************************************
*
*   int Tetris_RestaurarTerminal( void )
*
*   Qué hace:  Devuelve el cursor de la consola a como estaba antes de que
*              arrancara el programa.
*
*   Retorna:   0 siempre
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
int Tetris_RestaurarTerminal(void) {

    SetConsoleCursorInfo(g_hConsola, &g_cursorOriginal);
    return 0;
}

/*FN****************************************************************************
*
*   void Tetris_SalidaPrograma( void )
*
*   Qué hace:  Se llama sola cuando el programa termina (con atexit) para
*              dejar la consola como estaba al inicio.
*
*   Retorna:   Nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
void Tetris_SalidaPrograma(void) {

    Tetris_RestaurarTerminal();
}

/*FN****************************************************************************
*
*   int Tetris_LeerTecla( void )
*
*   Qué hace:  Revisa si el usuario presionó alguna tecla y, si es así,
*              la lee y la devuelve.
*
*   Retorna:   El valor de la tecla presionada, o -1 si no se presionó nada
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
int Tetris_LeerTecla(void) {

    if (_kbhit()) {
        return _getch();
    }

    return -1;
}

/*FN****************************************************************************
*
*   int Tetris_LimpiarPantalla( void )
*
*   Qué hace:  Mueve el cursor al inicio de la pantalla para redibujar
*              encima de lo anterior y así evitar que la imagen parpadee.
*
*   Retorna:   0 siempre
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 25/26      Andrés Felipe Trujillo
*******************************************************************************/
int Tetris_LimpiarPantalla(void) {

    COORD origen = {0, 0};
    SetConsoleCursorPosition(g_hConsola, origen);

    return 0;
}

/*FN****************************************************************************
*
*   int Tetris_PuedeMover( int nuevaFila, int nuevaColumna, int nuevaRotacion )
*
*   Qué hace:  Revisa si la pieza activa puede moverse a una nueva posición
*              sin salirse del tablero ni chocar con los bloques que ya están.
*
*   Retorna:   1 si el movimiento es válido, 0 si no se puede hacer
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
int Tetris_PuedeMover(int nuevaFila, int nuevaColumna, int nuevaRotacion) {

    int filaLocal;
    int columnaLocal;

    for (filaLocal = 0; filaLocal < DIM_PIEZA; filaLocal++) {
        for (columnaLocal = 0; columnaLocal < DIM_PIEZA; columnaLocal++) {

            int filaReal;
            int columnaReal;

            if (!g_piezaT[nuevaRotacion][filaLocal][columnaLocal]) {
                continue;
            }

            filaReal = nuevaFila + filaLocal;
            columnaReal = nuevaColumna + columnaLocal;

            if (filaReal < 0 || filaReal >= FILAS) {
                return 0;
            }

            if (columnaReal < 0 || columnaReal >= COLUMNAS) {
                return 0;
            }

            if (g_tableroBase[filaReal][columnaReal] == 1) {
                return 0;
            }
        }
    }

    return 1;
}

/*FN****************************************************************************
*
*   int Tetris_ReiniciarPieza( void )
*
*   Qué hace:  Vuelve a poner la pieza T en la parte de arriba del tablero
*              para que siga cayendo desde el principio.
*
*   Retorna:   0 siempre
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
int Tetris_ReiniciarPieza(void) {

    g_piezaFila = 0;
    g_piezaColumna = 2;   /* centro para 8 columnas: (8-4)/2 = 2 */
    g_rotacion = 0;

    return 0;
}

/*FN****************************************************************************
*
*   int Tetris_MoverHorizontal( int desplazamiento )
*
*   Qué hace:  Mueve la pieza activa una columna a la izquierda o a la
*              derecha, siempre y cuando el movimiento sea válido.
*
*   Retorna:   0 siempre
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
int Tetris_MoverHorizontal(int desplazamiento) {

    if (Tetris_PuedeMover(g_piezaFila, g_piezaColumna + desplazamiento, g_rotacion)) {
        g_piezaColumna += desplazamiento;
    }

    return 0;
}

/*FN****************************************************************************
*
*   int Tetris_RotarPieza( void )
*
*   Qué hace:  Rota la pieza T en sentido horario si la nueva posición
*              resultante es válida.
*
*   Retorna:   0 siempre
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
int Tetris_RotarPieza(void) {

    int nuevaRotacion = (g_rotacion + 1) % 4;

    if (Tetris_PuedeMover(g_piezaFila, g_piezaColumna, nuevaRotacion)) {
        g_rotacion = nuevaRotacion;
    }

    return 0;
}

/*FN****************************************************************************
*
*   int Tetris_BajarPieza( void )
*
*   Qué hace:  Baja la pieza una fila. Si ya no puede bajar más porque chocó,
*              la reinicia desde arriba para que el demo siga funcionando.
*
*   Retorna:   1 si la pieza bajó, 0 si se reinició desde arriba
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
int Tetris_BajarPieza(void) {

    if (Tetris_PuedeMover(g_piezaFila + 1, g_piezaColumna, g_rotacion)) {
        g_piezaFila++;
        return 1;
    }

    Tetris_ReiniciarPieza();
    return 0;
}

/*FN****************************************************************************
*
*   int Tetris_ConstruirTableroVisible( int tableroVisible[FILAS][COLUMNAS] )
*
*   Qué hace:  Copia el tablero base y le pone encima la pieza que está
*              cayendo, para armar lo que se va a mostrar en pantalla.
*
*   Retorna:   0 siempre
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
int Tetris_ConstruirTableroVisible(int tableroVisible[FILAS][COLUMNAS]) {

    int f;
    int c;

    for (f = 0; f < FILAS; f++) {
        for (c = 0; c < COLUMNAS; c++) {
            tableroVisible[f][c] = g_tableroBase[f][c];
        }
    }

    for (f = 0; f < DIM_PIEZA; f++) {
        for (c = 0; c < DIM_PIEZA; c++) {

            int filaReal;
            int columnaReal;

            if (!g_piezaT[g_rotacion][f][c]) {
                continue;
            }

            filaReal = g_piezaFila + f;
            columnaReal = g_piezaColumna + c;

            if (filaReal >= 0 && filaReal < FILAS &&
                columnaReal >= 0 && columnaReal < COLUMNAS) {
                tableroVisible[filaReal][columnaReal] = 1;
            }
        }
    }

    return 0;
}

/*FN****************************************************************************
*
*   int Tetris_Encabezado( void )
*
*   Qué hace:  Muestra el título del juego y el borde superior del panel
*              de información que va a la derecha del tablero.
*
*   Retorna:   0 siempre
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 23/26      Andrés Felipe Trujillo
*   MAR 23/26      Sofia Vega
*   MAR 23/26      Juan Sanchez
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
int Tetris_Encabezado(void) {

    printf("\n");
    printf("<!========================!>   +--------------------+\n");
    printf("<!  T  E  T  R  I  S     !>   |  HITO 2  -  PC     |\n");
    printf("<!========================!>   +--------------------+\n");

    return 0;
}

/*FN****************************************************************************
*
*   int Tetris_Tablero( void )
*
*   Qué hace:  Muestra el tablero animado junto con el panel lateral que
*              tiene los controles del juego y el estado actual de la pieza.
*
*   Retorna:   0 siempre
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
int Tetris_Tablero(void) {

    int f;
    int c;
    int tableroVisible[FILAS][COLUMNAS];
    const char *panel[FILAS] = {
        "|  ANIMACION        |",
        "|  PIEZA T ACTIVA   |",
        "+-------------------+",
        "|  CONTROLES        |",
        "|  A : IZQUIERDA    |",
        "|  D : DERECHA      |",
        "|  W : ROTAR        |",
        "|  S : BAJAR        |",
        "|  Q : SALIR        |",
        "+-------------------+",
        "|  ESTADO           |",
        "|                   |",   /* f==11: sobreescrito con FILA     */
        "|                   |",   /* f==12: sobreescrito con COLUMNA  */
        "|                   |",   /* f==13: sobreescrito con ROTACION */
        "+-------------------+",
        "|  HITO 2 DISPRO    |"
    };

    Tetris_ConstruirTableroVisible(tableroVisible);

    for (f = 0; f < FILAS; f++) {
        printf("<!");
        for (c = 0; c < COLUMNAS; c++) {
            printf(tableroVisible[f][c] ? "[C]" : " . ");
        }

        if (f == 11) {
            printf("!>   |  FILA:   %02d       |\n", g_piezaFila);
        }
        else if (f == 12) {
            printf("!>   |  COLUMNA:%02d       |\n", g_piezaColumna);
        }
        else if (f == 13) {
            printf("!>   |  ROTACION:%d        |\n", g_rotacion);
        }
        else {
            printf("!>   %s\n", panel[f]);
        }
    }

    return 0;
}

/*FN****************************************************************************
*
*   int Tetris_Integrantes( void )
*
*   Qué hace:  Muestra el borde inferior del tablero y la lista de los
*              integrantes del grupo en el panel lateral derecho.
*
*   Retorna:   0 siempre
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 23/26      Andrés Felipe Trujillo
*   MAR 23/26      Sofia Vega
*   MAR 23/26      Juan Sanchez
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
int Tetris_Integrantes(void) {

    printf("<!========================!>   +--------------------+\n");
    printf("                               |  INTEGRANTES       |\n");
    printf("                               |  %-18s|\n", g_integrantes[0]);
    printf("                               |  %-18s|\n", g_integrantes[1]);
    printf("                               |  %-18s|\n", g_integrantes[2]);
    printf("                               +--------------------+\n");
    printf("\n");
    printf("Presione A, D, W, S o Q.\n");

    return 0;
}

/*FN****************************************************************************
*
*   int Tetris_DibujarPantalla( void )
*
*   Qué hace:  Dibuja toda la pantalla del demo: encabezado, tablero
*              con la pieza e integrantes del grupo.
*
*   Retorna:   0 siempre
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
int Tetris_DibujarPantalla(void) {

    Tetris_LimpiarPantalla();
    Tetris_Encabezado();
    Tetris_Tablero();
    Tetris_Integrantes();
    fflush(stdout);

    return 0;
}

/*FN****************************************************************************
*
*   int Tetris_ProcesarEntrada( void )
*
*   Qué hace:  Lee una tecla del teclado y hace la acción que le corresponde
*              en el juego (moverse, rotar, bajar o salir).
*
*   Retorna:   0 siempre
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
int Tetris_ProcesarEntrada(void) {

    int tecla = Tetris_LeerTecla();

    switch (tecla) {

        case 'a':
        case 'A':
            Tetris_MoverHorizontal(-1);
            break;

        case 'd':
        case 'D':
            Tetris_MoverHorizontal(1);
            break;

        case 'w':
        case 'W':
            Tetris_RotarPieza();
            break;

        case 's':
        case 'S':
            Tetris_BajarPieza();
            break;

        case 'q':
        case 'Q':
            g_juegoActivo = 0;
            break;

        default:
            break;
    }

    return 0;
}

/*FN****************************************************************************
*
*   int Tetris_ActualizarAnimacion( void )
*
*   Qué hace:  Hace bajar la pieza sola cada cierto tiempo usando un contador
*              en milisegundos para controlar la velocidad de caída.
*
*   Retorna:   0 siempre
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
int Tetris_ActualizarAnimacion(void) {

    long tiempoActual = Tetris_TiempoActualMs();

    if ((tiempoActual - g_ultimoDescensoMs) >= CAIDA_MS) {
        Tetris_BajarPieza();
        g_ultimoDescensoMs = tiempoActual;
    }

    return 0;
}

/*FN****************************************************************************
*
*   int Tetris_InicializarJuego( void )
*
*   Qué hace:  Prepara todo el estado inicial del juego antes de entrar
*              al ciclo principal del demo del Hito 2.
*
*   Retorna:   0 siempre
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
int Tetris_InicializarJuego(void) {

    Tetris_ReiniciarPieza();
    g_ultimoDescensoMs = Tetris_TiempoActualMs();

    return 0;
}

/*FN****************************************************************************
*
*   int main( void )
*
*   Qué hace:  Punto de entrada del programa. Configura la consola, arranca
*              el juego y sigue actualizando la pantalla en un ciclo hasta
*              que el usuario presione Q para salir.
*
*   Retorna:   0 siempre
*
*   Registro Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 23/26      Andrés Felipe Trujillo
*   MAR 23/26      Sofia Vega
*   MAR 23/26      Juan Sanchez
*   MAR 24/26      Andrés Felipe Trujillo
*******************************************************************************/
int main(void) {

    if (Tetris_ConfigurarTerminal() == -1) {
        printf("Error al configurar la consola.\n");
        return 1;
    }

    atexit(Tetris_SalidaPrograma);

    Tetris_InicializarJuego();

    while (g_juegoActivo) {
        Tetris_ProcesarEntrada();
        Tetris_ActualizarAnimacion();
        Tetris_DibujarPantalla();
        Sleep(RETARDO_MS);
    }

    return 0;
}