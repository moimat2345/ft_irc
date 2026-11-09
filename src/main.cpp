/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mateon <mateon@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 21:39:18 by mateon            #+#    #+#             */
/*   Updated: 2026/11/02 22:45:54 by mateon           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../header/serveur.hpp"

static bool isValidPort(const std::string &s)
{
	if (s.empty() || s.length() > 5)
		return (0);
	for (size_t i = 0; i < s.length(); i++)
		if (!std::isdigit(static_cast<unsigned char>(s[i])))
			return (0);
	int port = std::atoi(s.c_str());
	return (port >= 1 && port <= 65535);
}

static bool isValidPassword(const std::string &s)
{
	if (s.empty())
		return (0);
	for (size_t i = 0; i < s.length(); i++)
		if (std::isspace(static_cast<unsigned char>(s[i])))
			return (0);
	return (1);
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return (1);
	}
	if (!isValidPort(argv[1])) {
		std::cerr << "Error: port must be a number between 1 and 65535." << std::endl;
		return (1);
	}
	if (!isValidPassword(argv[2])) {
		std::cerr << "Error: password must be non-empty and contain no spaces." << std::endl;
		return (1);
	}
	try {
		Server server(std::atoi(argv[1] ), argv[2]);
		server.run();
	} catch (const std::exception &e) {
		std::cerr << "Fatal error: " << e.what() << std::endl;
		return (1);
	}
	return (0);
}