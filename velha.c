/**
*   Copyright (C) 2007 by Marcelo Jorge Vieira (metal) <metal@alucinados.com>
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the
*  Free Software Foundation, Inc.,
*  59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
* Public License can be found at http://www.gnu.org/copyleft/gpl.html
*
* @author Thadeu Cascardo <cascardo@minaslivre.org>
* @author Marcelo Jorge Vieira (metal) <metal@alucinados.com>
*
*/

#include <glib.h>
#include "velha.h"

gboolean myread (GIOChannel *, GIOCondition, gpointer);

struct _client_t
{
	char *table;
	gint game;
	char turn;
	GString *buffer;
	client_t *partner;
	GIOChannel *channel;
	guint watch;
};

GList *clients = NULL;

struct
{
	char *name;
	void (*func) (char*, int r, client_t*);
}
commands[] =
{
	{ "play", play_cb },
	{ "start", start_cb },
	{ "join", join_cb },
	{ "list", list_cb },
	{ "quit", quit_cb },
	{ NULL, NULL }
};


client_t *
client_new (int fd)
{
	client_t *client;

	client = g_slice_new (client_t);
	client->table = NULL;
	client->game = -1;
	client->buffer = g_string_sized_new (BUFSIZ);
	client->partner = NULL;
	client->channel = g_io_channel_unix_new (fd);
	g_io_channel_set_close_on_unref (client->channel, TRUE);
	g_io_channel_set_flags (client->channel, g_io_channel_get_flags (client->channel) | G_IO_FLAG_NONBLOCK, NULL);
	client->watch = g_io_add_watch (client->channel, G_IO_IN, myread, client);
	return client;
}

void client_destroy (client_t *client)
{
	clients = g_list_remove (clients, client);
	if (client->table)
		g_slice_free1 (9, client->table);
	if (client->buffer)
		g_string_free (client->buffer, TRUE);
	if (client->channel)
		g_io_channel_unref (client->channel);
	if (client->watch)
		g_source_remove (client->watch);
	g_slice_free (client_t, client);
}

void
client_write (client_t *client, GString *str)
{
	int fd;

	fd = g_io_channel_unix_get_fd (client->channel);
	write (fd, str->str, str->len);
}

void
cancel_game (client_t *client)
{
	clients = g_list_remove (clients, client);

	if (client->partner)
	{
		GString *str;;
		str = g_string_new ("CANCEL\r\n");
		client_write (client->partner, str);
		g_string_free (str, TRUE);
		client_destroy (client->partner);
		client->partner = NULL;
		client->table = NULL;
	}

	if (client->table)
	{
		g_free (client->table);
		client->table = NULL;
	}
	client->game = -1;
}

gint
client_compare (client_t * a, client_t * b)
{
	return a->game - b->game;
}

gboolean
new_game (client_t *client)
{
	GList *l;
	GList *last;
	client_t *other;
	gint prev = -1;

	if (clients == NULL)
	{
		client->game = 0;
		clients = g_list_prepend (clients, client);
		return TRUE;
	}

	for (l = g_list_first (clients); l != NULL; l = g_list_next (l))
	{
		other = l->data;
		if (other->game > prev + 1)
		{
			client->game = prev + 1;
			clients = g_list_insert_sorted (l, client, (GCompareFunc) client_compare);
			return TRUE;
		}
		prev = other->game;
		last = l;
	}

	if (l == NULL)
	{
		other = last->data;
		if (other->game == G_MAXINT)
			return FALSE;
		client->game = other->game + 1;
		clients = g_list_insert_sorted (last, client, (GCompareFunc) client_compare);
		return TRUE;
	}
	return FALSE;
}

void
start_cb (char *buffer, int len, client_t *client)
{
	if (client->game != -1)
	{
		cancel_game (client);
	}

	if (new_game (client) == FALSE)
	{
		cancel_game (client);
		client_destroy (client);
	}
	else
	{
		GString *str;
		str = g_string_sized_new (BUFSIZ);
		g_string_printf (str, "%d\r\n", client->game);
		client_write (client, str);
		g_string_free (str, TRUE);
	}
}

