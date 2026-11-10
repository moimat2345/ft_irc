/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serveur_commands.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mateon <mateon@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/10 02:58:50 by mateon            #+#    #+#             */
/*   Updated: 2026/11/10 10:08:52 by mateon           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../header/serveur.hpp"

// --- helpers ---

void Server::sendReply(Client &c, int code, const std::string &text) {
	std::string nick = c.getNick().empty() ? "*" : c.getNick();
	std::ostringstream oss;
	oss << ":" << _serverName << " " << code << " " << nick << " " << text << "\r\n";
	c.queueMessage(oss.str());
}

/// returns NULL if nobody has this nick
Client *Server::findClientByNick(const std::string &nick) {
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->second->getNick() == nick) {
			return it->second;
		}
	}
	return NULL;
}

// returns NULL if the channel doesn't exist
Channel *Server::findChannel(const std::string &name) {
	std::map<std::string, Channel *>::iterator it = _channels.find(name);
	if (it != _channels.end()) {
		return it->second;
	}
	return NULL;
}

// sends the registration numeric(s) once, right when a client finishes PASS+NICK+USER
void Server::sendWelcome(Client &c) {
	sendReply(c, 1, "Welcome to the IRC network " + c.getPrefix());
}

// --- processing ---

// parse a line into a Message and dispatch it
void Server::processLine(Client &client, const std::string &line) {
	Message msg;
	msg.parse(line);
	dispatch(client, msg);
}

// pick the right cmdXxx() for the command
void Server::dispatch(Client &client, const Message &msg) {
	const std::string &cmd = msg.getCommand();
	if (cmd.empty())
		return;
	if (!client.isRegistered() && cmd != "PASS" && cmd != "NICK" && cmd != "USER" && cmd != "QUIT") {
		sendReply(client, 451, ":You have not registered");
		return;
	}
	if (cmd == "PASS") {
		cmdPass(client, msg);
	} else if (cmd == "NICK") {
		cmdNick(client, msg);
	} else if (cmd == "USER") {
		cmdUser(client, msg);
	} else if (cmd == "JOIN") {
		cmdJoin(client, msg);
	} else if (cmd == "PART") {
		cmdPart(client, msg);
	} else if (cmd == "PRIVMSG") {
		cmdPrivmsg(client, msg);
	} else if (cmd == "QUIT") {
		cmdQuit(client, msg);
	} else if (cmd == "KICK") {
		cmdKick(client, msg);
	} else if (cmd == "INVITE") {
		cmdInvite(client, msg);
	} else if (cmd == "TOPIC") {
		cmdTopic(client, msg);
	} else if (cmd == "MODE") {
		cmdMode(client, msg);
	} else {
		sendReply(client, 421, cmd + " :Unknown command");
	}
}

// --- commands ---

// PASS: check the server password
void Server::cmdPass(Client &c, const Message &m) {
	if (m.getParams().size() < 1) {
		sendReply(c, 461, "PASS :Not enough parameters");
		return;
	}
	if (c.isRegistered()) {
		sendReply(c, 462, ":You may not reregister");
		return;
	}
	const std::string &password = m.getParams()[0];
	if (password == _password) {
		c.setPassOk(true);
		if (c.tryRegister()) {
			sendWelcome(c);
		}
	} else {
		sendReply(c, 464, ":Password incorrect");
	}
}

// NICK: set a unique nickname
void Server::cmdNick(Client &c, const Message &m) {
	if (m.getParams().size() < 1) {
		sendReply(c, 461, "NICK :Not enough parameters");
		return;
	}
	const std::string &nick = m.getParams()[0];
	if (findClientByNick(nick) != NULL) {
		sendReply(c, 433, nick + " :Nickname is already in use");
		return;
	}
	if (c.isRegistered()) {
		std::string nickChangeMsg = ":" + c.getPrefix() + " NICK :" + nick + "\r\n";
		c.setNick(nick);
		c.queueMessage(nickChangeMsg);
		for (std::map<std::string, Channel *>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
			if (it->second->isMember(&c)) {
				it->second->broadcast(nickChangeMsg, &c);
			}
		}
		return;
	}
	c.setNick(nick);
	if (c.tryRegister()) {
		sendWelcome(c);
	}
}

// USER: set username and real name
void Server::cmdUser(Client &c, const Message &m) {
	if (m.getParams().size() < 4) {
		sendReply(c, 461, "USER :Not enough parameters");
		return;
	}
	if (c.isRegistered()) {
		sendReply(c, 462, ":You may not reregister");
		return;
	}
	c.setUsername(m.getParams()[0]);
	c.setRealname(m.getParams()[3]);
	if (c.tryRegister()) {
		sendWelcome(c);
	}
}

// JOIN: enter (or create) a channel
void Server::cmdJoin(Client &c, const Message &m) {
	if (m.getParams().size() < 1) {
		sendReply(c, 461, "JOIN :Not enough parameters");
		return;
	}
	const std::string &channelName = m.getParams()[0];
	if (channelName.empty() || channelName[0] != '#') {
		sendReply(c, 476, channelName + " :Invalid channel name");
		return;
	}
	Channel *channel = findChannel(channelName);
	if (channel == NULL) {
		channel = new Channel(channelName);
		_channels[channelName] = channel;
		channel->addMember(&c);
		channel->addOperator(&c);
	} else {
		if (channel->isMember(&c))
			return;
		if (channel->isInviteOnly() && !channel->isInvited(&c)) {
			sendReply(c, 473, channelName + " :Cannot join channel (+i)");
			return;
		}
		if (!channel->getKey().empty()) {
			std::string key;
			if (m.getParams().size() > 1)
				key = m.getParams()[1];
			if (key != channel->getKey()) {
				sendReply(c, 475, channelName + " :Cannot join channel (+k)");
				return;
			}
		}
		if (channel->getUserLimit() > 0 && channel->getMemberCount() >= channel->getUserLimit()) {
			sendReply(c, 471, channelName + " :Cannot join channel (+l)");
			return;
		}
		channel->addMember(&c);
	}
	std::string joinMsg = ":" + c.getPrefix() + " JOIN " + channelName + "\r\n";
	channel->broadcast(joinMsg, NULL);
	if (!channel->getTopic().empty()) {
		sendReply(c, 332, channelName + " :" + channel->getTopic());
	}
	std::string namesList;
	const std::vector<Client *> &members = channel->getMembers();
	for (std::vector<Client *>::const_iterator it = members.begin(); it != members.end(); ++it) {
		if (it != members.begin())
			namesList += " ";
		if (channel->isOperator(*it))
			namesList += "@";
		namesList += (*it)->getNick();
	}
	sendReply(c, 353, "= " + channelName + " :" + namesList);
	sendReply(c, 366, channelName + " :End of NAMES list");
}

// PART: leave a channel
void Server::cmdPart(Client &c, const Message &m) {
	if (m.getParams().size() < 1) {
		sendReply(c, 461, "PART :Not enough parameters");
		return;
	}
	const std::string &channelName = m.getParams()[0];
	Channel *channel = findChannel(channelName);
	if (channel == NULL || !channel->isMember(&c)) {
		sendReply(c, 442, channelName + " :You're not on that channel");
		return;
	}
	std::string partMsg = ":" + c.getPrefix() + " PART " + channelName + "\r\n";
	channel->broadcast(partMsg, NULL);
	channel->removeMember(&c);
	if (channel->getMemberCount() == 0) {
		delete channel;
		_channels.erase(channelName);
	}
}

// PRIVMSG: message to a user or a channel
void Server::cmdPrivmsg(Client &c, const Message &m) {
	if (m.getParams().size() < 2) {
		sendReply(c, 461, "PRIVMSG :Not enough parameters");
		return;
	}
	const std::string &target = m.getParams()[0];
	const std::string &message = m.getParams()[1];
	Client *targetClient = findClientByNick(target);
	if (targetClient != NULL) {
		std::string privmsg = ":" + c.getPrefix() + " PRIVMSG " + target + " :" + message + "\r\n";
		targetClient->queueMessage(privmsg);
	} else {
		Channel *channel = findChannel(target);
		if (channel != NULL && channel->isMember(&c)) {
			std::string privmsg = ":" + c.getPrefix() + " PRIVMSG " + target + " :" + message + "\r\n";
			channel->broadcast(privmsg, &c);
		} else if (channel == NULL) {
			sendReply(c, 401, target + " :No such nick/channel");
		} else if (!channel->isMember(&c)) {
			sendReply(c, 404, target + " :Cannot send to channel");
		} else {
			std::string privmsg = ":" + c.getPrefix() + " PRIVMSG " + target + " :" + message + "\r\n";
			channel->broadcast(privmsg, &c);
		}
	}
}

// QUIT: disconnect
void Server::cmdQuit(Client &c, const Message &m) {
	std::string reason = "Client quit";
	if (!m.getParams().empty()) {
		reason = m.getParams()[0];
	}
	std::string quitMsg = ":" + c.getPrefix() + " QUIT :" + reason + "\r\n";
	std::map<std::string, Channel *>::iterator it = _channels.begin();
	while (it != _channels.end()) {
		Channel *channel = it->second;
		if (channel->isMember(&c)) {
			channel->broadcast(quitMsg, &c);
			channel->removeMember(&c);
			if (channel->getMemberCount() == 0) {
				delete channel;
				_channels.erase(it++);
				continue;
			}
		}
		++it;
	}
	removeClient(c.getFd());
}

// KICK: operator removes a user from a channel
void Server::cmdKick(Client &c, const Message &m) {
	if (m.getParams().size() < 2) {
		sendReply(c, 461, "KICK :Not enough parameters");
		return;
	}
	const std::string &channelName = m.getParams()[0];
	const std::string &targetNick = m.getParams()[1];
	Channel *channel = findChannel(channelName);
	if (channel == NULL || !channel->isMember(&c)) {
		sendReply(c, 442, channelName + " :You're not on that channel");
		return;
	}
	if (!channel->isOperator(&c)) {
		sendReply(c, 482, channelName + " :You're not a channel operator");
		return;
	}
	Client *targetClient = findClientByNick(targetNick);
	if (targetClient == NULL || !channel->isMember(targetClient)) {
		sendReply(c, 441, targetNick + " " + channelName + " :They aren't on that channel");
		return;
	}
	std::string kickMsg = ":" + c.getPrefix() + " KICK " + channelName + " " + targetNick + "\r\n";
	channel->broadcast(kickMsg, NULL);
	channel->removeMember(targetClient);
	if (channel->getMemberCount() == 0) {
		delete channel;
		_channels.erase(channelName);
	}
}

// INVITE: operator invites a user
void Server::cmdInvite(Client &c, const Message &m) {
	if (m.getParams().size() < 2) {
		sendReply(c, 461, "INVITE :Not enough parameters");
		return;
	}
	const std::string &targetNick = m.getParams()[0];
	const std::string &channelName = m.getParams()[1];
	Channel *channel = findChannel(channelName);
	if (channel == NULL || !channel->isMember(&c)) {
		sendReply(c, 442, channelName + " :You're not on that channel");
		return;
	}
	if (!channel->isOperator(&c)) {
		sendReply(c, 482, channelName + " :You're not a channel operator");
		return;
	}
	Client *targetClient = findClientByNick(targetNick);
	if (targetClient == NULL) {
		sendReply(c, 401, targetNick + " :No such nick/channel");
		return;
	}
	channel->invite(targetClient);
	std::string inviteMsg = ":" + c.getPrefix() + " INVITE " + targetNick + " :" + channelName + "\r\n";
	targetClient->queueMessage(inviteMsg);
	sendReply(c, 341, targetNick + " " + channelName);
}

// TOPIC: view or change the channel topic
void Server::cmdTopic(Client &c, const Message &m) {
	if (m.getParams().size() < 1) {
		sendReply(c, 461, "TOPIC :Not enough parameters");
		return;
	}
	const std::string &channelName = m.getParams()[0];
	Channel *channel = findChannel(channelName);
	if (channel == NULL || !channel->isMember(&c)) {
		sendReply(c, 442, channelName + " :You're not on that channel");
		return;
	}
	if (m.getParams().size() == 1) {
		if (channel->getTopic().empty()) {
			sendReply(c, 331, channelName + " :No topic is set");
		} else {
			std::string topic = channel->getTopic();
			sendReply(c, 332, channelName + " :" + topic);
		}
	} else {
		if (channel->isTopicRestricted() && !channel->isOperator(&c)) {
			sendReply(c, 482, channelName + " :You're not a channel operator");
		} else {
			const std::string &newTopic = m.getParams()[1];
			channel->setTopic(newTopic);
			std::string topicMsg = ":" + c.getPrefix() + " TOPIC " + channelName + " :" + newTopic + "\r\n";
			channel->broadcast(topicMsg, NULL);
		}
	}
}

// MODE: change channel modes (i, t, k, o, l)
static void recordMode(std::string &out, char &lastSign, bool adding, char mode) {
	char sign = adding ? '+' : '-';
	if (sign != lastSign) {
		out += sign;
		lastSign = sign;
	}
	out += mode;
}

void Server::cmdMode(Client &c, const Message &m) {
	if (m.getParams().size() < 1) {
		sendReply(c, 461, "MODE :Not enough parameters");
		return;
	}
	const std::string &channelName = m.getParams()[0];
	Channel *channel = findChannel(channelName);
	if (channel == NULL || !channel->isMember(&c)) {
		if (channel == NULL) {
			sendReply(c, 403, channelName + " :No such channel");
		} else {
			sendReply(c, 442, channelName + " :You're not on that channel");
		}
		return;
	}
	if (m.getParams().size() == 1) {
		std::string modes = "+";
		if (channel->isInviteOnly()) modes += "i";
		if (channel->isTopicRestricted()) modes += "t";
		if (!channel->getKey().empty()) modes += "k";
		if (channel->getUserLimit() > 0) modes += "l";
		sendReply(c, 324, channelName + " " + modes);
		return;
	}
	if (!channel->isOperator(&c)) {
		sendReply(c, 482, channelName + " :You're not a channel operator");
		return;
	}
	const std::string &modeString = m.getParams()[1];
	bool adding = true;
	size_t paramIndex = 2;
	std::string appliedModes;
	std::string appliedParams;
	char lastSign = 0;
	for (size_t i = 0; i < modeString.size(); ++i) {
		char mode = modeString[i];
		if (mode == '+') {
			adding = true;
		} else if (mode == '-') {
			adding = false;
		} else if (mode == 'i') {
			channel->setInviteOnly(adding);
			recordMode(appliedModes, lastSign, adding, 'i');
		} else if (mode == 't') {
			channel->setTopicRestricted(adding);
			recordMode(appliedModes, lastSign, adding, 't');
		} else if (mode == 'k') {
			if (adding) {
				if (paramIndex >= m.getParams().size()) {
					sendReply(c, 461, "MODE :Not enough parameters for +k");
					continue;
				}
				const std::string &key = m.getParams()[paramIndex];
				paramIndex++;
				channel->setKey(key);
				recordMode(appliedModes, lastSign, adding, 'k');
				appliedParams += " " + key;
			} else {
				channel->setKey("");
				recordMode(appliedModes, lastSign, adding, 'k');
			}
		} else if (mode == 'l') {
			if (adding) {
				if (paramIndex >= m.getParams().size()) {
					sendReply(c, 461, "MODE :Not enough parameters for +l");
					continue;
				}
				const std::string &limitStr = m.getParams()[paramIndex];
				long limit = std::strtol(limitStr.c_str(), NULL, 10);
				paramIndex++;
				if (limit <= 0) {
					sendReply(c, 461, "MODE :Invalid limit for +l");
					continue;
				}
				channel->setUserLimit(static_cast<size_t>(limit));
				recordMode(appliedModes, lastSign, adding, 'l');
				appliedParams += " " + limitStr;
			} else {
				channel->setUserLimit(0);
				recordMode(appliedModes, lastSign, adding, 'l');
			}
		} else if (mode == 'o') {
			if (paramIndex >= m.getParams().size()) {
				sendReply(c, 461, "MODE :Not enough parameters for +o/-o");
				continue;
			}
			const std::string &targetNick = m.getParams()[paramIndex];
			paramIndex++;
			Client *targetClient = findClientByNick(targetNick);
			if (targetClient == NULL) {
				sendReply(c, 401, targetNick + " :No such nick/channel");
				continue;
			}
			if (!channel->isMember(targetClient)) {
				sendReply(c, 441, targetNick + " " + channelName + " :They aren't on that channel");
				continue;
			}
			if (adding) {
				channel->addOperator(targetClient);
			} else {
				channel->removeOperator(targetClient);
			}
			recordMode(appliedModes, lastSign, adding, 'o');
			appliedParams += " " + targetNick;
		} else {
			sendReply(c, 472, std::string(1, mode) + " :is unknown mode char to me");
			continue;
		}
	}
	if (!appliedModes.empty()) {
		std::string modeMsg = ":" + c.getPrefix() + " MODE " + channelName + " " + appliedModes + appliedParams + "\r\n";
		channel->broadcast(modeMsg, NULL);
	}
}