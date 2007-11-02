/* Copyright (c) 2006, Nokia Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of the Nokia Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DBUS_API_SUBJECT_TO_CHANGE
#define DBUS_API_SUBJECT_TO_CHANGE
#endif /*DBUS_API_SUBJECT_TO_CHANGE*/

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <errno.h>
#include <string.h> /* for strlen */
#include <modest-runtime.h>
#include <libgnomevfs/gnome-vfs.h>
#include <tny-fs-stream.h>
#include <tny-camel-account.h>
#include <tny-status.h>
#include <tny-camel-transport-account.h>
#include <tny-camel-imap-store-account.h>
#include <tny-camel-pop-store-account.h>
#include "modest-hildon-includes.h"

#include "modest-maemo-utils.h"
#include "modest-platform.h"

/*
 * For getting and tracking the Bluetooth name
 */
#define BTNAME_SERVICE                  "org.bluez"
#define BTNAME_REQUEST_IF               "org.bluez.Adapter"
#define BTNAME_SIGNAL_IF                "org.bluez.Adapter"
#define BTNAME_REQUEST_PATH             "/org/bluez/hci0"
#define BTNAME_SIGNAL_PATH              "/org/bluez/hci0"

#define BTNAME_REQ_GET                  "GetName"
#define BTNAME_SIG_CHANGED              "NameChanged"

#define BTNAME_MATCH_RULE "type='signal',interface='" BTNAME_SIGNAL_IF \
                          "',member='" BTNAME_SIG_CHANGED "'"


static osso_context_t *__osso_context = NULL; /* urgh global */

osso_context_t *
modest_maemo_utils_get_osso_context (void)
{
	if (!__osso_context) 
		g_warning ("%s: __osso_context == NULL", __FUNCTION__);
	
	return __osso_context;
}

void
modest_maemo_utils_set_osso_context (osso_context_t *osso_context)
{
	g_return_if_fail (osso_context);
	__osso_context = osso_context;
}


GQuark
modest_maemo_utils_get_supported_secure_authentication_error_quark (void)
{
	return g_quark_from_static_string("modest-maemo-utils-get-supported-secure-authentication-error-quark");
}

GtkWidget*
modest_maemo_utils_menubar_to_menu (GtkUIManager *ui_manager)
{
	GtkWidget *main_menu;
	GtkWidget *menubar;
	GList *children, *iter;

	g_return_val_if_fail (ui_manager, NULL);
	
	/* Create new main menu */
	main_menu = gtk_menu_new();

	/* Get the menubar from the UI manager */
	menubar = gtk_ui_manager_get_widget (ui_manager, "/MenuBar");

	iter = children = gtk_container_get_children (GTK_CONTAINER (menubar));
	while (iter) {
		GtkWidget *menu;

		menu = GTK_WIDGET (iter->data);
		gtk_widget_reparent(menu, main_menu);

		iter = g_list_next (iter);
	}
	g_list_free (children);
	return main_menu;
}


static void
update_device_name_from_msg (DBusMessage *message)
{
	DBusError error;
	DBusMessageIter iter;

	dbus_error_init (&error);

	if (dbus_set_error_from_message (&error, message)) {
		g_printerr ("modest: failed to get bluetooth name: %s\n", error.message);
		dbus_error_free (&error);
	} else {
		const gchar *device_name;
		if (!dbus_message_iter_init (message, &iter)) {
			g_printerr ("modest: message did not have argument\n");
			return;
		}
		dbus_message_iter_get_basic (&iter, &device_name);
		modest_conf_set_string (modest_runtime_get_conf(),
					MODEST_CONF_DEVICE_NAME, device_name,
					NULL);
	}
}


static void
on_device_name_received (DBusPendingCall *call, void *user_data)
{
	DBusMessage *message;
	
	g_return_if_fail (dbus_pending_call_get_completed (call));
	
	message = dbus_pending_call_steal_reply (call);
	if (!message) {
		g_printerr ("modest: no reply on device name query\n");
		return;
	}

	update_device_name_from_msg (message);
	dbus_message_unref (message);
}


