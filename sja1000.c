//*****************************************************************************
//
// Title        : AVR based USB<>CAN adaptor
// Authors      : Michael Wolf
// File Name    : 'sja1000.c'
// Date         : August 24, 2005
// Version      : 1.00
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
// 1.01     29/08/2005  Michael Wolf  + routines for register read
//                                      and write
// 1.02     05/09/2005  Michael Wolf  - bug fix in ext. frame ID calculation
// 1.03     02/08/2009  Michael Wolf  + added support for CANdip module
//
//*****************************************************************************
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "sja1000.h"

volatile uint16_t CAN_flags;	// diverse CAN flags
volatile uint8_t last_ecr;	  // content of ECR when bus error interrupt triggers
volatile uint8_t last_alc;	  // content of ALC when arbitration lost interrupt triggers

struct {
    uint8_t format;		// Extended/Standard Frame
    uint32_t id;		    // Frame ID
    uint8_t rtr;		    // RTR/Data Frame
    uint8_t len;		    // Data Length
    uint8_t data[8];		// Data Bytes
} CAN_tx_msg;			      // length 15 byte

struct {
    uint8_t format;		// Extended/Standard Frame
    uint32_t id;		    // Frame ID
    uint8_t rtr;		    // RTR/Data Frame
    uint8_t len;		    // Data Length
    uint8_t data[8];		// Data Bytes
} CAN_rx_msg;			      // length 15 byte

// CAN init values for ACR, AMR and BTR registers after power up
struct {
    uint8_t acr[4];
    uint8_t amr[4];
    uint8_t btr0;
    uint8_t btr1;
    uint8_t fixed_rate;
} CAN_init_val = { {
0x00, 0x00, 0x00, 0x00}, {
0xff, 0xff, 0xff, 0xff}, BTR0_100k, BTR1_100k, 0};

/*
**---------------------------------------------------------------------------
**
** Abstract: Init SJA1000 controler
**
**
** Parameters: none
**
**
** Returns: status of command execution
**          CR = OK
**          ERROR = Error
**
**---------------------------------------------------------------------------
*/
uint8_t
init_CAN (void)
{
    // pin and port configuration
    CAN_CS_DIR |= _BV (CAN_CS);	// set CS signal output
    CAN_RST_DIR |= _BV (CAN_RST);	// set Reset signal output

    CAN_RST_PORT &= ~_BV (CAN_RST);	// force SJA1000 hardware reset
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    CAN_RST_PORT |= _BV (CAN_RST);	// pull Reset high
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");

    CAN_INT_PORT |= _BV (CAN_INT);	// enable pull-up on CAN interrupt signal

    // turn of error LED because controller is reseted
    LED_PORT &= ~_BV (LED_RED);
    // clear CAN error flags
    CAN_flags &= 0x00FF;

    // no listen only, no self test, dual acceptance filter, no sleep
    // request reset mode
    do {			// do as long as RM_RR_Bit is not set
      #if defined(ENABLE_SELFTEST)
        ModeControlReg = _BV (RM_RR_Bit) | _BV (STM_Bit);
      #else
        ModeControlReg = _BV (RM_RR_Bit);
      #endif
    } while ((ModeControlReg & _BV (RM_RR_Bit)) != _BV (RM_RR_Bit));

    // set PeliCAN mode, enable comperator bypass, CLKOUT = fosc
    ClockDivideReg = _BV (CANMode_Bit) | _BV (CBP_Bit) | DivBy1;

    // disable CAN interrupts
    InterruptEnReg = 0x00;

    // check for fix bit rate setting
     if (CAN_init_val.fixed_rate != 0) {
         switch (CAN_init_val.fixed_rate) {
             // 10k
               case '0':
                   BusTiming0Reg = BTR0_10k;
                   BusTiming1Reg = BTR1_10k;
                   break;
             // 20k
               case '1':
                   BusTiming0Reg = BTR0_20k;
                   BusTiming1Reg = BTR1_20k;
                   break;
             // 50k
               case '2':
                   BusTiming0Reg = BTR0_50k;
                   BusTiming1Reg = BTR1_50k;
                   break;
             // 100k
               case '3':
                   BusTiming0Reg = BTR0_100k;
                   BusTiming1Reg = BTR1_100k;
                   break;
             // 125k
               case '4':
                   BusTiming0Reg = BTR0_125k;
                   BusTiming1Reg = BTR1_125k;
                   break;
             // 250k
               case '5':
                   BusTiming0Reg = BTR0_250k;
                   BusTiming1Reg = BTR1_250k;
                   break;
             // 500k
               case '6':
                   BusTiming0Reg = BTR0_500k;
                   BusTiming1Reg = BTR1_500k;
                   break;
             // 800k
               case '7':
                   BusTiming0Reg = BTR0_800k;
                   BusTiming1Reg = BTR1_800k;
                   break;
             // 1M
               case '8':
                   BusTiming0Reg = BTR0_1M;
                   BusTiming1Reg = BTR1_1M;
                   break;
 			// 95.238k
               case '9':
             	  BusTiming0Reg = BTR0_95k;
             	  BusTiming1Reg = BTR1_95k;
             	  break;
             // 83.333k
               case 'A':
               case 'a':
             	  BusTiming0Reg = BTR0_83k;
             	  BusTiming1Reg = BTR1_83k;
             	  break;
             // 47.619k
               case 'B':
               case 'b':
             	  BusTiming0Reg = BTR0_47k;
             	  BusTiming1Reg = BTR1_47k;
             	  break;
 	  		// 33.333k
               case 'C':
               case 'c':
             	  BusTiming0Reg = BTR0_33k;
             	  BusTiming1Reg = BTR1_33k;
             	  break;              
         	// 5k
               case 'D':
               case 'd':
             	  BusTiming0Reg = BTR0_5k;
             	  BusTiming1Reg = BTR1_5k;
             	  break;
               default:
                   return ERROR;
        }
    }
    else {
        // store user values
        BusTiming0Reg = CAN_init_val.btr0;
        BusTiming1Reg = CAN_init_val.btr1;
    }

    // set TX1 in float, TX0 in push/pull, normal mode
#if defined(CANDIP)
    OutControlReg = Tx0Float | Tx1PshPull | NormalMode;
#else
    OutControlReg = Tx1Float | Tx0PshPull | NormalMode;
#endif
    
    // set acceptance code register
    AcceptCode0Reg = CAN_init_val.acr[0];
    AcceptCode1Reg = CAN_init_val.acr[1];
    AcceptCode2Reg = CAN_init_val.acr[2];
    AcceptCode3Reg = CAN_init_val.acr[3];

    // set acceptance mask register
    AcceptMask0Reg = CAN_init_val.amr[0];
    AcceptMask1Reg = CAN_init_val.amr[1];
    AcceptMask2Reg = CAN_init_val.amr[2];
    AcceptMask3Reg = CAN_init_val.amr[3];

    // enable CAN interrupts
    InterruptEnReg = 0xEF;	// enable all interrupts, except Wake-Up

    return CR;
}


