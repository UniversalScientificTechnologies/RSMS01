// LCD modul pro ovladani dvouradkoveho LCD modulu se standardnim Hitachi radicem
// (c)miho 2002,2005
//
// Historie:
//
// 0.00  Uvodni verze se snadnou definici portu LCD displeje
// 0.01  Oprava portu (zapomenute stare identifikatory)
// 0.02  Doplnena moznost pripojeni datoveho portu LCD na libovolne porty
// 0.03  Doplnena procedura lcd_clr pro smazani displeje
//
//
// Funkce:
//
//   lcd_init()            inicializuje LCD displej a porty, nutno volat jako prvni
//
//   lcd_putc(c)           zapis snaku do lcd displeje, zpracovava nasledujici ridici znaky
//                         \f = \x0C   - nova stranka - smazani displeje
//                         \n = \x0A   - odradkovani (prechod na druhou radku)
//                         \b = \x08   - backspace - posunuti kurzoru o 1 pozici zpet
//                         \r = \x0D   - goto home to position 1,1
//                         \0  .. \7   - definovatelne znaky v pozicich 0 az 7 v CGRAM
//                         \20 .. \27  - alternativne zapsane znaky (oktalove) v pozicich 0 az 7 CGRAM
//                         Pozor na to, ze funkce printf konci tisk pokud narazi na \0 (konec retezce)
//
//   lcd_gotoxy(x,y)       presune kurzor na uvedenou adresu
//                         nekontroluje parametry
//
//   lcd_cursor_on         zapne kurzor
//   lcd_cursor_off        vypne kurzor
//
//   lcd_clr               smaze displej
//
//   lcd_define_char(Index, Def)       Makro, ktere definuje znaky od pozice Index obsahem definicniho
//                                     retezce Def. Kazdych 8 znaku retezce Def definuje dalsi znak v CGRAM.
//                                     Kapacita CGRAM je celkem 8 znaku s indexem 0 az 7.
//                                     Na konci se provede lcd_gotoxy(1,1).
//                                     Na konci teto knihovny je priklad pouziti definovanych znaku
//
//
// Definice portu:                     // Datovy port displeje pripojeny na 4 bity za sebou na jeden port
//
// #define LCD_RS          PIN_B2      // rizeni registru LCD displeje
// #define LCD_E           PIN_B1      // enable LCD displeje
// #define LCD_DATA_LSB    PIN_C2      // pripojeni LSB bitu datoveho portu LCD displeje (celkem 4 bity vzestupne za sebou)
//
//
// Alternativni definice:              // Datovy port displeje pripojeny na libovolne 4 bitove porty (vede na kod delsi asi o 25 slov)
//
// #define LCD_RS          PIN_B2      // rizeni registru LCD displeje
// #define LCD_E           PIN_B1      // enable LCD displeje
// #define LCD_D0          PIN_C2      // D0 - datove bity pripojene na libovolne porty
// #define LCD_D1          PIN_C3      // D1
// #define LCD_D2          PIN_C4      // D2
// #define LCD_D3          PIN_C5      // D3




// Privatni sekce, cist jen v pripade, ze neco nefunguje




#ifdef LCD_DATA_LSB
// Generovane defince portu pro ucely teto knihovny aby kod generoval spravne IO operace a soucasne
// bylo mozne jednoduse deklarovat pripojene piny LCD displeje pri pouziti teto knihovny. Problem spociva
// v tom, ze se musi spravne ridit smery portu a soucasne datovy port zabira jen 4 bity ze zadaneho portu
//
#define LCD_SHIFT (LCD_DATA_LSB&7)                 // pocet bitu posuvu datoveho kanalu v datovem portu
#define LCD_PORT  (LCD_DATA_LSB>>3)                // adresa LCD datoveho portu
#define LCD_TRIS  (LCD_PORT+0x80)                  // adresa prislusneho TRIS registru
#define LCD_MASK  (0xF<<LCD_SHIFT)                 // maska platnych bitu
//
#if LCD_SHIFT>4                                    // kontrola mezi
#error LCD data port LSB bit not in range 0..4
#endif
#endif


