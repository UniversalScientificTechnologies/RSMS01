// LCD Terminal pro menic pro solarni vozitko
// (c)miho 2005

// Pouziva PIC16F84 (mozno i jakykoli jiny, nepouzivaji se zadne specialni funkce).
// Na vstupnim vyvodu RS_IN ocekava seriovou komunikaci rychlosti RS_BOUD. Pri detekci
// start bitu pomoci preruseni dojde k programovemu prijmu znaku a jeho zarazeni do
// fronty FIFO. Po navratu z preruseni dochazi k vypisu na pripojeny dvouradkovy
// LCD displej. Program zpracovava ridici kody dle knihovny LCD.C. Pri delsich vypisech
// musi vysilajici strana ponechat (obcas) nejaky cas na zpracovani (napriklad 20ms).

#include <16F84.h>                  // define standardnich konstant procesoru
#use delay(clock=4000000)           // standardni krystal
#fuses HS, NOWDT, NOPUT, NOPROTECT


// Parmetry komuniace
//
#define INV                         // definuje polaritu
#define RS_BOUD         9600        // komunikacni rychlost
#define RS_IN           PIN_B0      // musi to byt vstup extrniho preruseni


// Pripojeni LCD displeje
//
#define LCD_RS          PIN_A0      // rizeni registru LCD displeje
#define LCD_E           PIN_A1      // enable LCD displeje
#define LCD_DATA_LSB    PIN_B4      // pripojeni LSB bitu datoveho portu LCD displeje (celkem 4 bity vzestupne za sebou)

#include "LCD.C"


// Vstup seriovky
//
#ifdef INV
#use RS232 (BAUD=RS_BOUD, RCV=RS_IN, PARITY=N, INVERT)
#else
#use RS232 (BAUD=RS_BOUD, RCV=RS_IN, PARITY=N)
#endif


// Buffer FIFO
//
#define MAX 40                      // delka bufferu

char c[MAX];                        // bufer FIFO
unsigned int ci;                    // ukazatel na bunku kam se bude ukladat novy znak
unsigned int co;                    // ukazatel na bunku odkud se bude cist znak

// Preruseni - ukladani dat ze seriovky do bufferu
//
#int_ext                               // preruseni od zacatku znaku (start bit)
void Interupt()
{
   c[ci]=getc();                       // nacti znak (asynchronni cteni programem)

   if (ci<(MAX-1)) ci++; else ci=0;    // posun ukazovatko do FIFO

   #ifdef INV
   while(input(PIN_B0));               // pockej na konec posledniho bitu
   #else
   while(~input(PIN_B0));              // pockej na konec posledniho bitu
   #endif
}


// Hlavni smycka
//
void main()
{
   char ch;                               // pomocna promenna pro 1 znak

   // Inicializace portu
   output_a(0);                           // vsechny porty vystupni
   output_b(0);                           // a nulove krome
   output_float(RS_IN);                   // portu pro RS232 (a preruseni)

   // Inicializace LCD
   lcd_init();                            // inicializace LCD
   printf(lcd_putc,"LCD Terminal 1.0");   // standardni vypis
   #ifdef INV
   printf(lcd_putc,"\nInverted");         // oznameni o inverzni variante
   #else
   printf(lcd_putc,"\nStandard");         // oznameni o inverzni variante
   #endif
   delay_ms(300);                         // cas na precteni
   printf(lcd_putc,"\f");                 // smazani displeje

   // Inicializace FIFO ukazatelu
   ci=0;
   co=0;

   // Inicializace preruseni
   #ifdef INV                             // dle polarity kominkace polarita preruseni
   ext_int_edge(L_TO_H);
   #else
   ext_int_edge(H_TO_L);
   #endif
   enable_interrupts(int_ext);            // povoleni preruseni od INT0
   enable_interrupts(global);             // povoleni globalniho preruseni

   // Hlavni smycka
   while (1)
   {

      // Test na neprazdny buffer
      while (ci==co);

      // Zobrazeni znaku
      lcd_putc(c[co]);

      // Posunuti ukazovatka
      if (co<(MAX-1)) co++; else co=0;
   }
}
