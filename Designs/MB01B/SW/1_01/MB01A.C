// Melodicky zvonek MB01A_1_00
//
// Prohram pro melodicky zvonek s pouzitim knihovny Sound_t1.C
//
// (c)miho 2004, pefi 2004
//
// Historie:
// 1.00  Uvodni verze
// 1.01  Zaveden WDT

#include <16F819.h>                                   // Definice procesoru
#fuses HS,NOPROTECT,WDT,PUT,NOLVP,NOBROWNOUT,WRT      // Definice konfiguracniho slova

#define  POWER_386   PIN_A4               // L zapne napajeni pro zesilovac LM386
#define  POWER_PIC   PIN_A3               // L pripoji GND pro procesor PIC

#define  SOUND_HI          PIN_B3         // Akusticky vystup
#define  SOUND_LO          PIN_B2         // Akusticky vystup
#define  SOUND_CLOCK       20000000       // Frekvence hodin
#define  SOUND_LowOctave   1              // O oktavu vys protoze mame moc rychly krystal
#define  SOUND_WDT         1              // Budeme pouzivat WDT
#include "Sound_T1.c"                     // Hudebni knihovna

#include "Data.c"                         // Datovy blok pro predpripravene skladby

#define  RXD         PIN_B1               // Port pro RS232
#use delay(CLOCK=20000000,RESTART_WDT)    // Konfigurace pro casovani RS232
#use rs232(BAUD=9600,RCV=RXD,RESTART_WDT,INVERT,PARITY=N,BITS=8)  // Prenosove parametry RS232


// Sada globalnich promennych
// --------------------------
// Vyuzivame globalni promenne protoze je to vyhodne z hlediska spotreby zdroju. Usetri
// se vyznmane mnoho pameti programu pri predavani parametru


unsigned int16 Adr;                       // Adresovy ukazatel do pameti skladeb
unsigned int16 Data;                      // Prectena data nebo data pro zapis
unsigned int1  Mode;                      // 0=rezim testovani, 1=rezim zapisu do FLASH

unsigned int16 Tempo;                     // Tempo (delka nejkratsi skladby v ms)
unsigned int16 Pause;                     // Delka mezery mezi notami v ms
unsigned int8  Octava;                    // Posunuti skladby v oktavach

unsigned int8  Beep;                      // Druh pipnuti pro proceduru SoundSpec
unsigned int1  Error;                     // Priznak chyby

unsigned int8  CisloSkladby;              // Cislo skladby pro proceduru Find a Play


// Zvuky, posloupnost zadaneho poctu tonu
#define SoundEndOfLine  0x01              // Kratke pipnuti na konci radky
#define SoundPGM        0x03              // Trilek pri vstupu do rezimu programovani
#define SoundPostPlay   0x03              // Po ukonceni prehravani
#define SoundERASE      0x02              // Zvuk pri smazani bloku FLASH pameti
#define SoundERR        0x05              // Chyba syntaxe

void SpecBeep()
// Data - pocet pipnuti, 0 znamena ticho
{
   int Oct;

   if (Error) Beep=SoundERR;

   for(Oct=2;Beep!=0;Beep--)
   {
      SoundNote(SOUND_A,Oct++,50);
   }

   Error=0;
}


// Precti slovo z pameti programu
int16 ReadData()
// Adr - adresa ze ktere se cte
// Data - prectena data
{
   int8 a,b;                              // Pomocne promenne

   (int8)*EEADR  = Adr;                   // Adresa, spodni cast
   (int8)*EEADRH = Adr >> 8;              // Adresa, horni cast
   bit_set(*EECON1,EECON1_EEPGD);         // Pamet programu
   bit_set(*EECON1,EECON1_RD);            // Operace cteni
   #ASM
   nop;                                   // Povinne nop
   nop;
   #ENDASM
   a = (int8)*EEDATA;                     // Prevezmi data
   b = (int8)*EEDATAH;
   Data=make16(b,a);                      // Sestav vysledek ze 2 bajtu
}