// Definice konstant pro LCD display
//
#define LCD_CURSOR_ON_  0x0E     // kurzor jako blikajici radka pod znakem
#define LCD_CURSOR_OFF_ 0x0C     // zadny kurzor
#define LCD_LINE_2      0x40     // adresa 1. znaku 2. radky


// Definice rezimu LCD displeje
//
BYTE const LCD_INIT_STRING[4] =
{
   0x28,                         // intrfejs 4 bity, 2 radky, font 5x7
   LCD_CURSOR_OFF_,              // display on, kurzor off,
   0x01,                         // clear displeje
   0x06                          // inkrement pozice kurzoru (posun kurzoru doprava)
};


// Odesle nibble do displeje (posle data a klikne signalem e)
//
void lcd_send_nibble( BYTE n )
{
   #ifdef LCD_DATA_LSB
      // data jsou za sebou na 4 bitech jednoho portu
      *LCD_PORT = (*LCD_PORT & ~LCD_MASK) | ((n << LCD_SHIFT) & LCD_MASK);      // nastav datove bity portu a ostatni zachovej
   #else
      // data jsou na libovolnych 4 bitech libovolnych portu
      output_bit(LCD_D0,bit_test(n,0));
      output_bit(LCD_D1,bit_test(n,1));
      output_bit(LCD_D2,bit_test(n,2));
      output_bit(LCD_D3,bit_test(n,3));
   #endif
   output_bit(LCD_E,1);       // vzestupna hrana
   delay_us(1);               // pockej alespon 450ns od e nebo alespon 195ns od dat
   output_bit(LCD_E,0);       // sestupna hrana (minimalni perioda e je 1us)
}


// Odesle bajt do registru LCD
//
// Pokud je Adr=0 .. instrukcni registr
// Pokud je Adr=1 .. datovy registr
//
void lcd_send_byte( BOOLEAN Adr, BYTE n )
{
   output_bit(LCD_RS,Adr);    // vyber registr
   swap(n);
   lcd_send_nibble(n);        // posli horni pulku bajtu
   swap(n);
   lcd_send_nibble(n);        // posli spodni pulku bajtu
   delay_us(40);              // minimalni doba na provedeni prikazu
}


// Provede inicializaci LCD displeje, smaze obsah a nastavi mod displeje
//
// Tato procedura se musi volat pred pouzitim ostatnich lcd_ procedur
//
void lcd_init()
{

   int i;                              // pocitadlo cyklu

   delay_ms(20);                       // spozdeni pro provedeni startu displeje po zapnuti napajeni

#ifdef LCD_DATA_LSB
   // data jsou na 4 bitech za sebou, nastav smer pro vsechny dalsi prenosy
   *LCD_TRIS = *LCD_TRIS & ~LCD_MASK;  // nuluj odpovidajici bity tris registru datoveho portu LCD
#endif

   output_bit(LCD_RS,0);               // nastav jako vystup a nastav klidovy stav
   output_bit(LCD_E, 0);               // nastav jako vystup a nastav klidovy stav

   for (i=0; i<3; i++)                 // nastav lcd do rezimu 8 bitu sbernice
   {
      delay_ms(2);                     // muze byt rozdelany prenos dat (2x 4 bity) nebo pomaly povel
      lcd_send_nibble(3);              // rezim 8 bitu
   }

   delay_us(40);                       // cas na zpracovani
   lcd_send_nibble(2);                 // nastav rezim 4 bitu (plati od nasledujiciho prenosu)
   delay_us(40);                       // cas na zpracovani

   for (i=0;i<3;i++)                   // proved inicializaci (nastaveni modu, smazani apod)
   {
      lcd_send_byte(0,LCD_INIT_STRING[i]);
      delay_ms(2);
   }
}


// Proved presun kurzoru
//
// Pozice 1.1 je domu
//
void lcd_gotoxy( BYTE x, BYTE y)
{

   BYTE Adr;

   Adr=x-1;
   if(y==2)
     Adr+=LCD_LINE_2;

   lcd_send_byte(0,0x80|Adr);
}


