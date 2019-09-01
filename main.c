//*****************************************************************************
//
// Title        : AVR based USB<>CAN adaptor
// Authors      : Michael Wolf
// File Name    : 'main.c'
// Date         : April 17, 2016
// Version      : 1.09
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
// 1.01     29/08/2005  Michael Wolf  + L, R, W command
// 1.02     02/09/2005  Michael Wolf  - fixed bug for command 's'
// 1.04     22/10/2005  Michael Wolf  + added support for remote frames
//                                      (command 'R' and 'r')  
// 1.05     17/08/2006  Michael Wolf  * Changed predefined Baudrate values to
//                                      fix possible bus errors
// 1.06     12/10/2006  Michael Wolf  + added command 'v' to read detailed
//                                      software version
// 1.07		02/08/2009	Michael Wolf  + added support for CANdip module
// 1.08		06/01/2010	Michael Wolf  * external memory bus is initialized now
//										before accessing the CAN controller first time
// 1.09		04/17/2016	Michael Wolf  * Changed BTR values for various CAN speeds
//
//  Used develompent tools:
//  Eclipse IDE
//  WinAVR (GCC)
//
//  [           Legend:          ]
//  [ + Added feature            ]
//  [ * Improved/changed feature ]
//  [ - Bug fixed (I hope)       ]
//
//*****************************************************************************
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "usb.h"
#include "sja1000.h"
#include "main.h"

// time stamp counter 0-59999ms
static uint16_t timestamp;

// define EEPROM settings
__attribute__ ((section (".eeprom")))
     uint8_t serial[] = SERIAL;	// store device serial
__attribute__ ((section (".eeprom")))
     uint8_t ee_timestamp_status = 0;	// store time stamp OFF

// copy of ee_time_stamp_status in SRAM to prevent unnecessary EEPROM access
     uint8_t ram_timestamp_status;

// execute command received via USB
     uint8_t exec_usb_cmd (uint8_t * cmd_buf);

// convert 2 byte ASCII to one byte
     uint8_t ascii2byte (uint8_t * val);

