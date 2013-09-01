/**
 * @file srtp.h
 * @brief external SRTP interface
 * @author Sam Macpherson
 * @author Chris Ponticello
 *
 * Copyright 2013  Sam Macpherson, Chris Ponticello
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _SRTP_H_
#define _SRTP_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief create an special SRTP socket
 *
 * @param domain not used
 * @param type not used
 * @param protocol not used
 *
 * @return a special socket file descriptor
 */
int srtp_socket( int domain, int type, int protocol );

/**
 * @brief listen for inbound connections on the SRTP socket \em socket
 *
 * @param socket a special SRTP socket file descriptor
 * @param backlog not used
 *
 * @return 0 on success, -1 otherwise
 */
int srtp_listen( int socket, int backlog );

/**
 * @brief binds an address to the special SRTP socket
 *
 * @param socket the special SRTP socket
 * @param address the address structure
 * @param address_len the length of the address structure
 *
 * @return 0 on success, -1 otherwise
 */
int srtp_bind( int socket, const struct sockaddr* address, socklen_t address_len );

/**
 * @brief accepts an incoming connection, this function will block indefinitely
 * until a new connection is established or the socket your listening on is
 * closed
 *
 * @param socket a SRTP socket
 * @param address the source address of the connection
 * @param address_len the source address structure length
 *
 * @return a SRTP socket file descriptor, -1 otherwise
 */
int srtp_accept( int socket, struct sockaddr* address, socklen_t* address_len );

/**
 * @brief attempts to connect to an SRTP server
 *
 * @param socket a SRTP socket file descriptor
 * @param address the destination address
 * @param address_len the length of the destination address structure
 *
 * @return 0 on success, -1 otherwise
 */
int srtp_connect( int socket, const struct sockaddr* address, socklen_t address_len );

/**
 * @brief closes a SRTP socket
 *
 * @param socket the SRTP socket to be closed
 * @param how an enumerated constant describing the reason for the shutdown
 *
 * @return 0 on success, -1 otherwise
 */
int srtp_close( int socket, int how );

#ifdef __cplusplus
}
#endif

#endif // _SRTP_H_