// Zapis znaku na displej, zpracovani ridicich znaku
//
void lcd_putc( char c)
{

   switch (c)
   {
      case '\f'   : lcd_send_byte(0,1);            // smaz displej
                    delay_ms(2);
                                            break;
      case '\n'   : lcd_gotoxy(1,2);        break; // presun se na 1. znak 2. radky
      case '\r'   : lcd_gotoxy(1,1);        break; // presun home
      case '\b'   : lcd_send_byte(0,0x10);  break; // posun kurzor o 1 zpet
      default     : if (c<0x20) c&=0x7;            // preklopeni definovatelnych znaku na rozsah 0 az 0x1F
                    lcd_send_byte(1,c);     break; // zapis znak
   }
}


// Zapni kurzor
//
void lcd_cursor_on()
{
   lcd_send_byte(0,LCD_CURSOR_ON_);
}


// Vypni kurzor
//
void lcd_cursor_off()
{
   lcd_send_byte(0,LCD_CURSOR_OFF_);
}


// Smaz displej
//
void lcd_clr()
{
   lcd_putc('\f');
}


// Definice vlastnich fontu
//
// Vlastnich definic muze byt jen 8 do pozic 0 az 7 pameti CGRAM radice lcd displeje
// Pro snadne definovani jsou pripraveny nasledujici definice a na konci souboru je uveden
// priklad pouziti definovanych znaku.


// Pomocna procedura pro posilani ridicich dat do radice displeje
//
void lcd_putc2(int Data)
{
   lcd_send_byte(1,Data);
}


// Pomocne definice pro programovani obsahu CGRAM
//
#define lcd_define_start(Code)      lcd_send_byte(0,0x40+(Code<<3)); delay_ms(2)
#define lcd_define_def(String)      printf(lcd_putc2,String);
#define lcd_define_end()            lcd_send_byte(0,3); delay_ms(2)


// Vlastni vykonne makro pro definovani fontu do pozice Index CGRAM s definicnim retezcem Def
//
#define lcd_define_char(Index, Def) lcd_define_start(Index); lcd_define_def(Def); lcd_define_end();