void
quit_cb (char *buffer, int len, client_t *client)
{
	cancel_game (client);
	client_destroy (client);
}

void
win_game (client_t *client)
{
	GString *str;

	str = g_string_new ("200 WIN\r\n");
	client_write (client, str);
	str = g_string_assign (str, "200 LOOSE\r\n");
	client_write (client->partner, str);
	g_string_free (str, TRUE);
	cancel_game (client);
	client_destroy (client);
}

void tie_game (client_t *client)
{
	GString *str;

	str = g_string_new ("200 TIE\r\n");
	client_write (client, str);
	client_write (client->partner, str);
	g_string_free (str, TRUE);
	cancel_game (client);
	client_destroy (client);
}

void
check_game (client_t *client, int pos)
{
	int line;
	int column;

	line = pos % 3;
	column = pos / 3;

	if ((client->table[INDEX(column, 0)] == client->table[INDEX(column, 1)] &&
			client->table[INDEX(column, 0)] == client->table[INDEX(column, 2)]) ||
		(client->table[INDEX(0, line)] == client->table[INDEX(1, line)] &&
			client->table[INDEX(0, line)] == client->table[INDEX(2, line)]) ||
		((pos == 4 || pos % 4 == 2) &&
			client->table[INDEX(0,2)] == client->table[INDEX(1,1)] &&
			client->table[INDEX(0,2)] == client->table[INDEX(2,0)]) ||
		((pos == 4 || pos % 4 == 0) &&
			client->table[INDEX(2,2)] == client->table[INDEX(1,1)] &&
			client->table[INDEX(2,2)] == client->table[INDEX(0,0)]))
	{
		win_game (client);
	}
	else if (memchr (client->table, 0, 9) == NULL)
	{
		tie_game (client);
	}
}

void
play_cb (char *buffer, int len, client_t *client)
{
	char *p;
	char *end;
	int pos;
	GString *str;

	if (client->partner == NULL)
	{
		GString *str;
		str = g_string_new ("400 Wait for your partner\r\n");
		client_write (client, str);
		g_string_free (str, TRUE);
		return;
	}

	if (client->turn == 0)
	{
		GString *str;
		str = g_string_new ("400 It is not your turn! Duh!\r\n");
		client_write (client, str);
		g_string_free (str, TRUE);
		return;
	}
	buffer[len - 1] = 0;
	errno = 0;
	p = buffer + sizeof ("play");
	pos = strtol (p, &end, 0);

	if (end == p || errno != 0 || pos < 0 || pos > 8)
	{
		GString *str;
		str = g_string_new ("400 Invalid message format\r\n");
		client_write (client, str);
		g_string_free (str, TRUE);
		return;
	}

	if (client->table[pos] != 0)
	{
		GString *str;
		str = g_string_new ("400 Already played. Choose another.\r\n");
		client_write (client, str);
		g_string_free (str, TRUE);
		return;
	}

	client->table[pos] = client->turn;
	client->partner->turn = client->turn == 1 ? 2 : 1;
	client->turn = 0;
	str = g_string_sized_new (BUFSIZ);
	g_string_printf (str, "PLAY %d\r\n", pos);
	client_write (client->partner, str);
	g_string_printf (str, "200 OK\r\n");
	client_write (client, str);
	g_string_free (str, TRUE);
	check_game (client, pos);
}

