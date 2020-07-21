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

#ifndef __NM_ANYCONNECT_EDITOR_PLUGIN_H__
#define __NM_ANYCONNECT_EDITOR_PLUGIN_H__

#define ANYCONNECT_TYPE_EDITOR_PLUGIN                (anyconnect_editor_plugin_get_type ())
#define ANYCONNECT_EDITOR_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANYCONNECT_TYPE_EDITOR_PLUGIN, AnyconnectEditorPlugin))
#define ANYCONNECT_EDITOR_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANYCONNECT_TYPE_EDITOR_PLUGIN, AnyconnectEditorPluginClass))
#define ANYCONNECT_IS_EDITOR_PLUGIN(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANYCONNECT_TYPE_EDITOR_PLUGIN))
#define ANYCONNECT_IS_EDITOR_PLUGIN_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), ANYCONNECT_TYPE_EDITOR_PLUGIN))
#define ANYCONNECT_EDITOR_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANYCONNECT_TYPE_EDITOR_PLUGIN, AnyconnectEditorPluginClass))

typedef struct _AnyconnectEditorPlugin AnyconnectEditorPlugin;
typedef struct _AnyconnectEditorPluginClass AnyconnectEditorPluginClass;

struct _AnyconnectEditorPlugin {
	GObject parent;
};

struct _AnyconnectEditorPluginClass {
	GObjectClass parent;
};

GType anyconnect_editor_plugin_get_type (void);

typedef NMVpnEditor *(*NMVpnEditorFactory) (NMVpnEditorPlugin *editor_plugin,
                                            NMConnection *connection,
                                            GError **error);

NMVpnEditor *
nm_vpn_editor_factory_anyconnect (NMVpnEditorPlugin *editor_plugin,
                               NMConnection *connection,
                               GError **error);

#endif /* __NM_ANYCONNECT_EDITOR_PLUGIN_H__ */
