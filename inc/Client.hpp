/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mwallage <mwallage@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/12 17:26:30 by mwallage          #+#    #+#             */
/*   Updated: 2024/05/12 17:55:27 by mwallage         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <string>
#include <poll.h>

class Client {
private:
	pollfd		_clientPoll;
	std::string _nickname;
	std::string _username;
public:
	Client(int socket);
	Client(Client const &);
	Client & operator=(Client const &);
	~Client();

	void				setClientPoll(pollfd const &);
	pollfd const & 		getClientPoll() const;
	std::string const &	getNickname() const;
	void				setNickname(std::string const &);
	std::string const & getUsername() const;
	void				setUsername(std::string const &);
};
