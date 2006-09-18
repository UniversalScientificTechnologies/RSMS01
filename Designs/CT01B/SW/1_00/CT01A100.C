// Firmware pro pripravek pro testovani slunecnich clanku CT01A
// (c) miho 2005
//
// 1.00 Zakladni verze

#include <16F88.h>
#fuses INTRC_IO, NOWDT, NOPUT, NOPROTECT, NOBROWNOUT, MCLR, NOLVP, NOCPD, NODEBUG, CCPB3

#use delay(clock=8000000)  // interni RC oscilator

#use RS232 (baud=9600, xmit=PIN_B5, rcv=PIN_B2)

#include <eeprom.c>        // Podpora zapisu promennych do EEPROM

#define LCD_E  PIN_A2
#define LCD_RS PIN_A7
#define LCD_D0 PIN_A3
#define LCD_D1 PIN_A4
#define LCD_D2 PIN_B7
#define LCD_D3 PIN_B6

#include <LCD.C>           // podpora LCD displeje


// Globalni nastaveni a globalni promenne
//
#define Ofset 5            // ofset PWM pro nulovy proud
float Vref;                // konstanta voltmetru (napeti 1 LSB prevodniku)
int1 Xrs;                  // 1 znamena vystup na RS232
int1 Xlcd;                 // 1 znamena vystup na LCD displej


signed int16 Convert(int8 Chanel)
// Prevod AD prevodnikem ze zadaneho kanalu
// Vysledek je na 10 bitu, doba prevodu 1.8ms
{
   unsigned int16 Data;
   int i;

   // AD prevod s prumerovanim 32x
   Data=0;
   *ADCON0 = 0x41 | Chanel << 3;          // frekvence f/16, zapnout, cislo kanalu
   *ADCON1 = 0xC0;                        // right justify, Vdd a Vss jako reference
   delay_us(100);                         // ustaleni vstupu
   for(i=32;i!=0;i--)
   {
      *ADCON0 |= 4;                       // start prevodu
      delay_us(50);                       // prevod
      Data += (int16)*ADRESH<<8|*ADRESL;  // vysledek se nascita
   }
   Data=Data>>5;                          // odcin prumerovani

   // Vysledek
   return Data;                           // vysledek 0 az 1023
}


float GetVoltage()
// Provede nacteni dat z AD prevodniku a prevod na float napeti
{
   float Data;
   Data=(Convert(0)-Convert(1))*Vref;
   return Data;
}


void SetPWM(int8 Data)
// Nastaveni dat do PWM vystupu
// Celych 8 bitu, doba behu 10ms
{
   *CCPR1L = Data>>2;                           // hornich 6 bitu
   *CCP1CON = *CCP1CON & 0x0F | (Data & 3)<<4;  // spodni 2 bity
   delay_ms(50);                                // doba na ustaleni
}


void GetString(char *s, int max)
// Nacte ze seriovky retezec,
// dela echo a hlida delku retezce
{
   int len;                // aktualni delka
   char c;                 // nacteny znak

   max--;
   len=0;
   do {
     c=getc();
     if(c==8) {            // Backspace
        if(len>0) {
          len--;
          putc(c);
          putc(' ');
          putc(c);
        }
     } else if ((c>=' ')&&(c<='~'))
       if(len<max) {
         s[len++]=c;
         putc(c);
       }
   } while(c!=13);
   s[len]=0;
}


float atof(char *s)
// Prevod retezce na float
{
   float pow10 = 1.0;
   float result = 0.0;
   int sign = 0;
   char c;
   int ptr = 0;

   c = s[ptr++];

   if ((c>='0' && c<='9') || c=='+' || c=='-' || c=='.') {
      if(c == '-') {
         sign = 1;
         c = s[ptr++];
      }
      if(c == '+')
         c = s[ptr++];

      while((c >= '0' && c <= '9')) {
         result = 10*result + c - '0';
         c = s[ptr++];
      }

      if (c == '.') {
         c = s[ptr++];
         while((c >= '0' && c <= '9')) {
             pow10 = pow10*10;
             result += (c - '0')/pow10;
             c = s[ptr++];
         }
      }

   }

   if (sign == 1)
      result = -result;
   return(result);
}


signed int atoi(char *s)
// Preved retezec na int (jen dekadicka cisla)
{
   signed int result;
   int sign, index;
   char c;

   index = 0;
   sign = 0;
   result = 0;

   // Omit all preceeding alpha characters
   if(s)
      c = s[index++];

   // increase index if either positive or negative sign is detected
   if (c == '-')
   {
      sign = 1;         // Set the sign to negative
      c = s[index++];
   }
   else if (c == '+')
   {
      c = s[index++];
   }

   while (c >= '0' && c <= '9')
   {
      result = 10*result + (c - '0');
      c = s[index++];
   }

   if (sign == 1)
       result = -result;

   return(result);
}


void Xputc(char c)
// Spolecna procedura pro vystup znaku na LCD a RS232
// dle stavu promennych Xrs a Xlcd
{
   if (Xrs)
      if(c!='\n') putc(c); // vystup na RS232 (neposilej LF)
   if (Xlcd) lcd_putc(c);  // vystup na LCD displej
}