// Smazani cele pameti vyhrazene pro skladby
void Erase()
{
   for(Adr=STARTMEM; Adr<=ENDMEM; Adr++)  // Cela oblast
   {
      ReadData();
      if (Data!=0x3FFF)                   // Mazu jen bloky, ktere to potrebuji
      {
         if (input(POWER_PIC)!=0) return; // Nezapisuj pokud neni jumper povoleni programovani
         (int8)*EEADR  = Adr;             // Adresa bloku, spodni cast
         (int8)*EEADRH = Adr >> 8;        // Adresa bloku, horni cast
         bit_set(*EECON1,EECON1_EEPGD);   // Pamet programu
         bit_set(*EECON1,EECON1_WREN);    // Povolit zapis
         bit_set(*EECON1,EECON1_FREE);    // Operace mazani
         (int8)*EECON2 = 0x55;            // Povinna sekvence pro zapis
         (int8)*EECON2 = 0xAA;
         bit_set(*EECON1,EECON1_WR);      // Zahajeni mazani
         #ASM
         nop;                             // Povinne prazdne instrukce
         nop;
         #ENDASM
         bit_clear(*EECON1,EECON1_WREN);  // Uz ne zapis
         bit_clear(*EECON1,EECON1_FREE);  // Uz ne mazani
         bit_clear(*EECON1,EECON1_EEPGD); // Uz ne pamet programu
         Beep=SoundERASE;
         SpecBeep();
      }
   }
}


// Zapis do pameti programu po jednotlivych slovech.
// Soucastka vyzaduje zapis vzdy celych osmi slov najednou. Vyuziva se toho, ze pamet FLASH
// umi zapisovat jen smerem do nuly a tak staci na pozice, ktere zrovna nechceme programovat
// zapsat same jednicky cimz se stav jejich stav nezmeni.
void WriteData()
// Adr - adresa, kam se bude zapisovat
// Data - data, ktera se budou zapisovat
{
   int i;

   bit_set(*EECON1,EECON1_EEPGD);         // Pamet programu
   bit_set(*EECON1,EECON1_WREN);          // Zapis
   (int8)*EEADR  = Adr & ~3;              // Adresa, spodni cast, zaokrouhleno dolu na nasobek 8
   (int8)*EEADRH = Adr >> 8;              // Adresa, horni cast
   for (i=0; i<4; i++)
   {
      if ((Adr & 3) == i)                 // Pokud je adresa slova v bloku totozna s pozadovanou
      {
         (int8)*EEDATA =Data;             // Platne slovo, spodni cast
         (int8)*EEDATAH=Data >> 8;        // Platne slovo, horni cast
      }
      else                                // Ostatni bunky nemenime
      {
         (int8)*EEDATA =0xFF;             // Zbytek same jednicky
         (int8)*EEDATAH=0xFF;
      }
      (int8)*EECON2 = 0x55;               // Povinna sekvence pro zapis
      (int8)*EECON2 = 0xAA;
      bit_set(*EECON1,EECON1_WR);         // Zahajeni zapisu
      #ASM
      nop;                                // Povinne dve prazdne instrukce
      nop;
      #ENDASM
      ((int8)*EEADR) ++;                  // Dalsi slovo
   }
   bit_clear(*EECON1,EECON1_WREN);        // Konec zapisu
   bit_clear(*EECON1,EECON1_EEPGD);       // Uz ne pamet programu (bezpecnost)
}


// Zapise data Data na adresu Adr a provede posun na dalsi adresu, zapisuje se jen do
// dovolene oblasti pameti a jen v pripade, ze je Mode=1
void WriteDataInc()
// Data - co se zapisuje
// Adr - kam se zapisuje, po zapisu se adresa posouva
{
   if (~Mode) return;                     // Neni rezim zapisu
   if ( (Adr>=STARTMEM) & (Adr<=ENDMEM) & (input(POWER_PIC)==0) )
   {
      WriteData();
      Adr++;
   }
   else
   {
      Error=1;
   }
}


// Najdi zacatek zadaneho cisla skladby. Promenna Adr ukazuje na zacatek skladby.
// Neni-li skladba nalezena ukazuje Adr na koncovou znacku (0x3FFF) posledni skladby
// nebo na konec pameti.
int1 Find()
// CisloSkladby - poradove cislo skladby
// Adr - adresa zacatku skladby
{
   Adr=STARTMEM-1;                        // Od zacatku oblasti pro skladby
   for(;1;)
   {
      Adr++;
      ReadData();                         // Precti data
      if (Data==ENDOFDATA) return 1;      // Priznak konce dat
      if (Adr==ENDMEM+1)   return 1;      // Uz jsme prosli celou pamet
      if ((Data&MASKBEGIN)==DATABEGIN)    // Priznak zacatku skladby
      {
         CisloSkladby--;                  // Otestuj pocitadlo skladeb
         if (CisloSkladby==0) return 0;   // Je to tato skladba
      }
   }
}


