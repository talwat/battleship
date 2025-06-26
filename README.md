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

This packet is sent by the server to the player right after `LOGIN`, notifying the player of their ID (either `1` or `2` for now). 

| Field | Size   |
| ----- | ------ |
| `ID`  | 1 Byte |

### `0x2` - `SETUP`

Content Length: 16 bytes.

This packet is sent to both players by the server when it is ready to recieve ship placements/start setup.

| Field               | Size     |
| ------------------- | -------- |
| `OPPONENT_USERNAME` | 16 Bytes |

### `0x3` - `PLACE`

Content Length: 10 bytes.

This packet is sent by each player to the server after `START`.
It determines where each player wants to put each ships, determined by their leftmost or uppermost coordinate.

| Field        | Size   |
| ------------ | ------ |
| `CARRIER`    | 2 Bytes |
| `BATTLESHIP` | 2 Bytes |
| `CRUISER`    | 2 Bytes |
| `SUBMARINE`  | 2 Bytes |
| `DESTROYER`  | 2 Bytes |

Each ship has the first byte be `0` if it is horizontal and `1` if it is vertical.
Then, the next byte is the position. This is identical to the position of `SELECT`, for instance.

### `0x4` - `TURN`

Content Length: 1 byte.

This is sent to both player by the server, informing them of who's turn it is.

| Field      | Size   |
| ---------- | ------ |
| `PLAYER`   | 1 Byte |

`PLAYER` is just the ID of the active player.

### `0x5` - `SELECT`

Content Length: 1 byte.

This is sent to the server by a player in order for them to select a tile to strike.

| Field      | Size   |
| ---------- | ------ |
| `POSITION` | 1 Byte |

Here, in `POSITION` the lower 4 bits determine the x-coordinate (1-10) and the higher four bits the y-coordinate (A-J).

### `0x6` - `TURN_RESULT`

Content Length: 3 bytes.

This is sent to both players to notify them whether the previous strike was successful or not, and where it was.

| Field      | Size   |
| ---------- | ------ |
| `PLAYER`   | 1 Byte |
| `POSITION` | 1 Byte |
| `RESULT`   | 1 Byte |

- `PLAYER` is the ID of the player who's turn it was.
- `POSITION` is in the same as in the `SELECT` packet.
- `RESULT` can either be 0 for a miss, 1 for a hit, and 2 for a sink.

### `0x7` - `END`

Content Length: 1 byte.

This is sent to both players when the game has ended, and one has won.
This will either be sent after a TURN_RESULT, or after one player quits.

| Field      | Size   |
| ---------- | ------ |
| `WINNER`   | 1 Byte |

### `0x8` - `QUIT`

Content Length: 0 bytes.

This can be sent by a player at any time, and will end the game right after it is sent to the server.
