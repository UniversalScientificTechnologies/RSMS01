// DART01A verze programu 1.00
// (c)miho 2005

#include <16F88.h>                     // standardni definice konstant
#include <16F88_Reg.h>                 // standardni definice vsech FSR

#device adc=8                          // AD prevodnik jen na 8 bitu
#define xclock 4168000                 // muj konkretni PIC je trochu rychlejsi
#use delay(clock=xclock,restart_wdt)
#fuses INTRC_IO, WDT, NOPUT, NOPROTECT, NOBROWNOUT, MCLR, NOLVP, NOCPD, NODEBUG, CCPB3
