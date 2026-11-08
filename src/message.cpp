/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   message.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mateon <mateon@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 22:53:22 by mateon            #+#    #+#             */
/*   Updated: 2026/11/08 23:17:16 by mateon           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../header/message.hpp"

// constructor
Message::Message() {}

// copy constructor
Message::Message(const Message &other)
	: _command(other._command), _params(other._params) {}
	
// assignment operator
Message &Message::operator=(const Message &other) {
	if (this != &other) {
		_command = other._command;
		_params = other._params;
	}
	return *this;
}

// destructor
Message::~Message() {}


/// --- parsing ---
void Message::parse(const std::string &line) {
	_command.clear();
	_params.clear();

	std::istringstream iss(line);
	std::string token;

	// Get the command
	if (iss >> token) {
		_command = token;
	}

	// Get the parameters
	while (iss >> token) {
		if (token[0] == ':') {
			// The rest of the line is a single parameter
			std::string rest;
			std::getline(iss, rest);
			_params.push_back(token.substr(1) + rest);
			break;
		} else {
			_params.push_back(token);
		}
	}
}

// --- getters ---

const std::string &Message::getCommand() const {
	return _command;
}

const std::vector<std::string> &Message::getParams() const {
	return _params;
}
