# battleship

A very simple implementation of battleship in C with basic TCP multiplayer.

Yes, this is my first C program, and yes, I learned Rust before I learned C.
Why? Higher-level API's. This is more about me learning how to use sockets than something real.

## Packet Format

Now I'll describe my, mostly pointless, custom packet format.

### Header

Header Length: 1 byte.

Each packet will begin with a header, as such:

| Field         | Size   |
| ------------- | ------ |
| `PACKET_TYPE` | 1 Byte |

For now, it's literally just a single byte that determines what the data portion of the packet will look like.

### `0x0` - `LOGIN`

Content Length: 16 bytes.

This packet is the first sent by the player, specifying their username in ASCII-only characters. 

| Field      | Size     |
| ---------- | -------- |
| `USERNAME` | 16 Bytes |

### `0x1` - `LOGIN_CONFIRM`

Content Length: 1 byte.

This packet is sent by the server to the player right after `LOG_IN`, notifying the player of their ID (either `1` or `2` for now). 

| Field | Size   |
| ----- | ------ |
| `ID`  | 1 Byte |