static DBusHandlerResult
handle_dbus_signal (DBusConnection *conn, DBusMessage *msg, gpointer data)
{
	if (dbus_message_is_signal(msg, BTNAME_SIGNAL_IF, BTNAME_SIG_CHANGED))
		update_device_name_from_msg (msg);

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


static void
get_device_name_from_dbus ()
{
	static DBusConnection *conn = NULL;
	DBusMessage *request;
	DBusError error;
	DBusPendingCall *call = NULL;
	
	dbus_error_init (&error);
	if (!conn) {
		conn = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
		if (!conn) {
			g_printerr ("modest: cannot get on the dbus: %s: %s\n",
				    error.name, error.message);
			dbus_error_free (&error);
			return;
		}
	}
	
	request = dbus_message_new_method_call (BTNAME_SERVICE, BTNAME_REQUEST_PATH,
						BTNAME_REQUEST_IF, BTNAME_REQ_GET);
	if (!request) {
		/* should we free the connection? */
		g_printerr ("modest: dbus_message_new_method_call failed\n");
		return;
	}
	dbus_message_set_auto_start (request, TRUE);
	if (dbus_connection_send_with_reply (conn, request, &call, -1)) {
		dbus_pending_call_set_notify (call, on_device_name_received,
					      NULL, NULL);
		dbus_pending_call_unref (call);
	}
	dbus_message_unref (request);
	
	dbus_connection_setup_with_g_main (conn, NULL);
	dbus_bus_add_match (conn, BTNAME_MATCH_RULE, &error);
	if (dbus_error_is_set(&error)) {
		g_printerr ("modest: dbus_bus_add_match failed: %s\n", error.message);
		dbus_error_free (&error);
	}

	if (!dbus_connection_add_filter(conn, handle_dbus_signal, NULL, NULL))
		g_printerr ("modest: dbus_connection_add_filter failed\n");
}


void
modest_maemo_utils_get_device_name (void)
{
	get_device_name_from_dbus ();
}

gboolean 
modest_maemo_utils_folder_writable (const gchar *filename)
{
	g_return_val_if_fail (filename, FALSE);

	if (!filename)
		return FALSE;
	
	if (g_strncasecmp (filename, "obex", 4) != 0) {
		GnomeVFSFileInfo folder_info;
		gchar *folder;
		folder = g_path_get_dirname (filename);
		gnome_vfs_get_file_info (folder, &folder_info,
					 GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS);
		g_free (folder);
		if (!((folder_info.permissions & GNOME_VFS_PERM_ACCESS_WRITABLE) ||
		      (folder_info.permissions & GNOME_VFS_PERM_USER_WRITE))) {
			return FALSE;
		}
	}
	return TRUE;
}

gboolean 
modest_maemo_utils_file_exists (const gchar *filename)
{
	GnomeVFSURI *uri = NULL;
	gboolean result = FALSE;

	uri = gnome_vfs_uri_new (filename);
	if (uri) {
		result = gnome_vfs_uri_exists (uri);
		gnome_vfs_uri_unref (uri);
	}
	return result;
}

TnyFsStream *
modest_maemo_utils_create_temp_stream (const gchar *orig_name, const gchar *hash_base, gchar **path)
{
	gint fd;
	gchar *filepath = NULL;
	gchar *tmpdir;
	guint hash_number;

	/* hmmm... maybe we need a modest_text_utils_validate_file_name? */
	g_return_val_if_fail (orig_name || strlen(orig_name) == 0, NULL);
	if (strlen(orig_name) > 200) {
		g_warning ("%s: filename too long ('%s')",
			   __FUNCTION__, orig_name);
		return NULL;
	}
	
	if (g_strstr_len (orig_name, strlen(orig_name), "/") != NULL) {
		g_warning ("%s: filename contains '/' character(s) (%s)",
			   __FUNCTION__, orig_name);
		return NULL;
	}
		
	/* make a random subdir under /tmp or /var/tmp */
	if (hash_base != NULL) {
		hash_number = g_str_hash (hash_base);
	} else {
		hash_number = (guint) random ();
	}
	tmpdir = g_strdup_printf ("%s/%u", g_get_tmp_dir (), hash_number);
	if ((g_access (tmpdir, R_OK) == -1) && (g_mkdir (tmpdir, 0755) == -1)) {
		g_warning ("%s: failed to create dir '%s': %s",
			   __FUNCTION__, tmpdir, g_strerror(errno));
		g_free (tmpdir);
		return NULL;
	}

	/* try to write the file there */
	filepath = g_strconcat (tmpdir, "/", orig_name, NULL);
	fd = g_open (filepath, O_CREAT|O_WRONLY|O_TRUNC, 0644);
	if (fd == -1) {
		g_warning ("%s: failed to create '%s': %s",
			   __FUNCTION__, filepath, g_strerror(errno));
		g_free (tmpdir);
		g_free (filepath);
		return NULL;
	}

	g_free (tmpdir);

	if (path)
		*path = filepath;

	return TNY_FS_STREAM (tny_fs_stream_new (fd));
}

typedef struct 
{
	gboolean cancel;
	GList *result;
	GtkWidget* dialog;
	GtkWidget* progress;
	GError* error;
} ModestGetSupportedAuthInfo;

static void on_camel_account_get_supported_secure_authentication_status (
	GObject *self, TnyStatus *status, gpointer user_data)
{
	/*ModestGetSupportedAuthInfo* info = (ModestGetSupportedAuthInfo*) user_data;*/
}

static gboolean
on_idle_secure_auth_finished (gpointer user_data)
{
	ModestGetSupportedAuthInfo *info = (ModestGetSupportedAuthInfo*)user_data;
	/* Operation has finished, close the dialog. Control continues after
	 * gtk_dialog_run in modest_maemo_utils_get_supported_secure_authentication_methods() */

	/* This is a GDK lock because we are an idle callback and
	 * the code below is or does Gtk+ code */

	gdk_threads_enter(); /* CHECKED */
	gtk_dialog_response (GTK_DIALOG (info->dialog), GTK_RESPONSE_ACCEPT);
	gdk_threads_leave(); /* CHECKED */

	return FALSE;
}

static void
on_camel_account_get_supported_secure_authentication (
  TnyCamelAccount *self, gboolean cancelled,
  TnyList *auth_types, GError *err, 
  gpointer user_data)
{
	ModestGetSupportedAuthInfo *info = (ModestGetSupportedAuthInfo*)user_data;
	g_return_if_fail (info);
	

	/* Free everything if the actual action was canceled */
	if (info->cancel)
	{
		/* The operation was canceled and the ownership of the info given to us
		 * so that we could still check the cancel flag. */
		g_slice_free (ModestGetSupportedAuthInfo, info);
		info = NULL;
	}
	else
	{
		if (err)
		{
			if (info->error) {
				g_error_free (info->error);
				info->error = NULL;
			}
			
			info->error = g_error_copy (err);
		}

		if (!auth_types) {
			g_warning ("DEBUG: %s: auth_types is NULL.\n", __FUNCTION__);
		}
		else if (tny_list_get_length(auth_types) == 0) {
			g_warning ("DEBUG: %s: auth_types is an empty TnyList.\n", __FUNCTION__);
		} else
		{
			ModestPairList* pairs = modest_protocol_info_get_auth_protocol_pair_list ();
  
			/* Get the enum value for the strings: */
			GList *result = NULL;
			TnyIterator* iter = tny_list_create_iterator(auth_types);
			while (!tny_iterator_is_done(iter)) {
				TnyPair *pair = TNY_PAIR(tny_iterator_get_current(iter));
				const gchar *auth_name = NULL;
				if (pair) {
					auth_name = tny_pair_get_name(pair);
					g_object_unref (pair);
					pair = NULL;
				}

				printf("DEBUG: %s: auth_name=%s\n", __FUNCTION__, auth_name);

				ModestAuthProtocol proto = modest_protocol_info_get_auth_protocol (auth_name);
				if(proto != MODEST_PROTOCOL_AUTH_NONE)
						result = g_list_prepend(result, GINT_TO_POINTER(proto));

				tny_iterator_next(iter);
			}
			g_object_unref (iter);

			modest_pair_list_free (pairs);
	
			info->result = result;
		}

		printf("DEBUG: finished\n");
				
		/* Close the dialog in a main thread */
		g_idle_add(on_idle_secure_auth_finished, info);
	}
}

static void on_secure_auth_cancel(GtkWidget* dialog, int response, gpointer user_data)
{
	if(response == GTK_RESPONSE_REJECT || response == GTK_RESPONSE_DELETE_EVENT)
	{
		ModestGetSupportedAuthInfo *info = (ModestGetSupportedAuthInfo*)user_data;
		g_return_if_fail(info);
		/* This gives the ownership of the info to the worker thread. */
		info->result = NULL;
		info->cancel = TRUE;
	}
}

GList*
modest_maemo_utils_get_supported_secure_authentication_methods (ModestTransportStoreProtocol proto, 
	const gchar* hostname, gint port, const gchar* username, GtkWindow *parent_window, GError** error)
{
	g_return_val_if_fail (proto != MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN, NULL);
	
	/* We need a connection to get the capabilities; */
	if (!modest_platform_connect_and_wait (GTK_WINDOW (parent_window), NULL))
		return NULL;
	 
	/*
	result = g_list_append (result, GINT_TO_POINTER (MODEST_PROTOCOL_AUTH_CRAMMD5));
	*/
	
	/* Create a TnyCamelAccount so we can use 
	 * tny_camel_account_get_supported_secure_authentication(): */
	TnyAccount * tny_account = NULL;
	switch (proto) {
	case MODEST_PROTOCOL_TRANSPORT_SENDMAIL:
	case MODEST_PROTOCOL_TRANSPORT_SMTP:
		tny_account = TNY_ACCOUNT(tny_camel_transport_account_new ()); break;
	case MODEST_PROTOCOL_STORE_POP:
		tny_account = TNY_ACCOUNT(tny_camel_pop_store_account_new ()); break;
	case MODEST_PROTOCOL_STORE_IMAP:
		tny_account = TNY_ACCOUNT(tny_camel_imap_store_account_new ()); break;
	case MODEST_PROTOCOL_STORE_MAILDIR:
	case MODEST_PROTOCOL_STORE_MBOX:
		tny_account = TNY_ACCOUNT(tny_camel_store_account_new()); break;
	default:
		tny_account = NULL;
	}

	
	if (!tny_account) {
		g_printerr ("%s could not create tny account.", __FUNCTION__);
		return NULL;
	}
	
	/* Set proto, so that the prepare_func() vfunc will work when we call 
	 * set_session(): */
	 /* TODO: Why isn't this done in account_new()? */
	tny_account_set_proto (tny_account,
			       modest_protocol_info_get_transport_store_protocol_name(proto));

	tny_account_set_hostname (tny_account, hostname);
	/* Required for POP, at least */
	tny_account_set_user (tny_account, username);
			       
	if(port > 0)
		tny_account_set_port (tny_account, port);
		
	/* Set the session for the account, so we can use it: */
	ModestTnyAccountStore *account_store = modest_runtime_get_account_store ();
	TnySessionCamel *session = 
		modest_tny_account_store_get_session (TNY_ACCOUNT_STORE (account_store));
	g_return_val_if_fail (session, NULL);
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT(tny_account), session);
	
	
	/* Ask camel to ask the server, asynchronously: */
	ModestGetSupportedAuthInfo *info = g_slice_new (ModestGetSupportedAuthInfo);
	info->result = NULL;
	info->cancel = FALSE;
	info->error = NULL;
	info->progress = gtk_progress_bar_new();
	/* TODO: Need logical_ID for the title: */
	info->dialog = gtk_dialog_new_with_buttons(_("Authentication"),
	                                           parent_window, GTK_DIALOG_MODAL,
	                                           _("mcen_bd_dialog_cancel"),
	                                           GTK_RESPONSE_REJECT,
	                                           NULL);
	//gtk_window_set_default_size(GTK_WINDOW(info->dialog), 300, 100);
	
	g_signal_connect(G_OBJECT(info->dialog), "response", G_CALLBACK(on_secure_auth_cancel), info);
	
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(info->dialog)->vbox),
	                  gtk_label_new("Checking for supported authentication types..."));
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(info->dialog)->vbox), info->progress);
	gtk_widget_show_all(info->dialog);
	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(info->progress));
	
	printf ("DEBUG: %s: STARTING.\n", __FUNCTION__);
	tny_camel_account_get_supported_secure_authentication (
		TNY_CAMEL_ACCOUNT (tny_account),
		on_camel_account_get_supported_secure_authentication,
		on_camel_account_get_supported_secure_authentication_status,
		info);

	gtk_dialog_run (GTK_DIALOG (info->dialog));
	
	gtk_widget_destroy(info->dialog);
			
	GList *result = info->result;
	if (!info->cancel)
	{
		if (info->error) {
			gchar * debug_url_string = tny_account_get_url_string  (tny_account);
			g_warning ("DEBUG: %s:\n  error: %s\n  account url: %s", __FUNCTION__, info->error->message, 
				debug_url_string);
			g_free (debug_url_string);
			
			g_propagate_error(error, info->error);
			info->error = NULL;
		}

		g_slice_free (ModestGetSupportedAuthInfo, info);
		info = NULL;
	}
	else
	{
		// Tell the caller that the operation was canceled so it can
		// make a difference
		g_set_error(error,
		            modest_maemo_utils_get_supported_secure_authentication_error_quark(),
		            MODEST_MAEMO_UTILS_GET_SUPPORTED_SECURE_AUTHENTICATION_ERROR_CANCELED,
			    "User has canceled query");
	}

	return result;
}

