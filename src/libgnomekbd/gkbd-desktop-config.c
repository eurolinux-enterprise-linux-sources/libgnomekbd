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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/keysym.h>

#include <glib/gi18n.h>

#include <gkbd-desktop-config.h>
#include <gkbd-config-private.h>

/**
 * GkbdDesktopConfig
 */
#define GKBD_DESKTOP_CONFIG_KEY_PREFIX  GKBD_CONFIG_KEY_PREFIX "/general"

const gchar GKBD_DESKTOP_CONFIG_DIR[] = GKBD_DESKTOP_CONFIG_KEY_PREFIX;
const gchar GKBD_DESKTOP_CONFIG_KEY_DEFAULT_GROUP[] =
    GKBD_DESKTOP_CONFIG_KEY_PREFIX "/defaultGroup";
const gchar GKBD_DESKTOP_CONFIG_KEY_GROUP_PER_WINDOW[] =
    GKBD_DESKTOP_CONFIG_KEY_PREFIX "/groupPerWindow";
const gchar GKBD_DESKTOP_CONFIG_KEY_HANDLE_INDICATORS[] =
    GKBD_DESKTOP_CONFIG_KEY_PREFIX "/handleIndicators";
const gchar GKBD_DESKTOP_CONFIG_KEY_LAYOUT_NAMES_AS_GROUP_NAMES[]
    = GKBD_DESKTOP_CONFIG_KEY_PREFIX "/layoutNamesAsGroupNames";
const gchar GKBD_DESKTOP_CONFIG_KEY_LOAD_EXTRA_ITEMS[]
    = GKBD_DESKTOP_CONFIG_KEY_PREFIX "/loadExtraItems";

/**
 * static common functions
 */

static gboolean
    gkbd_desktop_config_get_lv_descriptions
    (GkbdDesktopConfig * config,
     XklConfigRegistry * registry,
     const gchar ** layout_ids,
     const gchar ** variant_ids,
     gchar *** short_layout_descriptions,
     gchar *** long_layout_descriptions,
     gchar *** short_variant_descriptions,
     gchar *** long_variant_descriptions) {
	const gchar **pl, **pv;
	guint total_layouts;
	gchar **sld, **lld, **svd, **lvd;
	XklConfigItem *item = xkl_config_item_new ();

	if (!
	    (xkl_engine_get_features (config->engine) &
	     XKLF_MULTIPLE_LAYOUTS_SUPPORTED))
		return FALSE;

	pl = layout_ids;
	pv = variant_ids;
	total_layouts = g_strv_length ((char **) layout_ids);
	sld = *short_layout_descriptions =
	    g_new0 (char *, total_layouts + 1);
	lld = *long_layout_descriptions =
	    g_new0 (char *, total_layouts + 1);
	svd = *short_variant_descriptions =
	    g_new0 (char *, total_layouts + 1);
	lvd = *long_variant_descriptions =
	    g_new0 (char *, total_layouts + 1);

	while (pl != NULL && *pl != NULL) {

		xkl_debug (100, "ids: [%s][%s]\n", *pl,
			   pv == NULL ? NULL : *pv);

		g_snprintf (item->name, sizeof item->name, "%s", *pl);
		if (xkl_config_registry_find_layout (registry, item)) {
			*sld = g_strdup (item->short_description);
			*lld = g_strdup (item->description);
		} else {
			*sld = g_strdup ("");
			*lld = g_strdup ("");
		}

		if (*pv != NULL) {
			g_snprintf (item->name, sizeof item->name, "%s",
				    *pv);
			if (xkl_config_registry_find_variant
			    (registry, *pl, item)) {
				*svd = g_strdup (item->short_description);
				*lvd = g_strdup (item->description);
			} else {
				*svd = g_strdup ("");
				*lvd = g_strdup ("");
			}
		} else {
			*svd = g_strdup ("");
			*lvd = g_strdup ("");
		}

		xkl_debug (100, "description: [%s][%s][%s][%s]\n",
			   *sld, *lld, *svd, *lvd);
		sld++;
		lld++;
		svd++;
		lvd++;

		pl++;

		if (*pv != NULL)
			pv++;
	}

	g_object_unref (item);
	return TRUE;
}

