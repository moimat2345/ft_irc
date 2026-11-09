/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   message.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mateon <mateon@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 21:28:51 by mateon            #+#    #+#             */
/*   Updated: 2026/11/02 21:29:56 by mateon           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include "includes.hpp"

class Message {
private:
	std::string                 _command;   // command name ("PASS", "JOIN"...)
	std::vector<std::string>    _params;    // parameters; the last one may contain spaces (after ':')
public:
    Message();                                      // default constructor
    Message(const Message &other);                  // copy constructor
    Message &operator=(const Message &other);       // assignment operator
    ~Message();                                     // destructor

    void                            parse(const std::string &line);     // split a raw line into command + params
    const std::string               &getCommand() const;                // returns the command, e.g. "JOIN"
    const std::vector<std::string>  &getParams() const;                 // returns the list of parameters
};

#endif