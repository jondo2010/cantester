//
//	packet.h
//
//	Functions for scheduling, printing, and broadcasting packets for the
//	CAN testing device.
//
//	Michael Jean <michael.jean@shaw.ca>
//

#ifndef _PACKET_H
#define _PACKET_H

#include "can.h"

#define	MAX_TX_MOBS 14	/* 1 */

//
//	1.	We have 15 message objects on the can controller. So we can schedule
//		at most 15 messages at the same time before the system gets overrun.
//

typedef enum packet_dir_t
{
	input, 		/* packet originated from elsewhere on the bus */
	output 		/* packet was scheduled locally */
}
packet_dir_t;

typedef struct can_packet_t
{
	uint32_t		time;			/* rx or tx time in ms since start */
	packet_dir_t	direction;		/* packet direction (input, output) */
	id_type_t		id_type;		/* id type (standard, remote) */
	packet_type_t	packet_type;	/* packet type (payload, remote) */
	uint32_t		id;				/* packet id */
	uint8_t			data_length;	/* number of bytes in packet (0-8) */
	uint8_t			data[8];		/* packet data */
} can_packet_t;

//
//	Handle a transmission complete message from the CAN driver.
//

void
tx_callback
(
	uint8_t mob_index	/* message object */
);


//
//	Handle an incoming message on the bus
//

void
rx_callback
(
	uint8_t mob_index,
	uint32_t id,
	packet_type_t type
);

void
start_receiving (void);


//
//	Broadcast a packet over the CAN bus. The packet will be transmitted as soon
//	as the bus becomes available.
//
//	There are seven message objects allocated for transmission. This function
//	will automatically pick the next free message object to use.
//

void
broadcast_packet
(
	can_packet_t *packet	/* packet to broadcast */
);

//
//	Pretty-print the details of a CAN packet over the serial link.
//

void
print_packet
(
	can_packet_t *packet	/* packet to print */
);

//
//	Pretty-print a header for packets printed with the `print_packet'
//	function.
//

void
print_packet_header (void);

//
//	Read a list of packets to be scheduled from standard input. Place each
//	packet into the array of packets pointed to by `packets'. Schedule at
//	most `n' packets. The array of packets must be already allocated with
//	at least `n' packets.
//
//	Returns the total number of packets actually scheduled.
//
//	Data packets are input as:		@time:eid:id:dlc:data\n
//	Remote packets are input as:	$time:eid:id\n
//
//	Field	Description						Values
//	-----	-------------------------------	--------------------------------
//	time	Time (in ms) to broadcast		0 -> (2^32)-1
//	eid		Extended frame format flag		0: Standard, 1: Extended
//	id		Packet identifier				0 -> (2^11)-1 or 0 -> (2^29)-1
//	dlc		Number of payload bytes			0-8
//	data	Payload bytes					String of hexadecimal numbers
//
//	e.g., 	a standard 2.0A frame format data packet with id 0x7ff and four
//			data bytes (1a, 2b, 3c, and 4d), scheduled to occur at one second
//			into the schedule:
//
//			@1000:0:0x7FF:4:1A2B3C4D\n
//
//	e.g.,	an extended 2.0B frame format remote packet with id 0x4567,
//			scheduled to occur at two seconds into the schedule:
//
//			$1005:1:0x4567\n
//
//	A schedule is terminated by the `!\n' character sequence.
//
//	At most seven packets may be scheduled for the same time. They will each be
//	loaded into a separate transmission buffer and transmitted in the order
//	they appear in the schedule.
//

uint8_t
read_schedule_from_stdin
(
	can_packet_t 	*packets,	/* pointer into schedule */
	uint8_t 		n			/* max number of packets to schedule */
);

#endif
