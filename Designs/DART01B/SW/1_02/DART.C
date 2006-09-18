// DART01A verze programu 1.02
// (c)miho 2005
//
// 1.00 Uvodni verze
// 1.01 Doplneno nasatvovani parametru rozjezdu P2 u standardniho algoritmu
// 1.02 Doplnena deaktivace vyvodu pro LED (LED tato verze programu nepouziva)

#include "DART.h"


#define BEEP0     PIN_A6   // pipak, prvni vystup
#define BEEP1     PIN_A7   // pipak, druhy vystup
#define PWM       PIN_B3   // PWM vystup pro menic
#define REFPOWER  PIN_B1   // napajeni zdroje Vref
#define MOTOR     PIN_B2   // zapinani motoru
#define SW0       PIN_B7   // konfiguracni prepinac 0
#define SW1       PIN_B6   // konfiguracni prepinac 1
#define LED       PIN_B4   // dioda LED v elektornice DART01B


void InitRS232()
// Inicializace HW RS232 (pro ladici vystupy)
{
   SPBRG=xclock/9600/16-1; // ryclost 9600Bd
   RCSTA=0b10000000;       // enable USART
   TXSTA=0b00100100;       // BRGH=1, TX enable
}


void Putc(char c)
// Posilani znaku pres HW RS232
{
   while(TRMT==0);         // cekej na prazdny TX buffer
   TXREG=c;                // predej data
}


// Globalni promenna pro data posilana na SSP
//   Nastavuje se funkci MotorPatternSet()
//   Vyuziva se v prerusovaci rutine IntSSP()
unsigned int8 MotorPattern;   // aktualni data pro SSP jednotku


void MotorPatternSet(unsigned int Gear)
// Na zaklade rychlostniho stupne nastavi MotorPattern pro SSP
// Rychlost 0 znamena stop, rychlost 8 je maximum
{
   // Tabulka rychlost -> pattern pro SSP
   unsigned int8 const ExpTab[8] = {0x02,0x06,0x0e,0x1e,0x3e,0x7e,0xfe,0xff};

   // Vyber patternu
   if (Gear==0)                     // stav 0 znamena stop
   {
      output_low(MOTOR);            // klidovy stav
      SSPSTAT = 0;
      SSPCON1 = 0;                  // SPI stop
      disable_interrupts(INT_SSP);  // neni preruseni od SSP
   }
   else                             // rizeny vykon
   {
      if (Gear>7)                   // stav 8 a vice znamena plny vykon
      {
         Gear=8;                    // plny plyn
      }

      MotorPattern=ExpTab[--Gear];  // prevod z hodnoty plynu na data pro SSP
      output_low(MOTOR);            // klidovy stav
      SSPSTAT = 0;
      SSPCON1 = 0x22;               // SPI OSC/64

      SSPBUF=MotorPattern;          // prvni data pro vyslani
      enable_interrupts(INT_SSP);   // az budou vyslana prijde interrupt od SSP
   }
}


// Obsluha preruseni od SSP jednotky, posila data z promenne MotorRun do SSP.
#INT_SSP
void IntSSP()
{
   SSPBUF=MotorPattern;             // znova hdnota PWM patternu na SSP
}


void MotorSet(unsigned int Gear)
// Nastavi vykon motoru dle hodnoty Gear a zahaji posilani PWM dat pres SSP pod prerusenim
// od SSP jednotky
//    0   stop
//    1-7 pocet 1/8 vykonu
//    >7  plny vykon
{
   // Nastav PWM pattern
   MotorPatternSet(Gear);           // nastav PWM pattern pro SSP

   // Povol preruseni
   enable_interrupts(GLOBAL);       // povol preruseni
}


void InitT0()
// Inicializace casovace T0 (cca 1000x za sekundu)
{
   setup_timer_0(RTCC_INTERNAL|RTCC_DIV_4);  // T0 z internich hodin 1/4
   enable_interrupts(INT_RTCC);              // generuj preruseni od T0
   enable_interrupts(GLOBAL);                // povol preruseni
}