// Pripravene definice fontu vybranych znaku
// V tabulce nesmi byt 00 (konec retezce v printf()), misto toho davame 80
//
#define LCD_CHAR_BAT100 "\x0E\x1F\x1F\x1F\x1F\x1F\x1F\x1F"      /* symbol plne baterie       */
#define LCD_CHAR_BAT50  "\x0E\x1F\x11\x11\x13\x17\x1F\x1F"      /* symbol polovicni baterie  */
#define LCD_CHAR_BAT0   "\x0E\x1F\x11\x11\x11\x11\x11\x1F"      /* symbol vybite baterie     */
#define LCD_CHAR_UP     "\x80\x04\x0E\x15\x04\x04\x04\x80"      /* symbol sipka nahoru       */
#define LCD_CHAR_DOWN   "\x80\x04\x04\x04\x15\x0E\x04\x80"      /* symbol Sipka dolu         */
#define LCD_CHAR_LUA    "\x04\x0E\x11\x11\x1F\x11\x11\x80"      /* A s carkou                */
#define LCD_CHAR_LLA    "\x01\x02\x0E\x01\x1F\x11\x0F\x80"      /* a s carkou                */
#define LCD_CHAR_HUC    "\x0A\x0E\x11\x10\x10\x11\x0E\x80"      /* C s hackem                */
#define LCD_CHAR_HLC    "\x0A\x04\x0E\x10\x10\x11\x0E\x80"      /* c s hackem                */
#define LCD_CHAR_HUD    "\x0A\x1C\x12\x11\x11\x12\x1C\x80"      /* D s hackem                */
#define LCD_CHAR_HLD    "\x05\x03\x0D\x13\x11\x11\x0F\x80"      /* d s hackem                */
#define LCD_CHAR_LUE    "\x04\x1F\x10\x10\x1E\x10\x1F\x80"      /* E s carkou                */
#define LCD_CHAR_LLE    "\x01\x02\x0E\x11\x1F\x10\x0E\x80"      /* e s carkou                */
#define LCD_CHAR_HUE    "\x0A\x1F\x10\x1E\x10\x10\x1F\x80"      /* E s hackem                */
#define LCD_CHAR_HLE    "\x0A\x04\x0E\x11\x1F\x10\x0E\x80"      /* e s hackem                */
#define LCD_CHAR_LUI    "\x04\x0E\x04\x04\x04\x04\x0E\x80"      /* I s carkou                */
#define LCD_CHAR_LLI    "\x02\x04\x80\x0C\x04\x04\x0E\x80"      /* i s carkou                */
#define LCD_CHAR_HUN    "\x0A\x15\x11\x19\x15\x13\x11\x80"      /* N s hackem                */
#define LCD_CHAR_HLN    "\x0A\x04\x16\x19\x11\x11\x11\x80"      /* n s hackem                */
#define LCD_CHAR_LUO    "\x04\x0E\x11\x11\x11\x11\x0E\x80"      /* O s carkou                */
#define LCD_CHAR_LLO    "\x02\x04\x0E\x11\x11\x11\x0E\x80"      /* o s carkou                */
#define LCD_CHAR_HUR    "\x0A\x1E\x11\x1E\x14\x12\x11\x80"      /* R s hackem                */
#define LCD_CHAR_HLR    "\x0A\x04\x16\x19\x10\x10\x10\x80"      /* r s hackem                */
#define LCD_CHAR_HUS    "\x0A\x0F\x10\x0E\x01\x01\x1E\x80"      /* S s hackem                */
#define LCD_CHAR_HLS    "\x0A\x04\x0E\x10\x0E\x01\x1E\x80"      /* s s hackem                */
#define LCD_CHAR_HUT    "\x0A\x1F\x04\x04\x04\x04\x04\x80"      /* T s hackem                */
#define LCD_CHAR_HLT    "\x0A\x0C\x1C\x08\x08\x09\x06\x80"      /* t s hackem                */
#define LCD_CHAR_LUU    "\x02\x15\x11\x11\x11\x11\x0E\x80"      /* U s carkou                */
#define LCD_CHAR_LLU    "\x02\x04\x11\x11\x11\x13\x0D\x80"      /* u s carkou                */
#define LCD_CHAR_CUU    "\x06\x17\x11\x11\x11\x11\x0E\x80"      /* U s krouzkem              */
#define LCD_CHAR_CLU    "\x06\x06\x11\x11\x11\x11\x0E\x80"      /* u s krouzkem              */
#define LCD_CHAR_LUY    "\x02\x15\x11\x0A\x04\x04\x04\x80"      /* Y s carkou                */
#define LCD_CHAR_LLY    "\x02\x04\x11\x11\x0F\x01\x0E\x80"      /* y s carkou                */
#define LCD_CHAR_HUZ    "\x0A\x1F\x01\x02\x04\x08\x1F\x80"      /* Z s hackem                */
#define LCD_CHAR_HLZ    "\x0A\x04\x1F\x02\x04\x08\x1F\x80"      /* z s hackem                */


// Priklad pouziti definovanych znaku
//
//
//void lcd_sample()
//{
//   lcd_define_char(0,LCD_CHAR_BAT50);                 // Priklad definice znaku baterie do pozice 0
//   lcd_define_char(2,LCD_CHAR_HLE LCD_CHAR_LUI);      // Priklad definice znaku e s hackem a I s carkou od pozice 2
//                                                      // vsimnete si, ze neni carka mezi retezci s definici (oba retezce definuji
//                                                      // jediny definicni retezec)
//   printf(lcd_putc,"\fZnaky:\20\22\23");              // priklad vypisu znaku z pozice 0, 2 a 3
//   delay_ms(1000);
//   lcd_define_char(0,LCD_CHAR_BAT0);                  // Predefinovani tvaru znaku v pozici 0
//   delay_ms(1000);
//}
