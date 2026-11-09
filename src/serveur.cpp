/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serveur.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mateon <mateon@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 20:11:26 by mateon            #+#    #+#             */
/*   Updated: 2026/11/08 20:16:45 by mateon           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../header/serveur.hpp"

void Server::setupSocket() {
	struct pollfd FirstPoll;
	_listenFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_listenFd < 0)
		throw(std::runtime_error("Error setting socket"));
	if (fcntl(_listenFd, F_SETFL, O_NONBLOCK) < 0)
		throw(std::runtime_error("Failed to set option (O_NONBLOCK) on the socket"));
	int opt = 1;
	if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw(std::runtime_error(std::strerror(errno)));
	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(_listenFd, reinterpret_cast< struct sockaddr *>(&addr), sizeof(addr)) < 0)
		throw(std::runtime_error("Error assigning a name to a socket"));
	if (listen(_listenFd, SOMAXCONN) < 0)
		throw(std::runtime_error("Error setting socket to listen for connections"));

	FirstPoll.fd = _listenFd;
	FirstPoll.events = POLLIN;
	FirstPoll.revents = 0;
	_pollFds.push_back(FirstPoll);
	LOG_SERVER("Server started on port " << _port);
	LOG_SERVER("Server started with the password " << "<" << _password << ">");
}

Server::Server(int port, const std::string &password)
	: _port(port), _password(password), _listenFd(-1) {
	setupSocket();
}

void Server::run() {
	LOG_SERVER("Waiting to accept a connection...");
	// while (true) {
	// 	if (poll(&_pollFds[0], _pollFds.size(), -1) == -1) {
	// 		if (errno == EINTR) continue; // interrupted by signal, retry poll()
	// 		throw(std::runtime_error("poll() faild"));
	// 	}
		
	// 	for (size_t i = 0; i < _pollFds.size(); ++i) {
	// 		if (_pollFds[i].revents == 0) continue;
	
	// 		if (_pollFds[i].revents & POLLIN) {
	// 			if (_pollFds[i].fd == _listenFd)
	// 				// setup a new client
	// 			else
	// 				// receive new data from a registered client(read, write)
	// 		}
	// 	}
	// }
}

Server::~Server() {
	// Close all client sockets and delete Client objects
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		close(it->first);
		delete it->second;
	}
	_clients.clear();

	// Delete all Channel objects
	for (std::map<std::string, Channel *>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
		delete it->second;
	}
	_channels.clear();

	// Close the listening socket
	if (_listenFd != -1) {
		close(_listenFd);
	}
}