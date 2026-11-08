/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mateon <mateon@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 21:33:18 by mateon            #+#    #+#             */
/*   Updated: 2026/11/08 21:27:38 by mateon           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "includes.hpp"
#include "client.hpp"

class Channel {

private:
    std::string             _name;              // channel name, starts with '#'
    std::string             _topic;             // channel topic
    std::vector<Client *>   _members;           // users inside (pointers, Server owns them)
    std::set<Client *>      _operators;         // users with operator rights
    std::set<Client *>      _invited;           // users allowed in while +i is on
    bool                    _inviteOnly;        // mode +i
    bool                    _topicRestricted;   // mode +t
    std::string             _key;               // mode +k, channel password
    size_t                  _userLimit;         // mode +l, max users (0 = unlimited)

public:
    // --- canonical form ---
    Channel(const std::string &name);               // create a channel with its name
    Channel(const Channel &other);                  // copy constructor
    Channel &operator=(const Channel &other);       // assignment operator
    ~Channel();                                     // destructor

    // --- identity & topic ---
    const std::string   &getName() const;           // channel name, e.g. "#general"
    const std::string   &getTopic() const;          // current topic
    void                setTopic(const std::string &topic);     // change the topic

    // --- members ---
    void    addMember(Client *client);              // add a user to the channel
    void    removeMember(Client *client);           // remove a user (also from operators/invited)
    bool    isMember(Client *client) const;         // is this user in the channel?
    size_t  getMemberCount() const;                 // number of users inside

    // --- operators ---
    void    addOperator(Client *client);            // give operator rights
    void    removeOperator(Client *client);         // take operator rights away
    bool    isOperator(Client *client) const;       // does this user have operator rights?

    // --- invites ---
    void    invite(Client *client);                 // allow a user to join an invite-only channel
    bool    isInvited(Client *client) const;        // has this user been invited?

    // --- messaging ---
    void    broadcast(const std::string &msg, Client *exclude = NULL);  // send a message to all members, except one

    // --- modes: i, t, k, l ---
    void    setInviteOnly(bool b);                  // +i / -i
    bool    isInviteOnly() const;                   // is the channel invite-only?
    void    setTopicRestricted(bool b);             // +t / -t
    bool    isTopicRestricted() const;              // is the topic reserved to operators?
    void    setKey(const std::string &key);         // +k / -k (empty string = no password)
    const std::string &getKey() const;              // channel password
    void    setUserLimit(size_t limit);             // +l / -l (0 = no limit)
    size_t  getUserLimit() const;                   // max number of users

};

#endif
