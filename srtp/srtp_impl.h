/**
 * @file srtp_impl.h
 * @brief internal srtp implementation procedures
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

#ifndef _SRTP_IMPL_H_
#define _SRTP_IMPL_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @see srtp_socket(int, int, int)
 */
int _srtp_socket( int domain, int type, int protocol );

/**
 * @see srtp_listen(int, int)
 */
int _srtp_listen( int socket, int backlog );

/**
 * @see srtp_bind(int, const struct sockaddr*, socklen_t)
 */
int _srtp_bind( int socket, const struct sockaddr* address, socklen_t address_len );

/**
 * @see srtp_accept(int, struct sockaddr*, socklen_t*)
 */
int _srtp_accept( int socket, struct sockaddr* address, socklen_t* address_len );

/**
 * @see srtp_connect(int, const struct sockaddr*, socklen_t)
 */
int _srtp_connect( int socket, const struct sockaddr* address, socklen_t address_len );

/**
 * @see srtp_close(int, int)
 */
int _srtp_close(int socket, int how);

/**
 * @see srtp_debug(bool)
 */
void _srtp_debug(bool enable);

#ifdef __cplusplus
}
#endif

#endif // _SRTP_IMPL_H_
