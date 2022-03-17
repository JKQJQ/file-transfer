# File Sender and Receiver Duplex
Each trader has records of 500x1000x1000, we split all the data to 500 segments along its first dimension. We use exchange 1 and 2 as duplex forwarders, i.e.
```
                   orders1 =>         orders1 =>
[0. 250):   trader 1 <><><> exchange 1 <><><> trader 2
                    <= orders2       <= orders2
                    orders1 =>         orders1 =>
[250, 500): trader 1 <><><> exchange 2 <><><> trader 2
                    <= orders2       <= orders2
```

<>: a duplex 1.5MB/s link, multiple ones indicate duplex links in parallel

Packets are of equal length N(for example 1400).

Traders are servers, while exchanges are clients. Traders and exchanges are both senders and receivers.

A receiver scans the data directory and pushes all the not found files with remainder related to their worker id and pushes these filenames to their request(task) queues.

After creating the connection, client and server are equal. They start a loop and inside the loop, `send()` first send `recv()`. Request packets have higer priority over data packets, but all of length N, padding end with random data if data length less than N.

- request packets: `R:requested_file_name:other packet`, other packets could either be data packets or nil packets. if `R::other packet`, this means no file is being requested. If no data, the other packet should be a nil packet(`N:`)
- data packets: `B:file_size:payload`(begin) or `M:payload`(middle)
- nil packets: `N:`
- end packets: `E:`, it should be the 


It is the receivers' obligation to record the file_size and how many bytes left to read. All the paddings in the end of the last packet of a binary file should be abandoned.

The receiver is also a requester that sends the request packet. The request queue head is the file being processed. The request is no longer in the queue IFF the .success file exists for the file being requested on the receiver.

It is the senders' obligation to make sure that each packet is of equal length, i.e., N. If the data left is not enough for the current one being processed, paddings should apply.

To make sure all the packets are of length N: resend the ones not sent in a N-bytes packet if the nBytesSent return value of `send()` is not N.


The path should include file path.