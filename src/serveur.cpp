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

Server::Server(int port, const std::string &password)
	: _port(port), _password(password), _listenFd(-1) {
	setupSocket();
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