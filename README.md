# Tetris Multi-Plataforma — Proyecto 1 DISPRO

Proyecto de **Diseño de Sistemas con Procesador (DISPRO)** que consiste en desarrollar un motor de juego de Tetris modular escrito en C, capaz de ejecutarse tanto en un computador personal (PC) como en un microcontrolador ATmega328P en modo stand-alone con matrices LED de 8×8.

## Integrantes

| Nombre                 |
|------------------------|
| Sofia Vega             |
| Juan Sanchez           |
| Andrés Felipe Trujillo |

## Descripción del Proyecto

El juego de Tetris se diseñó con alta portabilidad en mente, separando la lógica del juego de la capa de hardware (HAL). Se compila para dos plataformas distintas:

- **PC:** Versión en consola usando caracteres ASCII.
- **Sistema Embebido:** ATmega328P controlando dos matrices LED 8×8 mediante registros de desplazamiento 74HC595.

## Estructura del Repositorio

```
Proyecto1_Dispro/
│
├── README.md
│
├── Hito1/                          # Hito 1 - Control de Matriz y Driver 74HC595
│   ├── Paper/
│   │   └── TetrisMultiplataform_Hito1.pdf
│   ├── SisComputador/
│   │   └── CodigoHito1PC.c         # Versión PC: imagen estática en consola
│   └── SisEmbebido/
│       └── CodigoHito1SE.ino        # Versión embebida: imagen estática en matrices LED
│
├── Hito2/                          # Hito 2 - Animación y Gestión de Entradas
│   └── SisComputador/
│       └── CodifoHito2PC.c          # Versión PC: animación con pieza T y controles
│
└── EntregaFinal/                   # Entrega Final - Sistema Tetris Completo
    ├── SisComputador/               # (Pendiente)
    └── SisEmbebido/                 # (Pendiente)
```

## Hitos del Proyecto

### Hito 1 — Control de Matriz y Driver 74HC595
- Mostrar una imagen estática (corazón y cara feliz) en las dos matrices LED 16×8.
- Mostrar la misma figura en consola con caracteres ASCII.
- Implementar el driver de bajo nivel para el 74HC595.

### Hito 2 — Animación y Gestión de Entradas
- Animación de una pieza T con caída automática y controles de teclado (A/D/W/S/Q).
- Lectura de 4 teclas en el sistema embebido directamente desde los registros PINx.
- Uso de Timer para la base de tiempo de la animación y debounce en botones.

### Entrega Final — Sistema Tetris Completo
- Lógica completa del Tetris: generación aleatoria de piezas, colisiones, eliminación de líneas y puntaje.
- Código unificado con directivas `#ifdef PLATFORM_AVR` y `#ifdef PLATFORM_PC`.

## Tecnologías y Herramientas

- **Lenguaje:** C (ANSI C)
- **PC:** Compilado con GCC (consola Windows/Linux)
- **Embebido:** ATmega328P (stand-alone, cristal 16 MHz)
- **Hardware:** Matrices LED 8×8, registros de desplazamiento 74HC595, protoboard

## Cómo Compilar
### Versión PC (Hito 1)
```bash
gcc CodifoHito1PC.c -o Tetris1.exe
```

### Versión PC (Hito 2)
```bash
gcc CodifoHito2PC.c -o Tetris1.exe
```

### Versión Embebida
Se programa el ATmega328P usando el IDE de Arduino.
