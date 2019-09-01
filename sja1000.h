//*****************************************************************************
//
// Title        : AVR based USB<>CAN adaptor
// Authors      : Michael Wolf
// File Name    : 'sja1000.h'
// Date         : August 24, 2005
// Version      : 1.03
// Target MCU   : Atmel AVR ATmega162
// Editor Tabs  : 2
//
// NOTE: The authors in no way will be responsible for damages that you
//       coul'd be using this code.
//       Use this code at your own risk.
//
//       This code is distributed under the GNU Public License
//       which can be found at http://www.gnu.org/licenses/gpl.txt
//
// Change Log
//
// Version  When        Who           What
// -------  ----        ---           ----
// 1.00     24/08/2005  Michael Wolf  Initial Release
// 1.01     17/08/2006  Michael Wolf  * Changed predefined Baudrate values to
//                                      fix possible bus errors
// 1.02		02/08/2009	Michael Wolf  + added support for CANdip module
//									  + added support for more fixed Baudrates
//									  * changed fixed Baudrates to Peak values
// 1.03		06/01/2010	Michael Wolf  * changed fixed Baudrates for 250K and 500K
//									  * CANDIP is undefined by default
// 1.04		04/17/2016	Michael Wolf  * Changed BTR values for various CAN speeds
//
//*****************************************************************************
#ifndef __SJA1000_H__
#define __SJA1000_H__

#include <stdint.h>

//#define CANDIP	// enable this to run the code on a CANdip module

#define CAN_BASE    0x1100	// external RAM adress, high nibble doesn't care
#define PeliCANMode		    // code will only work in PeliCAN mode!
//#define ENABLE_SELFTEST   // run transmission in self test mode
//#define SINGLE_SHOT       // use single shot transmission

#define CAN_CS          PC7	// SJA1000 Chip Select signal
#define CAN_CS_PORT     PORTC
#define CAN_CS_PIN      PINC
#define CAN_CS_DIR      DDRC

#define CAN_RST         PE2	// SJA1000 Reset signal
#define CAN_RST_PORT    PORTE
#define CAN_RST_PIN     PINE
#define CAN_RST_DIR     DDRE

#define CAN_INT         PD2	// SJA1000 Interrupt signal
#define CAN_INT_PORT    PORTD
#define CAN_INT_PIN     PIND
#define CAN_INT_DIR     DDRD

#define LED_PORT        PORTC	// Status LED's
#define LED_DIR         DDRC
#define LED_GREEN       PC5
#define LED_RED         PC6
#define CAN_LED_ONTIME  5	// milliseconds to indicate CAN traffic or Error

// fixed BTR values for recommended Baud rates
// Baudrate values used in Peak CAN interface
#if 0

#define BTR0_5k     0x7F
#define BTR1_5k     0x7F

#define BTR0_10k    0x67
#define BTR1_10k    0x2F

#define BTR0_20k    0x53
#define BTR1_20k    0x2F

#define BTR0_33k    0x1D
#define BTR1_33k    0x14

#define BTR0_47k    0x14
#define BTR1_47k    0x14

#define BTR0_50k    0x47
#define BTR1_50k    0x2F

#define BTR0_83k    0x4B
#define BTR1_83k    0x14

#define BTR0_95k    0xC3
#define BTR1_95k    0x4E

#define BTR0_100k   0x43
#define BTR1_100k   0x2F

#define BTR0_125k   0x03
#define BTR1_125k   0x1C

#define BTR0_250k   0x01
#define BTR1_250k   0x1C

#define BTR0_500k   0x00
#define BTR1_500k   0x1C

#define BTR0_800k   0x00
#define BTR1_800k   0x16

#define BTR0_1M     0x00
#define BTR1_1M     0x14

#else

#define BTR0_5k 0x7F
#define BTR1_5k 0x7F

#define BTR0_10k 0x67
#define BTR1_10k 0x2F

