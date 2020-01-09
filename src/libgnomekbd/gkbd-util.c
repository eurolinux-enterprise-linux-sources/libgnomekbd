/*
 * Copyright (C) 2006 Sergey V. Udaltsov <svu@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <gkbd-util.h>

#include <time.h>

#include <glib/gi18n.h>

#include <libxklavier/xklavier.h>

#include <gconf/gconf-client.h>

#include <gkbd-config-private.h>

static void
gkbd_log_appender (const char file[], const char function[],
		   int level, const char format[], va_list args)
{
	time_t now = time (NULL);
	g_log (NULL, G_LOG_LEVEL_DEBUG, "[%08ld,%03d,%s:%s/] \t",
	       (long) now, level, file, function);
	g_logv (NULL, G_LOG_LEVEL_DEBUG, format, args);
}

void
gkbd_install_glib_log_appender (void)
{
	xkl_set_log_appender (gkbd_log_appender);
}

#define GKBD_PREVIEW_CONFIG_KEY_PREFIX  GKBD_CONFIG_KEY_PREFIX "/preview"

const gchar GKBD_PREVIEW_CONFIG_DIR[] = GKBD_PREVIEW_CONFIG_KEY_PREFIX;
const gchar GKBD_PREVIEW_CONFIG_KEY_X[] =
    GKBD_PREVIEW_CONFIG_KEY_PREFIX "/x";
const gchar GKBD_PREVIEW_CONFIG_KEY_Y[] =
    GKBD_PREVIEW_CONFIG_KEY_PREFIX "/y";
const gchar GKBD_PREVIEW_CONFIG_KEY_WIDTH[] =
    GKBD_PREVIEW_CONFIG_KEY_PREFIX "/width";
const gchar GKBD_PREVIEW_CONFIG_KEY_HEIGHT[] =
    GKBD_PREVIEW_CONFIG_KEY_PREFIX "/height";

GdkRectangle *
gkbd_preview_load_position (void)
{
	GError *gerror = NULL;
	GdkRectangle *rv = NULL;
	gint x, y, w, h;
	GConfClient *conf_client = gconf_client_get_default ();

	if (conf_client == NULL)
		return NULL;

	x = gconf_client_get_int (conf_client,
				  GKBD_PREVIEW_CONFIG_KEY_X, &gerror);
	if (gerror != NULL) {
		xkl_debug (0, "Error getting the preview x: %s\n",
			   gerror->message);
		g_error_free (gerror);
		g_object_unref (G_OBJECT (conf_client));
		return NULL;
	}

	y = gconf_client_get_int (conf_client,
				  GKBD_PREVIEW_CONFIG_KEY_Y, &gerror);
	if (gerror != NULL) {
		xkl_debug (0, "Error getting the preview y: %s\n",
			   gerror->message);
		g_error_free (gerror);
		g_object_unref (G_OBJECT (conf_client));
		return NULL;
	}

	w = gconf_client_get_int (conf_client,
				  GKBD_PREVIEW_CONFIG_KEY_WIDTH, &gerror);
	if (gerror != NULL) {
		xkl_debug (0, "Error getting the preview width: %s\n",
			   gerror->message);
		g_error_free (gerror);
		g_object_unref (G_OBJECT (conf_client));
		return NULL;
	}

	h = gconf_client_get_int (conf_client,
				  GKBD_PREVIEW_CONFIG_KEY_HEIGHT, &gerror);
	if (gerror != NULL) {
		xkl_debug (0, "Error getting the preview height: %s\n",
			   gerror->message);
		g_error_free (gerror);
		g_object_unref (G_OBJECT (conf_client));
		return NULL;
	}

	g_object_unref (G_OBJECT (conf_client));

	rv = g_new (GdkRectangle, 1);
	if (x == -1 || y == -1 || w == -1 || h == -1) {
		/* default values should be treated as 
		 * "0.75 of the screen size" */
		GdkScreen *scr = gdk_screen_get_default ();
		gint w = gdk_screen_get_width (scr);
		gint h = gdk_screen_get_height (scr);
		rv->x = w >> 3;
		rv->y = h >> 3;
		rv->width = w - (w >> 2);
		rv->height = h - (h >> 2);
	} else {
		rv->x = x;
		rv->y = y;
		rv->width = w;
		rv->height = h;
	}
	return rv;
}

void
gkbd_preview_save_position (GdkRectangle * rect)
{
	GConfClient *conf_client = gconf_client_get_default ();
	GConfChangeSet *cs;
	GError *gerror = NULL;

	cs = gconf_change_set_new ();

	gconf_change_set_set_int (cs, GKBD_PREVIEW_CONFIG_KEY_X, rect->x);
	gconf_change_set_set_int (cs, GKBD_PREVIEW_CONFIG_KEY_Y, rect->y);
	gconf_change_set_set_int (cs, GKBD_PREVIEW_CONFIG_KEY_WIDTH,
				  rect->width);
	gconf_change_set_set_int (cs, GKBD_PREVIEW_CONFIG_KEY_HEIGHT,
				  rect->height);

	gconf_client_commit_change_set (conf_client, cs, TRUE, &gerror);
	if (gerror != NULL) {
		g_warning ("Error saving preview configuration: %s\n",
			   gerror->message);
		g_error_free (gerror);
	}
	gconf_change_set_unref (cs);
	g_object_unref (G_OBJECT (conf_client));
}
