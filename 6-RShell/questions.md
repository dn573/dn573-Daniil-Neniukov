1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

A remote shell client looks for a special delimiter sent by the server, for example, a null byte or ASCII 0x04, to know when output finishes, because TCP is a stream protocol, the client must store incoming data in a buffer until the delimiter is found, this way we can make sure even if data arrives in multiple chunks, the client can reassemble the full response, partial reads are handled by repeatedly calling recv() and appending the data until the end marker is reached

2. This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

To define the start and end of a command, the shell protocol must introduce a unique marker or a length prefix, without a boundary like this, commands could merge or be split unpredictably, causing parsing errors, the problem is ensuring the receiver continues reading until the boundary is found, rather than assuming a single read equals one command, this helps prevent corruption of the command stream and maintains predictable communication

3. Describe the general differences between stateful and stateless protocols.

Stateful protocols keep track of ongoing session information on the server or client side, allowing subsequent requests to depend on prior interactions, on the other hand, stateless protocols treat each request in isolation, requiring the request itself to contain all relevant data, so while stateful protocols can simplify certain interactions by remembering context, they can be harder to scale and recover, and stateless protocols, though simpler to manage at scale, often push more complexity to the client

4. Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?

Although UDP is “unreliable,” we still choose it when minimal latency and overhead are more important than guaranteed delivery, for real-time applications like video streaming, losing a few packets is preferable to the delays caused by TCP’s retransmission, UDP also avoids the congestion control overhead of TCP, making it more responsive, so, even though some data may be lost, the performance benefits are worth it in time-sensitive situations

5. What interface/abstraction is provided by the operating system to enable applications to use network communications?

The operating system presents a sockets API as the abstraction for network communications, with sockets, applications can create endpoints, bind them to addresses and ports, and send or receive data, the OS then handles the low-level protocol details, ensuring a consistent interface for TCP, UDP, or other methods, this sockets abstraction allows developers to treat network interactions much like file I/O
