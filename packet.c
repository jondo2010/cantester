//
//	packet.c
//
//	Michael Jean <michael.jean@shaw.ca>
//

#include <stdio.h>
#include <avr/io.h>
#include <util/atomic.h>

#include "can.h"
#include "packet.h"

static volatile uint8_t	tx_mob_index = 0;
static volatile uint8_t tx_mobs_in_use = 0;

extern volatile uint32_t clock;

#define	RX_BUF_LEN 16

static 			can_packet_t	rx_buf[RX_BUF_LEN];
static volatile	can_packet_t	*rx_start = rx_buf;		// start of valid data ptr
static volatile	can_packet_t 	*rx_end = rx_buf;		// end of valid data ptr

static volatile uint8_t 		rx_read_count = 0;		// total elements read
static volatile uint8_t			rx_write_count = 0;		// total elements written

void
tx_callback (uint8_t mob_index)
{
	tx_mobs_in_use--;
}

void
rx_callback (uint8_t mob_index, uint32_t id, packet_type_t type)
{
	int8_t buf_overrun = rx_write_count - rx_read_count == RX_BUF_LEN;

	if (buf_overrun)
	{
		rx_start++;
		rx_read_count++;

		if (rx_start == rx_buf + RX_BUF_LEN)
			rx_start = rx_buf;
	}

	rx_end++;
	rx_end->time = clock;
	rx_end->id = id;
	rx_end->direction = input;
	rx_end->packet_type = type;
	rx_end->data_length = can_read_data (mob_index, (uint8_t *)(rx_end->data), 8);

	rx_write_count++;

	if (rx_end == rx_buf + RX_BUF_LEN)
		rx_end = rx_buf;

	can_ready_to_receive (mob_index);
}

void
rx_print (void)
{
	can_packet_t p;
	p.id = 0;

	ATOMIC_BLOCK (ATOMIC_FORCEON)
	{
		if (rx_write_count - rx_read_count)
		{
			memcpy (&p, rx_start++, sizeof(can_packet_t));

			if (rx_start == rx_buf + RX_BUF_LEN)
				rx_start = rx_buf;

			rx_read_count++;
		}
	}

	if (p.id)
	{
		print_packet (&p);
	}
}

void
start_receiving (void)
{
	mob_config_t mob;

	mob.id = 0x00;
	mob.mask = 0x00;
	mob.id_type = extended;
	mob.rx_callback_ptr = rx_callback;
	mob.tx_callback_ptr = 0;
	can_config_mob (1, &mob);
	can_ready_to_receive (1);
}

void
broadcast_packet (can_packet_t *packet)
{
	mob_config_t config_desc;

	//config_desc.mode = transmit;

	config_desc.id = packet->id;
	config_desc.id_type = packet->id_type;

	config_desc.tx_callback_ptr = tx_callback;

	while (tx_mobs_in_use >= MAX_TX_MOBS)	/* 1 */
		;

	tx_mobs_in_use++;

	can_config_mob (tx_mob_index, &config_desc);
	can_load_data (tx_mob_index, packet->data, packet->data_length);
	can_ready_to_send (tx_mob_index);

	tx_mob_index = (tx_mob_index + 1) % MAX_TX_MOBS;
}

//
//	1. 	This relys on the transmit callback to free up the message object.
//		If you are scheduling more than MAX_TX_MOBS per millisecond, you
//		are going to be late.
//

void
print_packet (can_packet_t *packet)
{
	int i;

	switch (packet->packet_type)
	{
		case payload:

			printf (" %10ld     %s    %s   0x%08lX    Data     %d    ",
				packet->time,
				packet->direction == input ? "Input " : "Output",
				packet->id_type == standard ? "Standard" : "Extended",
				packet->id,
				packet->data_length
			);

			for (i = 0; i < packet->data_length; i++)
				printf ("0x%02X ", packet->data[i]);

			printf ("\n");
			break;

		case remote:

			printf (" %10ld     %s    %s   0x%08lX   Remote\n",
				packet->time,
				packet->direction == input ? "Input " : "Output",
				packet->id_type == standard ? "Standard" : "Extended",
				packet->id
			);

			break;
	}
}

void
print_packet_header (void)
{
	printf (" Time  (ms)   Direction    Format        ID        Type    DLC                     Data                  \n");
	printf ("============ =========== ========== ============ ======== ===== =========================================\n");
}

uint8_t
read_schedule_from_stdin (can_packet_t *packets, uint8_t n)
{
	char 			input_buf[42];		/* 1 */
	char			data_buf[16];
	char			data_substr[3];

	int				i;
	int				end_of_input = 0;

	can_packet_t	*packet_ptr = packets;
	uint8_t			num_scheduled = 0;

	while (num_scheduled < n && !end_of_input)
	{
		if (fgets (input_buf, 42, stdin) == NULL)
			continue;

		switch (input_buf[0])			/* 2 */
		{
			case '@':	/* data packet */

				packet_ptr->packet_type = payload;
				packet_ptr->direction = output;

				sscanf (input_buf, "@%ld:%hhu:%lx:%hhd:%18s\n",
					&(packet_ptr->time), (uint8_t *)&(packet_ptr->id_type),
					&(packet_ptr->id), &(packet_ptr->data_length), data_buf);

				for (i = 0; i < packet_ptr->data_length; i++)
				{
					data_substr[0] = *(data_buf + 2*i);
					data_substr[1] = *(data_buf + 2*i + 1);
					data_substr[2] = '\0'; 	/* 3 */

					sscanf(data_substr, "%hhx", (packet_ptr->data + i));
				}

				packet_ptr++;
				num_scheduled++;

				break;

			case '$':	/* remote packet */

				packet_ptr->packet_type = remote;
				packet_ptr->direction = output;

				sscanf(input_buf, "$%ld:%hhu:%lx\n",
					&(packet_ptr->time), (uint8_t *)&(packet_ptr->id_type),
					&(packet_ptr->id));

				packet_ptr++;
				num_scheduled++;

				break;

			case '!':	/* end of input */

				end_of_input = 1;
				break;
		}
	}

	return num_scheduled;
}

//
//	1. 	Incoming data should never exceed 42 bytes, including newline and
//		null terminator.
//
//	2. 	Notice we don't bother much with error checking. Make sure your
//		input file is sane.
//
//	3. 	We are using sscanf on this string, so we need to manually null
//		terminate it.
//
