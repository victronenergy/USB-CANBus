//*****************************************************************************
//
// Title        : AVR based USB<>CAN adaptor
// Authors      : Michael Wolf
// File Name    : 'usb.c'
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
//
//*****************************************************************************
#include <avr/io.h>
#include <stdint.h>

#include "usb.h"

/*
**---------------------------------------------------------------------------
**
** Abstract: USB get char
**
**
** Parameters: none
**
**
** Returns: 0 = no char received
**          else ASCII char
**
**
**---------------------------------------------------------------------------
*/
uint8_t
usb_getc (void)
{
    uint8_t rx_byte;

    // check for received characters
    if ((USB_RX_PIN & _BV (USB_RXF)) != _BV (USB_RXF))	// do if RXF low
    {
        USB_TX_PORT &= ~_BV (USB_RD);	// enable RD
        asm ("NOP");
        asm ("NOP");
        asm ("NOP");
        rx_byte = USB_DATA_PIN;	// get data byte
        USB_TX_PORT |= _BV (USB_RD);	// disable RD
        return (rx_byte & 0x7F);	// return ASCII char
    }
    else
    
    return 0;		// return no char
}


/*
**---------------------------------------------------------------------------
**
** Abstract: USB put char
**
**
** Parameters: character to send via USB
**
**
** Returns: none
**
**
**---------------------------------------------------------------------------
*/
void
usb_putc (uint8_t tx_byte)
{
    while ((USB_TX_PIN & _BV (USB_TXE)) == _BV (USB_TXE));	// wait for Tx ready

    USB_DATA_DIR = 0xFF;	// USB data port all output
    USB_DATA_PORT = tx_byte;	// send data byte

    USB_TX_PORT |= _BV (USB_WR);	// enable WR
    asm ("NOP");
    USB_TX_PORT &= ~_BV (USB_WR);	// disable WR
    asm ("NOP");

    USB_DATA_DIR = 0x00;	// USB data port all inputs
    USB_DATA_PORT = 0xFF;	// enable data port pull-ups
}


/*
**---------------------------------------------------------------------------
**
** Abstract: USB one byte as 2 ASCII chars
**
**
** Parameters: character to send via USB
**
**
** Returns: none
**
**
**---------------------------------------------------------------------------
*/
void
usb_byte2ascii (uint8_t tx_byte)
{
    // send high nibble
    usb_putc (((tx_byte >> 4) <
	       10) ? ((tx_byte >> 4) & 0x0f) + 48 : ((tx_byte >> 4) & 0x0f) +
	      55);
    // send low nibble
    usb_putc (((tx_byte & 0x0f) <
	       10) ? (tx_byte & 0x0f) + 48 : (tx_byte & 0x0f) + 55);
}


/*
**---------------------------------------------------------------------------
**
** Abstract: USB put SRAM string
**
**
** Parameters: pointer to string in SRAM
**
**
** Returns: none
**
**
**---------------------------------------------------------------------------
*/
void
usb_puts (uint8_t * tx_string)
{
    while (*tx_string)
        usb_putc (*tx_string++);	// send string char by char
}
