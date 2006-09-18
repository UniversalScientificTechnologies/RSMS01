// Knihovna pro generovani hudebnich zvuku dane frekvence a delky nebo
// dane noty temperovaneho ladeni a delky.
//
// Pro gnerovani pouziva casovac T0, T1 a jednotku CCP1.
//
// POZOR -- POZOR -- POZOR
// Pri nizsi frekvenci XTALu (nez asi 5MHz) je rezie preruseni tak velka, ze spotrebuje
// veskery strojovy cas a uz nedojde k ukonceni rozehraneho tonu vyssi frekvence (z oktavy 7
// nebo nad cca 8KHz). Resenim je pouzit vlastni INT proceduru s minimalni rezii.
//
// (c)miho 2004, pefi 2004,
//
// Historie
// 1.00 Uvodni verze
// 1.01 Pridana podpora zapnuteho WDT

// Konfiguracni parametry
//#define SOUND_HI     PIN_xx          // Pozitivni vystup
//#define SOUND_LO     PIN_xx          // Komplementarni vystup
//#define SOUND_WDT    1               // Pokud je pouzit WDT
#ifndef SOUND_CLOCK
#define SOUND_CLOCK  4000000           // Frelvence krystalu v Hz
#endif


// Definice hudebnich tonu (not) pro proceduru SoundNote()
#define SOUND_C     0
#define SOUND_Cis   1
#define SOUND_D     2
#define SOUND_Dis   3
#define SOUND_E     4
#define SOUND_F     5
#define SOUND_Fis   6
#define SOUND_G     7
#define SOUND_Gis   8
#define SOUND_A     9
#define SOUND_Ais   10
#define SOUND_H     11
#define SOUND_Space 12                 // Pomlka


// Prototypy verejnych procedur

void SoundBeep(unsigned int16 Frequency, unsigned int16 Duration);
// Predava se frekvence v Hz a doba trvani v ms (0 znamena ticho)

void SoundNote(unsigned int8 Note, Octave, unsigned int16 Duration);
// Predava se cislo noty (0 je C), posunuti v oktavach (0 nejnizsi ton,
// SOUND_Space je ticho), doba trvani v ms

// Alternativni makra pro generovani konstatnich tonu
// SoundBeepMacro(Frequency, Duration) - frekvence nesmi byt 0
// SoundNoteMacro(Note, Octave, Duration) - nepodporuje SOUND_Space
// SoundSpaceMacro(Duration) - hraje ticho

// Privatni cast


#int_ccp1
void IntCCP1()
// Preruseni od jednotky CCP1 generuje vystup
{
   volatile int1 Data;                    // Posledni stav vystupu
   output_bit(SOUND_HI,Data);             // Nastav vystup
   output_bit(SOUND_LO,~Data);
   Data=~Data;                            // Otoc stav vystupu
}


#if      SOUND_CLOCK < (65535*32*4*2)     // < 16.7 MHz
#define  SOUND_PRESCALE 1
#elif    SOUND_CLOCK < (65535*32*4*2*2)   // < 33.6 MHz
#define  SOUND_PRESCALE 2
#elif    SOUND_CLOCK < (65535*32*4*2*4)   // < 67.1 MHz
#define  SOUND_PRESCALE 4
#elif    SOUND_CLOCK < (65535*32*4*2*8)   // < 134 MHz
#define  SOUND_PRESCALE 8
#else
#error SOUND_CLOCK Frequency too high
#endif
#bit T0IF=0x0B.2
void SoundBeep(unsigned int16 Frequency, unsigned int16 Duration)
// Predava se frekvence v Hz a doba trvani v ms (0 znamena ticho)
// Rozumne frekvence jsou od 32Hz
{
   unsigned int16 Time;                   // Pocitadlo zlomkoveho casu

   // Inicializace casovace
   if (Frequency!=0)                      // Frekvence 0 znamena ticho
   {
      setup_ccp1(CCP_COMPARE_RESET_TIMER);
      set_timer1(0);
      (int16)CCP_1=SOUND_CLOCK/SOUND_PRESCALE/4/2/Frequency;
      #if   SOUND_PRESCALE==1
      setup_timer_1(T1_INTERNAL|T1_DIV_BY_1);
      #elif SOUND_PRESCALE==2
      setup_timer_1(T1_INTERNAL|T1_DIV_BY_2);
      #elif SOUND_PRESCALE==4
      setup_timer_1(T1_INTERNAL|T1_DIV_BY_4);
      #elif SOUND_PRESCALE==8
      setup_timer_1(T1_INTERNAL|T1_DIV_BY_8);
      #endif
      enable_interrupts(int_ccp1);
      enable_interrupts(global);
   }

   // Delka tonu merena casovacem T0 (bez preruseni)
   Time=0;
   setup_timer_0(RTCC_INTERNAL|RTCC_DIV_1);
   while (Duration)
   {
      #ifdef   SOUND_WDT
         restart_wdt();                   // Nuluj watchdog abychom se nezresetovali
      #endif
      if (T0IF)                           // Preteceni T0 - kazdych (1/CLK)*4*256
      {
         T0IF = 0;
         Time += (256*4*1000*2048+SOUND_CLOCK/2)/SOUND_CLOCK;
      }
      if (Time>>11)
      {
         Time -= 2048;
         Duration--;
      }
   }

   // Konec casovace
   disable_interrupts(int_ccp1);
   disable_interrupts(global);
}


