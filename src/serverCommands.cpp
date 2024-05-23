/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverCommands.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mwallage <mwallage@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/12 16:49:06 by mwallage          #+#    #+#             */
/*   Updated: 2024/05/22 15:45:13 by mwallage         ###   ########.fr       */
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

		enum Commands commandCase = _getCommand(message);
		switch (commandCase) {
			case PASS:
				_handlePassCommand(client, message);
				break;
			case NICK:
				_handleNickCommand(client, message);
				break;
			case USER:
				_handleUserCommand(client, message);
				break;
			case PRIVMSG:
				_handlePrivmsgCommand(client, message);
				break;
			case PING:
				_handlePongCommand(client);
				break;
			case JOIN:
				_handleJoinCommand(client, message);
				break;
			case QUIT:
				_handleQuitCommand(client, message, index);
				break;
			case INVALID:
				std::cout << "Invalid command :" << message << std::endl;
		}
	}

	std::cout << std::endl << "***list of clients***\n*" << std::endl;
	for (size_t j = 0; j < _clients.size(); j++)
	{
		std::cout << "* _clients[" << j << "].fd = " << _clients[j]->getClientSocket()->fd << "  ";
		std::cout << "_allSockets[" << j + 1 << "].fd = " << _allSockets[j + 1].fd << std::endl;;
	}
	std::cout << "*\n*********************" << std::endl << std::endl;
}

Commands Server::_getCommand(std::string & message)
{
	if (message.find("PASS") == 0)
		return PASS;
	else if (message.find("NICK") == 0)
		return NICK;
	else if (message.find("USER") == 0)
		return USER;
	else if (message.find("PRIVMSG") == 0)
		return PRIVMSG;
	else if (message.find("PING") == 0)
		return PING;
	else if (message.find("JOIN") == 0)
		return JOIN;
	else if (message.find("QUIT") == 0)
		return QUIT;
	return INVALID;
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

void Server::_handlePongCommand(Client & client)
{
	std::string str = "localhost";
	client.appendResponse(PONG(str));
}

void Server::_handleQuitCommand(Client & client, std::string & message, size_t index)
{
	for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (*it != &client)
		{
			std::vector<int>::const_iterator ContactFDListBegin = (*it)->getPrvtmsgContactFDList().begin();
			std::vector<int>::const_iterator ContactFDListEnd = (*it)->getPrvtmsgContactFDList().begin();
			if (std::find(ContactFDListBegin, ContactFDListEnd, (*it)->getClientSocket()->fd) != ContactFDListEnd)
				(*it)->appendResponse(QUIT_REASON(client.getNickname(), client.getUsername(), client.getHostname(), message.substr(6)));
			//else if ( both client are part of the same channel)
			// {
			//	Channel Notifications: Users in the same channels as the quitting user receive a notification. 
			// 	Channel Member List: The user's name will be removed from the list of channel members.
			// 	Potential Channel Events: Depending on the server configuration and channel settings, 
			// 	additional events might be triggered. For example, if the user held special privileges
			// 	(like being an operator), the server might handle the redistribution of those privileges.
			// }
		}
	}
	_delClient(index);
}