// Globalni promenna pro mereni casu
//   Nastavuje se procedurou TimeSet()
//   Testuje se funkci TimeIf()
//   Modifikuje se pri preruseni od casovace IntTo()
unsigned int16 TimeTime;


void TimerSet(unsigned int16 Time)
// Nastavi casovac na zadany pocet ms
// Test uplynuti casu se dela pomoci TimerIf()
{
   // Nastav hodnotu
   disable_interrupts(INT_RTCC);    // nesmi prijit preruseni
   TimeTime=Time;                   // pri nastavovani hodnoty
   enable_interrupts(INT_RTCC);     // promenne (o delce vice nez 8 bitu)
}


int1 TimerIf()
// Vraci TRUE pokud casovac jiz dobehl
{
   int1 Flag;                       // pomocna promenna

   // Otestuj casovac
   disable_interrupts(INT_RTCC);    // nesmi prijit preruseni
   Flag=(TimeTime==0);              // behem testu promenne
   enable_interrupts(INT_RTCC);     // ted uz muze

   // Navratova hodnota
   return Flag;                     // TRUE znamena dobehl casovac
}


// Globalni promenne pro akceleraci
//   Nastavuje se metodou MotorStart()
//   Pouziva se v obsluze preruseni IntT0()
unsigned int8 MotorTime;            // aktualni casovac pro rozjezd
unsigned int8 MotorDelay;           // spozdeni mezi razenim rychlosti
unsigned int8 MotorGear;            // rychlostni stupen


void MotorStart(unsigned int8 Delay)
// Provede rizeny rozjezd motoru
// Parametrem je prodleva mezi razenim rychlosti v ms
{
   disable_interrupts(INT_RTCC);
   MotorGear=1;
   MotorDelay=Delay;
   MotorTime=MotorDelay;
   enable_interrupts(INT_RTCC);

   MotorPatternSet(1);
}


#INT_TIMER0
void IntT0()
// Preruseni od casovace cca 1000x za sekundu
{
   // Odpocitavani casovace
   if (TimeTime) TimeTime--;

   // Obsluha akcelerace
   if (MotorTime) MotorTime--;                           // dekrementuj casovac rozjezdu
   if ((MotorGear>0) && (MotorGear<8) && (!MotorTime))   // dalsi rychlostni stupen
   {
      MotorTime=MotorDelay;         // znovu nastav casovac
      MotorGear++;                  // dalsi rychlost
      MotorPatternSet(MotorGear);   // nastav rychlost
   }
}


// Cteni dat z AD prevodniku, zadava se cislo kanalu
int8 ReadAD(int8 Ch)
{
   // Pokud merim Vref zapnu si jeho napajeni
   if (Ch==4) output_high(REFPOWER);

   // Inicializace a cislo kanalu
   ADCON1=0x30;         // Vref+-, bez deleni hodin, Left Justify
   ADCON0=0x41+(Ch<<3); // on, Tosc/8, cislo kanalu

   // Mereni
   delay_us(50);        // doba na prepnuti kanalu
   ADCON0 |= 4;         // start prevodu
   delay_us(50);        // doba na prevod

   // Vypnu napajeni Vref (vzdycky)
   output_low(REFPOWER);

   // Navrat hodnoty
   return ADRESH;
}


