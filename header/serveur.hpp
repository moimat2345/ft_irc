/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serveur.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mateon <mateon@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 21:30:06 by mateon            #+#    #+#             */
/*   Updated: 2026/11/10 03:08:20 by mateon           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVEUR_HPP
#define SERVEUR_HPP

#include "includes.hpp"
#include "client.hpp"
#include "channel.hpp"
#include "message.hpp"

class Server {
private:
    Server(const Server &other) { (void)other; };                    // copying is forbidden (declared, not implemented)
    Server &operator=(const Server &other) { (void)other; return (*this); };         // assignment is forbidden

    // --- network ---
    void    setupSocket();                          // socket(), setsockopt, non-blocking, bind(), listen()
    void    acceptClient();                         // accept() a new connection and register it
    void    handleClientMessage(int fd);
    void    readFromClient(int fd);                 // recv() data and process complete lines
    void    flushClient(int fd);                    // send() the pending data when the socket is ready
    void    removeClient(int fd);                   // remove from channels, close fd, delete the Client

    // --- processing ---
    void    processLine(Client &client, const std::string &line);   // parse a line into a Message and dispatch it
    void    dispatchCommand(Client &client, const Message &msg);           // pick the right cmdXxx() for the command

    // --- commands ---
    void    cmdPass(Client &c, const Message &m);       // PASS: check the server password
    void    cmdNick(Client &c, const Message &m);       // NICK: set a unique nickname
    void    cmdUser(Client &c, const Message &m);       // USER: set username and real name
    void    cmdJoin(Client &c, const Message &m);       // JOIN: enter (or create) a channel
    void    cmdPart(Client &c, const Message &m);       // PART: leave a channel
    void    cmdPrivmsg(Client &c, const Message &m);    // PRIVMSG: message to a user or a channel
    void    cmdQuit(Client &c, const Message &m);       // QUIT: disconnect
    void    cmdKick(Client &c, const Message &m);       // KICK: operator removes a user from a channel
    void    cmdInvite(Client &c, const Message &m);     // INVITE: operator invites a user
    void    cmdTopic(Client &c, const Message &m);      // TOPIC: view or change the channel topic
    void    cmdMode(Client &c, const Message &m);       // MODE: change channel modes (i, t, k, o, l)

    // --- helpers ---
    Client  *findClientByNick(const std::string &nick);     // returns NULL if nobody has this nick
    Channel *findChannel(const std::string &name);          // returns NULL if the channel doesn't exist
    void    sendReply(Client &c, int code, const std::string &text);    // send a numeric reply ("001", "433"...)
    void    sendWelcome(Client &c);                                     // send the registration numeric(s), once

    // --- data ---
    int                         _port;          // port the server listens on
    std::string                 _password;      // server password checked by PASS
    int                         _listenFd;      // listening socket
    std::vector<struct pollfd>  _pollFds;       // all fds watched by poll()
    std::map<int, Client *>     _clients;       // fd -> Client
    // std::vector<Client>       _clients;      // the vector of Clients
    std::map<std::string, Channel *> _channels; // channel name -> Channel
	std::string                 _serverName;     // server name, used in the prefix of outgoing messages
public:
    Server(int port, const std::string &password);  // store port and password
    ~Server();                                      // free all clients, channels and sockets
    void    run();                                  // main loop: poll() and handle events

};

#endif