void
join_cb (char *buffer, int len, client_t *client)
{
	char *p;
	char *end;
	gint game;
	GList *l;
	client_t *other;
	GString *str;

	if (client->game)
		cancel_game (client);

	buffer[len - 1] = 0;
	errno = 0;
	p = buffer + sizeof ("join");
	game = strtol (p, &end, 0);

	if (end == p || errno != 0 || game < 0 || game > G_MAXINT)
	{
		str = g_string_new ("400 Invalid message format\r\n");
		client_write (client, str);
		g_string_free (str, TRUE);
		return;
	}

	client->game = game;
	l = g_list_find_custom (clients, client, (GCompareFunc) client_compare);

	if (l == NULL)
	{
		str = g_string_new ("404 Game not found\r\n");
		client_write (client, str);
		g_string_free (str, TRUE);
		return;
	}

	other = l->data;

	if (other->partner)
	{
		str = g_string_new ("403 Game already started! Go away!\r\n");
		client_write (client, str);
		g_string_free (str, TRUE);
		return;
	}

	other->partner = client;
	client->partner = other;
	client->table = other->table = g_slice_alloc0 (9);
	client->turn = 1;
	other->turn = 0;
	str = g_string_new ("200 OK\r\n");
	client_write (client, str);
	g_string_free (str, TRUE);
}

void
list_cb (char *buffer, int len, client_t* client)
{
	GString *str;
	GList *l;
	client_t *other;

	str = g_string_sized_new (BUFSIZ);
	g_string_append (str, "START\r\n");

	for (l = g_list_first (clients); l != NULL; l = g_list_next (l))
	{
		other = l->data;
		g_string_append_printf (str, "%d\r\n", other->game);
	}

	g_string_append (str, "END\r\n");
	client_write (client, str);
	g_string_free (str, TRUE);
}

gboolean
myboo (client_t *client)
{
	int n;
	char *p;
	char *buffer;
	int i;

	for (n = 0, p = client->buffer->str; n < client->buffer->len; n++, p++)
	{
		if (*p == '\n' || (*p == '\r' && ++n < client->buffer->len && *(++p) == '\n'))
			break;
	}

	if (n == client->buffer->len)
	{
		//g_debug ("Time to learn how to count, cascardo.");
		return FALSE;
	}

	buffer = g_strndup (client->buffer->str, n + 1);
	g_string_erase (client->buffer, 0, n + 1);
	g_debug ("Command %s received", buffer);

	for (i = 0; commands[i].name != NULL; i++)
	{
		//g_debug ("%d %s", i, commands[i].name);
		if (g_ascii_strncasecmp (commands[i].name, buffer, strlen (commands[i].name)) == 0)
		{
			//g_debug ("%s running", commands[i].name);
			commands[i].func (buffer, n + 1, client);
			break;
		}
	}

	g_free (buffer);
	return TRUE;
}

gboolean
myread (GIOChannel *channel, GIOCondition cond, gpointer data)
{
	client_t *client;
	int fd;
	char buffer[BUFSIZ];
	int r;

	//g_debug ("New data");
	client = data;
	fd = g_io_channel_unix_get_fd (channel);
	r = read (fd, buffer, BUFSIZ);

	if ((r < 0 && errno != EAGAIN) || r == 0)
	{
		g_debug ("Error or finished connection.");
		cancel_game (client);
		client_destroy (client);
		return FALSE;
	}

	g_string_append_len (client->buffer, buffer, r);

	while (myboo (client));

	return TRUE;
}

gboolean
myaccept (GIOChannel *channel, GIOCondition cond, gpointer data)
{
	int fd;
	int newfd;
	client_t *client;

	fd = g_io_channel_unix_get_fd (channel);
	newfd = accept (fd, NULL, 0);
	client = client_new (newfd);

	return TRUE;
}

int main (int argc, char **argv)
{
	int fd;
	GIOChannel *channel;
	struct sockaddr_in saddr;
	int reuseaddr ;

	fd = socket (PF_INET, SOCK_STREAM, 0);
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons (5555);
	saddr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
	reuseaddr = 1;
	setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof (reuseaddr));
	bind (fd, (struct sockaddr*) &saddr, sizeof (struct sockaddr_in));
	listen (fd, 5);
	channel = g_io_channel_unix_new (fd);
	g_io_add_watch (channel, G_IO_IN, myaccept, NULL);
	g_main_loop_run (g_main_loop_new (g_main_context_default (), TRUE));

	return 0;
}
