// Program STOPKY pro mereni casu prujezdu mezi cidlama pro souteze solarnich robotu
//
// (c) miho 2004
//
// Historie
//
// 1.00 Uvodni verze

#define VER "1.00"

// Definice zakladnich parametru
//
#include <16F84.h>                     // procesor
#use delay(clock=4000000)              // krystal
#use rs232(baud=9600, xmit=PIN_B4)     // seriovka a jeji parametry
#fuses XT,PUT,NOWDT,NOPROTECT          // prepinace


// Definice pro pripojeni LCD displeje
//
#define LCD_RS          PIN_A2         // Signal RS
#define LCD_E           PIN_A3         // Signal E
#define LCD_D0          PIN_A4         // Data
#define LCD_D1          PIN_B1
#define LCD_D2          PIN_B2
#define LCD_D3          PIN_B3
#include <LCD.C>


// Definice pro pripojeni vstupu (cidel)
//
#define BUTTON_START    PIN_B7         // Tlacitko START
#define BUTTON_STOP     PIN_B6         // Tlacitko STOP
#define BUTTON_TEST     PIN_B5         // Tlacitko TEST pro prechod do testovaciho rezimu


// Defince pro pripojeni vystupu pro pipak
//
#define BEEP_PORT_P     PIN_A0         // Piezo pipak pozitivni
#define BEEP_PORT_N     PIN_A1         // Piezo pipak negativni


// Definice globalnich promennych
//
unsigned int Beep;                     // citac pulperiod pro pipnuti
short int Beep_state;                  // stav pipaku (0 nebo 1)
unsigned int State_run;                // 0 .. klid
                                       // 1 .. bezi mereni casu
                                       // 2 .. cas zmereny


// Obsluha vstupu cidel
//
// Cas se meri od signalu na tlacitku START po signal na tlacitku STOP
// Signaly na tlacitkach vyvolaji preruseni a dojde k preklopeni stavu v promenne State_run
//
#int_RB
RB_isr()
{
   if(State_run)
   {
      if(~input(BUTTON_STOP))
      {
         State_run=2;                  // Domereno
         Beep=100;
         disable_interrupts(INT_RB);   // Zakaz vstup tlacitek aby byl cas na zpracovani
      }
   }
   else
   {
      if(~input(BUTTON_START))
      {
         State_run=1;
         Beep=100;
      }
   }
}


// Obsluha preruseni od casovace
//
unsigned int32 Time;                   // citac casu (perioda 256us)

#int_RTCC
RTCC_isr()
{
   if(State_run==1) Time++;            // zvetsi citac

   if(Time==(3600000000>>8)) Time=0;   // Osetreni preteceni po 1 hodine

   if(Beep)                            // pokud se ma pipnout pipni
   {                                   //  Beep je pocet pulperiod
      Beep--;
      output_bit(BEEP_PORT_P,Beep_state);
      Beep_state=~Beep_state;
      output_bit(BEEP_PORT_N,Beep_state);
   }
}


// Obsluha vystupu a jeho presmerovani na LCD a RS232
//
int PutCharMode;
#define LCD    1
#define RS232  2
void PutChar(char c)
{
   if(PutCharMode & LCD)   LCD_Putc(c);
   if(PutCHarMode & RS232) putc(c);
}


// Vypis casu - dlouhy program ale jen na jedinem miste
//
void DisplayCas()
{
   int32 xTime;

   // Tisk vysledku v ms
   // printf(PutChar,"Cas:%8lums\n\r",(Time<<8)/1000);

   // Tisk vysledku v s
   xTime=(Time<<8)/1000;
   printf(PutChar,"Cas:%4lu.%03lus\n\r",xTime/1000,xTime%1000);
}


// Testovaci rezim
//
void TestMode()
{

   int T;

   // Otestuj tlascitko TEST
   if(input(BUTTON_TEST)) return;

   // Vypis na displej
   printf(LCD_Putc,"\fTest Mode\n");

   for(;1;)
   {
      T=0;
      if(~input(BUTTON_START))   T+=1;
      if(~input(BUTTON_STOP))    T+=2;
      printf(LCD_putc,"\n%d",T);
      if(T)
      {
         if(T==3) T=4;
         output_bit(BEEP_PORT_P,0);
         output_bit(BEEP_PORT_N,1);
         delay_ms(T);
         output_bit(BEEP_PORT_P,1);
         output_bit(BEEP_PORT_N,0);
         delay_ms(T);
      }
   }
}


// Hlavni program
//
void main()
{
   // Inicializace LCD displeje
   lcd_init();
   PutCharMode=LCD|RS232;
   printf(PutChar,"\n\rStopky v."VER"\r\n(c) miho 2004\n\r");
   delay_ms(100);

   // Inicializace stavovych promennych
   State_run=0;                        // stopky stoji

   // Inicializace vstupu (preruseni od zmeny stavu)
   input(BUTTON_START);                // precti RB aby se inicializoval registr zmen RB
   port_b_pullups(TRUE);               // zapni pull up odpory

   // Testovaci mod
   TestMode();

   // Inicializace casovace pod prerusenim
   setup_counters(RTCC_INTERNAL,WDT_18MS);      // Perioda je 256 instrukcnich taktu

   // Hlavni smycka porad dokola
   for(;1;)
   {

      // Nuluj citace
      Time=0;                          // nulovy cas
      Beep=0;                          // nepipej
      State_run=0;

      // Povoleni preruseni
      enable_interrupts(INT_RTCC);     // povol preruseni od casovace
      enable_interrupts(INT_RB);       // povol preruseni od zmeny portu RB
      enable_interrupts(global);       // povol preruseni

      // Cekej az se zacne merit
      for(;State_run==0;)
      {
      }

      // Prubezne vypisuj cas
      PutCharMode=LCD;                 // vystup jen na LCD
      for(;State_run==1;)
      {
         DisplayCas();
      }

      // Vypis vysledny cas - na LCD hned
      DisplayCas();

      // Vypis vysledny cas - na RS232 az po pipnuti
      delay_ms(100);                   // ted se pod prerusenim pipa
      PutCharMode=RS232;
      disable_interrupts(global);      // zakaz preruseni (aby fungovalo casovani seriovky)
      DisplayCas();
      enable_interrupts(global);       // opet povol preruseni

   }
}