/*
**---------------------------------------------------------------------------
**
** Abstract: SJA1000 Interrupt handler
**
**
** Parameters: none
**
**
** Returns: none
**
**---------------------------------------------------------------------------
*/
SIGNAL (SIG_INTERRUPT0)
{
    // read SJA1000 interrupt register
    uint8_t temp_int_reg;
    temp_int_reg = InterruptReg;

    /*
       WakeUp Interrupt  
     */

    /*
       TX Interrupt
     */
    if ((temp_int_reg & _BV (TI_Bit)) == _BV (TI_Bit)) {
        // clear busy flag after successfull transmit
        CLEARBIT (CAN_flags, TX_BUSY);
    }


    /*
       RX Interrupt
     */
    if ((temp_int_reg & _BV (RI_Bit)) == _BV (RI_Bit)) {
        // check for extented frame usage
        if ((RxFrameInfo & _BV (FF_Bit)) == _BV (FF_Bit))
            CAN_rx_msg.format = 1;
        else
            CAN_rx_msg.format = 0;
      
        // check for remote transmission request
        if ((RxFrameInfo & _BV (RTR_Bit)) == _BV (RTR_Bit))
            CAN_rx_msg.rtr = 1;
        else
            CAN_rx_msg.rtr = 0;
      
        CAN_rx_msg.len = RxFrameInfo & 0x0F;	// store message data length
      
        // see remark page 44, SJA1000 datasheet
        if (CAN_rx_msg.len > 8)
            CAN_rx_msg.len = 8;
      
        // copy ID and Data from SJA1000 to receive buffer
        if (!CAN_rx_msg.format) {	// Standard Frame
            CAN_rx_msg.id = ((uint32_t) RxBuffer1 << 3) & 0x7F8;
            CAN_rx_msg.id |= ((uint32_t) RxBuffer2 >> 5) & 0x07;
            CAN_rx_msg.data[0] = RxBuffer3;
            CAN_rx_msg.data[1] = RxBuffer4;
            CAN_rx_msg.data[2] = RxBuffer5;
            CAN_rx_msg.data[3] = RxBuffer6;
            CAN_rx_msg.data[4] = RxBuffer7;
            CAN_rx_msg.data[5] = RxBuffer8;
            CAN_rx_msg.data[6] = RxBuffer9;
            CAN_rx_msg.data[7] = RxBuffer10;
        }
        else {			// Extented Frame
            CAN_rx_msg.id = ((uint32_t) RxBuffer1 << 21) & 0x1FE00000;
            CAN_rx_msg.id |= ((uint32_t) RxBuffer2 << 13) & 0x1FE000;
            CAN_rx_msg.id |= ((uint32_t) RxBuffer3 << 5) & 0x1FFE;
            CAN_rx_msg.id |= ((uint32_t) RxBuffer4 >> 3) & 0x1F;
            CAN_rx_msg.data[0] = RxBuffer5;
            CAN_rx_msg.data[1] = RxBuffer6;
            CAN_rx_msg.data[2] = RxBuffer7;
            CAN_rx_msg.data[3] = RxBuffer8;
            CAN_rx_msg.data[4] = RxBuffer9;
            CAN_rx_msg.data[5] = RxBuffer10;
            CAN_rx_msg.data[6] = RxBuffer11;
            CAN_rx_msg.data[7] = RxBuffer12;
        }
      
        // release receive buffer
        CommandReg = _BV (RRB_Bit);
      
        // indicate waiting message
        SETBIT (CAN_flags, MSG_WAITING);
      
        CAN_LED (0);		// indicate CAN traffic
    }

    /*
       Data Overrun Detection Interrupt
     */
    if ((temp_int_reg & _BV (DOI_Bit)) == _BV (DOI_Bit)) {
        CAN_LED (1);		// indicate error
        SETBIT (CAN_flags, DATA_OVERRUN);
    }

    /*
       Error Warning Interrupt
     */
    if ((temp_int_reg & _BV (EI_Bit)) == _BV (EI_Bit)) {
        CAN_LED (1);		// indicate error
        SETBIT (CAN_flags, ERROR_WARNING);
    }

    /*
       Bus Error Interrupt
     */
    if ((temp_int_reg & _BV (BEI_Bit)) == _BV (BEI_Bit)) {
        last_ecr = ErrCodeCapReg;	// store actual error capture data

        CommandReg = _BV (AT_Bit);	// abort transmission on bus error
      
        CAN_LED (1);		// indicate error
        // indicate bus error
        SETBIT (CAN_flags, BUS_ERROR);
        // clear busy flag on error, transmission aborted
        CLEARBIT (CAN_flags, TX_BUSY);
    }

    /*
       Error Passive Interrupt
     */
    if ((temp_int_reg & _BV (EPI_Bit)) == _BV (EPI_Bit)) {
        CAN_LED (1);		// indicate error
        SETBIT (CAN_flags, ERROR_PASSIVE);
    }

    /*
       Arbitration Lost Interrupt
     */
    if ((temp_int_reg & _BV (ALI_Bit)) == _BV (ALI_Bit)) {
        last_alc = ArbLostCapReg;	// store actual arbitration lost data
        CAN_LED (1);		// indicate error
        SETBIT (CAN_flags, ARB_LOST);
    }
}


