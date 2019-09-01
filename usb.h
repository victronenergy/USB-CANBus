//*****************************************************************************
//
// Title        : AVR based USB<>CAN adaptor
// Authors      : Michael Wolf
// File Name    : 'usb.h'
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
#ifndef __USB_H__
#define __USB_H__

#define USB_TX_PORT		PORTD
#define USB_TX_PIN		PIND
#define USB_TX_DIR		DDRD

#define USB_RX_PORT   PORTE
#define USB_RX_PIN    PINE
#define USB_RX_DIR    DDRE

#define USB_DATA_PORT PORTB
#define USB_DATA_PIN  PINB
#define USB_DATA_DIR  DDRB

#define USB_RD   PD3		// FT245BM RD pin
#define USB_WR   PD4		// FT245BM WR pin
#define USB_TXE  PD5		// FT245BM TXE pin
#define USB_RXF  PE0		// FT245BM RXF pin

uint8_t usb_getc (void);
void usb_putc (uint8_t tx_byte);
void usb_byte2ascii (uint8_t tx_byte);
void usb_puts (uint8_t * tx_string);

#endif // __USB_H__