/*
**---------------------------------------------------------------------------
**
** Abstract: Main Routine
**
**
** Parameters: none
**
**
** Returns: never
**
**---------------------------------------------------------------------------
*/
int main (void)
{
    // USB_RD and USB_WR pins output
    USB_TX_DIR |= _BV (USB_RD) | _BV (USB_WR);

    // LED pins as output
    LED_DIR |= _BV (LED_RED) | _BV (LED_GREEN);

    // set wait states
    // EMCUCR |= _BV(SRW11)|_BV(SRW10)|_BV(SRW01)|_BV(SRW00);

    // enables Bus-Keeper who will ensure a defined logic on AD7..0 when they would
    // otherwise be tristatet
    SFIOR |= _BV (XMBK);

    //PortC is completely used for AD8..15, as we don't use them, it's better do disable this
    SFIOR |= _BV (XMM2) | _BV (XMM1) | _BV (XMM0);

    // enable external RAM = SJA1000 interface
    // if the communication with the CAN doesn't work, please add wait-states
    MCUCR |= _BV (SRE);

    /* INT0 must be low level activated, see AN97076 page 38 */
    GICR |= _BV (INT0);

    // power up init for CAN controller with default values
    init_CAN ();    
    
    // setup time stamp counter
    OCR0 = OCR_VALUE;		// timer0 capture at 1ms
    TCCR0 = _BV (WGM01) | _BV (CS01) | _BV (CS00);	// set Timer0 CTC mode, prescaler 64
    TIMSK = _BV (OCIE0);	// enable timer0 output compare interrupt
    timestamp = 0;

    // read status of time stamp setting
    ram_timestamp_status = eeprom_read_byte (&ee_timestamp_status);

    // command buffer
    uint8_t cmd_buf[CMD_BUFFER_LENGTH];

    // command buffer index
    uint8_t buf_ind = 0;

    uint8_t usb_rx_char;

    uint8_t i;			// for loop counter

    sei ();			// enable global interrupts

    for (;;) {
        if (CHECKBIT (CAN_flags, MSG_WAITING)) {
            // check frame format
            if (!CAN_rx_msg.format) {	// Standart Frame
                if (!CAN_rx_msg.rtr) {
                    usb_putc (SEND_11BIT_ID);
                }		// send command tag
                else {
                    usb_putc (SEND_R11BIT_ID);
                }
      
                // send high byte of ID
                if (((CAN_rx_msg.id >> 8) & 0x0F) < 10)
                    usb_putc (((uint8_t) (CAN_rx_msg.id >> 8) & 0x0F) + 48);
                else
                    usb_putc (((uint8_t) (CAN_rx_msg.id >> 8) & 0x0F) + 55);
                // send low byte of ID
                usb_byte2ascii ((uint8_t) CAN_rx_msg.id & 0xFF);
            }
            else {		// Extented Frame
                if (!CAN_rx_msg.rtr) {
                    usb_putc (SEND_29BIT_ID);
                }		// send command tag
                else {
                    usb_putc (SEND_R29BIT_ID);
                }
                // send ID bytes
                usb_byte2ascii ((uint8_t) (CAN_rx_msg.id >> 24) & 0xFF);
                usb_byte2ascii ((uint8_t) (CAN_rx_msg.id >> 16) & 0xFF);
                usb_byte2ascii ((uint8_t) (CAN_rx_msg.id >> 8) & 0xFF);
                usb_byte2ascii ((uint8_t) CAN_rx_msg.id & 0xFF);
            }
            // send data length code
            usb_putc (CAN_rx_msg.len + '0');
            if (!CAN_rx_msg.rtr) {	// send data only if no remote frame request
                // send data bytes
                for (i = 0; i < CAN_rx_msg.len; i++)
                    usb_byte2ascii (CAN_rx_msg.data[i]);
            }
            // send time stamp if required
            if (ram_timestamp_status != 0) {
                usb_byte2ascii ((uint8_t) (timestamp >> 8));
                usb_byte2ascii ((uint8_t) timestamp);
            }
            // send end tag
            usb_putc (CR);
            CLEARBIT (CAN_flags, MSG_WAITING);
        }
      
        // read char from USB
        usb_rx_char = usb_getc ();	// check for new chars from USB
        if (usb_rx_char == CR)	// check for end of command
        {
            // Execute USB command and return status to terminal
            usb_putc (exec_usb_cmd (cmd_buf));
      
            // flush command buffer
            for (buf_ind = 0; buf_ind < CMD_BUFFER_LENGTH; buf_ind++)
                cmd_buf[buf_ind] = 0x00;
      
            buf_ind = 0;	// point to start of command      
        }
        else if (usb_rx_char != 0)	// store new char in buffer
        {
            cmd_buf[buf_ind] = usb_rx_char;	// store char
            // check for buffer overflow
            if (buf_ind < sizeof (cmd_buf) - 1)
                buf_ind++;
        }
    }

    return 0;
}				// end of main