// Zahraj jednu notu
void PlayData()
// Data = zakodovana nota
// Tempo, Octava, Pause = parametry hrane noty
{
   SoundNote((int8)Data&0xF,Octava+(((int8)Data>>4)&0x7),Tempo*((Data>>7)&0x3F));   // Nota
   SoundNote(SOUND_Space,0,Pause);                                                  // Mezera
}


// Zahraj skladbu od zadane adresy v promenne Adr.
void Play()
// CisloSkladby - cislo skladby k hrani
{
   if (Find())                            // Najdi zacatek skladby v pameti skladeb
   {
      return;                             // Skladba nenalezena
   }

   Tempo=100;                             // Default delka noty
   Pause=100;                             // Default mezera mezi notami

   Octava=Data&~MASKBEGIN;                // Posunuti oktav (povinna soucast zacatku skladby)
   Adr++;

   for (;1;)
   {
      if (Adr==ENDMEM+1)                  return;                 // Konec pametove oblasti
      ReadData();                                                 // Vezmi data
      Adr++;                                                      // Posun adresu
      if (Data==ENDOFDATA)                return;                 // Konec dat
      if ((Data&MASKBEGIN)==DATABEGIN)    return;                 // Zacatek dalsi skladby
      if ((Data&MASKTEMPO)==DATATEMPO)    Tempo=Data&~DATATEMPO;  // Paramter TEMPO
      if ((Data&MASKPAUSE)==DATAPAUSE)    Pause=Data&~DATAPAUSE;  // Parametr PAUSE
      if ((Data&MASKNOTE)==0)                                     // Nota
      {
         PlayData();                      // Zahraj notu
      }
   }
}


// Vycisli cislo z bufferu, posune ukazovatko na prvni nezpracovany znak, preskakuje mezery
int16 Number(char line[], int *a, len)
{
   int16 Data;
   char c;

   while((line[*a]==' ')&(*a<len))        // Vynech mezery na zacatku
      (*a)++;                             // Posouvej ukazovatko

   Data=0;
   while (1)
   {
      if (*a>=len) return Data;           // Konec retezce
      c=line[*a];                         // Vezmi znak z pole
      if ((c<'0')|(c>'9')) return Data;   // Koncime pokud znak neni cislice
      Data = Data * 10 + (c-'0');         // Pouzij cislici
      (*a)++;                             // Dalsi znak
   }
}


// Vyhledej klicove slovo a vrat jeho zkraceny kod
// Pokud slovo neexistuje vraci -1
// Format definice - retezec ukonceny nulou + zastupny znak, na konci prazdny retezec (nula)
const char KeyWords[] =
      {
         'P','L','A','Y',0,         'P',
         'E','R','A','S','E',0,     'E',
         'T','E','M','P','O',0,     't',
         'P','A','U','S','E',0,     'p',
         'B','E','G','I','N',0,     'B',
         'T','E','S','T',0,         'b',
         'E','N','D',0,             'Z',
         'C',0.                     SOUND_C,
         'C','I','S',0,             SOUND_Cis,
         'D',0,                     SOUND_D,
         'D','I','S',0,             SOUND_Dis,
         'E',0,                     SOUND_E,
         'F',0,                     SOUND_F,
         'F','I','S',0,             SOUND_Fis,
         'G',0,                     SOUND_G,
         'G','I','S',0,             SOUND_Gis,
         'A',0,                     SOUND_A,
         'A','I','S',0,             SOUND_Ais,
         'H',0,                     SOUND_H,
         'S','P','A','C','E',0,     SOUND_Space
      };
