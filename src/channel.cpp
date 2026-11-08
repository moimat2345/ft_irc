/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mateon <mateon@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:17:06 by mateon            #+#    #+#             */
/*   Updated: 2026/11/08 22:08:21 by mateon           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../header/channel.hpp"

//constructor
Channel::Channel(const std::string &name)
	: _name(name), _inviteOnly(false), _topicRestricted(false), _userLimit(0) {
}

//copy constructor
Channel::Channel(const Channel &other)
	: _name(other._name), _topic(other._topic), _members(other._members),
	  _operators(other._operators), _invited(other._invited),
	  _inviteOnly(other._inviteOnly), _topicRestricted(other._topicRestricted),
	  _key(other._key), _userLimit(other._userLimit) {
}

//assignment operator
Channel &Channel::operator=(const Channel &other) {
	if (this != &other) {
		_name = other._name;
		_topic = other._topic;
		_members = other._members;
		_operators = other._operators;
		_invited = other._invited;
		_inviteOnly = other._inviteOnly;
		_topicRestricted = other._topicRestricted;
		_key = other._key;
		_userLimit = other._userLimit;
	}
	return *this;
}

//destructor
Channel::~Channel() {}


// --- identity & topic ---

const std::string &Channel::getName() const {
	return _name;
}

const std::string &Channel::getTopic() const {
	return _topic;
}

void Channel::setTopic(const std::string &topic) {
	_topic = topic;
}

// --- members ---

void Channel::addMember(Client *client) {
	if (!isMember(client)) {
		_members.push_back(client);
	}
}

void Channel::removeMember(Client *client) {
	_members.erase(std::remove(_members.begin(), _members.end(), client), _members.end());
	_operators.erase(client);
	_invited.erase(client);
}

bool Channel::isMember(Client *client) const {
	return std::find(_members.begin(), _members.end(), client) != _members.end();
}

size_t Channel::getMemberCount() const {
	return _members.size();
}

// --- operators ---

void Channel::addOperator(Client *client) {
	if (isMember(client)) {
		_operators.insert(client);
	}
}

void Channel::removeOperator(Client *client) {
	_operators.erase(client);
}

bool Channel::isOperator(Client *client) const {
	return _operators.find(client) != _operators.end();
}

// --- invites ---

void Channel::invite(Client *client) {
	_invited.insert(client);
}

bool Channel::isInvited(Client *client) const {
	return _invited.find(client) != _invited.end();
}

// --- messaging ---

void Channel::broadcast(const std::string &msg, Client *exclude) {
	for (std::vector<Client *>::iterator it = _members.begin(); it != _members.end(); ++it) {
		if (*it != exclude) {
			(*it)->queueMessage(msg);
		}
	}
}

// --- modes: i, t, k, l ---

void Channel::setInviteOnly(bool b) {
	_inviteOnly = b;
}

bool Channel::isInviteOnly() const {
	return _inviteOnly;
}

void Channel::setTopicRestricted(bool b) {
	_topicRestricted = b;
}

bool Channel::isTopicRestricted() const {
	return _topicRestricted;
}

void Channel::setKey(const std::string &key) {
	_key = key;
}

const std::string &Channel::getKey() const {
	return _key;
}

void Channel::setUserLimit(size_t limit) {
	_userLimit = limit;
}

size_t Channel::getUserLimit() const {
	return _userLimit;
}