/*
**---------------------------------------------------------------------------
**
** Abstract: Transmit CAN message
**
**
** Parameters: none, but message must be stored in CAN_tx_msg
**
** Returns: status of command execution
**          CR = OK
**          ERROR = Error
**
**---------------------------------------------------------------------------
*/
uint8_t
transmit_CAN (void)
{

    // if transmit buffer is unlocked
    if ((StatusReg & _BV (TBS_Bit)) != _BV (TBS_Bit))
        return ERROR;

    uint8_t temp_frame_info;

    temp_frame_info = CAN_tx_msg.len & 0x0F;	// store message data length

    // check for remote transmission request
    if (CAN_tx_msg.rtr)
        temp_frame_info |= _BV (RTR_Bit);

    // check for extented frame usage
    if (CAN_tx_msg.format)
        temp_frame_info |= _BV (FF_Bit);

    TxFrameInfo = temp_frame_info;

    // copy ID and Data from buffer to SJA1000 transmit buffer
    if (!CAN_tx_msg.format) {	// Standard Frame
        TxBuffer1 = (CAN_tx_msg.id >> 3) & 0xff;
        TxBuffer2 = (CAN_tx_msg.id << 5) & 0xe0;
        TxBuffer3 = CAN_tx_msg.data[0];
        TxBuffer4 = CAN_tx_msg.data[1];
        TxBuffer5 = CAN_tx_msg.data[2];
        TxBuffer6 = CAN_tx_msg.data[3];
        TxBuffer7 = CAN_tx_msg.data[4];
        TxBuffer8 = CAN_tx_msg.data[5];
        TxBuffer9 = CAN_tx_msg.data[6];
        TxBuffer10 = CAN_tx_msg.data[7];
    }
    else {			// Extented Frame
        TxBuffer1 = (CAN_tx_msg.id >> 21) & 0xff;
        TxBuffer2 = (CAN_tx_msg.id >> 13) & 0xff;
        TxBuffer3 = (CAN_tx_msg.id >> 5) & 0xff;
        TxBuffer4 = (CAN_tx_msg.id << 3) & 0xf8;
        TxBuffer5 = CAN_tx_msg.data[0];
        TxBuffer6 = CAN_tx_msg.data[1];
        TxBuffer7 = CAN_tx_msg.data[2];
        TxBuffer8 = CAN_tx_msg.data[3];
        TxBuffer9 = CAN_tx_msg.data[4];
        TxBuffer10 = CAN_tx_msg.data[5];
        TxBuffer11 = CAN_tx_msg.data[6];
        TxBuffer12 = CAN_tx_msg.data[7];
    }

    // only Selftest mode
#if !defined(SINGLE_SHOT) && defined(ENABLE_SELFTEST)
    // start Self Reception Request
    CommandReg = _BV (SRR_Bit);
#endif
    // only Single Shot mode
#if defined(SINGLE_SHOT) && !defined(ENABLE_SELFTEST)
    // start Single Shot transmission
    CommandReg = _BV (TR_Bit) | _BV (AT_Bit);
#endif
    // Normal mode
#if !defined(SINGLE_SHOT) && !defined(ENABLE_SELFTEST)
    // start normal transmission
    CommandReg = _BV (TR_Bit);
#endif

    CAN_LED (0);		// indicate CAN traffic
    return CR;
}


