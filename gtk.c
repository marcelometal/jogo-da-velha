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

#include <gtk/gtk.h>
#include "velha.h"

GdkPixbuf *pixbuf_O;
GdkPixbuf *pixbuf_X;
GtkWidget *statusbar;

GtkListStore* store;

GString *strbuffer;
GIOChannel *channel;

gboolean started;
gboolean played;
gboolean listed;

gint game;
gint lastpos;

GtkWidget *velha[9];

void
button_pressed (GtkButton *button, gpointer data)
{
	gint position;
	char *str;
	GString *str2;

	position = GPOINTER_TO_INT (data);
	str = g_strdup_printf ("Pressed button %d", position);
	gtk_statusbar_push (GTK_STATUSBAR (statusbar),
	gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), str), str);
	g_free (str);
	str2 = g_string_sized_new (BUFSIZ);
	g_string_printf (str2, "PLAY %d\r\n", position);
	write (g_io_channel_unix_get_fd (channel), str2->str, str2->len);
	g_string_free (str2, TRUE);
	lastpos = position;
	played = 1;
}

void
start_clicked (GtkButton *button, gpointer data)
{
	write (g_io_channel_unix_get_fd (channel), "START\r\n", 7);
	started = 1;
}

void
join_clicked (GtkButton *button, gpointer data)
{
	GtkTreeIter iter;
	GString *str;

	if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (data), &iter))
		return;
	gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (data)), &iter, 1, &game, -1);
	str = g_string_sized_new (BUFSIZ);
	g_string_printf (str, "JOIN %d\r\n", game);
	write (g_io_channel_unix_get_fd (channel), str->str, str->len);
	g_string_free (str, TRUE);
}

GtkWidget *
my_combo_new ()
{
	GtkWidget *combobox;
	GtkCellRenderer* renderer;

	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
	combobox = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), renderer, "text", 0, NULL);

	return combobox;

}

void
ui_init ()
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *entry;

	int i;
	int j;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "velha");
	gtk_container_set_border_width (GTK_CONTAINER (window), 10);
	vbox = gtk_vbox_new (FALSE, 5);
	gtk_container_add (GTK_CONTAINER (window), vbox);
	pixbuf_O = gdk_pixbuf_new_from_file_at_size ("velha_o.png", 100, 100, NULL);
	pixbuf_X = gdk_pixbuf_new_from_file_at_size ("velha_x.png", 100, 100, NULL);

	hbox = gtk_hbox_new (TRUE, 5);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);

	button = gtk_button_new_with_label ("Start new game");
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 5);
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (start_clicked), NULL);

	entry = my_combo_new ();
	gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);

	button = gtk_button_new_with_label ("Join existing game");
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (join_clicked), entry);

	for (i = 0; i < 3; i++)
	{
		hbox = gtk_hbox_new (TRUE, 5);
		gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 5);
		for (j = 0; j < 3; j++)
		{
			velha[INDEX(i,j)] = gtk_button_new ();
			gtk_container_add (GTK_CONTAINER (hbox), velha[INDEX(i,j)]);
			gtk_widget_set_usize (velha[INDEX(i,j)], 120, 120);
			g_signal_connect (G_OBJECT (velha[INDEX(i,j)]), "clicked", G_CALLBACK (button_pressed), GINT_TO_POINTER (INDEX(i,j)));
		}
	}

	statusbar = gtk_statusbar_new ();
	gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);
	gtk_widget_show_all (window);
}

