//////// Header file for the PIC16F88
#device PIC16F88
#nolist
//////// Program memory: 4096x14  Data RAM: 368  Stack: 8
//////// I/O: 16   Analog Pins: 7
//////// Data EEPROM: 256
//////// C Scratch area: 77   ID Location: 2000
// Fuses:
// Oscilator: LP         - oscilator LP
//            XT         - oscilator XT
//            HS         - oscilator HS
//            EC_IO      - externi vstup, RA6/CLKO je IO port
//            INTRC      - RC oscilator,  RA6/CLKO je CLKO, RA7/CLKI je IO port port,
//            INTRC_IO   - RC oscilator,  RA6 i RA7 je IO port
//            RC         - ext RC, RA6/CLKO je CLKO
//            RC_IO      - ext RC, RA6 je IO port
// Watch:     NOWDT      - neni watchog
//            WDT        - je watchdog
// PUT:       NOPUT      - neni power up timer
//            PUT        - je power up timer
// MCLR:      MCLR       - RA5/MCLR je MCLR
//            NOMCLR     - RA5/MCLR je IO port
// BOR:       BROWNOUT   - BOR povolen
//            NOBROWNOUT - BOR zakazan
// LVP:       LVP        - RB3/PGM je PGM
//            NOLVP      - RB3/PGM je IO port
// CPD:       CPD        - je ochrana EEPROM
//            NOCPD      - neni ochrana EEPROM
// WRT        WRT        - zakaz zapisu do pameti programu
//            NOWRT      - povolen zapis do pameti programu
// DEBUG:     DEBUG      - RB6 a RB7 jsou ICD port
//            NODEBUG    - RB6 a RB7 jsou IO port
// CCPMX:     CCPB0      - CCP/PWM na RB0
//            CCPB3      - CCP/PWM na RB3
// CP:        PROTECT    - pamet programu je chranena
//            NOPROTECT  - pamet programu neni chranena
//

////////////////////////////////////////////////////////////////// I/O
// Discrete I/O Functions: SET_TRIS_x(), OUTPUT_x(), INPUT_x(),
//                         PORT_B_PULLUPS(), INPUT(),
//                         OUTPUT_LOW(), OUTPUT_HIGH(),
//                         OUTPUT_FLOAT(), OUTPUT_BIT()
// Constants used to identify pins in the above are:



#define PIN_A0  40
#define PIN_A1  41
#define PIN_A2  42
#define PIN_A3  43
#define PIN_A4  44
#define PIN_A5  45
#define PIN_A6  46
#define PIN_A7  47

#define PIN_B0  48
#define PIN_B1  49
#define PIN_B2  50
#define PIN_B3  51
#define PIN_B4  52
#define PIN_B5  53
#define PIN_B6  54
#define PIN_B7  55

////////////////////////////////////////////////////////////////// Useful defines
#define FALSE 0
#define TRUE 1

#define BYTE int
#define BOOLEAN short int

#define getc getch
#define fgetc getch
#define getchar getch
#define putc putchar
#define fputc putchar
#define fgets gets
#define fputs puts

////////////////////////////////////////////////////////////////// Control
// Control Functions:  RESET_CPU(), SLEEP(), RESTART_CAUSE()
// Constants returned from RESTART_CAUSE() are:
#define WDT_FROM_SLEEP  0
#define WDT_TIMEOUT     8
#define MCLR_FROM_SLEEP 16
#define NORMAL_POWER_UP 24


////////////////////////////////////////////////////////////////// Timer 0
// Timer 0 (AKA RTCC)Functions: SETUP_COUNTERS() or SETUP_TIMER0(),
//                              SET_TIMER0() or SET_RTCC(),
//                              GET_TIMER0() or GET_RTCC()
// Constants used for SETUP_TIMER0() are:
#define RTCC_INTERNAL   0
#define RTCC_EXT_L_TO_H 32
#define RTCC_EXT_H_TO_L 48

#define RTCC_DIV_1      8
#define RTCC_DIV_2      0
#define RTCC_DIV_4      1
#define RTCC_DIV_8      2
#define RTCC_DIV_16     3
#define RTCC_DIV_32     4
#define RTCC_DIV_64     5
#define RTCC_DIV_128    6
#define RTCC_DIV_256    7


#define RTCC_8_BIT      0

// Constants used for SETUP_COUNTERS() are the above
// constants for the 1st param and the following for
// the 2nd param:

////////////////////////////////////////////////////////////////// WDT
// Watch Dog Timer Functions: SETUP_WDT() or SETUP_COUNTERS() (see above)
//                            RESTART_WDT()
//
#define WDT_18MS        8
#define WDT_36MS        9
#define WDT_72MS       10
#define WDT_144MS      11
#define WDT_288MS      12
#define WDT_576MS      13
#define WDT_1152MS     14
#define WDT_2304MS     15

////////////////////////////////////////////////////////////////// Timer 1
// Timer 1 Functions: SETUP_TIMER_1, GET_TIMER1, SET_TIMER1
// Constants used for SETUP_TIMER_1() are:
//      (or (via |) together constants from each group)
#define T1_DISABLED         0
#define T1_INTERNAL         0x85
#define T1_EXTERNAL         0x87
#define T1_EXTERNAL_SYNC    0x83

#define T1_CLK_OUT          8

#define T1_DIV_BY_1         0
#define T1_DIV_BY_2         0x10
#define T1_DIV_BY_4         0x20
#define T1_DIV_BY_8         0x30

////////////////////////////////////////////////////////////////// Timer 2
// Timer 2 Functions: SETUP_TIMER_2, GET_TIMER2, SET_TIMER2
// Constants used for SETUP_TIMER_2() are:
#define T2_DISABLED         0
#define T2_DIV_BY_1         4
#define T2_DIV_BY_4         5
#define T2_DIV_BY_16        6

////////////////////////////////////////////////////////////////// CCP
// CCP Functions: SETUP_CCPx, SET_PWMx_DUTY
// CCP Variables: CCP_x, CCP_x_LOW, CCP_x_HIGH
// Constants used for SETUP_CCPx() are:
#define CCP_OFF                         0
#define CCP_CAPTURE_FE                  4
#define CCP_CAPTURE_RE                  5
#define CCP_CAPTURE_DIV_4               6
#define CCP_CAPTURE_DIV_16              7
#define CCP_COMPARE_SET_ON_MATCH        8
#define CCP_COMPARE_CLR_ON_MATCH        9
#define CCP_COMPARE_INT                 0xA
#define CCP_COMPARE_RESET_TIMER         0xB
#define CCP_PWM                         0xC
#define CCP_PWM_PLUS_1                  0x1c
#define CCP_PWM_PLUS_2                  0x2c
#define CCP_PWM_PLUS_3                  0x3c
long CCP_1;
#byte   CCP_1    =                      0x15
#byte   CCP_1_LOW=                      0x15
#byte   CCP_1_HIGH=                     0x16
////////////////////////////////////////////////////////////////// COMP
// Comparator Variables: C1OUT, C2OUT
// Constants used in setup_comparators() are:
#define A0_A3_A1_A2  4
#define A0_A2_A1_A2  3
#define NC_NC_A1_A2  5
#define NC_NC_NC_NC  7
#define A0_VR_A1_VR  2
#define A3_VR_A2_VR  10
#define A0_A2_A1_A2_OUT_ON_A3_A4 6
#define A3_A2_A1_A2  9

//#bit C1OUT = 0x1f.6
//#bit C2OUT = 0x1f.7

////////////////////////////////////////////////////////////////// VREF
// Constants used in setup_vref() are:
#define VREF_LOW  0xa0
#define VREF_HIGH 0x80
#define VREF_A2   0x40

////////////////////////////////////////////////////////////////// INT
// Interrupt Functions: ENABLE_INTERRUPTS(), DISABLE_INTERRUPTS(),
//                      EXT_INT_EDGE()
//
// Constants used in EXT_INT_EDGE() are:
#define L_TO_H              0x40
#define H_TO_L                 0
// Constants used in ENABLE/DISABLE_INTERRUPTS() are:
#define GLOBAL                    0x0BC0
#define INT_RTCC                  0x0B20
#define INT_RB                    0x0B08
#define INT_EXT                   0x0B10
#define INT_TBE                   0x8C10
#define INT_RDA                   0x8C20
#define INT_TIMER1                0x8C01
#define INT_TIMER2                0x8C02
#define INT_CCP1                  0x8C04
#define INT_SSP                   0x8C08
#define INT_COMP                  0x8D40
#define INT_EEPROM                0x8D10
#define INT_TIMER0                0x0B20
#list