/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mwallage <mwallage@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/12 16:49:06 by mwallage          #+#    #+#             */
/*   Updated: 2024/05/20 14:28:57 by mwallage         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"

bool	Server::_isNickInUse(std::string const & nick)
{
	for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if ((*it)->getNickname() == nick) {
			return true;
		}
	}
	return false;
}

void Server::_printBuffer(char* buff)
{
	std::cout << "|buffer| " << MAGENTA;
	for (int j = 0; buff[j] != 0; ++j) {
		if (std::isprint(buff[j])) {
			std::cout << buff[j];
		} else {
			std::cout << "\\x" << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)buff[j];
		}
	}
	std::cout << RESET << std::endl;
	//std::cout << "\n|----- end --------|\n\n" << std::endl;
	//std::cout << "|-- buffer ascii :|\n" << buff << "|----- end --------|\n" <<std::endl;
}

ssize_t	Server::_fillBuffer(size_t index, std::string & buffer)
{
	char temp[BUFFER_SIZE];
	static int counter;

	counter++;
	std::cout << "Call number " << counter << " to recv\n";
	ssize_t bytesRead = recv(_allSockets[index].fd, temp, BUFFER_SIZE, 0);
	_printBuffer(temp);
	if (bytesRead > 0)
		buffer.append(temp);
	return bytesRead;
}

std::string Server::_getNextLine(size_t & index, std::string & buffer)
{
	std::size_t pos;
	if ((pos = buffer.find("\r\n")) == std::string::npos)
		return "";
	(void)index; // to remove later is not needed
	std::string line = buffer.substr(0, pos);
	buffer.erase(0, pos + 2); // Remove the line including \r\n
	return line;
}