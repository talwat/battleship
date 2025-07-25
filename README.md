# battleship

A very simple implementation of battleship in C with basic TCP multiplayer and an ncurses interface.

Yes, this is my first C program, and yes, I learned Rust before I learned C.
Why? Higher-level APIs. This is more about me learning how to use sockets than something real.

This program also, probably, leaks memory. So make sure you use a kernel that isn't 30 years old.

## Compiling

### Dependencies

You'll need SDL2 (plus headers), and that's about it. This is because the client uses it to
play the TTS voice when something happens.

To compile, you will also need your distro's build-essentials group (or equivalent) which you should have already.

### Make

Once you have the dependencies, you should be able to run `make`, or optionally, `make server`/`make client` if you
only want to compile the server or client, for whatever reason. This will also download libsam automatically.

## Packet Format

Now I'll describe my, mostly pointless, custom packet format.

### Header

Header Length: 1 byte.

Each packet will begin with a header, as such:

| Field         | Size   |
| ------------- | ------ |
| `PACKET_TYPE` | 1 byte |

It's just a single byte that determines what the data portion of the packet will look like.
This is handy, since when reading the packet you need only read the first byte to know
how much more to read.

### `0x0` - `NONE`

Content Length: 0 bytes.

This packet should never be sent, and if it has been sent, then the socket was probably closed without prior notice.

### `0x1` - `LOGIN`

Content Length: 16 bytes.

This packet is the first sent by the player, specifying their username in ASCII-only characters. 

| Field      | Size     |
| ---------- | -------- |
| `USERNAME` | 16 bytes |

### `0x2` - `LOGIN_CONFIRM`

Content Length: 1 byte.

This packet is sent by the server to the player right after `LOGIN`, notifying the player of their ID (either `1` or `2` for now). 

| Field | Size   |
| ----- | ------ |
| `ID`  | 1 byte |

### `0x3` - `SETUP`

Content Length: 16 bytes.

This packet is sent to both players by the server when it is ready to recieve ship placements/start setup.

| Field               | Size     |
| ------------------- | -------- |
| `OPPONENT_USERNAME` | 16 bytes |

### `0x4` - `PLACE`

Content Length: 10 bytes.

This packet is sent by each player to the server after `START`.
It determines where each player wants to put each ships, determined by their leftmost or uppermost coordinate.

| Field        | Size   |
| ------------ | ------ |
| `CARRIER`    | 2 bytes |
| `BATTLESHIP` | 2 bytes |
| `CRUISER`    | 2 bytes |
| `SUBMARINE`  | 2 bytes |
| `DESTROYER`  | 2 bytes |

Each ship has the first byte be `0` if it is horizontal and `1` if it is vertical.
Then, the next byte is the position. This is identical to the position of `SELECT`, for instance.

### `0x5` - `TURN`

Content Length: 1 byte.

This is sent to both player by the server, informing them of who's turn it is.

| Field      | Size   |
| ---------- | ------ |
| `PLAYER`   | 1 byte |

`PLAYER` is just the ID of the active player.

### `0x6` - `SELECT`

Content Length: 1 byte.

This is sent to the server by a player in order for them to select a tile to strike.

| Field      | Size   |
| ---------- | ------ |
| `POSITION` | 1 byte |

Here, in `POSITION` the lower 4 bits determine the x-coordinate (1-10) and the higher four bits the y-coordinate (A-J).

### `0x7` - `TURN_RESULT`

Content Length: 3 bytes.

This is sent to both players to notify them whether the previous strike was successful or not, and where it was.

| Field      | Size   |
| ---------- | ------ |
| `PLAYER`   | 1 byte |
| `POSITION` | 1 byte |
| `RESULT`   | 1 byte |

- `PLAYER` is the ID of the player who's turn it was.
- `POSITION` is in the same as in the `SELECT` packet.
- `RESULT` can either be 0 for a miss, 1 for a hit, 2 for a sink, and 3 for a win.

### `0x8` - `QUIT`

Content Length: 0 bytes.

This can be sent by a player at any time, and will end the game right after it is sent to the server.
In this case, the server will then send back the `QUIT` packet to both clients, indicating that a player quit.

Additionally, if a client sends an invalid packet due to a mistake on the client side, then this packet will also
be sent by the server to both players.