void
modest_maemo_utils_setup_images_filechooser (GtkFileChooser *chooser)
{
	gchar *images_folder;
	GtkFileFilter *file_filter;
	GList *image_mimetypes_list;
	GList *node;

	g_return_if_fail (GTK_IS_FILE_CHOOSER (chooser));

	/* Set the default folder to images folder */
	images_folder = g_build_filename (g_get_home_dir (), 
					  MODEST_MAEMO_UTILS_MYDOCS_FOLDER,
					  MODEST_MAEMO_UTILS_DEFAULT_IMAGE_FOLDER, NULL);
	gtk_file_chooser_set_current_folder (chooser, images_folder);
	g_free (images_folder);

	/* Set the images mime filter */
	file_filter = gtk_file_filter_new ();
#ifdef MODEST_HAVE_HILDON0_WIDGETS
	image_mimetypes_list = osso_mime_get_mime_types_for_category (OSSO_MIME_CATEGORY_IMAGES);
#else
	image_mimetypes_list = hildon_mime_get_mime_types_for_category (HILDON_MIME_CATEGORY_IMAGES);
#endif
	for (node = image_mimetypes_list; node != NULL; node = g_list_next (node)) {
		gtk_file_filter_add_mime_type (file_filter, node->data);
	}
	gtk_file_chooser_set_filter (chooser, file_filter);
#ifdef MODEST_HAVE_HILDON0_WIDGETS
	osso_mime_types_list_free (image_mimetypes_list);
#else
	hildon_mime_types_list_free (image_mimetypes_list);
#endif

}