// Definice casu pulperody pro nejnizsi oktavu, v kvantech casovace T1
// Pulperiody tonu v dalsich oktavach se ziskavaji rotaci vpravo
#define SOUND_Peri_C   ((30578*(SOUND_CLOCK/1000)/1000/4/2)/SOUND_PRESCALE) // Perioda 30578us
#define SOUND_Peri_Cis ((28862*(SOUND_CLOCK/1000)/1000/4/2)/SOUND_PRESCALE) // Perioda 28862us
#define SOUND_Peri_D   ((27242*(SOUND_CLOCK/1000)/1000/4/2)/SOUND_PRESCALE) // Perioda 27242us
#define SOUND_Peri_Dis ((25713*(SOUND_CLOCK/1000)/1000/4/2)/SOUND_PRESCALE) // Perioda 25713us
#define SOUND_Peri_E   ((24270*(SOUND_CLOCK/1000)/1000/4/2)/SOUND_PRESCALE) // Perioda 24270us
#define SOUND_Peri_F   ((22908*(SOUND_CLOCK/1000)/1000/4/2)/SOUND_PRESCALE) // Perioda 22908us
#define SOUND_Peri_Fis ((21622*(SOUND_CLOCK/1000)/1000/4/2)/SOUND_PRESCALE) // Perioda 21622us
#define SOUND_Peri_G   ((20408*(SOUND_CLOCK/1000)/1000/4/2)/SOUND_PRESCALE) // Perioda 20408us
#define SOUND_Peri_Gis ((19263*(SOUND_CLOCK/1000)/1000/4/2)/SOUND_PRESCALE) // Perioda 19263us
#define SOUND_Peri_A   ((18182*(SOUND_CLOCK/1000)/1000/4/2)/SOUND_PRESCALE) // Perioda 18182us
#define SOUND_Peri_Ais ((17161*(SOUND_CLOCK/1000)/1000/4/2)/SOUND_PRESCALE) // Perioda 17161us
#define SOUND_Peri_H   ((16198*(SOUND_CLOCK/1000)/1000/4/2)/SOUND_PRESCALE) // Perioda 16198us

// Kontrola na delku konstanty (musi byt mensi nez delka citace)
#if SOUND_Peri_C > 65535
#error "SOUND_Peri_C too long (Note C requires delay > 65535 cycles)"
#endif

// Casove konstanty pro noty nejnizsi oktavy
const unsigned int16 Table[12] =
                        SOUND_Peri_C,     SOUND_Peri_Cis,   SOUND_Peri_D,     SOUND_Peri_Dis,
                        SOUND_Peri_E,     SOUND_Peri_F,     SOUND_Peri_Fis,   SOUND_Peri_G,
                        SOUND_Peri_Gis,   SOUND_Peri_A,     SOUND_Peri_Ais,   SOUND_Peri_H;


void SoundNote(unsigned int8 Note, Octave, unsigned int16 Duration)
// Predava se cislo noty (0 je C), posunuti v oktavach (0 nejnizsi ton)
// doba trvani v ms (0 znamena ticho)
// Zahraje zadanou notu v zadane oktave dane delky
{
   unsigned int16 Time;                   // Pocitadlo zlomkoveho casu

   // Inicializace casovace
   if (Note!=SOUND_Space)                 // Pokud se ma hrat spust hrani
   {
      setup_ccp1(CCP_COMPARE_RESET_TIMER);
      set_timer1(0);
      (int16)CCP_1=Table[Note]>>Octave;
      #if   SOUND_PRESCALE==1
      setup_timer_1(T1_INTERNAL|T1_DIV_BY_1);
      #elif SOUND_PRESCALE==2
      setup_timer_1(T1_INTERNAL|T1_DIV_BY_2);
      #elif SOUND_PRESCALE==4
      setup_timer_1(T1_INTERNAL|T1_DIV_BY_4);
      #elif SOUND_PRESCALE==8
      setup_timer_1(T1_INTERNAL|T1_DIV_BY_8);
      #endif
      enable_interrupts(int_ccp1);
      enable_interrupts(global);
   }

   // Delka tonu merena casovacem T0 (bez preruseni)
   Time=0;
   setup_timer_0(RTCC_INTERNAL|RTCC_DIV_1);
   while (Duration)
   {
      if (T0IF)                           // Preteceni T0 - kazdych (1/CLK)*4*256
      {
         T0IF = 0;
         Time += (256*4*1000*2048+SOUND_CLOCK/2)/SOUND_CLOCK;
      }
      if (Time>>11)
      {
         Time -= 2048;
         Duration--;
      }
      #ifdef   SOUND_WDT
         restart_wdt();                   // Nuluj watchdog abychom se nezresetovali
      #endif
   }

   // Konec casovace
   disable_interrupts(int_ccp1);
   disable_interrupts(global);
}
