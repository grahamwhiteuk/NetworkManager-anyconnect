/*
 * network-manager-anyconnect - Anyconnect integration with NetworkManager
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "nm-default.h"

#include "nm-anyconnect-editor.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "nm-utils/nm-shared-utils.h"

/*****************************************************************************/

#define BLOCK_HANDLER_ID "block-handler-id"

/*****************************************************************************/

typedef void (*ChangedCallback) (GtkWidget *widget, gpointer user_data);

/*****************************************************************************/

static const char *
nm_find_anyconnect (void)
{
	static const char *anyconnect_binary_paths[] = {
		"/opt/cisco/anyconnect/bin/vpn",
		NULL
	};
	const char  **anyconnect_binary = anyconnect_binary_paths;

	while (*anyconnect_binary != NULL) {
		if (g_file_test (*anyconnect_binary, G_FILE_TEST_EXISTS))
			break;
		anyconnect_binary++;
	}

	if (*anyconnect_binary == NULL) {
	  g_critical ("Unable to find the anyconnect binary: /opt/cisco/anyconnect/bin/vpn");
	}

	return *anyconnect_binary;
}

/*****************************************************************************/

static void anyconnect_editor_plugin_widget_interface_init (NMVpnEditorInterface *iface_class);

typedef struct {
	GtkBuilder *builder;
	GtkWidget *widget;
	GtkWindowGroup *window_group;
	gboolean window_added;
	GHashTable *advanced;
	gboolean new_connection;
	GtkWidget *tls_user_cert_chooser;
} AnyconnectEditorPrivate;

G_DEFINE_TYPE_EXTENDED (AnyconnectEditor, anyconnect_editor_plugin_widget, G_TYPE_OBJECT, 0,
                        G_ADD_PRIVATE (AnyconnectEditor)
                        G_IMPLEMENT_INTERFACE (NM_TYPE_VPN_EDITOR,
                                               anyconnect_editor_plugin_widget_interface_init))

/*****************************************************************************/

static gboolean
check_validity (AnyconnectEditor *self, GError **error)
{
	AnyconnectEditorPrivate *priv = anyconnect_editor_plugin_widget_get_instance_private (self);
	GtkWidget *widget;

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "gateway_combo"));
	if (widget != NULL)
		gtk_style_context_remove_class (gtk_widget_get_style_context (widget), "error");
	else {
		gtk_style_context_add_class (gtk_widget_get_style_context (widget), "error");
		g_set_error (error,
		             NMV_EDITOR_PLUGIN_ERROR,
		             NMV_EDITOR_PLUGIN_ERROR_INVALID_PROPERTY,
		             NM_ANYCONNECT_GATEWAY);
		return FALSE;
	}

	return TRUE;
}

// https://stackoverflow.com/questions/40538792/remove-first-few-characters-from-a-string
static gchar *
chopN( gchar *s, size_t n )
{
    char *src = s;
    while ( *src && n ) --n, ++src;
    if ( n == 0 && src != s ) {
        for ( char *dst = s; (  *dst++ = *src++ ); );
    }
    return s;
}

#define ANYCONNECT_HOST_CHOP_CHARS 6

static void
populate_gateway_combo (GtkComboBox *box)
{
	GtkListStore *store;
	GtkTreeIter iter;
	const char *anyconnect_binary = NULL;
	gchar *tmp, **items, **item;
	char *argv[3];
	GError *error = NULL;
	gboolean success = TRUE;

  // find the AnyConnect binary
	anyconnect_binary = nm_find_anyconnect ();
	if (!anyconnect_binary)
		return;

	// create the list store for the gateway_combo
	store = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_combo_box_set_model (box, GTK_TREE_MODEL (store));

  // prepare to run AnyConnect with the hosts command
	argv[0] = (char *) anyconnect_binary;
	argv[1] = "hosts";
	argv[2] = NULL;

  // run "AnyConnect hosts"
	success = g_spawn_sync ("/", argv, NULL, 0, NULL, NULL, &tmp, NULL, NULL, &error);
	if (!success) {
		g_warning ("%s: couldn't determine gateways: %s", __func__, error->message);
		g_error_free (error);
		return;
	}

  // split the output by line
	items = g_strsplit (tmp, "\n", 0);
	g_free (tmp);

  // loop over the lines of the output
	for (item = items; *item; item++) {
		char *gtsign;
		gchar *gateway;

    // ignore any line that doesn't have a > character
		gtsign = strchr (*item, '>');
		if (!gtsign)
			continue;

    // chop off the first n characters of the "AnyConnect hosts" output
		gateway = chopN(*item, ANYCONNECT_HOST_CHOP_CHARS);

    // add the gateway to the store
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
												0, _(gateway),
												-1);
	}

  // default to showing the first gateway in the list
  gtk_combo_box_set_active (box, 0);
	g_object_unref (store);
}

static void
stuff_changed_cb (GtkWidget *widget, gpointer user_data)
{
	g_signal_emit_by_name (ANYCONNECT_EDITOR (user_data), "changed");
}

static gboolean
init_editor_plugin (AnyconnectEditor *self, NMConnection *connection)
{
	AnyconnectEditorPrivate *priv = anyconnect_editor_plugin_widget_get_instance_private (self);
	GtkWidget *widget;

  // check we've got the gateway combo box, otherwise time to barf
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "gateway_combo"));
	g_return_val_if_fail (widget != NULL, FALSE);

  populate_gateway_combo (GTK_COMBO_BOX (widget));

	g_signal_connect (G_OBJECT (widget), "changed", G_CALLBACK (stuff_changed_cb), self);

	return TRUE;
}

