#nolist
//
// Komplete definition of all Special Feature Registers for CCS C compiler
//
//    PIC16F87
//    PIC16F88
//
// (c)miho 2005
//
// History:
//
// 1.00 First Version, not verified yet


// SFR Registers in Memory Bank 0
//
#byte INDF           = 0x00
#byte TMR0           = 0x01
#byte PCL            = 0x02
#byte STATUS         = 0x03
   #bit IRP          = STATUS.7
   #bit RP1          = STATUS.6
   #bit RP0          = STATUS.5
   #bit TO           = STATUS.4
   #bit PD           = STATUS.3
   #bit Z            = STATUS.2
   #bit DC           = STATUS.1
   #bit C            = STATUS.0
#byte FSR            = 0x04
#byte PORTA          = 0x05
#byte PORTB          = 0x06
#byte PCLATH         = 0x0A
#byte INTCON         = 0x0B
   #bit GIE          = INTCON.7
   #bit PEIE         = INTCON.6
   #bit TMR0IE       = INTCON.5
   #bit INT0IE       = INTCON.4
   #bit RBIE         = INTCON.3
   #bit TMR0IF       = INTCON.2
   #bit INT0IF       = INTCON.1
   #bit RBIF         = INTCON.0
#byte PIR1           = 0x0C
   #bit ADIF         = PIR1.6
   #bit RCIF         = PIR1.5
   #bit TXIF         = PIR1.4
   #bit SSPIF        = PIR1.3
   #bit CCP1IF       = PIR1.2
   #bit TMR2IF       = PIR1.1
   #bit TMR1IF       = PIR1.0
#byte PIR2           = 0x0D
   #bit OSFIF        = PIR2.7
   #bit CMIF         = PIR2.6
   #bit EEIF         = PIR2.4
#byte TMR1L          = 0x0E
#byte TMR1H          = 0x0F
#byte T1CON          = 0x10
   #bit T1RUN        = T1CON.6
   #bit T1CKPS1      = T1CON.5
   #bit T1CKPS0      = T1CON.4
   #bit T1OSCEN      = T1CON.3
   #bit T1SYNC       = T1CON.2
   #bit TMR1CS       = T1CON.1
   #bit TMR1ON       = T1CON.0
#byte TMR2           = 0x11
#byte T2CON          = 0x12
   #bit TOUTPS3      = T2CON.6
   #bit TOUTPS2      = T2CON.5
   #bit TOUTPS1      = T2CON.4
   #bit TOUTPS0      = T2CON.3
   #bit TMR2ON       = T2CON.2
   #bit T2CKPS1      = T2CON.1
   #bit T2CKPS0      = T2CON.0
#byte SSPBUF         = 0x13
#byte SSPCON1        = 0x14
   #bit WCOL         = SSPCON1.7
   #bit SSPOV        = SSPCON1.6
   #bit SSPEN        = SSPCON1.5
   #bit CKP          = SSPCON1.4
   #bit SSPM3        = SSPCON1.3
   #bit SSPM2        = SSPCON1.2
   #bit SSPM1        = SSPCON1.1
   #bit SSPM0        = SSPCON1.0
#byte CCPR1L         = 0x15
#byte CCPR1H         = 0x16
#byte CCP1CON        = 0x17
   #bit CCP1X        = CCP1CON.5
   #bit CCP1Y        = CCP1CON.4
   #bit CCP1M3       = CCP1CON.3
   #bit CCP1M2       = CCP1CON.2
   #bit CCP1M1       = CCP1CON.1
   #bit CCP1M0       = CCP1CON.0
#byte RCSTA          = 0x18
   #bit SPEN         = RCSTA.7
   #bit RX9          = RCSTA.6
   #bit SREN         = RCSTA.5
   #bit CREN         = RCSTA.4
   #bit ADDEN        = RCSTA.3
   #bit FERR         = RCSTA.2
   #bit OERR         = RCSTA.1
   #bit RX9D         = RCSTA.0
#byte TXREG          = 0x19
#byte RCREG          = 0x1A
#byte ADRESH         = 0x1E         // F88 only
#byte ADCON0         = 0x1F         // F88 only
   #bit ADCS1        = ADCON0.7
   #bit ADCS0        = ADCON0.6
   #bit CHS2         = ADCON0.5
   #bit CHS1         = ADCON0.4
   #bit CHS0         = ADCON0.3
   #bit GO           = ADCON0.2
   #bit ADON         = ADCON0.0


