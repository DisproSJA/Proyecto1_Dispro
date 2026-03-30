#ifndef CONFIG_H
#define CONFIG_H

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define PLATFORM_AVR

#define ANCHO_TABLERO  8
#define ALTO_TABLERO   16

#define BLOQUE_PIEZA   4
#define NUM_ROTACIONES 4
#define NUM_TETROMINOS 7

#endif