void main()
{
   unsigned int8 Debug;    // Promenna pro rezim cinnosti (stav prepinacu)
   unsigned int8 i;

   // Hodiny
   OSCCON = 0x62;          // 4 MHz interni RC oscilator

   // Digitalni vystupy
   output_low(PWM);        // PWM vystup
   output_low(MOTOR);      // Proud do motoru
   output_low(REFPOWER);   // Napajeni Vref
   output_low(LED);        // LED dioda nesviti
   port_b_pullups(TRUE);   // Zbyvajici vyvody portu B

   // Watch Dog
   PSA=0;                  // preddelic prirazen casovaci
   WDTCON=0x0E;            // Watch Dog cca 130ms

   // Analogove vstupy
   ANSEL = 0x1F;           // AN0 az AN4

   // nastaveni RS232
   InitRS232();            // inicializace HW RS232 (nutno pockat cca 10ms)

   // Pipnuti (a cekani)
   for (i=1;i<30;i++)      // pocet 1/2 period
   {
      int1 beep;           // stavova promenna pro pipak

      output_bit(BEEP0,beep);
      beep=~beep;
      output_bit(BEEP1,beep);
      delay_us(1000);
   }

   // Rozhodnuti o rezimu cinnosti (cteni stavu prepinacu)
   Debug=0;
   if (~input(SW0)) Debug|=1;    // precti bit 0
   if (~input(SW1)) Debug|=2;    // precti bit 1
   output_low(SW0);              // nastav L aby se snizila spotreba
   output_low(SW1);              // na obou vstupech

   // Zobrazeni rezimu (na ladici seriovy vystup)
   printf(Putc,"\fMode:%d",Debug);

   // Inicializace PWM
   PR2     = 0x1F;      // perioda PWM casovace
   T2CON   = 0x04;      // povoleni casovace T2 bez preddelicu a postdelicu
   CCP1CON = 0x0C;      // PWM mode, lsb bity nulove
   CCPR1L  =    0;      // na zacatku nulova data
   output_low(PWM);     // PWM vystup

   // Inicializace casovace
   InitT0();            // nastav casovac na cca 1ms

   // ALG=1 Test menice PWM a rozjezdoveho PWM
   // ========================================
   // P1 nastavuje primo stridu hlavniho PWM menice
   // P2 nastavuje rychlostni stupen spinace motoru (rychlostni stupne 0-8)
   // Trvale nacita P1 a P2 a nastavuje podle nich akcni hodnoty menicu
   if (Debug==1)
   {
      unsigned int8 Data1;       // poteniometr P1 = PWM
      unsigned int8 Data2;       // poteniometr P2 = Rozjezd

      while (1)
      {
         // watch dog
         restart_wdt();

         // mereni vstupu
         Data1=ReadAD(0);        // nacti parametr pro PWM
         Data1>>=2;              // redukuj rozsah na 0 az 63
         Data2=ReadAD(1);        // nacti parametr pro rozjezd
         Data2>>=4;              // redukuj rozsah na 0 az 15

         // zobrazeni
         printf(Putc,"\nPWM:%03u RUN:%03u",Data1,Data2);
         delay_ms(20);

         // nastaveni parametru PWM
         CCPR1L = Data1;

         // nastaveni parametru RUN
         MotorSet(Data2);
      }
   }

   // ALG=2 Testovani rozjezdu
   // ========================
   // P2 nastavuje cas mezi stupni razeni pro rozjezd v ms
   // Po resetu 2 sekundy pocka, 2 sekundy jede a nakonec zastavi motor
   if (Debug==2)
   {
      int8 Data;
      int8 Start;

      Start=0;                               // uvodni stav
      while(1)
      {
         // Nacti a zobraz parametr
         Data=ReadAD(1);                     // potenciometr P2 = rozjezd
         printf(Putc,"\nRUN:%3ums ",Data);   // zobraz
         delay_ms(10);                       // prodleva pro terminal

         // Uvodni pauza
         if (Start==0)                       // spousti se 1x na zacatku
         {
            Start++;                         // dalsi stav je cekani
            TimerSet(2000);                  // na dokonceni uvodni prodlevy
         }

         // Rozjezd
         if ((Start==1) && TimerIf())
         {
            Start++;
            printf(Putc,"R");
            MotorStart(Data);                // rozjezd s nastavenim prodlevy

            TimerSet(2000);                  // nastav celkovy cas jizdy
         }

         // Zastaveni
         if ((Start==2) && TimerIf())
         {
            Start++;
            printf(Putc,"S");
            MotorSet(0);                     // pokud dobehl casovac zastav motor
         }

         // watch dog
         restart_wdt();
      }
   }

   // ALG=3 Test nabijeciho algoritmu
   // ===============================
   // P1 nastavuje pozadovane napeti na clancich (meri se Vref vuci napajeni)
   // Nacitani P1 probiha stale dokola, pro rizeni je pouzit stejny
   // algoritmus jako pro standardni jizdu
   if (Debug==3)
   {
      unsigned int8 PwmOut;   // akcni hodnota pro PWM
      unsigned int8 Req;      // pozadovana hodnota z P1
      unsigned int8 Vref;     // merena hodnota vref

      // Inicializace stavove promenne
      PwmOut=0;

      // Hlavni smycka
      while (1)
      {
         // watch dog
         restart_wdt();

         // pozadovana hodnota (potenciometr P1)
         Req=ReadAD(0);
         Req=50+(ReadAD(0)>>1);                      // 50 az 177

         // napeti na napajeni (vref)
         Vref=ReadAD(4);

         // ricici algoritmus
         if ((Vref<Req) &&(PwmOut<30)) PwmOut++;
         if ((Vref>=Req)&&(PwmOut> 0)) PwmOut--;
         Vref+=10;
         if ((Vref<(Req))&&(PwmOut<30)) PwmOut++;    // urychleni nabehu

         // nastaveni parametru PWM
         if (PwmOut>24) PwmOut=24;     // saturace
         CCPR1L = PwmOut;              // pouziti vystupu

         // zobrazeni
         printf(Putc,"\nALG:%03u %03u %03u",Req,Vref,PwmOut);
         delay_ms(10);
      }
   }

   // ALG=0 Standardni jizda
   // ======================
   // P1 nastavuje pozadovane napeti na clancich
   // P2 nastavuje prodlevu razeni pri rozjezdu, nacita se jen 1x na zacatku
   // Po resetu cca 14.5 sekundy akumuluje do kondenzatoru a pak provede
   // rozjezd motoru. Po celou dobu probiha rizeni zateze slunecnich clanku.
   // Parametry P1 a P2 jsou chapany stejne jako v algoritmech 2 a 3.
   if (Debug==0)
   {
      unsigned int8 PwmOut;   // akcni hodnota pro PWM
      unsigned int8 Req;      // pozadovana hodnota z P1
      unsigned int8 Vref;     // merena hodnota vref
      int8 Delay;             // pozadovana honota prodlevy razeni z P2
      int1 Run;

      // Nacti parametr rozjezdu
      Delay=ReadAD(1);                    // potenciometr P2 = rozjezd
      printf(Putc," RUN:%3ums ",Delay);  // zobraz
      delay_ms(10);                       // prodleva pro terminal

      // Inicializace stavove promenne
      PwmOut=0;
      TimerSet(14000);        // casovani startu
      Run=1;

      // Hlavni smycka
      while (1)
      {
         // watch dog
         restart_wdt();

         // pozadovana hodnota (potenciometr P1)
         Req=ReadAD(0);
         Req=50+(ReadAD(0)>>1);                      // 50 az 177

         // napeti na napajeni (vref)
         Vref=ReadAD(4);

         // ricici algoritmus
         if ((Vref<Req) &&(PwmOut<30)) PwmOut++;
         if ((Vref>=Req)&&(PwmOut> 0)) PwmOut--;
         Vref+=10;
         if ((Vref<(Req))&&(PwmOut<30)) PwmOut++;    // urychleni nabehu

         // nastaveni parametru PWM
         if (PwmOut>24) PwmOut=24;     // saturace
         CCPR1L = PwmOut;              // pouziti vystupu

         // zobrazeni
         printf(Putc,"\nALG:%03u %03u %03u",Req,Vref,PwmOut);
         delay_ms(10);

         // rozjezd
         if (TimerIf()&&Run)
         {
            Run=0;
            MotorStart(Delay);         // prodleva razeni z P2
         }
      }
   }
}