// SFR Registers in Memory Bank 1
//
#byte INDF_1         = 0x80         // miror
#byte OPTION         = 0x81
   #bit RBPU         = OPTION.7
   #bit INTEDG       = OPTION.6
   #bit T0CS         = OPTION.5
   #bit T0SE         = OPTION.4
   #bit PSA          = OPTION.3
   #bit PS2          = OPTION.2
   #bit PS1          = OPTION.1
   #bit PS0          = OPTION.0
#byte PCL            = 0x82
#byte STATUS_1       = 0x83         // mirror
   #bit IRP_1        = STATUS_1.7
   #bit RP1_1        = STATUS_1.6
   #bit RP0_1        = STATUS_1.5
   #bit TO_1         = STATUS_1.4
   #bit PD_1         = STATUS_1.3
   #bit Z_1          = STATUS_1.2
   #bit DC_1         = STATUS_1.1
   #bit C_1          = STATUS_1.0
#byte FSR            = 0x84
#byte TRISA          = 0x85
#byte TRISB          = 0x86
#byte PCLATH_1       = 0x8A         // mirror
#byte INTCON_1       = 0x8B         // mirror
   #bit GIE_1        = INTCON_1.7
   #bit PEIE_1       = INTCON_1.6
   #bit TMR0IE_1     = INTCON_1.5
   #bit INT0IE_1     = INTCON_1.4
   #bit RBIE_1       = INTCON_1.3
   #bit TMR0IF_1     = INTCON_1.2
   #bit INT0IF_1     = INTCON_1.1
   #bit RBIF_1       = INTCON_1.0
#byte PIE1           = 0x8C
   #bit ADIE         = PIE1.6
   #bit RCIE         = PIE1.5
   #bit TXIE         = PIE1.4
   #bit SSPIE        = PIE1.3
   #bit CCP1IE       = PIE1.2
   #bit TMR2IE       = PIE1.1
   #bit TMR1IE       = PIE1.0
#byte PIE2           = 0x8D
   #bit OSFIE        = PIE2.7
   #bit CMIE         = PIE2.6
   #bit EEIE         = PIE2.4
#byte PCON           = 0x8E
   #bit POR          = PCON.1
   #bit BOR          = PCON.0
#byte OSCCON         = 0x8F
   #bit IRCF2        = OSCCON.6
   #bit IRCF1        = OSCCON.5
   #bit IRCF0        = OSCCON.4
   #bit OSTS         = OSCCON.3
   #bit IOFS         = OSCCON.2
   #bit SCS1         = OSCCON.1
   #bit SCS0         = OSCCON.0
#byte OSCTUNE        = 0x90
   #bit TUN5         = OSCTUNE.5
   #bit TUN4         = OSCTUNE.4
   #bit TUN3         = OSCTUNE.3
   #bit TUN2         = OSCTUNE.2
   #bit TUN1         = OSCTUNE.1
   #bit TUN0         = OSCTUNE.0
#byte PR2            = 0x92
#byte SSPADD         = 0x93
#byte SSPSTAT        = 0x94
   #bit SMP          = SSPSTAT.7
   #bit CKE          = SSPSTAT.6
   #bit DA           = SSPSTAT.5
   #bit P            = SSPSTAT.4
   #bit S            = SSPSTAT.3
   #bit RW           = SSPSTAT.2
   #bit UA           = SSPSTAT.1
   #bit BF           = SSPSTAT.0
#byte TXSTA          = 0x98
   #bit CSRC         = TXSTA.7
   #bit TX9          = TXSTA.6
   #bit TXEN         = TXSTA.5
   #bit SYNC         = TXSTA.4
   #bit BRGH         = TXSTA.2
   #bit TRMT         = TXSTA.1
   #bit TX9D         = TXSTA.0
#byte SPBRG          = 0x99
#byte ANSEL          = 0x9B         // F88 only
   #bit ANS6         = ANSEL.6
   #bit ANS5         = ANSEL.5
   #bit ANS4         = ANSEL.4
   #bit ANS3         = ANSEL.3
   #bit ANS2         = ANSEL.2
   #bit ANS1         = ANSEL.1
   #bit ANS0         = ANSEL.0
#byte CMCON          = 0x9C
   #bit C2OUT        = CMCON.7
   #bit C1OUT        = CMCON.6
   #bit C2INV        = CMCON.5
   #bit C1INV        = CMCON.4
   #bit CIS          = CMCON.3
   #bit CM2          = CMCON.2
   #bit CM1          = CMCON.1
   #bit CM0          = CMCON.0
