/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mateon <mateon@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 21:32:05 by mateon            #+#    #+#             */
/*   Updated: 2026/11/02 21:32:44 by mateon           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "includes.hpp"

class Client {
	
private:
    int         _fd;            // client's socket descriptor
    std::string _nick;          // nickname, must be unique on the server
    std::string _username;      // login name
    std::string _realname;      // free-text real name
    std::string _hostname;      // client's address, used in the prefix
    std::string _recvBuffer;    // incomplete data received so far (partial lines)
    std::string _sendBuffer;    // data waiting to be written to the socket
    bool        _passOk;        // has the client sent the right password?
    bool        _registered;    // fully authenticated and allowed to use commands

public:
    Client(int fd);                                 // create a client bound to its socket fd
    Client(const Client &other);                    // copy constructor
    Client &operator=(const Client &other);         // assignment operator
    ~Client();                                      // destructor

    int                 getFd() const;              // socket file descriptor
    const std::string   &getNick() const;           // current nickname
    const std::string   &getUsername() const;       // username given with USER
    const std::string   &getRealname() const;       // real name given with USER
    const std::string   &getHostname() const;       // client's address, as stored by setHostname()
    std::string         getPrefix() const;          // builds "nick!user@host" used in outgoing messages

    void    setNick(const std::string &nick);       // store the nickname (validated by the caller)
    void    setUsername(const std::string &user);   // store the username
    void    setRealname(const std::string &real);   // store the real name
    void    setHostname(const std::string &host);   // store the client's address (set once, right after accept())
    void    setPassOk(bool ok);                     // mark the password as accepted

    bool    isPassOk() const;                       // true if PASS was correct
    bool    isRegistered() const;                   // true once PASS + NICK + USER are all done
    void    tryRegister();                          // switch to registered if every step is complete

    void    appendToRecvBuffer(const std::string &data);    // add freshly received bytes to the buffer
    bool    extractLine(std::string &line);                 // pop one full line ("\r\n" or "\n"); false if none yet
    void    queueMessage(const std::string &msg);           // add a message to the send queue
    bool    hasPendingData() const;                         // true if something is waiting to be sent
    std::string &getSendBuffer();                           // access the send queue (used when writing to the socket)
};

#endif