signed int Word(char line[], unsigned int8 *a, len)
{
   unsigned int8 i;                       // Index do pole klicovych slov
   unsigned int8 j;                       // index do zpracovavane radky

   while((line[*a]==' ')&(*a<len))        // Vynech mezery na zacatku
      (*a)++;                             // Posouvej ukazovatko

   for (i=0;i<sizeof(KeyWords);)          // Slova ze slovniku
   {
      for (j=*a;(j<len)&(KeyWords[i]!=0)&(KeyWords[i]==line[j]);i++,j++)   // Znaky ze slova
      {
      }
      if ((KeyWords[i]==0)&((line[j]==' ')|(j==len)))
      {
         if (j>=len) j=len-1;             // Korekce abychom se nedostali za konec retezce
         *a=j+1;                          // Posun ukazovatko za zpracovane slovo

         return KeyWords[i+1];            // Vrat zastupnou hodnotu z tabulky klicovych slov
      }
      while(KeyWords[i]!=0) i++;          // Preskoc zbytek slova v tabulce
      i++;                                // Preskoc oddelovac
      i++;                                // Preskoc polozku se zastupnou hodnotou
   }
   return -1;                             // Prosli jsme cely slovnik a nedoslo ke shode
}


// Programovani pres RS232
#define LINELEN   40                      // Delka radky pro RS232
#define CR        0x0D                    // Odradkovani
#define BS        0x08                    // Back Space

#separate
void Download()
{
   char  line[LINELEN];                   // Buffer na radku
   unsigned char c;                       // Znak
   unsigned int8 a;                       // Ukazovatko do bufferu
   unsigned int8 len;                     // Delka retezce v radce
   unsigned int8 Oct;                     // Cislo oktavy u noty

   output_low(POWER_386);                 // Zapni napajeni zesilovace
   SoundNote(SOUND_Space,3,10);           // Mezera
   Beep=SoundPGM;
   Error=0;
   SpecBeep();                            // Pipni na znameni vstupu do programovani

   Tempo=100;                             // Default hodnoty
   Pause=100;
   Octava=0;
   Mode=0;                                // Mod hrani
   Oct=0;
   a=0;                                   // Na zacatku je radka prazdna

   for(;input(POWER_PIC)==0;)             // Opakuj vse dokud je PGM rezim
   {
   Loop:
      c=Getc();                           // Vezmi znak ze seriovky
      if (c>=0x80) goto Loop;             // Ignoruj znaky nad ASCII
      if (c>=0x60) c=c-0x20;              // Preved velka pismena na velka pismena
      if ((c==CR)|(c=='/'))               // Konec radky nebo komentar
      {
         while (c!=CR) c=Getc();          // Zpracuj znaky komentare
         len=a;                           // Zapamatuj si delku radky
         a=0;                             // Postav se na zacatek radky
         Beep=SoundEndOfLine;             // Default zuk na konci radky
         // Zpracovani radky
         while(a<len)
         {
            restart_wdt();                // Nuluj watchdog abychom se nezresetovali
            c=Word(line,&a,len);
            if (c==-1)                    // Nezname klicove slovo
            {
               if (a<len)                 // Nejsme uz na konci radky ?
               {
                  if ((line[a]>='0')&(line[a]<='9'))  // Stojime na cislici -> je to cislo
                  {
                     // Oct=Number(line,&a,len)&0x7;  // tohle nefunguje protoze je chyba v prekladaci
                     Oct=Number(line,&a,len);         // prekladac prepoklada, z W obsahuje spodni bajt vysledku
                     Oct&=0x7;                        // ale k navratu pouziva RETLW 0 coz smaze W !
                  }
                  else                                // Stojime na pismenu nebo oddelovaci
                  {
                     if (line[a]!=' ') Error=1;       // Neni to oddelovac - chyba
                     a++;                             // Preskocim 1 znak (a kdyz to nepomuze dostanu se zase sem)
                  }
               }
            }
            else if (c=='P')              // Play
            {
               CisloSkladby=Number(line,&a,len);
               Mode=0;
               Play();
               Beep=SoundPGM;
            }
            else if (c=='E')              // Erase
            {
               Mode=0;
               Erase();
               Beep=SoundPGM;
            }
            else if (c=='t')              // Tempo
            {
               Tempo=Number(line,&a,len)&~MASKTEMPO;
               if (Tempo==0) Tempo=100;
               Data=Tempo|DATATEMPO;
               WriteDataInc();            // Podmineny zapis do FLASH a posun na dalsi adresu
            }
            else if (c=='p')              // Pause
            {
               Pause=Number(line,&a,len)&~MASKPAUSE;
               if (Pause==0) Pause=100;
               Data=Pause|DATAPAUSE;
               WriteDataInc();            // Podmineny zapis do FLASH a posun na dalsi adresu
            }
            else if (c=='B')              // Begin
            {
               CisloSkladby=~0;           // Neplatne cislo skladby
               Find();                    //   najde konec posledni skladby
               Octava=Number(line,&a,len)&~MASKBEGIN;
               Data=DATABEGIN|Octava;     // Zacatecni znacka
               Mode=1;                    // Mod zapisu do FLASH pameti
               WriteDataInc();            // Podmineny zapis do FLASH a posun na dalsi adresu
            }
            else if (c=='b')              // Test
            {
               Octava=Number(line,&a,len)&~MASKBEGIN;
               Mode=0;
            }
            else if (c=='Z')              // End
            {
               Mode=0;
            }
            else                          // Nota
            {
               Data=Number(line,&a,len);  // Delka noty (nepovinna)
               Data&=0x3F;                // Jen platny rozsah
               if (Data==0) Data++;       // Je-li nulova delka - dej jednotkovou
               Data<<=7;                  // Delka
               Data|=c;                   // Nota
               Data|=(Oct<<4);            // Oktava
               WriteDataInc();            // Podmineny zapis do FLASH a posun na dalsi adresu
               if (~Mode)
               {
                  PlayData();             // Zahraj notu
                  Beep=0;                 // Po zahrani noty uz nepipej na konci radky
               }
            }
         }
         a=0;                             // Radka zpracovana, nuluj jeji delku
         SpecBeep();                      // Pipni
         goto Loop;
      }
      if ((c==BS)&(a>0)) {a--;goto Loop;} // Smaz znak
      if ((c==',')|(c<=' ')) c=' ';       // Vsechny ostatni ridici znaky i carka jsou oddelovac
      if (a<LINELEN) line[a++]=c;         // Zapis znak do bufferu a posun ukazovatko
   }
}