#define BTR0_20k 0x53
#define BTR1_20k 0x2F

#define BTR0_33k 0x8B
#define BTR1_33k 0x2F

#define BTR0_47k 0x14
#define BTR1_47k 0x14

#define BTR0_50k 0x47
#define BTR1_50k 0x2F

#define BTR0_83k 0x85
#define BTR1_83k 0x2B

#define BTR0_95k 0xC3
#define BTR1_95k 0x4E

#define BTR0_100k 0x43
#define BTR1_100k 0x2F

#define BTR0_125k 0x03
#define BTR1_125k 0x1C

#define BTR0_250k 0x01
#define BTR1_250k 0x1C

#define BTR0_500k 0x00
#define BTR1_500k 0x1C

#define BTR0_800k 0x00
#define BTR1_800k 0x16

#define BTR0_1M 0x00
#define BTR1_1M 0x14

#endif

// Functions prototypes
uint8_t init_CAN (void);
uint8_t transmit_CAN (void);
void CAN_LED (uint8_t led_no);
uint8_t read_CAN_reg (uint8_t reg);
void write_CAN_reg (uint8_t reg,
		    uint8_t data);

// define bit macros
#define SETBIT(x,y) (x |= (y))	// Set bit y in byte x
#define CLEARBIT(x,y) (x &= (~y))	// Clear bit y in byte x
#define CHECKBIT(x,y) (x & (y))	// Check bit y in byte x

// define local CAN status flags
#define CAN_INIT          0x0001	// set if CAN controller is initalized
#define MSG_WAITING       0x0002	// set if Rx message is waiting
#define BUS_ON            0x0004	// set if CAN controller is in oper mode
#define TX_BUSY           0x0008	// set if transmit is in progress

#define ERROR_WARNING     0x0400	// set if error warning was detected
#define DATA_OVERRUN      0x0800	// set if data overrun was detected
#define ERROR_PASSIVE     0x2000	// set if controller changes to error passive mode
#define ARB_LOST          0x4000	// set if arbitation lost was detected
#define BUS_ERROR         0x8000	// set if bus error was detected

#if !defined(PeliCANMode)
#error "### This code will only work in PeliCAN mode! ###"
#endif

/* address and bit definitions for the Mode & Control Register */
#define ModeControlReg  *(uint8_t *) (CAN_BASE + 0)
#define RM_RR_Bit   0		/* reset mode (request) bit */

#if defined (PeliCANMode)
#define LOM_Bit   1		/* listen only mode bit */
#define STM_Bit   2		/* self test mode bit */
#define AFM_Bit   3		/* acceptance filter mode bit */
#define SM_Bit    4		/* enter sleep mode bit */
#endif

/* address and bit definitions for the
Interrupt Enable & Control Register */
#if defined (PeliCANMode)
#define InterruptEnReg  *(uint8_t *) (CAN_BASE + 4)	/* PeliCAN mode */
#define RIE_Bit   0		/* receive interrupt enable bit */
#define TIE_Bit   1		/* transmit interrupt enable bit */
#define EIE_Bit   2		/* error warning interrupt enable bit */
#define DOIE_Bit  3		/* data overrun interrupt enable bit */
#define WUIE_Bit  4		/* wake-up interrupt enable bit */
#define EPIE_Bit  5		/* error passive interrupt enable bit */
#define ALIE_Bit  6		/* arbitration lost interr. enable bit */
#define BEIE_Bit  7		/* bus error interrupt enable bit */
#else /* BasicCAN mode */
#define InterruptEnReg  *(uint8_t *)(CAN_BASE + 0)	/* Control Register */
#define RIE_Bit   1		/* Receive Interrupt enable bit */
#define TIE_Bit   2		/* Transmit Interrupt enable bit */
#define EIE_Bit   3		/* Error Interrupt enable bit */
#define DOIE_Bit  4		/* Overrun Interrupt enable bit */
#endif

