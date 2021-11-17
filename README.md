Copyright Micu Florian-Luis 321CA 2021 - Server-Client Application

1. The flow of the program

Server:
The server starts with 3 sockets: STDIN, UDP and a passive TCP socket. The
STDIN socket is used to take commands from the keyboard, the only allowed 
command being "exit" which closes the server and all of the connected clients.
The UDP socket is used for communication with a UDP client that provides 
content updates for a certain topic. The TCP passive socket is used for 
listening to connection requests from other TCP clients. These clients will
receive the topic update from the UDP client based on their subscription 
status. A new socket will be created for each new valid TCP client. The
server will only print to STDOUT the status of a client 
(connected/disconnected/already connected) and if any processing operation
failed, STDERR message will be printed. The Nagle algorithm is disabled for
every TCP socket that is created when a client connects.

Client:
The client will start with 2 sockets: STDIN and a TCP socket. The STDIN socket
will be used for processing the "exit" command or the "subscribe/unsubscribe"
command. The input is verified to have the right "subscribe/unsubscribe" usage
as it will be processed into a client message structure. The TCP socket is used
for communication with the server. On this socket, the client can receive 
updates from a certain topic in the form of a TCP message structure. The 
contents of the structure will be printed to STDOUT, other content that can be
printed is the status of a subscription (subscribed/unsubscribed). The Nagle
algorithm is disabled for the TCP socket.

2. Structures

A message received from a UDP client will be first transformed to a UDP message
structure which has the following members:

struct __attribute__((packed)) udp_msg {
    char topic[50];      // topic
    uint8_t data_type;   // data type
    char content[1501];  // content
};

This structure will serve as a base for the TCP structure message that will be 
sent to all of the TCP clients that are subscribed to a particular topic. After
the message is transformed into a "udp_msg" structure, the data type member 
will be used to determine how to process to content accordingly. The data type
member is represented as an int from 0 to to 3 which translate into the 
following types: "INT", "SHORT_INT", "FLOAT" and "STRING". The translated data
type and the processed content will be added to a TCP message structure. 
Moreover, the IP address of the UDP client and the port from which the message
was received are extracted and added to the TCP message structure which looks
as follows:

struct __attribute__((packed)) tcp_msg {
    char ip_src[16];      // UDP client's IP address
    unsigned short port;  // Port from which the message was received
    char topic[51];       // Topic
    char data_type[11];   // Data Type
    char content[1501];   // Content
};

After the structure is completed, it will be sent to the clients that are 
subscribed to its topic if they are online, otherwise it will be stored 
temporarily for when they reconnect to the server.

A message received from a client will be a client structure which has the
following members:

struct __attribute__((packed)) client_msg {
    bool subscribe;  // Subscribe/Unsubscribe (true or false)
    char topic[51];  // Topic
    bool sf;         // SF (true of false)
};

Based on this structure, links will be made between a certain topic and a
client.

3. Data Structures:

The server requires various data structures in order to conserve the properties
of the Store-and-Forward as well as to verify which clients are online or not.

* unordered_map<string, unordered_set<string>> topics_id_off_map
Description: Creates an association between a topic and set of clients that 
have the SF flag active.
Usage: When a UDP client sends a message to the server, some of the clients 
that are subscribed to a topic might not be online. This data structure will be
used to verify if a client's ID, that is subscribed to a certain topic, has set
the value for the SF to true. If it is true, the ID will be present in the set
associated with the topic.

* unordered_map<string, unordered_set<string>> topics_id_all_map
Description: Creates an association between a topic and set of clients 
regardless of the SF flag.
Usage: Useful for iterating through all of the clients' ID that are subscribed
to the received topic, as they have to receive the message.

* unordered_map<int, string> fd_id_map / unordered_map<string, int> id_fd_map
Description: Creates an association between a client's FD and its ID.
Usage: The majority of the data structures use the ID as a means of remembering
a certain client, since the FD for the same client could change when he 
reconnects. Moreover, the main loop that determines which socket is active in 
the server iterates through all of the FDs in the temporary FD set (used for 
select()), which is why a FD to ID association must be created. An ID to FD 
association must be created to know to which FD to send a message based on the
client's ID.

* unordered_map<string, queue<struct tcp_msg>> id_history_map
Description: Creates a place to store all of the messages that must be sent to
a client (represented by an ID) when he reconnects to the server. 
Usage: All of the messages are stored because the SF for the topics they 
represent was set to active by the client. The messages are stored in a queue
to preserve the order they were received from the UDP client.

* unordered_set<string> online_set
Description: A set that contains all of the clients (represented by their ID)
that are online right now.
Usage: When a UDP message is received, the program iterates through all of the
clients that are subscribed to that particular topic (topics_id_all_map), 
however it must know which client is online or not to determine if it can send
the message right away or store it.

Unordered maps were used to create a link between two objects, plus they have
O(1) time complexities for search, insert, remove. Unordered sets were used to
store clients IDs as I did not need to store a link, and it has the same 
complexities of the unordered_map. Moreover, the ID of each client is unique 
(no two online clients with the same ID can coexist), which preserves the 
properties of the set. A queue was used because it has O(1) time complexity for
push() and pop() operations and it preserves the order of the arrival. 

4. Observations
Every return value is not ignored and it is verified with the "DIE" function to
maintain a safe API. I considered it is the responsibility of the sender to 
create a message in the right format that is why I do not verify the message 
sent by the UDP client. Furthermore, every input is verified to have a right 
usage. The same goes for the "subscriber" and "server" executables which must 
have the right number of arguments.

The code was made in C++ as it had data structures already made, however I 
created the API in a C style approach with some C++ sugar.

In some instances, the checker failed. A computer restart or a change of port
solved the problem.

5. Feedback
Great homework! It helped me better understand the concepts from the TCP and
UDP labs and it was not to hard to implement. However, I saw that in practice
everyone uses the boost API for server-clients application as it is more stable
than our approach.