// Tabulka pro prekodovani tlacitek
const int8 KeyTranslate[16] = {0, 1, 2, 5, 3, 6, 8, 11, 4, 7, 9, 12, 10, 13, 14, 15};

void main()
{
Start:

   if (restart_cause()==WDT_FROM_SLEEP)   // Osetreni probuzeni od WDT
   {
      sleep();
   }
   // Inicializace WDT
   //   setup_wdt(WDT_16MS);              // nelze pouzit, zabira 15 instukci
   #asm
      bsf  0x03,5                         // banka 1 nebo 3 aby se mohlo k OPTION registu
      bcf  0x81,3                         // WDT primo tedy 16ms
   #endasm

   // Inicializace
   port_b_pullups(TRUE);                  // Zapni pull-up odpory na portu B
   set_tris_a(0b00001000);                // Nastav nepouzite vyvody jako vystupy
   set_tris_b(0b11110000);                //   1 znamena vstup
   *0x9F = 6;                             // Vsechny vstupy jsou digitalni

   // Test na rezim programovani
   if ((input(POWER_PIC)==0)&(input(RXD)==0))      // Podminka programovani
      Download();                                  //   Pripojen RS232 a propojka na PGM

   // Zapnuti napajeni
   output_low(POWER_PIC);                 // Pripoj GND pro procesor
   SoundNote(SOUND_Space,3,10);           // Mezera
   output_low(POWER_386);                 // Zapni napajeni zesilovace
   SoundNote(SOUND_Space,3,100);          // Mezera (jinak se chybne detekuje tlacitko)

   // Cteni tlacitek
   #use FAST_IO(B)
   CisloSkladby=(input_b() >> 4) ^ 0xFF & 0xF;     // Precti stav tlacitek
   #use STANDARD_IO(B)
   CisloSkladby=KeyTranslate[CisloSkladby];        // Prekoduj je do binarniho kodu

   // Prehrani skladby
   Play();                                // Zahraj skladbu

   // Po odehrani usni a cekej na zmacknuti tlacitka
   #use FAST_IO(B)
   if (input_B());                        // Precti stav portu B (vuci tomuto stavu se bude hlidat zmena)
   #use STANDARD_IO(B)
   bit_clear(*0xB,0);                     // Nuluj priznak preruseni od zmeny stavu portu B
   enable_interrupts(INT_RB);             // Povol preruseni od zmeny stavu portu B

   output_high(POWER_386);                // Vypni napajeni zesilovace
   output_float(POWER_PIC);               // Odpoj GND

   Sleep();                               // Usni aby byla minimalni klidova spotreba

   disable_interrupts(INT_RB);            // Zakaz preruseni od portu
   goto Start;                            // Po probuzeni skoc na zacatek
}