static GObject *
get_widget (NMVpnEditor *iface)
{
	AnyconnectEditor *self = ANYCONNECT_EDITOR (iface);
	AnyconnectEditorPrivate *priv = anyconnect_editor_plugin_widget_get_instance_private (self);

	return G_OBJECT (priv->widget);
}

static gboolean
update_connection (NMVpnEditor *iface,
                   NMConnection *connection,
                   GError **error)
{
	AnyconnectEditor *self = ANYCONNECT_EDITOR (iface);
	AnyconnectEditorPrivate *priv = anyconnect_editor_plugin_widget_get_instance_private (self);
	NMSettingVpn *s_vpn;
	GtkWidget *widget;
	gs_free char *auth_type = NULL;
	gboolean valid = FALSE;
	GtkTreeModel *model;
	GtkTreeIter iter;
	const char *gateway;
	gboolean success;

	if (!check_validity (self, error))
		return FALSE;

	s_vpn = NM_SETTING_VPN (nm_setting_vpn_new ());
	g_object_set (s_vpn, NM_SETTING_VPN_SERVICE_TYPE, NM_VPN_SERVICE_TYPE_ANYCONNECT,
    NM_SETTING_VPN_USER_NAME, g_get_user_name (),
    NULL);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "gateway_combo"));
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
	success = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter);
	if (success) {
		gtk_tree_model_get (model, &iter, 0, &gateway, -1);
	}

	if (gateway && gateway[0])
		nm_setting_vpn_add_data_item (s_vpn, NM_ANYCONNECT_GATEWAY, gateway);

	nm_connection_add_setting (connection, NM_SETTING (s_vpn));
	valid = TRUE;

	return valid;
}

static void
is_new_func (const char *key, const char *value, gpointer user_data)
{
	gboolean *is_new = user_data;

	/* If there are any VPN data items the connection isn't new */
	*is_new = FALSE;
}

/*****************************************************************************/

static void
anyconnect_editor_plugin_widget_init (AnyconnectEditor *plugin)
{
}

NMVpnEditor *
anyconnect_editor_new (NMConnection *connection, GError **error)
{
	gs_unref_object NMVpnEditor *object = NULL;
	AnyconnectEditorPrivate *priv;
	gboolean new = TRUE;
	NMSettingVpn *s_vpn;

	g_return_val_if_fail (NM_IS_CONNECTION (connection), NULL);
	g_return_val_if_fail (!error || !*error, NULL);

	object = g_object_new (ANYCONNECT_TYPE_EDITOR, NULL);

	priv = anyconnect_editor_plugin_widget_get_instance_private ((AnyconnectEditor *) object);

	priv->builder = gtk_builder_new ();

	gtk_builder_set_translation_domain (priv->builder, GETTEXT_PACKAGE);

	if (!gtk_builder_add_from_resource (priv->builder, "/org/freedesktop/network-manager-anyconnect/nm-anyconnect-dialog.ui", error))
		g_return_val_if_reached (NULL);

	priv->widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "anyconnect-vbox"));
	if (!priv->widget) {
		g_set_error_literal (error, NMV_EDITOR_PLUGIN_ERROR, 0, _("could not load UI widget"));
		g_return_val_if_reached (NULL);
	}
	g_object_ref_sink (priv->widget);

	priv->window_group = gtk_window_group_new ();

	s_vpn = nm_connection_get_setting_vpn (connection);
	if (s_vpn)
		nm_setting_vpn_foreach_data_item (s_vpn, is_new_func, &new);
	priv->new_connection = new;

	if (!init_editor_plugin (ANYCONNECT_EDITOR (object), connection))
		g_return_val_if_reached (NULL);

	return g_steal_pointer (&object);
}

static void
dispose (GObject *object)
{
	AnyconnectEditor *plugin = ANYCONNECT_EDITOR (object);
  AnyconnectEditorPrivate *priv = anyconnect_editor_plugin_widget_get_instance_private (plugin);

	g_clear_object (&priv->window_group);

	g_clear_object (&priv->widget);

	g_clear_object (&priv->builder);

	g_clear_pointer (&priv->advanced, g_hash_table_destroy);

	G_OBJECT_CLASS (anyconnect_editor_plugin_widget_parent_class)->dispose (object);
}

static void
anyconnect_editor_plugin_widget_interface_init (NMVpnEditorInterface *iface_class)
{
	/* interface implementation */
	iface_class->get_widget = get_widget;
	iface_class->update_connection = update_connection;
}

static void
anyconnect_editor_plugin_widget_class_init (AnyconnectEditorClass *req_class)
{

}

/*****************************************************************************/

#if !((NETWORKMANAGER_COMPILATION) & NM_NETWORKMANAGER_COMPILATION_WITH_LIBNM_UTIL)

#include "nm-anyconnect-editor-plugin.h"

G_MODULE_EXPORT NMVpnEditor *
nm_vpn_editor_factory_anyconnect (NMVpnEditorPlugin *editor_plugin,
                               NMConnection *connection,
                               GError **error)
{
	g_type_ensure (NMA_TYPE_CERT_CHOOSER);
	return anyconnect_editor_new (connection, error);
}
#endif