#if 0
static void
on_hide (GtkDialog *dialog, gpointer user_data)
{
	/* Just destroy the dialog: */
	gtk_widget_destroy (GTK_WIDGET (dialog));
}
#endif

#if 0 /* Not used now. */
/* user_data for the idle callback: */
typedef struct 
{
	GtkWindow *parent_window;
	gchar *message;
} ModestIdleNoteInfo;

static gboolean
on_idle_show_information(gpointer user_data)
{
	ModestIdleNoteInfo *info = (ModestIdleNoteInfo*)user_data;
	
	modest_maemo_show_information_note_and_forget (info->parent_window, info->message);
	
	g_free (info->message);
	g_slice_free (ModestIdleNoteInfo, info);
	
	return FALSE; /* Don't call this again. */
}

void 
modest_maemo_show_information_note_in_main_context_and_forget (GtkWindow *parent_window, 
							       const gchar* message)
{
	ModestIdleNoteInfo *info = g_slice_new (ModestIdleNoteInfo);
	info->parent_window = parent_window;
	info->message = g_strdup (message);
	
	g_idle_add (on_idle_show_information, info);
}
#endif

void 
modest_maemo_show_dialog_and_forget (GtkWindow *parent_window, 
				     GtkDialog *dialog)
{
	gtk_window_set_transient_for (GTK_WINDOW (dialog), parent_window);
	
	/* Destroy the dialog when it is closed: */
	g_signal_connect_swapped (dialog, 
				  "response", 
				  G_CALLBACK (gtk_widget_destroy), 
				  dialog);

	gtk_widget_show (GTK_WIDGET (dialog));
}



