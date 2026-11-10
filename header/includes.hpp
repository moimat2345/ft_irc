/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   includes.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mateon <mateon@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 21:28:38 by mateon            #+#    #+#             */
/*   Updated: 2026/11/10 09:14:02 by mateon           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDES_HPP
# define INCLUDES_HPP

# define BLUE   "\033[34m"
# define GREEN  "\033[32m"
# define RED    "\033[31m"
# define RESET  "\033[0m"

# define LOG_ERR(message) \
    std::cout << RED message RESET << std::endl

# define LOG_INFO(message) \
    std::cout << message << std::endl

# define LOG_CLIENT(message) \
    std::cout << GREEN "Client -> " RESET << message << std::endl

# define LOG_SERVER(message) \
    std::cout << BLUE "Server -> " RESET << message << std::endl

// ---------- C++ standard library ----------
# include <iostream>     // std::cout, std::cerr: logs and error messages
# include <string>       // std::string: all text data (nick, buffers, messages)
# include <vector>       // std::vector: list of channel members, pollfd array
# include <map>          // std::map: fd -> Client, name -> Channel
# include <set>          // std::set: channel operators and invited users
# include <algorithm>    // std::find: look for / remove an element in a container
# include <sstream>      // std::stringstream: convert numbers <-> strings (no std::to_string in C++98)

// ---------- C standard library ----------
# include <cstdlib>      // strtol / atoi (parse the port), exit
# include <cstring>      // memset, strerror: init structs and print system errors
# include <cctype>      // isdigit, isalpha, etc.
# include <cerrno>       // errno: detect EAGAIN, EADDRINUSE, EINTR...
# include <csignal>      // signal, SIGINT: stop the server cleanly with Ctrl+C

// ---------- system / network ----------
# include <sys/socket.h> // socket, setsockopt, bind, listen, accept, send, recv
# include <netinet/in.h> // sockaddr_in, htons, INADDR_ANY: IPv4 address structs
# include <arpa/inet.h>  // inet_ntoa: convert a client's IP address to text
# include <poll.h>       // poll, struct pollfd: watch many sockets at once
# include <fcntl.h>      // fcntl: set sockets to non-blocking mode
# include <unistd.h>     // close: close a socket file descriptor

//for mac os
# include <netdb.h>		 // getaddrinfo, freeaddrinfo, gai_strerror
#endif