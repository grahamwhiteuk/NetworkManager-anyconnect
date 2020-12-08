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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <pwd.h>
#include <grp.h>
#include <glib-unix.h>
#ifdef HAVE_LIBNOTIFY_07
#include <libnotify/notify.h>
#endif

#include "nm-utils/nm-shared-utils.h"
#include "nm-utils/nm-vpn-plugin-macros.h"

/* for get_local_ip_address() ************************************************/

#include <netdb.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
/* #define _GNU_SOURCE  To get definitions of NI_MAXSERV and NI_MAXHOST */

/* Global Stuff **************************************************************/

#if !defined(DIST_VERSION)
# define DIST_VERSION VERSION
#endif

static struct {
  gboolean debug;
  int log_level;
  int log_level_anyconnect;
  bool log_syslog;
} gl/*obal*/;

#define NM_TYPE_ANYCONNECT_PLUGIN            (nm_anyconnect_plugin_get_type ())
#define NM_ANYCONNECT_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_ANYCONNECT_PLUGIN, NMAnyconnectPlugin))
#define NM_ANYCONNECT_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_ANYCONNECT_PLUGIN, NMAnyconnectPluginClass))

/* See
  https://developer.gnome.org/NetworkManager/stable/gdbus-org.freedesktop.NetworkManager.VPN.Plugin.html
  - or - locally some where like
  file:///usr/share/gtk-doc/html/NetworkManager/gdbus-org.freedesktop.NetworkManager.VPN.Plugin.html
*/

typedef struct {
  NMVpnServicePlugin parent;
} NMAnyconnectPlugin;

typedef struct {
  NMVpnServicePluginClass parent;
} NMAnyconnectPluginClass;

GType nm_anyconnect_plugin_get_type (void);

NMAnyconnectPlugin *nm_anyconnect_plugin_new (const char *bus_name);

typedef struct {
  GPid pid; // pid of AnyConnect
  uid_t uid; // uid to run AnyConnect as
  gid_t gid; // gid to run AnyConnect as
  gboolean connected; // whether we're connected (or connecting)
  char *last_warning; // the most recent warning from AnyConnect
  char *last_error; // the most recent error from AnyConnect
  GIOChannel *stdout_channel;
  guint socket_channel_stdout_eventid;
} NMAnyconnectPluginPrivate;

//G_DEFINE_TYPE (NMAnyconnectPlugin, nm_anyconnect_plugin, NM_TYPE_VPN_SERVICE_PLUGIN)

//#define NM_ANYCONNECT_PLUGIN_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), NM_TYPE_ANYCONNECT_PLUGIN, NMAnyconnectPluginPrivate))

G_DEFINE_TYPE_WITH_CODE (NMAnyconnectPlugin, nm_anyconnect_plugin, NM_TYPE_VPN_SERVICE_PLUGIN, G_ADD_PRIVATE (NMAnyconnectPlugin));

/* Logging functions *********************************************************/

#define _NMLOG(level, ...) \
  G_STMT_START { \
    if (gl.log_level >= (level)) { \
      g_print ("nm-anyconnect[%ld] %-7s " _NM_UTILS_MACRO_FIRST (__VA_ARGS__) "\n", \
               (long) getpid (), \
               nm_utils_syslog_to_str (level) \
               _NM_UTILS_MACRO_REST (__VA_ARGS__)); \
    } \
  } G_STMT_END

#define _LOGD(...) _NMLOG(LOG_INFO,    __VA_ARGS__)
#define _LOGI(...) _NMLOG(LOG_NOTICE,  __VA_ARGS__)
#define _LOGW(...) _NMLOG(LOG_WARNING, __VA_ARGS__)

/* Functions to send the AnyConnect config to NetworkManager *****************/

typedef struct {
  char *ip4addr;
  char *ip4mask;
  char *ip6addr;
  int ip6mask;
} IPAddrInfo;

