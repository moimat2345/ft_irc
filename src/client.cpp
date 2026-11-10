/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mateon <mateon@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 20:43:06 by mateon            #+#    #+#             */
/*   Updated: 2026/11/08 21:10:10 by mateon           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../header/client.hpp"

//constructor
Client::Client(int fd)
	: _fd(fd), _passOk(false), _registered(false) {
}

//copy constructor
Client::Client(const Client &other)
	: _fd(other._fd), _nick(other._nick), _username(other._username),
	  _realname(other._realname), _hostname(other._hostname),
	  _recvBuffer(other._recvBuffer), _sendBuffer(other._sendBuffer),
	  _passOk(other._passOk), _registered(other._registered) {
}

//assignment operator
Client &Client::operator=(const Client &other) {
	if (this != &other) {
		_fd = other._fd;
		_nick = other._nick;
		_username = other._username;
		_realname = other._realname;
		_hostname = other._hostname;
		_recvBuffer = other._recvBuffer;
		_sendBuffer = other._sendBuffer;
		_passOk = other._passOk;
		_registered = other._registered;
	}
	return *this;
}

//destructor
Client::~Client() {}



// Getters

int Client::getFd() const {
	return _fd;
}

const std::string &Client::getNick() const {
	return _nick;
}

const std::string &Client::getUsername() const {
	return _username;
}

const std::string &Client::getRealname() const {
	return _realname;
}

const std::string &Client::getHostname() const {
	return _hostname;
}

std::string Client::getPrefix() const {
	return _nick + "!" + _username + "@" + _hostname;
}


// Setters

void Client::setNick(const std::string &nick) {
	_nick = nick;
}

void Client::setUsername(const std::string &user) {
	_username = user;
}

void Client::setRealname(const std::string &real) {
	_realname = real;
}

void Client::setHostname(const std::string &host) {
	_hostname = host;
}

void Client::setPassOk(bool ok) {
	_passOk = ok;
}

// Status checks

bool Client::isPassOk() const {
	return _passOk;
}

bool Client::isRegistered() const {
	return _registered;
}

bool Client::tryRegister() {
	if (!_registered && _passOk && !_nick.empty() && !_username.empty()) {
		_registered = true;
		return true;
	}
	return false;
}

// Buffer management

void Client::appendToRecvBuffer(const std::string &data) {
	_recvBuffer += data;
}

bool Client::extractLine(std::string &line) {
	size_t pos = _recvBuffer.find("\r\n");
	if (pos == std::string::npos) {
		pos = _recvBuffer.find('\n');
	}
	if (pos != std::string::npos) {
		line = _recvBuffer.substr(0, pos);
		_recvBuffer.erase(0, pos + (pos + 1 < _recvBuffer.size() && _recvBuffer[pos] == '\r' ? 2 : 1));
		return true;
	}
	return false;
}

void Client::queueMessage(const std::string &msg) {
	_sendBuffer += msg;
}

bool Client::hasPendingData() const {
	return !_sendBuffer.empty();
}

std::string &Client::getSendBuffer() {
	return _sendBuffer;
}