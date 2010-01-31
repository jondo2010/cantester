//
//	uart.c
//
//	Michael Jean <michael.jean@shaw.ca>
//

#include <stdio.h>
#include <avr/io.h>

#include "uart.h"

static FILE mystdin  = FDEV_SETUP_STREAM (NULL, uart_getchar, _FDEV_SETUP_READ); 	/* 1 */
static FILE mystdout = FDEV_SETUP_STREAM (uart_putchar, NULL, _FDEV_SETUP_WRITE);

//
//	1. 	See the AVR-libc documentation on stdio.h for a better understanding of the
//		FDEV_SETUP_STREAM function.
//
//		http://www.nongnu.org/avr-libc/user-manual/group__avr__stdio.html
//

void
uart_init (void)
{
	UBRR0 = 8;								/* 115200 baud at 16 MHz */
	UCSR0B = _BV (RXEN0) | _BV (TXEN0);		/* enable transmitter and receiver */

	stdin = &mystdin;		/* 1 */
	stdout = &mystdout;
}

//
//	1. 	By tying stdin and stdout to the newly created streams, we can use printf,
//		scanf, and friends as if we were writing a console applicaiton.
//

int
uart_getchar (FILE *stream)
{
	uint8_t data;

	loop_until_bit_is_set (UCSR0A, RXC0);
	data = UDR0;

	if (data == '\r')	/* 1 */
		data = '\n';

	return data;
}

//
//	1.	MacOS X sends '\r' but not '\n' when you press enter. Stupid.
//

int
uart_putchar (char c, FILE *stream)
{
	if (c == '\n')	/* 1 */
		uart_putchar ('\r', stream);

	loop_until_bit_is_set (UCSR0A, UDRE0);
	UDR0 = c;

	return 0;
}

//
//	1.	If you output '\n' without a '\r' carriage-return, UNIX-like systems will not
//		return the cursor to the start of the line.
//

