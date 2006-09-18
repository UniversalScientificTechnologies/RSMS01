// EEPROM.C - knihovna pro pohodlny zapis a cteni promennych do a z pameti
// EEPROM a to pro ruzne typy promennych.
//
// (c)miho 2002
//
// Historie:
//
//    0.00  Uvodni verze
//    0.01  Formalni zmena


// Priklad:
//
//       int32 MyInt;            // deklarace typu (libovolne delky)
//       EE_WR(10, MyInt);       // zapis promenne MyInt do EEPROM od adresy 10
//       EE_RD(10, MyInt);       // zpetne nacteni promenne MyInt z EEPROM
//


// Makro pro jednotne ukladani a vybirani dat (promennych) do a z pameti EEPROM
//
#define EE_WR(EEAddress, Data)    EE_Write(EEAddress, &Data, sizeof(Data))
#define EE_RD(EEAddress, Data)    EE_Read (EEAddress, &Data, sizeof(Data))


// Ulozeni promenne do pameti EEPROM
//
void EE_Write(int EEAddress, DataPtr, Len)
{
   do
      write_eeprom(EEAddress++, *DataPtr++);
   while (--Len);
}


// Nacteni promenne z pameti EEPROM
//
void EE_Read(int EEAddress, DataPtr, Len)
{
   do
      *DataPtr++ = read_eeprom(EEAddress++);
   while (--Len);
}
