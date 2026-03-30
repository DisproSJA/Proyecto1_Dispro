#ifndef DRIVER_AVR_H
#define DRIVER_AVR_H

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include "../tetris/config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BTN_ROT_BIT   PD2
#define BTN_IZQ_BIT   PD3
#define BTN_BAJ_BIT   PD4
#define BTN_DER_BIT   PD5

#define DATA_BIT      PD6
#define RESET595_BIT  PD7

#define CLOCK_BIT     PB0
#define LATCH_BIT     PB1
#define OE_BIT        PB2

    typedef struct {
        bool izq;
        bool der;
        bool rot;
        bool baj;
    } BotonesEstado;

    typedef struct {
        uint8_t estableActual;
        uint8_t ultimaLectura;
        uint32_t ultimoCambioMs;
    } DebounceBotones;

    void driver_avr_inicializarHardware( void );
    void driver_avr_apagarTodo( void );

    void driver_avr_refrescarDisplay(
        const uint8_t framebuffer[ALTO_TABLERO][ANCHO_TABLERO] );

    uint8_t driver_avr_leerBotonesCrudo( void );

    void driver_avr_actualizarBotones( DebounceBotones *debounce,
                                       BotonesEstado *botones,
                                       uint32_t tiempoActualMs );

    uint32_t driver_avr_get_millis( void );
    void driver_avr_delay_us( uint16_t tiempoUs );

#ifdef __cplusplus
}
#endif

#endif