void Calibrate()
// Procedura pro kalibraci
{
   #define LINE_LEN 40     // delka retezce
   char Line[LINE_LEN];    // retezec
   int8 Data;              // nacteny proud 0 az 250
   float FData;            // nactene rozdilove napeti

   lcd_clr();
   printf(Xputc,"\n\rCalibration\r\n");
   for(;1;)
   {
      Xrs=1;
      Xlcd=1;
      GetString(Line,LINE_LEN);
      if (*Line=='q')
      {
         // Ukonceni procesu kalibrace
         SetPWM(0);                    // vypni proud
         printf("\n\r");               // odradkuj na terminalu
         EE_WR(0,Vref);                // uloz kalibraci do EEPROM
         return;                       // navrat
      }
      else if (*Line=='v')
      {
         // Zadani nove hodnoty Vref
         Vref=atof(Line+1)/1023;       // referencni napeti na 1 LSB
         printf("\r\n");
      }
      else if(*Line)
      {
         // Zadan novy proud
         Data=atoi(Line);              // preved retezec na cislo
         printf(Xputc,"       Set %3umA\r\n",Data);
         SetPWM(Data+Ofset);           // nastav proud
         delay_ms(100);                // cas na ustaleni
      }
      // Jeden cyklus mereni
      FData=GetVoltage();
      printf(Xputc,"%1.2fV \r\n",FData);
   }
   lcd_clr();                          // smaz displej
}


void AutoRun()
// AutoRun - automaticke mereni cele zatezovaci krivky
{
   float FData;               // zmerene napeti
   int8 i;                    // promenna cyklu - proud v mA

   Xrs=0;                     // vystup neni na RS232
   Xlcd=1;                    // vystup je na LCD
   printf(Xputc,"\fAutoRun"); // napis na LCD
   Xrs=1;                     // hlavika jen na RS232
   Xlcd=0;
   printf(Xputc,"\r\nI[mA] U[V] P[mW]");
   Xlcd=1;

   SetPWM(0);                 // vypni proud
   delay_ms(100);             // klidova podminka
   for(i=0;i<=250;i++)        // cyklus pres proud 0 az 250mA
   {
      SetPWM(i+Ofset);        // nastav proud
      FData=GetVoltage();     // zmer napeti
      if (FData>0) printf(Xputc,"\r\n%03u %1.2f %3.1f",i,FData,FData*i);
      else i=250;             // predcasne ukonceni
   }
   printf(Xputc,"\r\n");      // na konci odradkuj
   SetPWM(0);                 // vypni proud
   lcd_clr();                 // smaz displej
}


void main()
{
   // Hodiny
   *0x8F = 0x72;           // 8 MHz interni RC oscilator

   // Digitalni vystupy
   output_low(PIN_B0);     // nepouzity
   output_low(PIN_B1);     // nepouzity
   output_low(PIN_B3);     // PWM vystup
   output_high(PIN_B5);    // TX data
   port_b_pullups(TRUE);   // vstupy s pull up odporem

   // Analogove vstupy
   *ANSEL = 0x03;          // AN0 a AN1

   // Inicializace LCD
   lcd_init();
   Xrs=1;
   Xlcd=1;
   printf(Xputc,"\fSolar Cell\r\nTester 1.00\r");

   // Inicializace PWM 8 bitu
   *PR2 = 0x3F;            // perioda PWM casovace
   *T2CON = 0x04;          // povoleni casovace T2 bez preddelicu a postdelicu
   *CCP1CON = 0x0C;        // PWM mode, lsb bity nulove
   *CCPR1L = 0;            // na zacatku nulova data
   output_low (PIN_B3);    // PWM vystup

   // Kalibrace pri drzenem tlacitku
   EE_RD(0,Vref);          // vytahni kalibracni konstantu z EEPROM
   if (input(PIN_B4)==0)   // otestuj tlacitko
      {
         delay_ms(200);
         Calibrate();      // pokud je stalceno spust kalibraci
      }
   else
      {
         delay_ms(1000);   // jinak jen 1s spozdeni
      }
   lcd_clr();              // smaz displej

   // Hlavni smycka
   {
      int8 il,ih,im;                // spodni a horni mez a maximum proudu
      int8 i;                       // promenna cyklu
      float Voltage,Power;          // zmerene rozdilova napeti a vypocteny vykon
      float MaxVoltage,MaxPower;    // maximalni hodnoty

      // Cihej na stisk tlacitka
      0==PORTB;                     // jen precti port B
      RBIF=0;                       // nuluj priznak preruseni od zmeny

      // Pocatecni meze
      il=0;
      ih=10;

      // Trvale prohledavani
      for(;1;)
      {

         if (RBIF)                           // kdyz je tlacitko
         {
            AutoRun();
            while (~input(PIN_B4));          // cti port B a cekej na uvolneni
            RBIF=0;
         }

         Xrs=0;
         Xlcd=1;
         printf(Xputc,"\rOpt. [mA V mW]");   // napis na LCD

         MaxVoltage=0;                       // inicializace maxim
         MaxPower=0;
         im=0;

         for(i=il;i<=ih;i++)                 // dilci cyklus hledani
         {
            SetPWM(i+Ofset);                 // nastav proud
            Voltage=GetVoltage();            // precti rozdilove napeti
            Power=Voltage*i;                 // vypocti vykon
            if (Power>MaxPower)              // zkontroluj maximu
            {
               MaxVoltage=Voltage;           // zapamatuj si maximum
               MaxPower=Power;
               im=i;
            }
         }

         // Zobrazeni vysledku
         Xrs=0;
         Xlcd=1;
         printf(Xputc,"\r\n%3u %1.2f %3.1f  ", im, MaxVoltage, MaxPower);

         // Natav nove meze
         if (im>5) il=im-5; else il=0;
         if (il>240) il=240;
         ih=il+10;
      }
   }
}