void
modest_maemo_set_thumbable_scrollbar (GtkScrolledWindow *win, 
				      gboolean thumbable)
{
	g_return_if_fail (GTK_IS_SCROLLED_WINDOW(win));
#ifdef MODEST_HAVE_HILDON1_WIDGETS		
	hildon_helper_set_thumb_scrollbar (win, thumbable);
#endif /* MODEST_HAVE_HILDON1_WIDGETS */
}

void
modest_maemo_toggle_action_set_active_block_notify (GtkToggleAction *action, gboolean value)
{
	GSList *proxies = NULL;

	g_return_if_fail (GTK_IS_TOGGLE_ACTION (action));

	for (proxies = gtk_action_get_proxies (GTK_ACTION (action));
	     proxies != NULL; proxies = g_slist_next (proxies)) {
		GtkWidget *widget = (GtkWidget *) proxies->data;
		gtk_action_block_activate_from (GTK_ACTION (action), widget);
	}

	gtk_toggle_action_set_active (action, value);

	for (proxies = gtk_action_get_proxies (GTK_ACTION (action));
	     proxies != NULL; proxies = g_slist_next (proxies)) {
		GtkWidget *widget = (GtkWidget *) proxies->data;
		gtk_action_unblock_activate_from (GTK_ACTION (action), widget);
	}

}



FILE*
modest_maemo_open_mcc_mapping_file (void)
{
	FILE* result;
	
	/* Load the file one line at a time: */
#ifdef MODEST_HILDON_VERSION_0
	const gchar* filepath = PROVIDER_DATA_DIR "/mcc_mapping";
#else
	const gchar* filepath = "/usr/share/operator-wizard/mcc_mapping";
#endif /*MODEST_HILDON_VERSION_0*/
	
	result = fopen (filepath, "r");	
	if (!result) 
		g_printerr ("modest: failed to open mcc mapping file");
	
	return result;
}


