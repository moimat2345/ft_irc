> *This file gives much wider information on the rules and concepts provided by the subject*

- ssize_t recv(size_t size; 
                        int sockfd, void buf[size], size_t size,
                        int flags);
    
    :used to receive messages from a socket. They may be used to receive data on both **connectionless and connection-oriented sockets**. The only difference between recv() and read(2) is hte presence of **flags**.

    Return Type - the size of the message on successful completion. 
    1. If a message is too long to fir in the supplied buffer, excess bytes might be discarded depending on the type of the socket the message is received from. 
    2. If no messages are avaibable at the socket, the receive calls wait for a message to arrive if socket is blocking.

    An application can use select(2), poll(2), or epoll(7) to
       determine when more data arrives on a socket.

- int fcntl(int fd, int op, ...);

- int poll(struct pollfd *fds, nfds_t nfds, int timeout);
    :waits for one of a set of file descriptors to become ready to perform I/O operations. 

    The set of file descriptors to be monitored is specified in the **fds** argument, which is an array of structures of the following form - 

        struct pollfd {
            int   fd;         /* file descriptor */
            short events;     /* requested events */
            short revents;    /* returned events */
        };
    
- int socket(int domain, int type, int protocol);
    :create an endpoint for communication and returns a file descriptor that refers to that endpoint. 

### Connectionless Socket. VS. Connection-oriented Socket.
These two types define how data is transmited between a client and a server and differ mainly in 

**A connection-oriented service** establishes a dedicated communication path between the sender and receiver before any data is transferred. This process involves a handshake to set up the connection, ensuring both parties are ready for communication. **Once** the connection is established, data packets are transmitted in a sequence and reliable way. 

## Blocking and Non-Blocking File Descriptors Under the Hood

