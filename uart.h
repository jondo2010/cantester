//
//	uart.h
//
//	Ties the internal AVR uart to the standard io functions like printf and
//	gets by way of the stdin and stdout streams.
//
//	Michael Jean <michael.jean@shaw.ca>
//

#ifndef _UART_H
#define _UART_H

#define UART_BAUD_RATE	115200

//
//	Initialize the internal uart for text output. Set the baud rate and enable
//	the transmitter and receiver. Tie stdin and stdout to the uart's putchar
//	and getchar functions.
//

void
uart_init (void);

//
//	Read a character from the uart. Returns the character read.
//

int
uart_getchar
(
	FILE *stream		/* stream being read from -- not used */
);

//
//	Write a character to the uart. If a new-line character is written,
//	a carriage-return character is written out before the new-line.
//
//	Returns zero, always. The return code and stream argument are required
//	by the stdio library prototype for streams made with FDEV_SETUP_STREAM.
//

int
uart_putchar
(
	char 	c,			/* character to write */
	FILE 	*stream		/* stream being written to -- not used */
);

#endif