#ifdef HAVE_LIBNOTIFY_07
/* attempt to show a libnotify notification to the user */
static void
show_notification_full(NMAnyconnectPlugin *plugin, const char *summary, const char *body, const char *icon) {
  NMAnyconnectPluginPrivate *priv = nm_anyconnect_plugin_get_instance_private(plugin);
  NotifyNotification *notification;
  gboolean notificationReturn = FALSE;
  GError *notificationError = NULL;
  gchar *dbusPath;
  gid_t gid;
  uid_t uid;
  int e;

  // try to initialise libnotify
  if (notify_init ("nm-anyconnect-service")) {
    if (gl.debug)
      g_message("libnotify initialised successfully");

    // work out the DBUS_SESSION_BUS_ADDRESS of the user
    dbusPath = g_strdup_printf("unix:path=/run/user/%d/bus", priv->uid);
    if (gl.debug) {
      g_message("DBUS_SESSION_BUS_ADDRESS for libnotify notifications is %s", dbusPath);
      g_message("DISPLAY for libnotify notifications is :0");
    }

    // configure the DBUS_SESSION_BUS_ADDRESS and DISPLAY in the environment
    g_setenv("DBUS_SESSION_BUS_ADDRESS", dbusPath, TRUE);
    g_setenv("DISPLAY", ":0", TRUE);

    // start setting up our notification
    notification = notify_notification_new (summary, body, icon);

    // remember which gid and uid we're currently running as (should be 0)
    gid = getegid();
    uid = geteuid();

    // we need to execute notify_notification_show() as the user so attempt to setgid and setuid here
    if (priv->gid != 0) {
      setgroups(1, &priv->gid);

      e = setgid (priv->gid);
      if (e < 0)
        g_warning("Unable to show notification as group with GID: %d", priv->gid);
    }

    if (priv->uid != 0) {
      e = setuid (priv->uid);
      if (e < 0)
        g_warning("Unable to show notification as user with UID: %d", priv->uid);
    }

    // display the notification to the user
    // The following line needs to be executed under priv->uid (and not root or other users)
    notificationReturn = notify_notification_show (notification, &notificationError);

    // return the uid and gid to their saved values
    setgid(gid);
    setuid(uid);

    if (!notificationReturn)
      g_warning("message not shown: %s", notificationError->message);

  } else {
    g_warning("libnotify initialisation failed");
  }
}

static void
show_notification(NMAnyconnectPlugin *plugin, const char *body) {
  show_notification_full(plugin, "Cisco AnyConnect VPN", body, "network-vpn-no-route");
}
#endif

#ifndef HAVE_LIBNOTIFY_07
static void
show_notification_full(NMAnyconnectPlugin *plugin, const char *summary, const char *body, const char *icon) {
  g_message(body);
}

static void
show_notification(NMAnyconnectPlugin *plugin, const char *body) {
  show_notification_full(body);
}
#endif

/* based on code from getifaddrs man page, see "man 3 getifaddrs" */
static IPAddrInfo
get_local_ip_address(void)
{
  struct ifaddrs *ifaddr, *ifa;
  int family, s1, i, j;
  char host[NI_MAXHOST];
  IPAddrInfo info = {
    malloc(NI_MAXHOST),
    malloc(NI_MAXHOST),
    malloc(INET6_ADDRSTRLEN),
    0
  };
  struct sockaddr_in *sa;
  char *s2;
  char ip_str[INET6_ADDRSTRLEN];
  unsigned char *c;
  unsigned char n;
  int total_len, len_to_chop;

  if (gl.debug)
    g_message("get_local_ip_address()");

  if (getifaddrs(&ifaddr) == -1) {
    g_warning("getifaddrs failed to get IP address of interface %s", NM_ANYCONNECT_TUNDEVICE);
    return info;
  }

  // Walk through linked list, maintaining head pointer so we can free list later
  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL)
      continue;

    family = ifa->ifa_addr->sa_family;

    if (strcmp(ifa->ifa_name, NM_ANYCONNECT_TUNDEVICE) == 0) {
      if (family == AF_INET || family == AF_INET6) {
        s1 = getnameinfo(ifa->ifa_addr,
          (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
          host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        sa = (struct sockaddr_in *) ifa->ifa_netmask;
        s2 = inet_ntoa(sa->sin_addr);

        if (s1 == 0 && family == AF_INET) {
          info.ip4addr = strdup(host);
          strcpy(info.ip4mask, s2);

        } else if (s1 == 0 && family == AF_INET6) {
          struct sockaddr_in6 *a = (struct sockaddr_in6 *)ifa->ifa_addr;
          inet_ntop(AF_INET6, &(a->sin6_addr), ip_str, INET6_ADDRSTRLEN);

          // count up the ip6 prefix
          c = ((struct sockaddr_in6 *)ifa->ifa_netmask)->sin6_addr.s6_addr;
          i = 0;
          j = 0;
          n = 0;
          while (i < 16) {
            n = c[i];
            while (n > 0) {
              if (n & 1) j++;
              n = n/2;
            }
            i++;
          }
          // the ip6 addr comes out with a % char in it so we remove the rest of the string after that
          total_len = strlen(host);
          len_to_chop = strlen(g_strstr_len (host, -1, "%"));
          host[total_len - len_to_chop] = '\0';

          info.ip6addr = strdup(host);
          info.ip6mask = j;
        }
      }
    }
  }
  freeifaddrs(ifaddr);
  return info;
}

static GVariant *
str_to_gvariant (const char *str, gboolean try_convert)
{
  if (gl.debug)
    g_message("str_to_gvariant()");

  /* Empty */
  if (!str || strlen (str) < 1)
    return NULL;

  if (!g_utf8_validate (str, -1, NULL)) {
    if (try_convert && !(str = g_convert (str, -1, "ISO-8859-1", "UTF-8", NULL, NULL, NULL)))
      str = g_convert (str, -1, "C", "UTF-8", NULL, NULL, NULL);

    if (!str)
      /* Invalid */
      return NULL;
  }

  return g_variant_new_string (str);
}

