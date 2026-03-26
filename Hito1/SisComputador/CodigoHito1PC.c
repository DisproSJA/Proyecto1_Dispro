#include <stdio.h>
#include "Tetris.h"

/*FN****************************************************************************
*
*   int Tetris_Encabezado( void )
*
*   Propósito: Imprime el marco del título del tablero de Tetris junto con
*              el borde superior del panel de información lateral derecho.
*
*   Retorno:   0 - siempre (éxito)
*
*   Registro de Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 23/26  Andrés Felipe Trujillo 
*   MAR 23/26  Sofia Vega
*   MAR 23/26  Juan Sanchez
*******************************************************************************/

int Tetris_Encabezado(void) {

    printf("\n");
    printf("<!==============================!>   +--------------------+\n");
    printf("<!      T  E  T  R  I  S       !>   |  TETRIS !DISPRO!   |\n");
    printf("<!==============================!>   +--------------------+\n");

    return 0;
}

/*FN****************************************************************************
*
*   int Tetris_Tablero( void )
*
*   Propósito: Imprime el tablero de juego de 20 filas (10 columnas de ancho)
*              junto al panel lateral derecho que muestra puntaje, nivel, hora,
*              siguiente pieza y controles. Las celdas vacías se muestran como
*              " . " y las ocupadas como "[C]". El panel usa un arreglo de
*              cadenas terminado en NULL, indexado en sincronía con cada fila.
*
*   Retorno:   0 - siempre (éxito)
*
*   Registro de Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 23/26  Andrés Felipe Trujillo 
*   MAR 23/26  Sofia Vega
*   MAR 23/26  Juan Sanchez
*******************************************************************************/

int Tetris_Tablero(void) {

/* Parte 1: Estado del tablero - 0 = celda vacía, 1 = bloque ocupado */

    int tablero[20][10] = {
        {0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,0,0,0,0,0},   /* Pieza-T cayendo - celda superior */
        {0,0,0,1,1,1,0,0,0,0},   /* Pieza-T cayendo - base           */
        {0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0},
        {0,1,0,0,0,0,0,0,0,0},
        {1,1,1,1,1,0,0,1,0,0},
        {1,1,0,1,1,1,1,1,0,1},
        {1,1,1,0,1,1,1,1,1,1}
    };

/* Parte 2: Panel lateral derecho - terminado en NULL, una entrada por fila del tablero */

    const char *panel[] = {
        "|  PUNTUACION        |",
        "|    001,250  pts    |",
        "+--------------------+",
        "|  NIVEL:   3        |",
        "|  LINEAS:  15       |",
        "+--------------------+",
        "|  HORA              |",
        "|    14:35:22        |",
        "+--------------------+",
        "|  SIGUIENTE PIEZA   |",
        "|                    |",
        "|    [C][C]          |",
        "|    [C][C]          |",
        "|                    |",
        "+--------------------+",
        "|  CONTROLES         |",
        "|  7:IZQ   9:DER     |",
        "|  8: ROTAR          |",
        "|  4:ACELERAR        |",
        "|  5:SOLTAR/CAER     |",
        NULL
    };

/* Parte 3: Imprime cada fila del tablero emparejada con su línea del panel */

    for (int f = 0; f < 20; f++) {
        printf("<!");
        for (int c = 0; c < 10; c++) {
            printf(tablero[f][c] ? "[C]" : " . ");
        }
        printf("!>   %s\n", panel[f]);
    }

    return 0;
}

/*FN****************************************************************************
*
*   int Tetris_Integrantes( void )
*
*   Propósito: Imprime el borde inferior del tablero de juego y la lista de
*              integrantes del proyecto en el panel lateral derecho.
*
*   Retorno:   0 - siempre (éxito)
*
*   Registro de Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 23/26  Andrés Felipe Trujillo 
*   MAR 23/26  Sofia Vega
*   MAR 23/26  Juan Sanchez  
*******************************************************************************/

int Tetris_Integrantes(void) {

    printf("<!==============================!>   +--------------------+\n");
    printf("                                     |  INTEGRANTES       |\n");
    printf("                                     |  Sofia Vega        |\n");
    printf("                                     |  Juan Sanchez      |\n");
    printf("                                     |  Andres Trujillo   |\n");
    printf("                                     +--------------------+\n\n");

    return 0;
}

/*FN****************************************************************************
*
*   int main( void )
*
*   Propósito: Punto de entrada. Llama cada función de visualización en
*              secuencia para renderizar la interfaz ASCII estática completa
*              del Tetris.
*
*   Retorno:   0 - siempre (éxito)
*
*   Registro de Revisiones:
*
*   FECHA          RESPONSABLE
*   -----------------------------------------------------------------------
*   MAR 23/26  Andrés Felipe Trujillo 
*   MAR 23/26  Sofia Vega
*   MAR 23/26  Juan Sanchez
*******************************************************************************/

int main(void) {

    Tetris_Encabezado();
    Tetris_Tablero();
    Tetris_Integrantes();

    return 0;
}
