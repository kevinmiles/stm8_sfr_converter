# stm8_headergen
This is an utility for converting *.sfr/*.ddf files supplied by IAR STM8 Embedded WorkBench to include files for other toolchains.

Requirements: 
Qt 5.8

# Compile: 
qmake && make

# Usage:

Install IAR EWB STM8 (Evaluation version is OK)

Select the proper sfr file from the following folder:

C:\Program Files (x86)\IAR Systems\Embedded Workbench 7.3\stm8\config\ddf

or 

C:\Program Files\IAR Systems\Embedded Workbench 7.3\stm8\config\ddf

on 32 bit systems

# Header generation for the Cosmic C compiler:

stm8_headergen -sfr iostm8af5286.sfr -header iostm8af5286.h -type cosmic

It will generate the registers bitfields in the following format:
```
typedef struct
{
  unsigned char IDR0        : 1;
  unsigned char IDR1        : 1;
  unsigned char IDR2        : 1;
  unsigned char IDR3        : 1;
  unsigned char IDR4        : 1;
  unsigned char IDR5        : 1;
  unsigned char IDR6        : 1;
  unsigned char IDR7        : 1;
} __BITS_PA_IDR;
volatile __BITS_PA_IDR  PA_IDR_bits					@0x5001;
volatile char PA_IDR					@0x5001;
```

# Header generation for SDCC C compiler:
stm8_headergen -sfr iostm8af5286.sfr -ddf iostm8af5286.ddf -header iostm8af5286.h -type sdcc

It will generate the registers bitfields in the following format:
```
volatile unsigned char __at(0x5000) PA_ODR; // Port A data output latch register
typedef struct
{
    unsigned char ODR0                  : 1;
    unsigned char ODR1                  : 1;
    unsigned char ODR2                  : 1;
    unsigned char ODR3                  : 1;
    unsigned char ODR4                  : 1;
    unsigned char ODR5                  : 1;
    unsigned char ODR6                  : 1;
    unsigned char ODR7                  : 1;
} __BITS_PA_ODR;
volatile __BITS_PA_ODR __at(0x5000) PA_ODR_bits;
```

It will also generate the interrupt vector ISR numbers (if the -ddf argument ius supplied):
```
// Offsets for the interrupt vector table
#define AWU_vector  1
#define CLK_CSS_vector 2
#define CLK_SWITCH_vector 2
#define EXTI0_vector 3
#define EXTI1_vector 4
#define EXTI2_vector 5
#define EXTI3_vector 6
#define EXTI4_vector 7
#define SPI_CRCERR_vector 10
#define SPI_MODF_vector 10
#define SPI_OVR_vector 10
#define SPI_RXNE_vector 10
#define SPI_TXE_vector 10
#define SPI_WKUP_vector 10
```
