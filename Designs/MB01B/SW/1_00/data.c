// Tento soubor definuje vse potrebne pro ulozeni skladeb do pameti programu.
// Dale definuje pislusna makra pro snadne definice. Tyto definice jsou voleny
// zamerne tak, aby mohl byt pouzit stejny soubor s definici skladby jak
// pro vlozeni primo do firmwaru v dobe prekladu tak i k dodatecnemu nahrani
// skladby pres seriovou linku.

// Definice hranic pameti pro ulozeni pisnicek
#define  STARTMEM       0x600    // Zde zacina pametoa oblast pro pisnicky
#define  ENDMEM         0x7ff    // Zde konci pametova oblast pro pisnicky

// Definice konstant pro kodovani dat
#define  ENDOFDATA      0x3FFF   // Priznak konce pisnicek (prazdna pamet)
#define  DATABEGIN      0x2000   // Kod zacatku skladby
#define  MASKBEGIN      0x3800
#define  DATATEMPO      0x2800   // Kod pro nastaveni tempa
#define  MASKTEMPO      0x3800
#define  DATAPAUSE      0x3000   // Kod pro nastaveni mezery mezi notami
#define  MASKPAUSE      0x3800
#define  MASKNOTE       0x2000   // Nejvissi bit 0 urcuje, ze jde o notu

// Pseudomakra pro jednoduchy zapis skladeb do zdrojaku jazyka C
#define  BEGIN          DATABEGIN+  // Zacatek pisnicky + posunuti oktav
#define  TEMPO          DATATEMPO+  // Delka nejskratsi noty v ms
#define  PAUSE          DATAPAUSE+  // Delka mezery mezi notami
#define  END                        // Konec skladby, zde nic neznamena

// Pseudomakra pro zapis not, zapisuje se cislo oktavy, nota, delka noty
#define  C              *16 + SOUND_C       + 128 *
#define  Cis            *16 + SOUND_Cis     + 128 *
#define  D              *16 + SOUND_D       + 128 *
#define  Dis            *16 + SOUND_Dis     + 128 *
#define  E              *16 + SOUND_E       + 128 *
#define  F              *16 + SOUND_F       + 128 *
#define  Fis            *16 + SOUND_Fis     + 128 *
#define  G              *16 + SOUND_G       + 128 *
#define  Gis            *16 + SOUND_Gis     + 128 *
#define  A              *16 + SOUND_A       + 128 *
#define  Ais            *16 + SOUND_Ais     + 128 *
#define  H              *16 + SOUND_H       + 128 *
#define  Space                SOUND_Space   + 128 *

// Pametova oblast
#ORG 0x600,0x7FF {}     // Vyhrazeni oblasti pameti pro ulozeni pisnicek
#ROM 0x600 = {          // Naplneni oblasti pameti daty

// Pisnicky jako soucast firwaru se vkladaji sem
// Museji mit stejny format jako uvedene priklady aby zafungovaly makra
#include "Skladby\TheFinalSoundDown.txt"
#include "Skladby\BednaOdWhisky.txt"
#include "Skladby\KdyzMeBaliZaVojacka.txt"
#include "Skladby\Medvedi.txt"

// Koncova znacka a konec oblasti pro skladby
ENDOFDATA
}

// Zruseni definic maker pro definici skladeb
// V dalsim programu uz nebudou potreba
#undef   BEGIN
#undef   TEMPO
#undef   PAUSE
#undef   END
#undef   C
#undef   Cis
#undef   D
#undef   Dis
#undef   E
#undef   F
#undef   Fis
#undef   G
#undef   Gis
#undef   A
#undef   Ais
#undef   H
#undef   Space
