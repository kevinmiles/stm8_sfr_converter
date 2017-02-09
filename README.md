# iarSTM8sfr2cosmic
This is an utility for converting *.sfr files supplied by IAR STM8 Embedded WorkBench include files to bu used with Cosmic compiler

Requirements: 
Qt 5.8

Compile: 
qmake 
make

Usage:

Install IAR EWB STM8 (Evaluation version is OK)

Select the proper sfr file from the following folder:

C:\Program Files (x86)\IAR Systems\Embedded Workbench 7.3\stm8\config\ddf

or 

C:\Program Files\IAR Systems\Embedded Workbench 7.3\stm8\config\ddf

on 32 bit systems

Then run:

stm8_headergen /home/mm/iostm8af5286.sfr /home/mm/iostm8af5286.h