static GVariant *
addr6_to_gvariant (const char *str)
{
  struct in6_addr temp_addr;
  GVariantBuilder builder;
  int i;

  if (gl.debug)
    g_message("addr6_to_gvariant()");

  /* Empty */
  if (!str || strlen (str) < 1)
    return NULL;

  if (inet_pton (AF_INET6, str, &temp_addr) <= 0)
    return NULL;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("ay"));
  for (i = 0; i < sizeof (temp_addr); i++)
    g_variant_builder_add (&builder, "y", ((guint8 *) &temp_addr)[i]);
  return g_variant_builder_end (&builder);
}

static GVariant *
addr4_to_gvariant (const char *str)
{
  struct in_addr  temp_addr;

  if (gl.debug)
    g_message("addr4_to_gvariant()");

  /* Empty */
  if (!str || strlen (str) < 1)
    return NULL;

  if (inet_pton (AF_INET, str, &temp_addr) <= 0)
    return NULL;

  return g_variant_new_uint32 (temp_addr.s_addr);
}

/* Once the connection is established, we need to tell NetworkManager some
details about the connection, at least a local IPv4 IP address is required
but we can also pass over some more info to NM too, see
NetworkManager/libnm-core/nm-vpn-dbus-interface.h */
static gboolean
send_network_config (NMAnyconnectPlugin *plugin)
{
  GVariantBuilder config, ip4config, ip6config;
  GVariant *val;
  IPAddrInfo addr_info;
  guint32 addr;

  if (gl.debug)
    g_message("send_network_config()");

  addr_info = get_local_ip_address();

  g_message("Local IPv4: %s / %s", addr_info.ip4addr, addr_info.ip4mask);
  if (addr_info.ip6addr && addr_info.ip6mask)
    g_message("Local IPv6: %s / %d", addr_info.ip6addr, addr_info.ip6mask);

  g_variant_builder_init (&config, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_init (&ip4config, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_init (&ip6config, G_VARIANT_TYPE_VARDICT);

  /*
   * GENERAL CONFIG
   */
  val = str_to_gvariant (NM_ANYCONNECT_TUNDEVICE, FALSE);
  g_variant_builder_add (&config, "{sv}", NM_VPN_PLUGIN_CONFIG_TUNDEV, val);

  val = str_to_gvariant (NM_ANYCONNECT_MTU, FALSE);
  g_variant_builder_add (&config, "{sv}", NM_VPN_PLUGIN_CONFIG_MTU, val);

  // attempt to stop NM from taking the tunnel down when changing networks
	g_variant_builder_add (&config, "{sv}", NM_VPN_PLUGIN_CAN_PERSIST, g_variant_new_boolean (TRUE));

  /*
   * IPv4 CONFIG
   */
  g_variant_builder_add (&config, "{sv}", NM_VPN_PLUGIN_CONFIG_HAS_IP4, g_variant_new_boolean (TRUE));

  /* Local IPv4 Address */
  val = addr4_to_gvariant (addr_info.ip4addr);
  g_variant_builder_add (&ip4config, "{sv}", NM_VPN_PLUGIN_IP4_CONFIG_ADDRESS, val);

  /* IPv4 Netmask */
  val = addr4_to_gvariant(addr_info.ip4mask);
  addr = g_variant_get_uint32 (val);
  g_variant_unref (val);
  val = g_variant_new_uint32 (nm_utils_ip4_netmask_to_prefix (addr));
  g_variant_builder_add (&ip4config, "{sv}", NM_VPN_PLUGIN_IP4_CONFIG_PREFIX, val);

  /* don't add default route */
  g_variant_builder_add (&ip4config, "{sv}", NM_VPN_PLUGIN_IP4_CONFIG_NEVER_DEFAULT, g_variant_new_boolean (TRUE));

  /* allow AnyConnect to setup the routes */
  g_variant_builder_add (&ip4config, "{sv}", NM_VPN_PLUGIN_IP4_CONFIG_PRESERVE_ROUTES, g_variant_new_boolean (TRUE));


  /*
   * IPv6 CONFIG
   */
  if (addr_info.ip6addr && addr_info.ip6mask) {
    g_variant_builder_add (&config, "{sv}", NM_VPN_PLUGIN_CONFIG_HAS_IP6, g_variant_new_boolean (TRUE));

    /* Local IPv6 Address */
    val = addr6_to_gvariant (addr_info.ip6addr);
    g_variant_builder_add (&ip6config, "{sv}", NM_VPN_PLUGIN_IP6_CONFIG_ADDRESS, val);

    /* IPv6 Prefix */
    val = g_variant_new_uint32 (addr_info.ip6mask);
    g_variant_builder_add (&ip6config, "{sv}", NM_VPN_PLUGIN_IP6_CONFIG_PREFIX, val);

    /* don't add default route */
    g_variant_builder_add (&ip6config, "{sv}", NM_VPN_PLUGIN_IP6_CONFIG_NEVER_DEFAULT, g_variant_new_boolean (TRUE));

    /* allow AnyConnect to setup the routes */
    g_variant_builder_add (&ip6config, "{sv}", NM_VPN_PLUGIN_IP6_CONFIG_PRESERVE_ROUTES, g_variant_new_boolean (TRUE));
  }


  /*
   * Send Configs
   */

  /* Send general config */
  nm_vpn_service_plugin_set_config ((NMVpnServicePlugin*) plugin,
                                    g_variant_builder_end (&config));

  /* Send IPv4 config */
  nm_vpn_service_plugin_set_ip4_config ((NMVpnServicePlugin*) plugin,
                                        g_variant_builder_end (&ip4config));

  /* Send IPv6 config */
  if (addr_info.ip6addr && addr_info.ip6mask)
    nm_vpn_service_plugin_set_ip6_config ((NMVpnServicePlugin*) plugin,
                                          g_variant_builder_end (&ip6config));

  return TRUE;
}

/* Functions to drive AnyConnect *********************************************/

/* Removes n characters from the start of string s */
static gchar *
chopN( gchar *s, size_t n )
{
    char *src = s;

    if (gl.debug)
      g_message("chopN()");

    while ( *src && n ) --n, ++src;
    if ( n == 0 && src != s ) {
      for ( char *dst = s; (  *dst++ = *src++ ); );
    }
    return s;
}

static void
args_add_str_take (GPtrArray *args, char *arg)
{
  nm_assert (args);
  nm_assert (arg);

  g_ptr_array_add (args, arg);
}

static void
_args_add_strv (GPtrArray *args, gboolean accept_optional, guint argn, ...)
{
  va_list ap;
  guint i;
  const char *arg;

  nm_assert (args);
  nm_assert (argn > 0);

  va_start (ap, argn);
  for (i = 0; i < argn; i++) {
    arg = va_arg (ap, const char *);
    if (!arg) {
      /* for convenience for the caller, we allow to pass %NULL with the
       * meaning to skip the argument. */
      nm_assert (accept_optional);
      continue;
    }
    args_add_str_take (args, g_strdup (arg));
  }
  va_end (ap);
}

#define args_add_strv(args, ...)  _args_add_strv (args, FALSE, NM_NARG (__VA_ARGS__), __VA_ARGS__)
#define args_add_strv0(args, ...) _args_add_strv (args, TRUE,  NM_NARG (__VA_ARGS__), __VA_ARGS__)

/* find the AnyConnect binary from the list of paths, a bit overkill */
static const char *
anyconnect_binary_find_exepath (void)
{
  static const char *paths[] = {
    "/opt/cisco/anyconnect/bin/vpn"
  };
  int i;

  if (gl.debug)
    g_message("anyconnect_binary_find_exepath()");

  for (i = 0; i < G_N_ELEMENTS (paths); i++) {
    if (g_file_test (paths[i], G_FILE_TEST_EXISTS))
      return paths[i];
  }
  return NULL;
}

/* This is called when the asynchronously spawned AnyConnect process write to
it's standard output */
static gboolean
anyconnect_stdout_cb (GIOChannel *source, GIOCondition condition, gpointer user_data)
{
  NMVpnServicePlugin *plugin = NM_VPN_SERVICE_PLUGIN (user_data);
  NMAnyconnectPluginPrivate *priv = nm_anyconnect_plugin_get_instance_private((NMAnyconnectPlugin *)plugin);
  gchar *str = NULL;
  char *pos;

  if (!(condition & G_IO_IN))
    return TRUE;

  if (g_io_channel_read_line (source, &str, NULL, NULL, NULL) != G_IO_STATUS_NORMAL)
    return TRUE;

  if (strlen (str) < 1) {
    g_free(str);
    return TRUE;
  }

  if ((pos=strchr(str, '\n')) != NULL)
      *pos = '\0';

  if (!g_str_has_prefix (str, "VPN> "))
    g_message("STDOUT: %s", str);

  if (g_str_has_prefix(str, "  >> state: Connected")) {
    priv->connected = TRUE;
    send_network_config((NMAnyconnectPlugin *)plugin);
    g_message("Network config sent to NetworkManager");
  } else if (g_str_has_prefix(str, "  >> warning: ")) {
    priv->last_warning = chopN(g_strdup(str), 14);
  } else if (g_str_has_prefix(str, "  >> error: ")) {
    priv->last_error = chopN(g_strdup(str), 12);
  }

  g_free(str);
  return TRUE;
}

/* This is called when the AnyConnect process terminates in any way */
static void
anyconnect_watch_cb (GPid pid, gint status, gpointer user_data)
{
  NMVpnServicePlugin *plugin = NM_VPN_SERVICE_PLUGIN (user_data);
  NMAnyconnectPluginPrivate *priv = nm_anyconnect_plugin_get_instance_private((NMAnyconnectPlugin *)plugin);
  guint error = 0;

  if (gl.debug)
    g_message("anyconnect_watch_cb()");

  if (WIFEXITED (status)) {
    error = WEXITSTATUS (status);
    if (error != 0)
      g_warning("AnyConnect exited with error code %d", error);

  }
  else if (WIFSTOPPED (status))
    g_warning("AnyConnect stopped unexpectedly with signal %d", WSTOPSIG (status));
  else if (WIFSIGNALED (status))
    g_warning("AnyConnect died with signal %d", WTERMSIG (status));
  else
    g_warning("AnyConnect died from an unknown cause");

  /* Reap child if needed. */
  waitpid (priv->pid, NULL, WNOHANG);
  priv->pid = 0;
  g_source_remove(priv->socket_channel_stdout_eventid);
  close (g_io_channel_unix_get_fd(priv->stdout_channel));

  /* if we're not now showing as disconnected, see if we can provide a bit more
  feedback in the logs as to what went wrong */
  if (!priv->connected) {
    if (strlen(priv->last_warning) > 0)
      g_warning("%s", priv->last_warning);

    if (strlen(priv->last_error) > 0) {
      g_warning("%s", priv->last_error);
      show_notification((NMAnyconnectPlugin *)plugin, priv->last_error);
    } else if (strlen(priv->last_warning) > 0) {
      show_notification((NMAnyconnectPlugin *)plugin, priv->last_warning);
    }
    nm_vpn_service_plugin_failure ((NMVpnServicePlugin *) plugin, NM_VPN_PLUGIN_FAILURE_CONNECT_FAILED);
  }
}

/* we need to run AnyConnect as the user that has configured it.  We assume that the
   user that configured the connection in NetworkManager is the correct one.  This
   function is called via the g_spawn invocation to execute AnyConnect in a child
   process */
static void
anyconnect_child_setup (gpointer user_data)
{
  NMVpnServicePlugin *plugin = NM_VPN_SERVICE_PLUGIN (user_data);
  NMAnyconnectPluginPrivate *priv = nm_anyconnect_plugin_get_instance_private((NMAnyconnectPlugin *)plugin);
  int e;

  if (gl.debug)
    g_message("anyconnect_child_setup()");

  if (priv->gid != 0) {
    setgroups(1, &priv->gid);

    e = setgid (priv->gid);
    if (e < 0)
      g_warning("Unable to run as group with GID: %d", priv->gid);

    if (priv->uid != 0) {
      e = setuid (priv->uid);
      if (e < 0)
        g_warning("Unable to run as user with UID: %d", priv->uid);
    }
  }

  /* After changing user owner, set env variables.
   * It should be done here and not as a parameter of g_spawn_*,
   * because if the functions above fail, we can capture exactly
   * which user it is. */
  if (!g_setenv ("USER", g_get_real_name (), TRUE))
    g_warning("Unable to set environment USER: %s", g_get_real_name ());

  if (!g_setenv ("HOME", g_get_home_dir (), TRUE))
    g_warning("Unable to set environment HOME: %s", g_get_home_dir ());
}

/* Run the AnyConnect binary with the connection command to start the VPN
connection process */
static gboolean
start_anyconnect_binary_connect (NMAnyconnectPlugin *plugin,
                                 NMConnection *connection,
                                 GError **error)
{
  NMAnyconnectPluginPrivate *priv = nm_anyconnect_plugin_get_instance_private(plugin);
  const char *anyconnect_binary, *host;
  gs_unref_ptrarray GPtrArray *args = NULL;
  GPid pid;
  NMSettingVpn *s_vpn;
  gs_free char *cmd_log = NULL;
  gchar *stdout;
  gint stdout_fd;

  if (gl.debug)
    g_message("start_anyconnect_binary_connect()");

  /* Find anyconnect and start setting up the args to execute*/
  anyconnect_binary = anyconnect_binary_find_exepath ();
  if (!anyconnect_binary) {
    g_set_error_literal (error,
                         NM_VPN_PLUGIN_ERROR,
                         NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
                         _("Could not find the anyconnect binary."));
    return FALSE;
  }

  args = g_ptr_array_new_with_free_func (g_free);
  args_add_strv (args, anyconnect_binary);

  /* Get ready to read the VPN settings */
  s_vpn = nm_connection_get_setting_vpn (connection);
  if (!s_vpn) {
    g_set_error_literal (error,
                         NM_VPN_PLUGIN_ERROR,
                         NM_VPN_PLUGIN_ERROR_INVALID_CONNECTION,
                         _("Could not process the request because the VPN connection settings were invalid."));
    return FALSE;
  }

  /* read the gateway to connect to from the NM VPN settings */
  host = nm_setting_vpn_get_data_item (s_vpn, NM_ANYCONNECT_GATEWAY);
  if (host) {
    args_add_strv (args, "connect", host);
  } else {
    g_set_error_literal (error,
                         NM_VPN_PLUGIN_ERROR,
                         NM_VPN_PLUGIN_ERROR_INVALID_CONNECTION,
                         _("Could not attempt connection, unable to find the setting for the gateway."));
    return FALSE;
  }

  g_ptr_array_add (args, NULL);

  _LOGD ("EXEC: '%s'", (cmd_log = g_strjoinv (" ", (char **) args->pdata)));

  /* reset our understanding of the most recent error/warning coming from AnyConnect */
  priv->last_error = "";
  priv->last_warning = "";

  g_message("Spawning AnyConnect: %s", (cmd_log = g_strjoinv (" ", (char **) args->pdata)));
  /* Spawn with pipes */
  if (!g_spawn_async_with_pipes (NULL, (char **) args->pdata, NULL,
      G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_LEAVE_DESCRIPTORS_OPEN,
      (GSpawnChildSetupFunc)anyconnect_child_setup, plugin,
      &pid, NULL, &stdout_fd, NULL, error))
    return FALSE;

  /* Add a watch for the AnyConnect stdout */
  priv->stdout_channel = g_io_channel_unix_new (stdout_fd);

  /* set encoding on channels */
  g_io_channel_set_encoding (priv->stdout_channel, "UTF-8", NULL);

  /* Set io watches on stdout */
  priv->socket_channel_stdout_eventid = g_io_add_watch (
    priv->stdout_channel,
    G_IO_IN,
    anyconnect_stdout_cb,
    plugin);

  /* Add a watch for the process */
  priv->pid = pid;
  g_child_watch_add (pid, anyconnect_watch_cb, plugin);

  return TRUE;
}

static gboolean
ensure_killed (gpointer data)
{
  int pid = GPOINTER_TO_INT (data);

  if (gl.debug)
    g_message("ensure_killed()");

  if (kill (pid, 0) == 0)
    kill (pid, SIGKILL);

  return FALSE;
}

/* Disconnection Functions ***************************************************/

/* synchronously calls the AnyConect binary to disconnect the VPN */
static gboolean
start_anyconnect_binary_disconnect (NMAnyconnectPlugin *plugin,
                                    GError **error)
{
  NMAnyconnectPluginPrivate *priv = nm_anyconnect_plugin_get_instance_private(plugin);
  const char *anyconnect_binary;
  gs_unref_ptrarray GPtrArray *args = NULL;
  gchar *stdout;
  gchar **stdout_lines, **stdout_line;
  gs_free char *cmd_log = NULL;

  if (gl.debug)
    g_message("start_anyconnect_binary_disconnect()");

  /* Find anyconnect and start setting up the args to execute*/
  anyconnect_binary = anyconnect_binary_find_exepath ();
  if (!anyconnect_binary) {
    g_set_error_literal (error,
                         NM_VPN_PLUGIN_ERROR,
                         NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
                         _("Could not find the anyconnect binary."));
    return FALSE;
  }

  args = g_ptr_array_new_with_free_func (g_free);
  args_add_strv (args, anyconnect_binary, "disconnect");
  g_ptr_array_add (args, NULL);

  /* reset our understanding of the most recent error/warning coming from AnyConnect */
  priv->last_error = "";
  priv->last_warning = "";

  g_message("Spawning AnyConnect: %s", (cmd_log = g_strjoinv (" ", (char **) args->pdata)));
  if (!g_spawn_sync ("/", (char **) args->pdata, NULL, G_SPAWN_DEFAULT,
    (GSpawnChildSetupFunc)anyconnect_child_setup, plugin, &stdout, NULL, NULL, error))
    return FALSE;

  // split the output by line
  stdout_lines = g_strsplit (stdout, "\n", 0);
  g_free (stdout);

  // loop over the lines of the output
  for (stdout_line = stdout_lines; *stdout_line; stdout_line++) {
    char *line;

    /* Remove leading/trailing whitespace (inc newlines) */
    line = g_strstrip(*stdout_line);

    g_message("STDOUT: %s", line);

    if (priv->connected && g_str_has_prefix(*stdout_line, ">> state: Disconnected")) {
      priv->connected = FALSE;
    } else if (g_str_has_prefix(*stdout_line, ">> warning: ")) {
      priv->last_warning = chopN(g_strdup(*stdout_line), 14);
    } else if (g_str_has_prefix(*stdout_line, ">> error: ")) {
      priv->last_error = chopN(g_strdup(*stdout_line), 12);
    }
  }

  if (priv->connected) {
    if (strlen(priv->last_warning) > 0)
      g_warning("%s", priv->last_warning);

    if (strlen(priv->last_error) > 0) {
      g_warning("%s", priv->last_error);
      show_notification(plugin, priv->last_error);
    } else if (strlen(priv->last_warning) > 0) {
      show_notification(plugin, priv->last_warning);
    }
    return FALSE;
  }

  return TRUE;
}



/* Main Functions ************************************************************/

/* Override the disconnect function */
static gboolean
real_disconnect (NMVpnServicePlugin *plugin,
                 GError **error)
{
  NMAnyconnectPluginPrivate *priv = nm_anyconnect_plugin_get_instance_private((NMAnyconnectPlugin *)plugin);

  if (gl.debug)
    g_message("real_disconnect()");

  /* Just in case the previous process is still hanging around */
  if (priv->pid) {
    if (kill (priv->pid, SIGTERM) == 0)
      g_timeout_add (2000, ensure_killed, GINT_TO_POINTER (priv->pid));
    else
      kill (priv->pid, SIGKILL);

    g_message ("Killed AnyConnect process with PID %d.", priv->pid);
    priv->pid = 0;
  }

  if (start_anyconnect_binary_disconnect (NM_ANYCONNECT_PLUGIN (plugin), error)) {
    g_message("Successfully disconnected from VPN");
  } else {
    g_warning("AnyConnect binary did not start (disconnect)");
  }

  return TRUE;
}

/* Override the connection function */
static gboolean
real_connect (NMVpnServicePlugin   *plugin,
              NMConnection  *connection,
              GError       **error)
{
  NMAnyconnectPluginPrivate *priv = nm_anyconnect_plugin_get_instance_private((NMAnyconnectPlugin *)plugin);
  GError *local = NULL;
  NMSettingVpn *s_vpn;
  const char *user_name;

  if (gl.debug)
    g_message("real_connect()");

  /* Get ready to read the VPN settings */
  s_vpn = nm_connection_get_setting_vpn (connection);
  if (!s_vpn) {
    g_set_error_literal (error,
                         NM_VPN_PLUGIN_ERROR,
                         NM_VPN_PLUGIN_ERROR_INVALID_CONNECTION,
                         _("Could not process the request because the VPN connection settings were invalid."));
    return FALSE;
  }

  /* Work out which user ID we need to execute AnyConnect as. */
  user_name = nm_setting_vpn_get_user_name(s_vpn);
  if (user_name) {
    priv->uid = getpwnam(user_name)->pw_uid;
    priv->gid = getpwnam(user_name)->pw_gid;
    g_message("User: %d:%s, group:%d", priv->uid, user_name, priv->gid);
  } else {
    g_warning("Running AnyConnect as UID: %d", priv->uid);
  }

  if (!real_disconnect (plugin, &local)) {
    _LOGW ("Could not clean up previous daemon run: %s", local->message);
    g_error_free (local);
  }

  if (start_anyconnect_binary_connect (NM_ANYCONNECT_PLUGIN (plugin), connection, error)) {
    g_message("AnyConnect binary started (connect)");
  } else {
    g_warning("AnyConnect binary did not start (connect)");
  }

  return TRUE;
}

static void
nm_anyconnect_plugin_init (NMAnyconnectPlugin *plugin)
{
  NMAnyconnectPluginPrivate *priv = nm_anyconnect_plugin_get_instance_private(plugin);
  if (gl.debug)
    g_message("Initialising plugin");
  priv->connected = FALSE;
  priv->pid = 0;
  priv->uid = 0;
  priv->gid = 0;
  priv->last_error = "";
  priv->last_warning = "";
}

static void
nm_anyconnect_plugin_class_init (NMAnyconnectPluginClass *plugin_class)
{
  NMVpnServicePluginClass *parent_class = NM_VPN_SERVICE_PLUGIN_CLASS (plugin_class);

  if (gl.debug)
    g_message("Initialising class");

  /* virtual methods */
  parent_class->connect      = real_connect;
  parent_class->disconnect   = real_disconnect;
}

NMAnyconnectPlugin *
nm_anyconnect_plugin_new (const char *bus_name)
{
  NMAnyconnectPlugin *plugin;
  GError *error = NULL;

  if (gl.debug)
    g_message("Constructing plugin");

  plugin = (NMAnyconnectPlugin *) g_initable_new (NM_TYPE_ANYCONNECT_PLUGIN, NULL, &error,
                                                  NM_VPN_SERVICE_PLUGIN_DBUS_SERVICE_NAME, bus_name,
                                                  NM_VPN_SERVICE_PLUGIN_DBUS_WATCH_PEER, !gl.debug,
                                                  NULL);

  if (!plugin) {
    _LOGW ("Failed to initialize a plugin instance: %s", error->message);
    g_error_free (error);
  }

  return plugin;
}

static gboolean
signal_handler (gpointer user_data)
{
  if (gl.debug)
    g_message("signal_handler()");
  g_main_loop_quit (user_data);
  return G_SOURCE_REMOVE;
}

static void
quit_mainloop (NMVpnServicePlugin *plugin, gpointer user_data)
{
  if (gl.debug)
    g_message("quit_mainloop()");
  g_main_loop_quit ((GMainLoop *) user_data);
}

int
main (int argc, char *argv[])
{
  gs_unref_object NMAnyconnectPlugin *plugin = NULL;
  gboolean persist = FALSE;
  GOptionContext *opt_ctx = NULL;
  gchar *bus_name = NM_DBUS_SERVICE_ANYCONNECT;
  GError *error = NULL;
  GMainLoop *loop;
  guint source_id_sigterm;
  guint source_id_sigint;
  gulong handler_id_plugin = 0;

  GOptionEntry options[] = {
    { "persist", 0, 0, G_OPTION_ARG_NONE, &persist, N_("Don’t quit when VPN connection terminates"), NULL },
    { "debug", 0, 0, G_OPTION_ARG_NONE, &gl.debug, N_("Enable verbose debug logging (may expose passwords)"), NULL },
    { "bus-name", 0, 0, G_OPTION_ARG_STRING, &bus_name, N_("D-Bus name to use for this instance"), NULL },
    {NULL}
  };

#if !GLIB_CHECK_VERSION (2, 35, 0)
  g_type_init ();
#endif

  if (getenv ("ANYCONNECT_DEBUG"))
    gl.debug = TRUE;

  g_message("Starting AnyConnect VPN Plugin");

  /* locale will be set according to environment LC_* variables */
  setlocale (LC_ALL, "");

  bindtextdomain (GETTEXT_PACKAGE, NM_ANYCONNECT_LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  /* Parse options */
  opt_ctx = g_option_context_new (NULL);
  g_option_context_set_translation_domain (opt_ctx, GETTEXT_PACKAGE);
  g_option_context_set_ignore_unknown_options (opt_ctx, FALSE);
  g_option_context_set_help_enabled (opt_ctx, TRUE);
  g_option_context_add_main_entries (opt_ctx, options, NULL);

  g_option_context_set_summary (opt_ctx,
                                _("nm-anyconnect-service provides integrated "
                                  "proprietary Cisco AnyConnect capability to NetworkManager."));

  if (!g_option_context_parse (opt_ctx, &argc, &argv, &error)) {
    g_printerr ("Error parsing the command line options: %s\n", error->message);
    g_option_context_free (opt_ctx);
    g_clear_error (&error);
    return EXIT_FAILURE;
  }
  g_option_context_free (opt_ctx);

  gl.log_level = _nm_utils_ascii_str_to_int64 (getenv ("NM_VPN_LOG_LEVEL"),
                                               10, 0, LOG_DEBUG, -1);
  if (gl.log_level >= 0) {
    if (gl.log_level >= LOG_DEBUG)
      gl.log_level_anyconnect = 10;
    else if (gl.log_level >= LOG_INFO)
      gl.log_level_anyconnect = 5;
    else if (gl.log_level > 0)
      gl.log_level_anyconnect = 2;
    else
      gl.log_level_anyconnect = 1;
  } else if (gl.debug)
    gl.log_level_anyconnect = 10;
  else {
    /* the default level is already "--verb 1", which is fine for us. */
    gl.log_level_anyconnect = -1;
  }

  if (gl.log_level < 0)
    gl.log_level = gl.debug ? LOG_INFO : LOG_NOTICE;

  gl.log_syslog = _nm_utils_ascii_str_to_int64 (getenv ("NM_VPN_LOG_SYSLOG"),
                                                10, 0, 1,
                                                gl.debug ? 0 : 1);

  _LOGD ("nm-anyconnect-service (version " DIST_VERSION ") starting...");

  plugin = nm_anyconnect_plugin_new (bus_name);
  if (!plugin)
    return EXIT_FAILURE;

  loop = g_main_loop_new (NULL, FALSE);

  if (!persist)
    handler_id_plugin = g_signal_connect (plugin, "quit", G_CALLBACK (quit_mainloop), loop);

  signal (SIGPIPE, SIG_IGN);
  source_id_sigterm = g_unix_signal_add (SIGTERM, signal_handler, loop);
  source_id_sigint = g_unix_signal_add (SIGINT, signal_handler, loop);

  g_main_loop_run (loop);

  nm_clear_g_source (&source_id_sigterm);
  nm_clear_g_source (&source_id_sigint);
  nm_clear_g_signal_handler (plugin, &handler_id_plugin);

  g_clear_object (&plugin);

  g_main_loop_unref (loop);
  return EXIT_SUCCESS;
}
