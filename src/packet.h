#ifndef PACKET_H
#define PACKET_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"

extern const bool packet_debug;

/**
 * @enum PacketType
 *
 * Defines the various packet types that can be sent or received.
 */
enum PacketType {
  NONE = 0x0,
  LOGIN = 0x1,
  LOGIN_CONFIRM = 0x2,
  SETUP = 0x3,
  PLACE = 0x4,
  TURN = 0x5,
  SELECT = 0x6,
  TURN_RESULT = 0x7,
  QUIT = 0x8,
};

typedef uint8_t PacketType;

/**
 * @struct packet
 *
 * Encapsulates the information required for a packet,
 * including its type, length, name, and a pointer to the data payload.
 */
struct packet {
  PacketType type;
  uint8_t length;
  const char *name;
  unsigned char *data;
};

/**
 * Initializes a packet structure by setting its type, length, name,
 * and data fields. The length and name are retrieved from the PACKETS array
 * based on the provided type.
 *
 * @param type The type of the packet, used as an index into the PACKETS array.
 * @param data Pointer to the data to be included in the packet.
 * @return struct packet The initialized packet structure.
 */
struct packet new_packet(int type, unsigned char *data);

/**
 * Reads the packet type from the file descriptor, allocates memory for the packet data
 * based on the expected length for that type, and reads the data into the allocated buffer.
 * Also constructs a new packet using the read type and data.
 *
 * @param fd The file descriptor to read the packet from.
 * @return struct packet The packet read from the file descriptor.
 *
 * @note The function allocates memory for the packet data, which should be freed appropriately to avoid memory leaks.
 */
struct packet read_packet(int fd);

/**
 * Reads pairs of bytes from the input data array, extracting the orientation,
 * x, and y coordinates for each ship and storing them in the provided placements array.
 * The x and y coordinates are extracted from the high and low nibbles of the second byte
 * in each pair, respectively.
 *
 * @param data Pointer to the input byte array containing placement data.
 * @param placements Array of ship structures to be filled with parsed placement information.
 * @return true if parsing is successful.
 */
bool parse_placements(unsigned char *data, struct ship placements[5]);

/**
 * Sends the type and data of the given packet to the provided
 * file descriptor. The packet's type is written first, followed by its data.
 *
 * @param fd The file descriptor to which the packet will be written.
 * @param packet Pointer to the packet structure containing the type and data to send.
 */
void write_packet(int fd, struct packet *packet);

/**
 * Frees the memory allocated for the packet's data.
 * This function should be called after the packet is no longer needed
 * to prevent memory leaks.
 *
 * @param packet Pointer to the packet structure whose data is to be freed.
 */
void free_packet(struct packet *packet);

#endif