/* address and bit definitions for the Command Register */
#define CommandReg  *(uint8_t *)(CAN_BASE + 1)
#define TR_Bit      0		/* transmission request bit */
#define AT_Bit      1		/* abort transmission bit */
#define RRB_Bit     2		/* release receive buffer bit */
#define CDO_Bit     3		/* clear data overrun bit */

#if defined (PeliCANMode)
#define SRR_Bit   4		/* self reception request bit */
#else /* BasicCAN mode */
#define GTS_Bit   4		/* goto sleep bit (BasicCAN mode) */
#endif

/* address and bit definitions for the Status Register */
#define StatusReg  *(uint8_t *)(CAN_BASE + 2)
#define RBS_Bit     0		/* receive buffer status bit */
#define DOS_Bit     1		/* data overrun status bit */
#define TBS_Bit     2		/* transmit buffer status bit */
#define TCS_Bit     3		/* transmission complete status bit */
#define RS_Bit      4		/* receive status bit */
#define TS_Bit      5		/* transmit status bit */
#define ES_Bit      6		/* error status bit */
#define BS_Bit      7		/* bus status bit */

/* address and bit definitions for the Interrupt Register */
#define InterruptReg  *(uint8_t *)(CAN_BASE + 3)
#define RI_Bit      0		/* receive interrupt bit */
#define TI_Bit      1		/* transmit interrupt bit */
#define EI_Bit      2		/* error warning interrupt bit */
#define DOI_Bit     3		/* data overrun interrupt bit */
#define WUI_Bit     4		/* wake-up interrupt bit */
#if defined (PeliCANMode)
#define EPI_Bit   5		/* error passive interrupt bit */
#define ALI_Bit   6		/* arbitration lost interrupt bit */
#define BEI_Bit   7		/* bus error interrupt bit */
#endif

/* address and bit definitions for the Bus Timing Registers */
#define BusTiming0Reg  *(uint8_t *)(CAN_BASE + 6)
#define BusTiming1Reg  *(uint8_t *)(CAN_BASE + 7)
#define SAM_Bit     7		/* sample mode bit
				   1 == the bus is sampled 3 times
				   0 == the bus is sampled once */

/* address and bit definitions for the Output Control Register */
#define OutControlReg  *(uint8_t *)(CAN_BASE + 8)
/* OCMODE1, OCMODE0 */
#define BiPhaseMode 0x00	/* bi-phase output mode */
#define NormalMode  0x02	/* normal output mode */
#define ClkOutMode  0x03	/* clock output mode */
/* output pin configuration for TX1 */
#define OCPOL1_Bit  5		/* output polarity control bit */
#define Tx1Float    0x00	/* configured as float */
#define Tx1PullDn   0x40	/* configured as pull-down */
#define Tx1PullUp   0x80	/* configured as pull-up */
#define Tx1PshPull  0xC0	/* configured as push/pull */
/* output pin configuration for TX0 */
#define OCPOL0_Bit  2		/* output polarity control bit */
#define Tx0Float    0x00	/* configured as float */
#define Tx0PullDn   0x08	/* configured as pull-down */
#define Tx0PullUp   0x10	/* configured as pull-up */
#define Tx0PshPull  0x18	/* configured as push/pull */

/* address definitions of Acceptance Code & Mask Registers */
#if defined (PeliCANMode)
#define AcceptCode0Reg  *(uint8_t *)(CAN_BASE + 16)
#define AcceptCode1Reg  *(uint8_t *)(CAN_BASE + 17)
#define AcceptCode2Reg  *(uint8_t *)(CAN_BASE + 18)
#define AcceptCode3Reg  *(uint8_t *)(CAN_BASE + 19)
#define AcceptMask0Reg  *(uint8_t *)(CAN_BASE + 20)
#define AcceptMask1Reg  *(uint8_t *)(CAN_BASE + 21)
#define AcceptMask2Reg  *(uint8_t *)(CAN_BASE + 22)
#define AcceptMask3Reg  *(uint8_t *)(CAN_BASE + 23)
#else /* BasicCAN mode */
#define AcceptCodeReg  *(uint8_t *)(CAN_BASE + 4)
#define AcceptMaskReg  *(uint8_t *)(CAN_BASE + 5)
#endif

