#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include "can.h"
#include "packet.h"
#include "uart.h"

#define ASCII_ESC 	27

#define TX_BUF_LEN	100		/* maximum number of packets we can schedule */

static can_packet_t		tx_packet_buf[TX_BUF_LEN];

static uint8_t			tx_bcast_index = 0;
static uint8_t 			num_tx_packets = 0;

volatile uint32_t 	clock = 0;

//
//	The counter ISR is just a simple time-keeper. All it does is increment
//	the system time.
//

ISR (TIMER0_COMP_vect)
{
	clock++;
}

//
//	Initialize the internal counter to fire every millisecond.
//

static void
timer_init (void)
{
	TCCR0A = _BV (WGM01) | _BV (CS01) | _BV (CS00);
	OCR0A = 249;
	TIMSK0 = _BV (OCIE0A);
}

//
//	Program entry point.
//

int
main (void)
{
	int 	i, p = 0;
	char 	progress[] = {'|', '/', '-', '\\'};

	uart_init ();
	timer_init ();
	can_init (can_baud_1000);

	printf ("%c[2J%c[H", ASCII_ESC, ASCII_ESC);
	printf ("CAN-Tester 1.0 (c) 2009-2010 UMSAE\n");
	printf ("==================================\n\n");

	printf ("Ready for packet schedule.\n");

	num_tx_packets = read_schedule_from_stdin (tx_packet_buf, TX_BUF_LEN);

	printf ("\n%d packets read from serial.\n", num_tx_packets);

	printf ("\nPacket Schedule:\n\n");
	printf (" Time  (ms)   Direction    Format        ID        Type    DLC                     Data                  \n");
	printf ("============ =========== ========== ============ ======== ===== =========================================\n");

	for (i = 0; i < num_tx_packets; i++)
		print_packet (&tx_packet_buf[i]);

	printf ("\nPress any key to start broadcasting.");
	fgetc (stdin);

	start_receiving ();

	sei ();

	printf ("\n\nBroadcasting... \n");

	for (;;)
	{
		if (tx_bcast_index < num_tx_packets && tx_packet_buf[tx_bcast_index].time <= clock)
		{
			print_packet (&tx_packet_buf[tx_bcast_index]);
			broadcast_packet (&tx_packet_buf[tx_bcast_index++]);

			//printf ("%c%c[D", progress[p], ASCII_ESC); /* 1 */
			//p = (p+1) % 4;
		}
		else if (tx_bcast_index == num_tx_packets)
			break;
	}

	printf ("done!\n");

	for (;;)
	{
		rx_print ();
	}

	fgetc (stdin);
	printf ("\n%d packets delivered in %ld ms.\nSystem halted.\n", tx_bcast_index, clock);

	cli ();

	return 0;
}

//
//	1.	This makes a cute little spinny progress bar.
//