void
myfar (char *buffer, int len)
{
	if (started)
	{
		/* Waits for game number */
		char *end;
		char *str;

		buffer[len - 1] = 0;
		errno = 0;
		game = strtol (buffer, &end, 0);
		if (end == buffer || errno != 0 || game < 0 || game > G_MAXINT)
		{
			str = g_strdup_printf ("Error starting game");
			gtk_statusbar_push (GTK_STATUSBAR (statusbar),
			gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), str), str);
			g_free (str);
			return;
		}
		str = g_strdup_printf ("Started game %d", game);
		gtk_statusbar_push (GTK_STATUSBAR (statusbar),
		gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), str), str);
		g_free (str);
		started = 0;
	}
	else if (played)
	{
		char *end;
		char *str;
		int status;

		buffer[len - 1] = 0;
		errno = 0;
		status = strtol (buffer, &end, 0);
		if (end == buffer || errno != 0 || status < 0 || status > G_MAXINT)
		{
			str = g_strdup_printf ("Error: broken server");
			gtk_statusbar_push (GTK_STATUSBAR (statusbar),
			gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), str), str);
			g_free (str);
			return;
		}
		if (status != 200)
		{
			str = g_strdup_printf ("Error playing: %s", end);
			gtk_statusbar_push (GTK_STATUSBAR (statusbar),
			gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), str), str);
			g_free (str);
		}
		else
		{
			GtkWidget *image;
			str = g_strdup_printf ("Played position: %d", lastpos);
			gtk_statusbar_push (GTK_STATUSBAR (statusbar),
			gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), str), str);
			g_free (str);
			image = gtk_image_new_from_pixbuf (pixbuf_O);
			gtk_button_set_image (GTK_BUTTON (velha[lastpos]), image);
		}
		played = 0;
	}
	else if (g_ascii_strncasecmp (buffer, "PLAY", 4) == 0)
	{
		/* Opponent has played */
		char *p;
		char *end;
		char *str;
		int position;

		buffer[len - 1] = 0;
		p = buffer + sizeof ("PLAY");
		errno = 0;
		position = strtol (p, &end, 0);
		if (end == p || errno != 0 || position < 0 || position > 8)
		{
			str = g_strdup_printf ("Error: broken server");
			gtk_statusbar_push (GTK_STATUSBAR (statusbar),
			gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), str), str);
			g_free (str);
			return;
		}
		else
		{
			GtkWidget *image;

			str = g_strdup_printf ("Opponent played position: %d", position);
			gtk_statusbar_push (GTK_STATUSBAR (statusbar),
			gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), str), str);
			g_free (str);
			image = gtk_image_new_from_pixbuf (pixbuf_X);
			gtk_button_set_image (GTK_BUTTON (velha[position]), image);
		}
	}
	else if (listed && g_ascii_strncasecmp (buffer, "END", 3) == 0)
	{
		listed = 0;
	}
	else if (listed && g_ascii_strncasecmp (buffer, "START", 5) == 0)
	{
		/* Who is responsible for this stupid protocol? Do nothing! */
	}
	else if (listed)
	{
		char *end;
		char *str;
		gint mygame;

		buffer[len - 1] = 0;
		errno = 0;
		mygame = strtol (buffer, &end, 0);
		if (end == buffer || errno != 0 || mygame < 0 || mygame > G_MAXINT)
		{
			str = g_strdup_printf ("Error: broken server");
			gtk_statusbar_push (GTK_STATUSBAR (statusbar),
			gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), str), str);
			g_free (str);
			return;
		}
		else
		{
			GtkTreeIter iter;

			str = g_strdup_printf ("Seen game: %d", mygame);
			gtk_statusbar_push (GTK_STATUSBAR (statusbar),
			gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), str), str);
			g_free (str);
			str = g_strdup_printf ("%d", mygame);
			gtk_list_store_prepend (store, &iter);
			gtk_list_store_set (store, &iter, 0, str, 1, mygame, -1);
		}
	}
	else if (g_ascii_strncasecmp (buffer, "200 WIN", 7) == 0)
	{
		char *str;

		str = g_strdup_printf ("You win!");
		gtk_statusbar_push (GTK_STATUSBAR (statusbar),
		gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), str), str);
		g_free (str);
	}
	else if (g_ascii_strncasecmp (buffer, "200 LOOSE", 9) == 0)
	{
		char *str;

		str = g_strdup_printf ("You loose!");
		gtk_statusbar_push (GTK_STATUSBAR (statusbar),
		gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), str), str);
		g_free (str);
	}
	else if (g_ascii_strncasecmp (buffer, "403 GAME", 8) == 0)
	{
		char *str;

		str = g_strdup_printf ("Game already started! Go away!");
		gtk_statusbar_push (GTK_STATUSBAR (statusbar),
		gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), str), str);
		g_free (str);
	}
	else if (g_ascii_strncasecmp (buffer, "200 TIE", 7) == 0)
	{
		char *str;

		str = g_strdup_printf ("Game tie!");
		gtk_statusbar_push (GTK_STATUSBAR (statusbar),
		gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), str), str);
		g_free (str);
	}
}

gboolean
myboo ()
{
	int n;
	char *p;
	char *buffer;

	for (n = 0, p = strbuffer->str; n < strbuffer->len; n++, p++)
	{
		if (*p == '\n' || (*p == '\r' && ++n < strbuffer->len && *(++p) == '\n'))
			break;
	}
	if (n == strbuffer->len)
	{
		//g_debug ("Time to learn how to count, cascardo.");
		return FALSE;
	}
	buffer = g_strndup (strbuffer->str, n + 1);
	g_string_erase (strbuffer, 0, n + 1);
	//g_debug ("Command %s received", buffer);
	myfar (buffer, n + 1);
	g_free (buffer);
	return TRUE;
}

gboolean
myread (GIOChannel *channel, GIOCondition cond, gpointer data)
{
	int fd;
	char buffer[BUFSIZ];
	int r;

	//g_debug ("New data");
	fd = g_io_channel_unix_get_fd (channel);
	r = read (fd, buffer, BUFSIZ);
	if ((r < 0 && errno != EAGAIN) || r == 0)
	{
		g_debug ("Error or finished connection.");
		g_io_channel_unref (channel);
		return FALSE;
	}
	g_string_append_len (strbuffer, buffer, r);
	while (myboo ());
	return TRUE;
}

void
net_init ()
{
	int fd;
	struct sockaddr_in saddr;

	fd = socket (PF_INET, SOCK_STREAM, 0);
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons (5555);
	saddr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
	connect (fd, (struct sockaddr*) &saddr, sizeof (struct sockaddr_in));
	channel = g_io_channel_unix_new (fd);
	g_io_channel_set_close_on_unref (channel, TRUE);
	g_io_channel_set_flags (channel, g_io_channel_get_flags (channel) | G_IO_FLAG_NONBLOCK, NULL);
	g_io_add_watch (channel, G_IO_IN, myread, NULL);
	strbuffer = g_string_sized_new (BUFSIZ);
	write (fd, "LIST\r\n", 6);
	listed = 1;
}

int
main (int argc, char **argv)
{
	gtk_init (&argc, &argv);

	ui_init ();
	net_init ();

	gtk_main ();

	return 0;
}
