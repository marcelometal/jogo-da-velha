/**
 *   Copyright (C) 2007 by Marcelo Jorge Vieira (metal) <metal@alucinados.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Public License can be found at http://www.gnu.org/copyleft/gpl.html
 *
 *
 * @author Thadeu Cascardo <cascardo@minaslivre.org>
 * @author Marcelo Jorge Vieira (metal) <metal@alucinados.com>
 *
 */

#ifndef VELHA_H
#define VELHA_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define INDEX(x,y) (3 * (x) + (y))

#ifdef BUFSIZ
#undef BUFSIZ
#endif

#define BUFSIZ 4096

typedef struct _client_t client_t;

void join_cb (char *buffer, int len, client_t *client);
void play_cb (char *buffer, int len, client_t *client);
void list_cb (char *buffer, int len, client_t* client);
void quit_cb (char *buffer, int len, client_t *client);
void start_cb (char *buffer, int len, client_t *client);

#endif
