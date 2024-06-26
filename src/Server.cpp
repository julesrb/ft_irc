#include "Server.hpp"

Server::Server(int port, std::string password, Config const &config)
	: _config(config),
	  _serverName(_config.get("Server", "Name")),
	  _port(port),
	  _password(password)
{
	pollfd serverSocket;
	serverSocket.fd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket.fd == -1)
		throw SocketCreationException();
	fcntl(serverSocket.fd, F_SETFL, O_NONBLOCK);

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	if (_port < 1500)
		throw std::runtime_error("invalid port number");
	serverAddress.sin_port = htons(_port);

	if (bind(serverSocket.fd, reinterpret_cast<sockaddr *>(&serverAddress),
			 sizeof(serverAddress)) == -1)
	{
		close(serverSocket.fd);
		throw SocketBindingException();
	}

	if (listen(serverSocket.fd, 10) == -1)
	{
		close(serverSocket.fd);
		throw SocketListeningException();
	}

	_initCommandMap();

	serverSocket.events = POLLIN;
	serverSocket.revents = 0;
	_allSockets.push_back(serverSocket);

	std::stringstream ss(_config.get("Server", "MaxClients"));
	if (!(ss >> _maxUsers))
		_maxUsers = 0;
	std::cout << YELLOW << "Server listening on port " << port << "\n"
			  << RESET << std::endl;
	std::cout << YELLOW << "Max users: " << _maxUsers << RESET << std::endl;
}

void Server::_initCommandMap()
{
	_commandMap["PASS"] = &Server::_handlePassCommand;
	_commandMap["NICK"] = &Server::_handleNickCommand;
	_commandMap["USER"] = &Server::_handleUserCommand;
	_commandMap["PRIVMSG"] = &Server::_handlePrivmsgCommand;
	_commandMap["PING"] = &Server::_handlePingCommand;
	_commandMap["JOIN"] = &Server::_handleJoinCommand;
	_commandMap["MODE"] = &Server::_handleModeCommand;
	_commandMap["QUIT"] = &Server::_handleQuitCommand;
	_commandMap["TOPIC"] = &Server::_handleTopicCommand;
	_commandMap["CAP"] = &Server::_handleCapCommand;
	_commandMap["WHO"] = &Server::_handleWhoCommand;
	_commandMap["INVITE"] = &Server::_handleInviteCommand;
	_commandMap["KICK"] = &Server::_handleKickCommand;
	_commandMap["PART"] = &Server::_handlePartCommand;
	_commandMap["INVALID"] = &Server::_handleInvalidCommand;
	_commandMap["MOTD"] = &Server::_handleMotdCommand;
	_commandMap["motd"] = &Server::_handleMotdCommand; // irssi sends motd in lowercase
}

Server::Server(Server const &other)
	: _clients(other._clients), _allSockets(other._allSockets), _config(other._config), _port(other._port), _password(other._password)
{
}

Server &Server::operator=(Server const &other)
{
	if (this != &other)
	{
		_clients = other._clients;
		_allSockets = other._allSockets;
		_config = other._config;
	}
	return *this;
}

Server::~Server()
{
	std::cout << "Closing server..." << std::endl;
	close(_allSockets[0].fd);
	for (size_t i = 1; i < _allSockets.size(); i++)
	{
		close(_allSockets[i].fd);
		delete _clients[i - 1];
	}
	for (std::vector<Channel *>::iterator it = _channelList.begin(); it != _channelList.end(); ++it)
	{
		delete *it;
	}
	_channelList.clear();
	_clients.clear();
}