void
gkbd_desktop_config_add_listener (GConfClient * conf_client,
				  const gchar * key,
				  GConfClientNotifyFunc func,
				  gpointer user_data, int *pid)
{
	GError *gerror = NULL;
	xkl_debug (150, "Listening to [%s]\n", key);
	*pid = gconf_client_notify_add (conf_client,
					key, func, user_data, NULL,
					&gerror);
	if (0 == *pid) {
		g_warning ("Error listening for configuration: [%s]\n",
			   gerror->message);
		g_error_free (gerror);
	}
}

void
gkbd_desktop_config_remove_listener (GConfClient * conf_client, int *pid)
{
	if (*pid != 0) {
		gconf_client_notify_remove (conf_client, *pid);
		*pid = 0;
	}
}

/**
 * extern GkbdDesktopConfig config functions
 */
void
gkbd_desktop_config_init (GkbdDesktopConfig * config,
			  GConfClient * conf_client, XklEngine * engine)
{
	GError *gerror = NULL;

	memset (config, 0, sizeof (*config));
	config->conf_client = conf_client;
	config->engine = engine;
	g_object_ref (config->conf_client);

	gconf_client_add_dir (config->conf_client,
			      GKBD_DESKTOP_CONFIG_DIR,
			      GCONF_CLIENT_PRELOAD_NONE, &gerror);
	if (gerror != NULL) {
		g_warning ("err: %s\n", gerror->message);
		g_error_free (gerror);
		gerror = NULL;
	}
}

void
gkbd_desktop_config_term (GkbdDesktopConfig * config)
{
	g_object_unref (config->conf_client);
	config->conf_client = NULL;
}

void
gkbd_desktop_config_load_from_gconf (GkbdDesktopConfig * config)
{
	GError *gerror = NULL;

	config->group_per_app =
	    gconf_client_get_bool (config->conf_client,
				   GKBD_DESKTOP_CONFIG_KEY_GROUP_PER_WINDOW,
				   &gerror);
	if (gerror != NULL) {
		g_warning ("Error reading configuration:%s\n",
			   gerror->message);
		config->group_per_app = FALSE;
		g_error_free (gerror);
		gerror = NULL;
	}
	xkl_debug (150, "group_per_app: %d\n", config->group_per_app);

	config->handle_indicators =
	    gconf_client_get_bool (config->conf_client,
				   GKBD_DESKTOP_CONFIG_KEY_HANDLE_INDICATORS,
				   &gerror);
	if (gerror != NULL) {
		g_warning ("Error reading configuration:%s\n",
			   gerror->message);
		config->handle_indicators = FALSE;
		g_error_free (gerror);
		gerror = NULL;
	}
	xkl_debug (150, "handle_indicators: %d\n",
		   config->handle_indicators);

	config->layout_names_as_group_names =
	    gconf_client_get_bool (config->conf_client,
				   GKBD_DESKTOP_CONFIG_KEY_LAYOUT_NAMES_AS_GROUP_NAMES,
				   &gerror);
	if (gerror != NULL) {
		g_warning ("Error reading configuration:%s\n",
			   gerror->message);
		config->layout_names_as_group_names = TRUE;
		g_error_free (gerror);
		gerror = NULL;
	}
	xkl_debug (150, "layout_names_as_group_names: %d\n",
		   config->layout_names_as_group_names);

	config->load_extra_items =
	    gconf_client_get_bool (config->conf_client,
				   GKBD_DESKTOP_CONFIG_KEY_LOAD_EXTRA_ITEMS,
				   &gerror);
	if (gerror != NULL) {
		g_warning ("Error reading configuration:%s\n",
			   gerror->message);
		config->load_extra_items = FALSE;
		g_error_free (gerror);
		gerror = NULL;
	}
	xkl_debug (150, "load_extra_items: %d\n",
		   config->load_extra_items);

	config->default_group =
	    gconf_client_get_int (config->conf_client,
				  GKBD_DESKTOP_CONFIG_KEY_DEFAULT_GROUP,
				  &gerror);
	if (gerror != NULL) {
		g_warning ("Error reading configuration:%s\n",
			   gerror->message);
		config->default_group = -1;
		g_error_free (gerror);
		gerror = NULL;
	}

	if (config->default_group < -1
	    || config->default_group >=
	    xkl_engine_get_max_num_groups (config->engine))
		config->default_group = -1;
	xkl_debug (150, "default_group: %d\n", config->default_group);
}

