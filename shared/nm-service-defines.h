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

#ifndef __NM_SERVICE_DEFINES_H__
#define __NM_SERVICE_DEFINES_H__

#define NM_VPN_SERVICE_TYPE_ANYCONNECT "org.freedesktop.NetworkManager.anyconnect"

#define NM_DBUS_SERVICE_ANYCONNECT    "org.freedesktop.NetworkManager.anyconnect"
#define NM_DBUS_INTERFACE_ANYCONNECT  "org.freedesktop.NetworkManager.anyconnect"
#define NM_DBUS_PATH_ANYCONNECT       "/org/freedesktop/NetworkManager/anyconnect"

#define NM_ANYCONNECT_GATEWAY   "gateway"
#define NM_ANYCONNECT_TUNDEVICE "cscotun0"
#define NM_ANYCONNECT_MTU       "1390"

#endif /* __NM_SERVICE_DEFINES_H__ */