/* address definitions of the Rx-Buffer */
#if defined (PeliCANMode)
#define RxFrameInfo  *(uint8_t *)(CAN_BASE + 16)
#define RxBuffer1  *(uint8_t *)(CAN_BASE + 17)
#define RxBuffer2  *(uint8_t *)(CAN_BASE + 18)
#define RxBuffer3  *(uint8_t *)(CAN_BASE + 19)
#define RxBuffer4  *(uint8_t *)(CAN_BASE + 20)
#define RxBuffer5  *(uint8_t *)(CAN_BASE + 21)
#define RxBuffer6  *(uint8_t *)(CAN_BASE + 22)
#define RxBuffer7  *(uint8_t *)(CAN_BASE + 23)
#define RxBuffer8  *(uint8_t *)(CAN_BASE + 24)
#define RxBuffer9  *(uint8_t *)(CAN_BASE + 25)
#define RxBuffer10  *(uint8_t *)(CAN_BASE + 26)
#define RxBuffer11  *(uint8_t *)(CAN_BASE + 27)
#define RxBuffer12  *(uint8_t *)(CAN_BASE + 28)
#else /* BasicCAN mode */
#define RxBuffer1  *(uint8_t *)(CAN_BASE + 20)
#define RxBuffer2  *(uint8_t *)(CAN_BASE + 21)
#define RxBuffer3  *(uint8_t *)(CAN_BASE + 22)
#define RxBuffer4  *(uint8_t *)(CAN_BASE + 23)
#define RxBuffer5  *(uint8_t *)(CAN_BASE + 24)
#define RxBuffer6  *(uint8_t *)(CAN_BASE + 25)
#define RxBuffer7  *(uint8_t *)(CAN_BASE + 26)
#define RxBuffer8  *(uint8_t *)(CAN_BASE + 27)
#define RxBuffer9  *(uint8_t *)(CAN_BASE + 28)
#define RxBuffer10  *(uint8_t *)(CAN_BASE + 29)
#endif

/* address definitions of the Tx-Buffer */
#if defined (PeliCANMode)
  /* write only addresses */
#define FF_Bit    7		/* Frame Format Bit */
#define RTR_Bit   6		/* Remote Transmission Request Bit */
#define TxFrameInfo *(uint8_t *)(CAN_BASE + 16)
#define TxBuffer1  *(uint8_t *)(CAN_BASE + 17)
#define TxBuffer2  *(uint8_t *)(CAN_BASE + 18)
#define TxBuffer3  *(uint8_t *)(CAN_BASE + 19)
#define TxBuffer4  *(uint8_t *)(CAN_BASE + 20)
#define TxBuffer5  *(uint8_t *)(CAN_BASE + 21)
#define TxBuffer6  *(uint8_t *)(CAN_BASE + 22)
#define TxBuffer7  *(uint8_t *)(CAN_BASE + 23)
#define TxBuffer8  *(uint8_t *)(CAN_BASE + 24)
#define TxBuffer9  *(uint8_t *)(CAN_BASE + 25)
#define TxBuffer10  *(uint8_t *)(CAN_BASE + 26)
#define TxBuffer11  *(uint8_t *)(CAN_BASE + 27)
#define TxBuffer12  *(uint8_t *)(CAN_BASE + 28)
  /* read only addresses */