/*
**---------------------------------------------------------------------------
**
** Abstract: read register content from SJA1000
**
**
** Parameters: Register number 0x00-0x7F
**
**
** Returns: Register content
**
**---------------------------------------------------------------------------
*/
uint8_t
read_CAN_reg (uint8_t reg)
{
    reg &= 0x7F;		// mask register 0-127
    return *((uint16_t *) (uint16_t) reg + CAN_BASE);
}


/*
**---------------------------------------------------------------------------
**
** Abstract: Write register content
**
**
** Parameters: Register number
**             Data byte
**
**
** Returns: none
**
**---------------------------------------------------------------------------
*/
void
write_CAN_reg (uint8_t reg,
	       uint8_t data)
{
    reg &= 0x7F;		// mask register 0-127
    uint8_t *pntr;
    pntr = (uint8_t *) CAN_BASE + (uint8_t) reg;
    *pntr = data;
}


/*
**---------------------------------------------------------------------------
**
** Abstract: Turn traffic or error LED on
**
**
** Parameters: 0 = Traffic LED - green
**             1 = Error LED - red
**
**
** Returns: none
**
**---------------------------------------------------------------------------
*/
void
CAN_LED (uint8_t led_no)
{
    if (!led_no)
        LED_PORT |= _BV (LED_GREEN);	// CAN activity LED on
    else
        LED_PORT |= _BV (LED_RED);	// Error LED on

    TCNT2 = LED_TCNT_VALUE;
    TIMSK |= _BV (TOIE2);	// enable overflow interrupt
    TCCR2 = _BV (CS22) | _BV (CS21) | _BV (CS20);	// start timer
}


/*
**---------------------------------------------------------------------------
**
** Abstract: Turn LED's off, except error LED on bus error
**
**
** Parameters: none
**
**
** Returns: none
**
**---------------------------------------------------------------------------
*/
SIGNAL (SIG_OVERFLOW2)
{
    TCCR2 = 0;			// stop timer
    TIMSK &= ~_BV (TOIE2);	// disable overflow interrupt
    LED_PORT &= ~_BV (LED_GREEN);	// CAN activity LED off
    // keep Error LED on if Bus Error was detected
    if (!CHECKBIT (CAN_flags, BUS_ERROR))
        LED_PORT &= ~_BV (LED_RED);	// Error LED off
}
