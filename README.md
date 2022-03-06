# File Sender and Receiver
Each server has records of 500x1000x1000, we split all the data to 500 segments along its first dimension. Each segment consists of 1e6 of records. Each part is too big to send in one packet, so we have to segment all the it further and sending in a sequence.

## System Architecture
- Client: sending packets containing a file path
- Server: sending packets to the client using

<!-- The client requests by sending the segment id. 24MB each segment, we may use LRU to limit the memory space used by the server. -->
Each side should set a directory as their data directory, and our goal is to make sure that both sides eventually have the same structure inside that data directory.

For client
- `local` directory: the files origianally on the machine
- `remote` directory: the files trying to download from the server
The path to specify is the using the remote `local` directory as the root directory.

For example, on one trader we have
``` 
|-data
 |
 |-local
  |-file_ab3f33.bin
  |-file_bd3323.bin
 |-remote
    (empty)
```
On the other trader, we should send requests containing the filename, e.g., `file_ab3f33.bin`


## Packet Format
The ordering of packets is guaranteed by TCP protocol. What we need to do is to mark the start and end of file so that the bytestream can be recovered on client side.

### Server-side Packet Format
There are 3 types of packets:
1. Begin packet
2. Middle packet
3. End packet

#### Begin Packet
- header: `{B}`, 3 bytes
- filename: `{file_ab3f33.bin}`, path to file, variable length
- data

#### Middle Packet
- header: `{M}`, 3 bytes
- filename: `{filename}`, path to file, variable length 
- data

#### End Packet
- header: `{E}`, 3 bytes

### Client-side Packet Format
There is only one type of packet.

- header: `{R}`
- chunk id: 0-500


## Extream conditions
When the client has sent a request and fetching data from the server:
1. If the server is down: the client should clear the previous cache and reading from start.
2. If the client is down: the server should catch the exception and stop sending, stop the current process, waiting for client to request again.