#define TxFrameInfoRd  *(uint8_t *)(CAN_BASE + 96)
#define TxBufferRd1  *(uint8_t *)(CAN_BASE + 97)
#define TxBufferRd2  *(uint8_t *)(CAN_BASE + 98)
#define TxBufferRd3  *(uint8_t *)(CAN_BASE + 99)
#define TxBufferRd4  *(uint8_t *)(CAN_BASE + 100)
#define TxBufferRd5  *(uint8_t *)(CAN_BASE + 101)
#define TxBufferRd6  *(uint8_t *)(CAN_BASE + 102)
#define TxBufferRd7  *(uint8_t *)(CAN_BASE + 103)
#define TxBufferRd8  *(uint8_t *)(CAN_BASE + 104)
#define TxBufferRd9  *(uint8_t *)(CAN_BASE + 105)
#define TxBufferRd10  *(uint8_t *)(CAN_BASE + 106)
#define TxBufferRd11  *(uint8_t *)(CAN_BASE + 107)
#define TxBufferRd12  *(uint8_t *)(CAN_BASE + 108)
#else /* BasicCAN mode */
#define TxBuffer1  *(uint8_t *)(CAN_BASE + 10)
#define TxBuffer2  *(uint8_t *)(CAN_BASE + 11)
#define TxBuffer3  *(uint8_t *)(CAN_BASE + 12)
#define TxBuffer4  *(uint8_t *)(CAN_BASE + 13)
#define TxBuffer5  *(uint8_t *)(CAN_BASE + 14)
#define TxBuffer6  *(uint8_t *)(CAN_BASE + 15
#define TxBuffer7  *(uint8_t *)(CAN_BASE + 16)
#define TxBuffer8  *(uint8_t *)(CAN_BASE + 17)
#define TxBuffer9  *(uint8_t *)(CAN_BASE + 18)
#define TxBuffer10  *(uint8_t *)(CAN_BASE + 19)
#endif

/* address definitions of Other Registers */
#if defined (PeliCANMode)
#define ArbLostCapReg  *(uint8_t *)(CAN_BASE + 11)
#define ErrCodeCapReg  *(uint8_t *)(CAN_BASE + 12)
#define ErrWarnLimitReg  *(uint8_t *)(CAN_BASE + 13)
#define RxErrCountReg  *(uint8_t *)(CAN_BASE + 14)
#define TxErrCountReg  *(uint8_t *)(CAN_BASE + 15)
#define RxMsgCountReg  *(uint8_t *)(CAN_BASE + 29)
#define RxBufStartAdr  *(uint8_t *)(CAN_BASE + 30)
#endif

/* address and bit definitions for the Clock Divider Register */
#define ClockDivideReg  *(uint8_t *) (CAN_BASE + 31)
#define DivBy1      0x07	/* CLKOUT = oscillator frequency */
#define DivBy2      0x00	/* CLKOUT = 1/2 oscillator frequency */
#define DivBy4      0x01	/* CLKOUT = 1/4 oscillator frequency */
#define DivBy6      0x02	/* CLKOUT = 1/6 oscillator frequency */
#define DivBy8      0x03	/* CLKOUT = 1/8 oscillator frequency */
#define DivBy10     0x04	/* CLKOUT = 1/10 oscillator frequency */
#define DivBy12     0x05	/* CLKOUT = 1/12 oscillator frequency */
#define DivBy14     0x06	/* CLKOUT = 1/14 oscillator frequency */
#define ClkOff_Bit  3		/* clock off bit,control of the CLK OUT pin */
#define RXINTEN_Bit 5		/* pin TX1 used for receive interrupt */
#define CBP_Bit     6		/* CAN comparator bypass control bit */
#define CANMode_Bit 7		/* CAN mode definition bit */

#if !defined(CR)
#define CR    13
#endif

#if !defined(ERROR)
#define ERROR    7
#endif

#define LED_TCNT_VALUE (255-(unsigned char)((unsigned long)(CAN_LED_ONTIME*1000L) / (1000000L / (float)((unsigned long)F_CPU / 1024L))))

#endif // __SJA1000_H__