/*
**---------------------------------------------------------------------------
**
** Abstract: Execution of command received via USB
**
**
** Parameters: Command Buffer
**
**
** Returns: status of command execution
**          CR = OK
**          ERROR = Error
**
**---------------------------------------------------------------------------
*/
uint8_t
exec_usb_cmd (uint8_t * cmd_buf)
{
    uint8_t cmd_len = strlen ((char *)cmd_buf);	// get command length

    uint8_t *cmd_buf_pntr = &(*cmd_buf);	// point to start of received string
    cmd_buf_pntr++;		// skip command identifier
    // check if all chars are valid hex chars
    while (*cmd_buf_pntr) {
        if (!isxdigit (*cmd_buf_pntr))
            return ERROR;
        ++cmd_buf_pntr;
    }
    cmd_buf_pntr = &(*cmd_buf);	// reset pointer

    uint8_t *tmp_pntr;		// temporary pointer for ACR/AMR
    uint8_t tmp_regdata;	// temporary used for register data

    switch (*cmd_buf_pntr) {
            // get serial number
        case GET_SERIAL:
            usb_putc (GET_SERIAL);
            usb_puts ((uint8_t *)SERIAL);
            return CR;
      
            // get hard- and software version
        case GET_VERSION:
            usb_putc (GET_VERSION);
            usb_byte2ascii (HW_VER);
            usb_byte2ascii (SW_VER);
            return CR;

            // get only software version
        case GET_SW_VERSION:
            usb_putc (GET_SW_VERSION);
            usb_byte2ascii (SW_VER_MAJOR);
            usb_byte2ascii (SW_VER_MINOR);
            return CR;
      
            // toggle time stamp option
        case TIME_STAMP:
            // read stored status
            ram_timestamp_status = eeprom_read_byte (&ee_timestamp_status);
            // toggle status
            if (ram_timestamp_status != 0)
                ram_timestamp_status = 0;	// disable time stamp
            else {
                ram_timestamp_status = 0xA5;	// enable time stamp
                timestamp = 0;	// reset time stamp counter
            }
            // store new status
            eeprom_write_byte (&ee_timestamp_status, ram_timestamp_status);
            return CR;
      
            // read status flag
        case READ_STATUS:
            // check if CAN controller is in reset mode
            if (!CHECKBIT (CAN_flags, BUS_ON))
                return ERROR;
      
            usb_putc (READ_STATUS);
            usb_byte2ascii ((uint8_t) (CAN_flags >> 8));
      
            // turn off Bus Error indication
            LED_PORT &= ~_BV (LED_RED);
            // reset error flags
            CAN_flags &= 0x00FF;
            return CR;
      
            // set AMR
        case SET_AMR:
            // set ACR
        case SET_ACR:
            // check valid cmd length and if CAN was initialized before
            if (cmd_len != 9)
                return ERROR;
            // check if CAN controller is in reset mode
            if (CHECKBIT (CAN_flags, BUS_ON))
                return ERROR;
      
            // assign pointer to AMR or ACR values depending on command
            if (*cmd_buf_pntr == SET_AMR)
                tmp_pntr = CAN_init_val.amr;
            else
                tmp_pntr = CAN_init_val.acr;
      
            // store AMR or ACR values
            *tmp_pntr = ascii2byte (++cmd_buf_pntr);
            *tmp_pntr <<= 4;
            *(tmp_pntr++) |= ascii2byte (++cmd_buf_pntr);
            *tmp_pntr = ascii2byte (++cmd_buf_pntr);
            *tmp_pntr <<= 4;
            *(tmp_pntr++) |= ascii2byte (++cmd_buf_pntr);
            *tmp_pntr = ascii2byte (++cmd_buf_pntr);
            *tmp_pntr <<= 4;
            *(tmp_pntr++) |= ascii2byte (++cmd_buf_pntr);
            *tmp_pntr = ascii2byte (++cmd_buf_pntr);
            *tmp_pntr <<= 4;
            *tmp_pntr |= ascii2byte (++cmd_buf_pntr);
            // init CAN controller with new values
            return init_CAN ();
      
            // set bitrate via BTR
        case SET_BTR:
            // set fix bitrate
        case SET_BITRATE:
            if ((cmd_len != 5) && (cmd_len != 2))
                return ERROR;	// check valid cmd length
      
            // check if CAN controller is in reset mode
            if (CHECKBIT (CAN_flags, BUS_ON))
                return ERROR;
      
            // store user or fixed bit rate
            if (*cmd_buf_pntr == SET_BTR) {
                CAN_init_val.btr0 = ascii2byte (++cmd_buf_pntr);
                CAN_init_val.btr0 <<= 4;
                CAN_init_val.btr0 |= ascii2byte (++cmd_buf_pntr);
                CAN_init_val.btr1 = ascii2byte (++cmd_buf_pntr);
                CAN_init_val.btr1 <<= 4;
                CAN_init_val.btr1 |= ascii2byte (++cmd_buf_pntr);
                CAN_init_val.fixed_rate = 0;
            }
            else {
                CAN_init_val.fixed_rate = *(++cmd_buf_pntr);
            }
            // init CAN controller
            SETBIT (CAN_flags, CAN_INIT);	// indicate initialized controller
            return init_CAN ();
      
            // open CAN channel
        case OPEN_CAN_CHAN:
            // return error if controller is not initialized or already open
            if (!CHECKBIT (CAN_flags, CAN_INIT))
          return ERROR;
      
            // check if CAN controller is in reset mode
            if (CHECKBIT (CAN_flags, BUS_ON))
          return ERROR;
      
            // switch to oper mode
            do {
              #if defined(ENABLE_SELFTEST)
                  ModeControlReg = _BV (STM_Bit);
              #else
                  ModeControlReg &= ~_BV (RM_RR_Bit);
              #endif
            } while ((ModeControlReg & _BV (RM_RR_Bit)) == _BV (RM_RR_Bit));
            SETBIT (CAN_flags, BUS_ON);
            return CR;
      
            // close CAN channel
        case CLOSE_CAN_CHAN:
            // check if CAN controller is in reset mode
            if (!CHECKBIT (CAN_flags, BUS_ON))
                return ERROR;
      
            // switch to reset mode
            do {		// do as long as RM_RR_Bit is not set
              #if defined(ENABLE_SELFTEST)
                  ModeControlReg = _BV (RM_RR_Bit) | _BV (STM_Bit);
              #else
                  ModeControlReg = _BV (RM_RR_Bit);
              #endif
            } while ((ModeControlReg & _BV (RM_RR_Bit)) != _BV (RM_RR_Bit));
            CLEARBIT (CAN_flags, BUS_ON);
            return CR;
      
            // send 11bit ID message
        case SEND_R11BIT_ID:
            // check if CAN controller is in reset mode or busy
            if (!CHECKBIT (CAN_flags, BUS_ON) || CHECKBIT (CAN_flags, TX_BUSY))
                return ERROR;
            // check valid cmd length (only 5 bytes for RTR)  
            if (cmd_len != 5)
                return ERROR;
      
            CAN_tx_msg.rtr = 1;	// remote transmission request    
      
            // store std. frame format
            CAN_tx_msg.format = 0;
            // store ID
            CAN_tx_msg.id = ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            // store data length
            CAN_tx_msg.len = ascii2byte (++cmd_buf_pntr);
      
            // if transmit buffer was empty send message
            return transmit_CAN ();
      
        case SEND_11BIT_ID:
            // check if CAN controller is in reset mode or busy
            if (!CHECKBIT (CAN_flags, BUS_ON) || CHECKBIT (CAN_flags, TX_BUSY))
                return ERROR;
      
            if ((cmd_len < 5) || (cmd_len > 21))
                return ERROR;	// check valid cmd length
      
            CAN_tx_msg.rtr = 0;	// no remote transmission request
      
            // store std. frame format
            CAN_tx_msg.format = 0;
            // store ID
            CAN_tx_msg.id = ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            // store data length
            CAN_tx_msg.len = ascii2byte (++cmd_buf_pntr);
            // check number of data bytes supplied against data lenght byte
            if (CAN_tx_msg.len != ((cmd_len - 5) / 2))
                return ERROR;
      
            // check for valid length
            if (CAN_tx_msg.len > 8)
                return ERROR;
            else {		// store data
                // cmd_len is no longer needed, so we can use it as counter here
                for (cmd_len = 0; cmd_len < CAN_tx_msg.len; cmd_len++) {
                    cmd_buf_pntr++;
                    CAN_tx_msg.data[cmd_len] = ascii2byte (cmd_buf_pntr);
                    CAN_tx_msg.data[cmd_len] <<= 4;
                    cmd_buf_pntr++;
                    CAN_tx_msg.data[cmd_len] += ascii2byte (cmd_buf_pntr);
                }
            }
            // if transmit buffer was empty send message
            return transmit_CAN ();
      
            // send 29bit ID message
        case SEND_R29BIT_ID:
            // check if CAN controller is in reset mode or busy
            if (!CHECKBIT (CAN_flags, BUS_ON) || CHECKBIT (CAN_flags, TX_BUSY))
                return ERROR;
      
            if (cmd_len != 10)
                return ERROR;	// check valid cmd length
      
            CAN_tx_msg.rtr = 1;	// remote transmission request
      
            // store ext. frame format
            CAN_tx_msg.format = 1;
            // store ID
            CAN_tx_msg.id = ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            // store data length
            CAN_tx_msg.len = ascii2byte (++cmd_buf_pntr);
            // if transmit buffer was empty send message
            return transmit_CAN ();
      
        case SEND_29BIT_ID:
            // check if CAN controller is in reset mode or busy
            if (!CHECKBIT (CAN_flags, BUS_ON) || CHECKBIT (CAN_flags, TX_BUSY))
                return ERROR;
      
            if ((cmd_len < 10) || (cmd_len > 26))
                return ERROR;	// check valid cmd length
      
            CAN_tx_msg.rtr = 0;	// no remote transmission request
      
            // store ext. frame format
            CAN_tx_msg.format = 1;
            // store ID
            CAN_tx_msg.id = ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            CAN_tx_msg.id <<= 4;
            CAN_tx_msg.id += ascii2byte (++cmd_buf_pntr);
            // store data length
            CAN_tx_msg.len = ascii2byte (++cmd_buf_pntr);
            // check number of data bytes supplied against data lenght byte
            if (CAN_tx_msg.len != ((cmd_len - 10) / 2))
                return ERROR;
      
            // check for valid length
            if (CAN_tx_msg.len > 8)
                return ERROR;
            else {		// store data
                // cmd_len is no longer needed, so we can use it as counter here
                for (cmd_len = 0; cmd_len < CAN_tx_msg.len; cmd_len++) {
                    cmd_buf_pntr++;
                    CAN_tx_msg.data[cmd_len] = ascii2byte (cmd_buf_pntr);
                    CAN_tx_msg.data[cmd_len] <<= 4;
                    cmd_buf_pntr++;
                    CAN_tx_msg.data[cmd_len] += ascii2byte (cmd_buf_pntr);
                }
            }
            // if transmit buffer was empty send message
            return transmit_CAN ();
      
            // read Error Capture Register
            // read Arbitration Lost Register
        case READ_ECR:
        case READ_ALCR:
            // check if CAN controller is in reset mode
            if (!CHECKBIT (CAN_flags, BUS_ON))
                return ERROR;
      
            if (*cmd_buf_pntr == READ_ECR) {
                usb_putc (READ_ECR);
                usb_byte2ascii (last_ecr);
            }
            else {
                usb_putc (READ_ALCR);
                usb_byte2ascii (last_alc);
            }
            return CR;
      
            // read SJA1000 register
        case READ_REG:
            if (cmd_len != 3)
                return ERROR;	// check valid cmd length
            // cmd_len is no longer needed, so we can use it as buffer
            // get register number
            cmd_len = ascii2byte (++cmd_buf_pntr);
            cmd_len <<= 4;
            cmd_len |= ascii2byte (++cmd_buf_pntr);
            usb_putc (READ_REG);
            usb_byte2ascii (read_CAN_reg (cmd_len));
            return CR;
      
            // write SJA1000 register
        case WRITE_REG:
            if (cmd_len != 5)
                return ERROR;	// check valid cmd length
      
            // cmd_len is no longer needed, so we can use it as buffer
            // get register number
            cmd_len = ascii2byte (++cmd_buf_pntr);
            cmd_len <<= 4;
            cmd_len |= ascii2byte (++cmd_buf_pntr);
            // get register data
            tmp_regdata = ascii2byte (++cmd_buf_pntr);
            tmp_regdata <<= 4;
            tmp_regdata |= ascii2byte (++cmd_buf_pntr);
            write_CAN_reg (cmd_len, tmp_regdata);
            return CR;
      
        case LISTEN_ONLY:
            // return error if controller is not initialized or already open
            if (!CHECKBIT (CAN_flags, CAN_INIT))
                return ERROR;
      
            // check if CAN controller is in reset mode
            if (CHECKBIT (CAN_flags, BUS_ON))
                return ERROR;
      
            // switch to listen only mode
            do {
                ModeControlReg = _BV (LOM_Bit);
            } while ((ModeControlReg & _BV (RM_RR_Bit)) == _BV (RM_RR_Bit));
            SETBIT (CAN_flags, BUS_ON);
            return CR;
      
            // end with error on unknown commands
        default:
            return ERROR;
    }				// end switch

    // we should never reach this return
    return ERROR;
}				// end exec_usb_cmd


/*
**---------------------------------------------------------------------------
**
** Abstract: Convert 1 char ASCII to 1 low nibble binary
**
**
** Parameters: Pointer to ASCII char
**
**
** Returns: Byte value
**
**
**---------------------------------------------------------------------------
*/
uint8_t
ascii2byte (uint8_t * val)
{
    uint8_t temp = *val;

    if (temp > 0x60)
        temp -= 0x27;		// convert chars a-f
    else if (temp > 0x40)
        temp -= 0x07;		// convert chars A-F
    temp -= 0x30;		// convert chars 0-9

    return temp & 0x0F;
}


/*
**---------------------------------------------------------------------------
**
** Abstract: Timer0 output capture interrupt handler
**           called in 1ms intervall
**
**
** Parameters: none
**
**
** Returns: time stamp counter 0-59999ms
**
**---------------------------------------------------------------------------
*/
SIGNAL (SIG_OUTPUT_COMPARE0)
{
    timestamp++;
    if (timestamp > 59999)
        timestamp = 0;
}