void
gkbd_desktop_config_save_to_gconf (GkbdDesktopConfig * config)
{
	GConfChangeSet *cs;
	GError *gerror = NULL;

	cs = gconf_change_set_new ();

	gconf_change_set_set_bool (cs,
				   GKBD_DESKTOP_CONFIG_KEY_GROUP_PER_WINDOW,
				   config->group_per_app);
	gconf_change_set_set_bool (cs,
				   GKBD_DESKTOP_CONFIG_KEY_HANDLE_INDICATORS,
				   config->handle_indicators);
	gconf_change_set_set_bool (cs,
				   GKBD_DESKTOP_CONFIG_KEY_LAYOUT_NAMES_AS_GROUP_NAMES,
				   config->layout_names_as_group_names);
	gconf_change_set_set_bool (cs,
				   GKBD_DESKTOP_CONFIG_KEY_LOAD_EXTRA_ITEMS,
				   config->load_extra_items);
	gconf_change_set_set_int (cs,
				  GKBD_DESKTOP_CONFIG_KEY_DEFAULT_GROUP,
				  config->default_group);

	gconf_client_commit_change_set (config->conf_client, cs, TRUE,
					&gerror);
	if (gerror != NULL) {
		g_warning ("Error saving active configuration: %s\n",
			   gerror->message);
		g_error_free (gerror);
		gerror = NULL;
	}
	gconf_change_set_unref (cs);
}

gboolean
gkbd_desktop_config_activate (GkbdDesktopConfig * config)
{
	gboolean rv = TRUE;

	xkl_engine_set_group_per_toplevel_window (config->engine,
						  config->group_per_app);
	xkl_engine_set_indicators_handling (config->engine,
					    config->handle_indicators);
	xkl_engine_set_default_group (config->engine,
				      config->default_group);

	return rv;
}

void
gkbd_desktop_config_lock_next_group (GkbdDesktopConfig * config)
{
	int group = xkl_engine_get_next_group (config->engine);
	xkl_engine_lock_group (config->engine, group);
}

void
gkbd_desktop_config_lock_prev_group (GkbdDesktopConfig * config)
{
	int group = xkl_engine_get_prev_group (config->engine);
	xkl_engine_lock_group (config->engine, group);
}

void
gkbd_desktop_config_restore_group (GkbdDesktopConfig * config)
{
	int group = xkl_engine_get_current_window_group (config->engine);
	xkl_engine_lock_group (config->engine, group);
}

void
gkbd_desktop_config_start_listen (GkbdDesktopConfig * config,
				  GConfClientNotifyFunc func,
				  gpointer user_data)
{
	gkbd_desktop_config_add_listener (config->conf_client,
					  GKBD_DESKTOP_CONFIG_DIR, func,
					  user_data,
					  &config->config_listener_id);
}

void
gkbd_desktop_config_stop_listen (GkbdDesktopConfig * config)
{
	gkbd_desktop_config_remove_listener (config->conf_client,
					     &config->config_listener_id);
}

gboolean
gkbd_desktop_config_load_group_descriptions (GkbdDesktopConfig
					     * config,
					     XklConfigRegistry *
					     registry,
					     const gchar **
					     layout_ids,
					     const gchar **
					     variant_ids,
					     gchar ***
					     short_group_names,
					     gchar *** full_group_names)
{
	gchar **sld, **lld, **svd, **lvd;
	gchar **psld, **plld, **plvd;
	gchar **psgn, **pfgn;
	gint total_descriptions;

	if (!gkbd_desktop_config_get_lv_descriptions
	    (config, registry, layout_ids, variant_ids, &sld, &lld, &svd,
	     &lvd)) {
		return False;
	}

	total_descriptions = g_strv_length (sld);

	*short_group_names = psgn =
	    g_new0 (gchar *, total_descriptions + 1);
	*full_group_names = pfgn =
	    g_new0 (gchar *, total_descriptions + 1);

	plld = lld;
	psld = sld;
	plvd = lvd;
	while (plld != NULL && *plld != NULL) {
		*psgn++ = g_strdup (*psld++);
		*pfgn++ = g_strdup (gkbd_keyboard_config_format_full_layout
				    (*plld++, *plvd++));
	}
	g_strfreev (sld);
	g_strfreev (lld);
	g_strfreev (svd);
	g_strfreev (lvd);

	return True;
}