#byte CVRCON         = 0x9D
   #bit CVREN        = CVRCON.7
   #bit CVROE        = CVRCON.6
   #bit CVRR         = CVRCON.5
   #bit CVR3         = CVRCON.3
   #bit CVR2         = CVRCON.2
   #bit CVR1         = CVRCON.1
   #bit CVR0         = CVRCON.0
#byte ADRESL         = 0x9E         // F88 only
#byte ADCON1         = 0x9F         // F88 only
   #bit ADFM         = ADCON1.7
   #bit ADCS2        = ADCON1.6
   #bit VCFG1        = ADCON1.5
   #bit VCFG0        = ADCON1.4


// SFR Registers in Memory Bank 2
//
#byte INDF_2         = 0x100        // mirror
#byte TMR0_2         = 0x101        // mirror
#byte PCL_2          = 0x102        // mirror
#byte STATUS_2       = 0x103        // mirror
   #bit IRP_2        = STATUS_2.7
   #bit RP1_2        = STATUS_2.6
   #bit RP0_2        = STATUS_2.5
   #bit TO_2         = STATUS_2.4
   #bit PD_2         = STATUS_2.3
   #bit Z_2          = STATUS_2.2
   #bit DC_2         = STATUS_2.1
   #bit C_2          = STATUS_2.0
#byte FSR_2          = 0x104        // mirror
#byte WDTCON         = 0x105
   #bit WDTPS3       = WDTCON.4
   #bit WDTPS2       = WDTCON.3
   #bit WDTPS1       = WDTCON.2
   #bit WDTPS0       = WDTCON.1
   #bit SWDTEN       = WDTCON.0
#byte PORTB_2        = 0x106        // mirror
#byte PCLATH_2       = 0x10A        // mirror
#byte INTCON_2       = 0x10B        // mirror
   #bit GIE_2        = INTCON_2.7
   #bit PEIE_2       = INTCON_2.6
   #bit TMR0IE_2     = INTCON_2.5
   #bit INT0IE_2     = INTCON_2.4
   #bit RBIE_2       = INTCON_2.3
   #bit TMR0IF_2     = INTCON_2.2
   #bit INT0IF_2     = INTCON_2.1
   #bit RBIF_2       = INTCON_2.0
#byte EEDATA         = 0x10C
#byte EEADR          = 0x10D
#byte EEDATH         = 0x10E
#byte EEADRH         = 0x10F


// SFR Registers in Memory Bank 3
//
#byte INDF_3         = 0x180        // mirror
#byte OPTION_3       = 0x181        // mirror
   #bit RBPU_3       = OPTION_3.7
   #bit INTEDG_3     = OPTION_3.6
   #bit T0CS_3       = OPTION_3.5
   #bit T0SE_3       = OPTION_3.4
   #bit PSA_3        = OPTION_3.3
   #bit PS2_3        = OPTION_3.2
   #bit PS1_3        = OPTION_3.1
   #bit PS0_3        = OPTION_3.0
#byte PCL_3          = 0x182        // mirror
#byte STATUS_3       = 0x183        // mirror
   #bit IRP_3        = STATUS_3.7
   #bit RP1_3        = STATUS_3.6
   #bit RP0_3        = STATUS_3.5
   #bit TO_3         = STATUS_3.4
   #bit PD_3         = STATUS_3.3
   #bit Z_3          = STATUS_3.2
   #bit DC_3         = STATUS_3.1
   #bit C_3          = STATUS_3.0
#byte FSR_3          = 0x184        // mirror
#byte TRISB_3        = 0x186        // mirror
#byte PLATH_3        = 0x18A        // mirror
#byte INTCON_3       = 0x18B        // mirror
   #bit GIE_3        = INTCON_3.7
   #bit PEIE_3       = INTCON_3.6
   #bit TMR0IE_3     = INTCON_3.5
   #bit INT0IE_3     = INTCON_3.4
   #bit RBIE_3       = INTCON_3.3
   #bit TMR0IF_3     = INTCON_3.2
   #bit INT0IF_3     = INTCON_3.1
   #bit RBIF_3       = INTCON_3.0
#byte EECON1         = 0x18C
   #bit EEPGD        = EECON1.7
   #bit FREE         = EECON1.4
   #bit WRERR        = EECON1.3
   #bit WREN         = EECON1.2
   #bit WR           = EECON1.1
   #bit RD           = EECON1.0
#byte EECON2         = 0x18D


#list
