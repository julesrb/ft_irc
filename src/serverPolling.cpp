/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverPolling.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mwallage <mwallage@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/18 16:35:02 by mwallage          #+#    #+#             */
/*   Updated: 2024/05/20 14:18:39 by mwallage         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"

void Server::startPolling()
{
	while (g_quit == false)
	{
		int numEvents = poll(_allSockets.data(), _allSockets.size(), -1);
		if (numEvents < 0)
			break;
		if (numEvents == 0)
			continue;
		if (_allSockets[0].revents & POLLIN)
			_acceptNewClient();
		_checkClients();
	}
}

void Server::_acceptNewClient()
{
	sockaddr_in clientAddress;
	socklen_t clientAddressSize = sizeof(clientAddress);
	int clientFd = accept(_allSockets[0].fd, reinterpret_cast<sockaddr *>(&clientAddress), &clientAddressSize);
	if (clientFd == -1)
	{
		std::cerr << color::RED << "failed to accept new connection" << color::RESET << std::endl;
		return;
	}

	std::cout << color::GREEN <<"Accepting new client " << clientFd << color::RESET << std::endl;
	fcntl(clientFd, F_SETFL, O_NONBLOCK);

	pollfd* clientSocket = new pollfd;
	clientSocket->fd = clientFd;
	clientSocket->events = POLLIN | POLLOUT;
	clientSocket->revents = 0;
	_allSockets.push_back(*clientSocket);

	Client* newClient = new Client(clientSocket);
	_clients.push_back(newClient);
}

void Server::_checkClients()
{
	for (size_t i = _allSockets.size() - 1; i > 0; i--)
	{
		if (_allSockets[i].revents & POLLIN)
		{
			std::string buffer;
 			if (_fillBuffer(i, buffer) > 0)
				_readBuffer(i, buffer);
			else
				_delClient(i);
		}
		std::string const & response = _clients[i - 1]->getResponse();
		if (!response.empty() && _allSockets[i].revents & POLLOUT) {
			send(_allSockets[i].fd, response.c_str(), response.size(), 0);
			_clients[i - 1]->clearResponse();
		}
	}
}

void Server::_delClient(size_t index)
{
	std::cerr << color::RED << "[Server] Client fd " << _allSockets[index].fd << " just disconnected" << color::RESET << std::endl;
	_allSockets.erase(_allSockets.begin() + index);
	delete _clients[index - 1];
	_clients.erase(_clients.begin() + index - 1);
}

