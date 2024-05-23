/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverCommands.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mwallage <mwallage@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/12 16:49:06 by mwallage          #+#    #+#             */
/*   Updated: 2024/05/23 15:42:41 by mwallage         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

void Server::_readBuffer(size_t index, std::string & buffer)
{
	Client &client = *_clients[index - 1];
	std::string message;

	while (!(message = _getNextLine(buffer)).empty())
	{
		std::cout << "[Client " << index << "] Message received from client " << std::endl;
		std::cout << "     FD " <<_allSockets[index].fd << "< " << CYAN << message << RESET << std::endl;

		std::string command = _getCommand(message);
		std::map<std::string, CommandFunction>::iterator it = _commandMap.find(command);
		if (it != _commandMap.end())
			(this->*_commandMap[command])(client, message);
		else 
			std::cout << "Invalid command :" << message << std::endl;
	}

	std::cout << std::endl << "***list of clients***\n*" << std::endl;
	for (size_t j = 0; j < _clients.size(); j++)
	{
		std::cout << "* _clients[" << j << "].fd = " << _clients[j]->getClientSocket()->fd << "  ";
		std::cout << "_allSockets[" << j + 1 << "].fd = " << _allSockets[j + 1].fd << std::endl;;
	}
	std::cout << "*\n*********************" << std::endl << std::endl;
}

std::string Server::_getCommand(std::string & message)
{
	size_t pos = message.find(" ");
	if (pos != std::string::npos)
		return message.substr(0, pos);
	return "INVALID";
}

void Server::_handlePassCommand(Client & client, std::string & message)
{
	std::vector<std::string> params = _splitString(message, ' ');
	if (client.isPassedWord())
		client.appendResponse("462 :You may not reregister\r\n");
	else if (client.passWordAttempted())
		client.appendResponse("you only get one password try\r\n");
	else if (params.size() != 2)
		client.appendResponse("wrong amount of arguments\r\n");
	else if (params[1] != _password)
		client.appendResponse("464 :Password incorrect\r\n");
	else {
		std::cout << "[Server  ] Password accepted for " << client.getNickname() << std::endl;
		client.acceptPassword();
	}
	client.passWordAttempt();
}

void Server::_handleNickCommand(Client & client, std::string & message)
{
	std::string nickname = message.substr(5);
	if (!client.isPassedWord())
	{
		client.appendResponse("No password given as first command");
		client.passWordAttempt();
	}
	else if (nickname.empty())
		client.appendResponse(ERR_NONICKNAMEGIVEN);
	else if (_isNickInUse(nickname))
		client.appendResponse(ERR_NICKNAMEINUSE(nickname));
	else if (client.getNickname().empty())
	{
		std::cout << "[Server  ] Nickname accepted for " << nickname << std::endl;
		client.setNickname(nickname);
	}
	else
	{
		std::string oldNick = client.getNickname();
		client.setNickname(nickname);
		client.appendResponse(RPL_NICK(oldNick, nickname));
	}
}


void Server::_handleUserCommand(Client & client, std::string & message)
{
	std::vector<std::string> params = _splitString(message, ' ');
	std::string username = message.substr(5);
	if (!client.isPassedWord())
	{
		client.appendResponse("No password given as first command");
		client.passWordAttempt();
	}
	else if (params.size() < 2)
	{
		client.appendResponse(":localhost 461 " + client.getNickname() + " USER :Not enough parameters\r\n");
	}
	else
	{
		client.setUsername(params[1]);
		if (params.size() > 4)
			client.setHostname(params[3]);
		if (params.size() > 5)
			client.setRealname(concatenateTokens(params, 4));
		if (!client.isFullyAccepted()) {
			std::cout << "[Server  ] Username accepted for " << client.getNickname() << std::endl;
			client.acceptFully();
			client.appendResponse(RPL_WELCOME(client.getNickname(), client.getUsername(), "localhost"));
		}
	}
}

void Server::_handlePrivmsgCommand(Client & client, std::string & message)
{
	std::string forward = client.getNickname() + " :" + message.substr(8) + "\r\n";
	for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (*it != &client)
			(*it)->appendResponse(forward);
	}
}

void Server::_handlePingCommand(Client & client, std::string &)
{
	std::string str = "localhost";
	client.appendResponse(PONG(str));
}