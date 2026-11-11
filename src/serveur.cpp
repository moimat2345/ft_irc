/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serveur.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mateon <mateon@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 20:11:26 by mateon            #+#    #+#             */
/*   Updated: 2026/11/11 19:21:10 by mateon           ###   ########.fr       */
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

void Server::acceptClient() {
	struct sockaddr_in cliaddr;
	struct pollfd NewCliPoll;
	socklen_t cliaddrlen = sizeof(cliaddr);

	int incofd = accept(_listenFd, reinterpret_cast< struct sockaddr *>(&cliaddr), &cliaddrlen);
	if (incofd < 0) { LOG_SERVER("accept() failed"); return; }
	if (fcntl(incofd, F_SETFL, O_NONBLOCK) < 0) { LOG_SERVER("fcntl() failed"); return; }

	NewCliPoll.fd = incofd;
	NewCliPoll.events = POLLIN;
	NewCliPoll.revents = 0;

	Client *client = new Client();
	client->setFd(incofd);
	// _clients.push_back(client);
	_clients.insert(std::make_pair(incofd, client));
	_pollFds.push_back(NewCliPoll);
	client->setHostname(inet_ntoa(cliaddr.sin_addr));
	LOG_CLIENT(incofd << " - Connected");
}

Client* Server::getClient(int fd) {
	std::map<int, Client *>::iterator it;

	it = _clients.find(fd);
	if (it == _clients.end())
		return (NULL);
	return (it->second);
}

void Server::readFromClient(int fd) {
	char buf[1024];
	std::memset(buf, 0, sizeof(buf));

	ssize_t bytes = recv(fd, buf, sizeof(buf) - 1, 0);
	if (bytes <= 0) {
		removeClient(fd);
		return ;
	}
	std::string line;
	Client *cli = getClient(fd);
	if (!cli)
		return ;
	cli->appendToRecvBuffer(std::string(buf, bytes)); //so we don't get a probleme even if they is a null character in the buffer
	while (cli->extractLine(line)) {
		processLine(*cli, line);
		if (!getClient(fd))
			return ;
	}
}

void Server::flushClient(int fd) {
	Client* cli = getClient(fd);
	if (!cli || !cli->hasPendingData())
		return ;
	const std::string &buf = cli->getSendBuffer();
	ssize_t sent = send(fd, buf.c_str(), buf.size(), 0);
	if (sent <= 0) {
		removeClient(fd);
		return ;
	}
	cli->trimSendBuffer(sent);
}

void Server::removeClient(int fd) {
	std::map<int, Client *>::iterator ClientIt;

	ClientIt = _clients.find(fd);
	if (ClientIt == _clients.end())
		return ;
	LOG_CLIENT(fd << " - Disconnected");
	std::map<std::string, Channel *>::iterator it = _channels.begin();
	while (it != _channels.end()) {
		if (it->second->isMember(ClientIt->second))
			it->second->removeMember(ClientIt->second);
		
		if (it->second->getMemberCount() == 0)
		{
			delete it->second;
			_channels.erase(it++);
		} else
			++it;
	}

	for (size_t i = 0; i < _pollFds.size(); ++i) {
		if (_pollFds[i].fd == fd) {
			_pollFds.erase(_pollFds.begin() + i);
			break ;
		}
	}

	close(fd);
	delete ClientIt->second;
	_clients.erase(ClientIt);
}

// ---------------- main part ------------------------------------- //

Server::Server(int port, const std::string &password)
	: _port(port), _password(password), _listenFd(-1), _serverName("ircserv") {
	setupSocket();
}

/**
 * 
 * When the condition _pollFds[i].fd == _listenFd is satisfied, it means 
 * that an event has occurred on the server socket _listenFd. In a server 
 * application, this type of event usually indicates that a new client is 
 * trying to connect to the server. The server socket _listenFd is 
 * responsible for receiving incoming connection requests from the clients.
 */

//gonna try to rebuild the run function, but i will do it in a different way, because the one i had was not working properly

// void Server::run() {
// 	LOG_SERVER("Waiting to accept a connection...");
// 	while (true) {
// 		if (poll(&_pollFds[0], _pollFds.size(), -1) < 0) {
// 			if (errno == EINTR) continue; // interrupted by signal, retry poll()
// 			throw(std::runtime_error("poll() failed"));
// 		}
		
// 		for (size_t i = 0; i < _pollFds.size(); ++i) {
// 			if (_pollFds[i].revents == 0) continue;
	
// 			if (_pollFds[i].revents & POLLIN) {
// 				if (_pollFds[i].fd == _listenFd)
// 					acceptClient();
// 				continue ;
// 			}

// 			if (_pollFds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
// 				removeClient(_pollFds[i].fd);
// 				continue ;
// 			}

// 			if (_pollFds[i].revents & POLLIN) readFromClient(_pollFds[i].fd);
// 			if (!getClient(_pollFds[i].fd))
// 				continue ;
// 			if (_pollFds[i].revents & POLLOUT) flushClient(_pollFds[i].fd);
// 		}
// 	}
// }

void Server::run() {
	LOG_SERVER("Waiting to accept a connection...");
	while (true) {
		// Poll the file descriptors for events
		for (size_t i = 1; i < _pollFds.size(); ++i) {
			_pollFds[i].events = POLLIN;
			Client* cli = getClient(_pollFds[i].fd);
			if (cli && cli->hasPendingData())
				_pollFds[i].events |= POLLOUT;
		}
		// Wait for events on the file descriptors
		if (poll(&_pollFds[0], _pollFds.size(), -1) < 0) {
			if (errno == EINTR) continue;
			throw(std::runtime_error("poll() failed"));
		}
		// Handle events for each file descriptor
		size_t i = 0;
		while (i < _pollFds.size()) {
			int fd = _pollFds[i].fd;
			short revents = _pollFds[i].revents;
			
			// If the event is on the listening socket, accept a new client
			if (fd == _listenFd) {
				if (revents & POLLIN) acceptClient();
				++i;
				continue;
			}
			// If the event is on a client socket, handle it
			if (revents & POLLIN) readFromClient(fd);
			// If the client has disconnected or an error occurred, remove it
			else if (revents & (POLLHUP | POLLERR | POLLNVAL)) {
				removeClient(fd);
			}
			// If the client has data to send, flush it
			if (getClient(fd) && (revents & POLLOUT)) flushClient(fd);
			// Move to the next file descriptor only if the current one still exists
			if (getClient(fd))
				++i;
		}
	}
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