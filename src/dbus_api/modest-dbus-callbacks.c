/* Copyright (c) 2007, Nokia Corporation
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
 
#include "modest-dbus-callbacks.h"
#include "modest-runtime.h"
#include "modest-account-mgr.h"
#include "modest-account-mgr-helpers.h"
#include "modest-tny-account.h"
#include "modest-tny-folder.h"
#include "modest-ui-actions.h"
#include "modest-debug.h"
#include "modest-search.h"
#include "widgets/modest-msg-edit-window.h"
#include "modest-tny-msg.h"
#include "modest-platform.h"
#include <libmodest-dbus-client/libmodest-dbus-client.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <stdio.h>
#include <string.h>
#include <glib/gstdio.h>
#ifdef MODEST_HAVE_HILDON0_WIDGETS
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#else
#include <libgnomevfs/gnome-vfs-mime.h>
#endif
#include <tny-fs-stream.h>

#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-simple-list.h>
#include <tny-merge-folder.h>
#include <tny-account.h>

#include <modest-text-utils.h>

typedef struct 
{
	gchar *to;
 	gchar *cc;
 	gchar *bcc;
 	gchar *subject;
 	gchar *body;
	gchar *attachments;
} ComposeMailIdleData;


static gboolean notify_error_in_dbus_callback (gpointer user_data);
static gboolean notify_msg_not_found_in_idle (gpointer user_data);
static gboolean on_idle_compose_mail(gpointer user_data);
static gboolean on_idle_top_application (gpointer user_data);

/** uri_unescape:
 * @uri An escaped URI. URIs should always be escaped.
 * @len The length of the @uri string, or -1 if the string is null terminated.
 * 
 * Decode a URI, or URI fragment, as per RFC 1738.
 * http://www.ietf.org/rfc/rfc1738.txt
 * 
 * Return value: An unescaped string. This should be freed with g_free().
 */
static gchar* 
uri_unescape(const gchar* uri, size_t len)
{
	if (!uri)
		return NULL;
		
	if (len == -1)
		len = strlen (uri);
	
	/* Allocate an extra string so we can be sure that it is null-terminated,
	 * so we can use gnome_vfs_unescape_string().
	 * This is not efficient. */
	gchar * escaped_nullterminated = g_strndup (uri, len);
	gchar *result = gnome_vfs_unescape_string (escaped_nullterminated, NULL);
	g_free (escaped_nullterminated);
	
	return result;
}

/** uri_parse_mailto:
 * @mailto A mailto URI, with the mailto: prefix.
 * @list_items_and_values: A pointer to a list that should be filled with item namesand value strings, 
 * with each name item being followed by a value item. This list should be freed with g_slist_free) after 
 * all the string items have been freed. This parameter may be NULL.
 * Parse a mailto URI as per RFC2368.
 * http://www.ietf.org/rfc/rfc2368.txt
 * 
 * Return value: The to address, unescaped. This should be freed with g_free().
 */
static gchar* 
uri_parse_mailto (const gchar* mailto, GSList** list_items_and_values)
{
	/* The URL must begin with mailto: */
	if (strncmp (mailto, "mailto:", 7) != 0) {
		return NULL;
	}
	const gchar* start_to = mailto + 7;

	/* Look for ?, or the end of the string, marking the end of the to address: */
	const size_t len_to = strcspn (start_to, "?");
	gchar* result_to = uri_unescape (start_to, len_to);
	printf("debug: result_to=%s\n", result_to);

	if (list_items_and_values == NULL) {
		return result_to;
	}

	/* Get any other items: */
	const size_t len_mailto = strlen (start_to);
	const gchar* p = start_to + len_to + 1; /* parsed so far. */
	const gchar* end = start_to + len_mailto;
	while (p < end) {
		const gchar *name, *value, *name_start, *name_end, *value_start, *value_end;
		name_start = p;
		name_end = strchr (name_start, '='); /* Separator between name and value */
		if (name_end == NULL) {
			g_debug ("Malformed URI: %s\n", mailto);
			return result_to;
		}
		value_start = name_end + 1;
		value_end = strchr (value_start, '&'); /* Separator between value and next parameter */

		name = g_strndup(name_start, name_end - name_start);
		if (value_end != NULL) {
			value = uri_unescape(value_start, value_end - value_start);
			p = value_end + 1;
		} else {
			value = uri_unescape(value_start, -1);
			p = end;
		}
		*list_items_and_values = g_slist_append (*list_items_and_values, (gpointer) name);
		*list_items_and_values = g_slist_append (*list_items_and_values, (gpointer) value);
	}
	
	return result_to;
}

static gboolean
check_and_offer_account_creation()
{
	gboolean result = TRUE;
	
	/* This is called from idle handlers, so lock gdk: */
	gdk_threads_enter ();
	
	if (!modest_account_mgr_has_accounts(modest_runtime_get_account_mgr(), TRUE)) {
		const gboolean created = modest_ui_actions_run_account_setup_wizard (NULL);
		if (!created) {
			g_debug ("modest: %s: no account exists even after offering, "
				 "or account setup was already underway.\n", __FUNCTION__);
			result = FALSE;
		}
	}
	
	gdk_threads_leave ();
	
	return result;
}

static gboolean
on_idle_mail_to(gpointer user_data)
{
	gchar *uri = (gchar*)user_data;
	GSList *list_names_and_values = NULL;
	gchar *to = NULL;
	const gchar *cc = NULL;
	const gchar *bcc = NULL;
	const gchar *subject = NULL;
	const gchar *body = NULL;

	if (!check_and_offer_account_creation ()) {
		g_idle_add (notify_error_in_dbus_callback, NULL);
		goto cleanup;
	}

	/* Get the relevant items from the list: */
	to = uri_parse_mailto (uri, &list_names_and_values);
	GSList *list = list_names_and_values;
	while (list) {
		GSList *list_value = g_slist_next (list);
		const gchar * name = (const gchar*)list->data;
		const gchar * value = (const gchar*)list_value->data;

		if (strcmp (name, "cc") == 0) {
			cc = value;
		} else if (strcmp (name, "bcc") == 0) {
			bcc = value;
		} else if (strcmp (name, "subject") == 0) {
			subject = value;
		} else if (strcmp (name, "body") == 0) {
			body = value;
		}

		list = g_slist_next (list_value);
	}

	gdk_threads_enter (); /* CHECKED */
	modest_ui_actions_compose_msg(NULL, to, cc, bcc, subject, body, NULL);
	gdk_threads_leave (); /* CHECKED */

cleanup:
	/* Free the to: and the list, as required by uri_parse_mailto() */
	g_free(to);
	g_slist_foreach (list_names_and_values, (GFunc)g_free, NULL);
	g_slist_free (list_names_and_values);

	g_free(uri);

	return FALSE; /* Do not call this callback again. */
}

static gint 
on_mail_to(GArray * arguments, gpointer data, osso_rpc_t * retval)
{
 	osso_rpc_t val;
 	gchar *uri;

	/* Get arguments */
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DBUS_MAIL_TO_ARG_URI);
 	uri = g_strdup (val.value.s);
 	
 	g_idle_add(on_idle_mail_to, (gpointer)uri);
 	
 	/* Note that we cannot report failures during sending, 
 	 * because that would be asynchronous. */
 	return OSSO_OK;
}


static gboolean
on_idle_compose_mail(gpointer user_data)
{
	GSList *attachments = NULL;
	ComposeMailIdleData *idle_data = (ComposeMailIdleData*)user_data;

	if (!check_and_offer_account_creation ()) {
		g_idle_add (notify_error_in_dbus_callback, NULL);
		goto cleanup;
	}

	/* it seems Sketch at least sends a leading ',' -- take that into account,
	 * ie strip that ,*/
	if (idle_data->attachments && idle_data->attachments[0]==',') {
		gchar *tmp = g_strdup (idle_data->attachments + 1);
		g_free(idle_data->attachments);
		idle_data->attachments = tmp;
	}
	if (idle_data->attachments != NULL) {
		gchar **list = g_strsplit(idle_data->attachments, ",", 0);
		gint i = 0;
		for (i=0; list[i] != NULL; i++) {
			attachments = g_slist_append(attachments, g_strdup(list[i]));
		}
		g_strfreev(list);
	}
	gdk_threads_enter (); /* CHECKED */
	modest_ui_actions_compose_msg(NULL, idle_data->to, idle_data->cc,
				      idle_data->bcc, idle_data->subject,
				      idle_data->body, attachments);
	gdk_threads_leave (); /* CHECKED */
cleanup:
	g_slist_foreach(attachments, (GFunc)g_free, NULL);
	g_slist_free(attachments);
	g_free (idle_data->to);
	g_free (idle_data->cc);
	g_free (idle_data->bcc);
	g_free (idle_data->subject);
	g_free (idle_data->body);
	g_free (idle_data->attachments);
	g_free(idle_data);

	return FALSE; /* Do not call this callback again. */
}

static gint 
on_compose_mail(GArray * arguments, gpointer data, osso_rpc_t * retval)
{
	ComposeMailIdleData *idle_data;
	osso_rpc_t val;
     	
 	idle_data = g_new0(ComposeMailIdleData, 1); /* Freed in the idle callback. */
 	
	/* Get the arguments: */
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DBUS_COMPOSE_MAIL_ARG_TO);
 	idle_data->to = g_strdup (val.value.s);
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DBUS_COMPOSE_MAIL_ARG_CC);
 	idle_data->cc = g_strdup (val.value.s);
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DBUS_COMPOSE_MAIL_ARG_BCC);
 	idle_data->bcc = g_strdup (val.value.s);
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DBUS_COMPOSE_MAIL_ARG_SUBJECT);
 	idle_data->subject = g_strdup (val.value.s);
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DBUS_COMPOSE_MAIL_ARG_BODY);
 	idle_data->body = g_strdup (val.value.s);
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DBUS_COMPOSE_MAIL_ARG_ATTACHMENTS);
 	idle_data->attachments = g_strdup (val.value.s);

	/* Use g_idle to context-switch into the application's thread: */
  	g_idle_add(on_idle_compose_mail, (gpointer)idle_data);
 	
 	return OSSO_OK;
}

static TnyMsg *
find_message_by_url (const char *uri,  TnyAccount **ac_out)
{
	ModestTnyAccountStore *astore;
	TnyAccount *account = NULL;
	TnyFolder *folder = NULL;
	TnyMsg *msg = NULL;

	astore = modest_runtime_get_account_store ();
	
	if (astore == NULL)
		return NULL;

	if (uri && g_str_has_prefix (uri, "merge://")) {
		/* we assume we're talking about outbox folder, as this 
		 * is the only merge folder we work with in modest */
		return modest_tny_account_store_find_msg_in_outboxes (astore, uri, ac_out);
	}
	account = tny_account_store_find_account (TNY_ACCOUNT_STORE (astore),
						  uri);
	
	if (account == NULL || !TNY_IS_STORE_ACCOUNT (account))
		goto out;
	*ac_out = account;

	folder = tny_store_account_find_folder (TNY_STORE_ACCOUNT (account), uri, NULL);

	if (folder == NULL)
		goto out;
	
	msg = tny_folder_find_msg (folder, uri, NULL);
	
out:
	if (account && !msg) {
		g_object_unref (account);
		*ac_out = NULL;
	}
	if (folder)
		g_object_unref (folder);

	return msg;
}

typedef struct {
	TnyAccount *account;
	gchar *uri;
	gboolean connect;
	guint animation_timeout;
	GtkWidget *animation;
} OpenMsgPerformerInfo;

static gboolean
on_show_opening_animation (gpointer userdata)
{
	OpenMsgPerformerInfo *info = (OpenMsgPerformerInfo *) userdata;
	info->animation = modest_platform_animation_banner (NULL, NULL, _("mail_me_opening"));
	info->animation_timeout = 0;
	
	return FALSE;
}

static gboolean
on_hide_opening_animation (gpointer userdata)
{
	OpenMsgPerformerInfo *info = (OpenMsgPerformerInfo *) userdata;

	if (info->animation_timeout>0) {
		g_source_remove (info->animation_timeout);
		info->animation_timeout = 0;
	}

	if (info->animation) {
		gtk_widget_destroy (info->animation);
		info->animation = NULL;
	}

	g_slice_free (OpenMsgPerformerInfo, info);
	return FALSE;
}

static void 
on_open_message_performer (gboolean canceled, 
			   GError *err,
			   GtkWindow *parent_window, 
			   TnyAccount *account, 
			   gpointer user_data)
{
	OpenMsgPerformerInfo *info;
	gchar *uri;
	TnyHeader *header;
	gchar *msg_uid;
	ModestWindowMgr *win_mgr;
	ModestWindow *msg_view = NULL;
	gboolean is_draft = FALSE;
	TnyFolder *folder = NULL;
	TnyMsg *msg = NULL;

	info = (OpenMsgPerformerInfo *) user_data;
	uri = info->uri;

	if (canceled || err) {
		goto frees;
	}

	/* Get folder */
	if (!account) {
		ModestTnyAccountStore *account_store;
		ModestTnyLocalFoldersAccount *local_folders_account;
		
		account_store = modest_runtime_get_account_store ();
		local_folders_account = MODEST_TNY_LOCAL_FOLDERS_ACCOUNT (
			modest_tny_account_store_get_local_folders_account (account_store));
		folder = modest_tny_local_folders_account_get_merged_outbox (local_folders_account);		
	} else {
		folder = tny_store_account_find_folder (TNY_STORE_ACCOUNT (account), uri, NULL);
	}
	if (modest_tny_folder_is_local_folder (folder) &&
	    (modest_tny_folder_get_local_or_mmc_folder_type (folder) == TNY_FOLDER_TYPE_DRAFTS)) {
		is_draft = TRUE;
	}

	/* Get message */
	msg = tny_folder_find_msg (folder, uri, NULL);
	if (!msg) {
		g_idle_add (notify_msg_not_found_in_idle, NULL);
		goto frees;
	}

	header = tny_msg_get_header (msg);
	if (header && (tny_header_get_flags (header)&TNY_HEADER_FLAG_DELETED)) {
		g_object_unref (header);
		g_object_unref (msg);
		g_idle_add (notify_msg_not_found_in_idle, NULL);
		goto frees;
	}
	msg_uid =  modest_tny_folder_get_header_unique_id (header); 
	win_mgr = modest_runtime_get_window_mgr ();

	if (modest_window_mgr_find_registered_header (win_mgr, header, &msg_view)) {
		gtk_window_present (GTK_WINDOW(msg_view));
	} else {
		const gchar *modest_account_name;

		/* g_debug ("creating new window for this msg"); */
		modest_window_mgr_register_header (win_mgr, header, NULL);

		if (account) {
			modest_account_name = 
				modest_tny_account_get_parent_modest_account_name_for_server_account (account);
		} else {
			modest_account_name = NULL;
		}
			
		/* Drafts will be opened in the editor, and others will be opened in the viewer */
		if (is_draft) {
			msg_view = modest_msg_edit_window_new (msg, modest_account_name, TRUE);
		} else {
			TnyHeader *header;
			header = tny_msg_get_header (msg);
			msg_view = modest_msg_view_window_new_for_search_result (msg, modest_account_name, msg_uid);
			if (! (tny_header_get_flags (header) & TNY_HEADER_FLAG_SEEN))
				tny_header_set_flag (header, TNY_HEADER_FLAG_SEEN);
			g_object_unref (header);
			
		}		
		modest_window_mgr_register_window (win_mgr, msg_view);
		gtk_widget_show_all (GTK_WIDGET (msg_view));
	}
	g_object_unref (header);
	g_object_unref (msg);
	g_object_unref (folder);

 frees:
	g_free (info->uri);
	if (info->account)
		g_object_unref (info->account);
	g_idle_add (on_hide_opening_animation, info);
}

static gboolean
on_idle_open_message_performer (gpointer user_data)
{
	ModestWindow *main_win = NULL;
	OpenMsgPerformerInfo *info = (OpenMsgPerformerInfo *) user_data;

	main_win = modest_window_mgr_get_main_window (modest_runtime_get_window_mgr(),
						      FALSE); /* don't create */

	/* Lock before the call as we're in an idle handler */
	gdk_threads_enter ();
	if (info->connect) {
		modest_platform_connect_and_perform (GTK_WINDOW (main_win), TRUE, info->account, 
						     on_open_message_performer, info);
	} else {
		on_open_message_performer (FALSE, NULL, GTK_WINDOW (main_win), info->account, info);
	}
	gdk_threads_leave ();

	return FALSE;
}

static gint 
on_open_message (GArray * arguments, gpointer data, osso_rpc_t * retval)
{
 	osso_rpc_t val;
 	gchar *uri;
	TnyAccount *account = NULL;
	gint osso_retval;
	gboolean is_merge;

	/* Get the arguments: */
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DBUS_OPEN_MESSAGE_ARG_URI);
 	uri = g_strdup (val.value.s);

	is_merge = g_str_has_prefix (uri, "merge:");

	/* Get the account */
	if (!is_merge)
		account = tny_account_store_find_account (TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()),
							  uri);

 	
	if (is_merge || account) {
		OpenMsgPerformerInfo *info;
		TnyFolder *folder = NULL;

		info = g_slice_new0 (OpenMsgPerformerInfo);
		if (account) 
			info->account = g_object_ref (account);
		info->uri = uri;
		info->connect = TRUE;
		info->animation_timeout = g_timeout_add (1000, on_show_opening_animation, info);

		/* Try to get the message, if it's already downloaded
		   we don't need to connect */
		if (account) {
			folder = tny_store_account_find_folder (TNY_STORE_ACCOUNT (account), uri, NULL);
		} else {
			ModestTnyAccountStore *account_store;
			ModestTnyLocalFoldersAccount *local_folders_account;

			account_store = modest_runtime_get_account_store ();
			local_folders_account = MODEST_TNY_LOCAL_FOLDERS_ACCOUNT (
				modest_tny_account_store_get_local_folders_account (account_store));
			folder = modest_tny_local_folders_account_get_merged_outbox (local_folders_account);
		}
		if (folder) {
			TnyMsg *msg = tny_folder_find_msg (folder, uri, NULL);
			if (msg) {
				info->connect = FALSE;
				g_object_unref (msg);
			}
			g_object_unref (folder);
		}

		/* We need to call it into an idle to get
		   modest_platform_connect_and_perform into the main
		   loop */
		g_idle_add (on_idle_open_message_performer, info);
		osso_retval = OSSO_OK;
	} else {
		g_free (uri);
		osso_retval = OSSO_ERROR; 
		g_idle_add (notify_error_in_dbus_callback, NULL);
 	}

	if (account)
		g_object_unref (account);
	return osso_retval;
}

static gboolean
on_idle_delete_message (gpointer user_data)
{
	TnyList *headers = NULL;
	TnyFolder *folder = NULL;
	TnyIterator *iter = NULL; 
	TnyHeader *header = NULL, *msg_header = NULL;
	TnyMsg *msg = NULL;
	TnyAccount *account = NULL;
	const char *uri = NULL, *uid = NULL;
	gint res = 0;
	ModestMailOperation *mail_op = NULL;
	ModestWindow *main_win = NULL, *msg_view = NULL;

	uri = (char *) user_data;
	
	msg = find_message_by_url (uri, &account);

	if (!msg) {
		g_warning ("%s: Could not find message '%s'", __FUNCTION__, uri);
		g_idle_add (notify_error_in_dbus_callback, NULL);
		return OSSO_ERROR; 
	}
	
	main_win = modest_window_mgr_get_main_window (modest_runtime_get_window_mgr(),
						      FALSE); /* don't create */
	
	msg_header = tny_msg_get_header (msg);
	uid = tny_header_get_uid (msg_header);
	folder = tny_msg_get_folder (msg);

	if (!folder) {
		g_warning ("%s: Could not find folder (uri:'%s')", __FUNCTION__, uri);
		g_object_unref (msg);
		g_idle_add (notify_error_in_dbus_callback, NULL);
		return OSSO_ERROR; 
	}

	headers = tny_simple_list_new ();
	tny_folder_get_headers (folder, headers, TRUE, NULL);
	iter = tny_list_create_iterator (headers);
	header = NULL;

	while (!tny_iterator_is_done (iter)) {
		const char *cur_id = NULL;

		header = TNY_HEADER (tny_iterator_get_current (iter));
		if (header)
			cur_id = tny_header_get_uid (header);
		
		if (cur_id && uid && g_str_equal (cur_id, uid)) {
			/* g_debug ("Found corresponding header from folder"); */
			break;
		}

		if (header) {
			g_object_unref (header);
			header = NULL;
		}
		
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	iter = NULL;
	g_object_unref (headers);
	headers = NULL;
	
	g_object_unref (msg_header);
	msg_header = NULL;
	g_object_unref (msg);
	msg = NULL;

	if (header == NULL) {
		if (folder)
			g_object_unref (folder);
		g_idle_add (notify_error_in_dbus_callback, NULL);			
		return OSSO_ERROR;
	}
		
	res = OSSO_OK;

	/* This is a GDK lock because we are an idle callback and
	 * the code below is or does Gtk+ code */
	gdk_threads_enter (); /* CHECKED */

	mail_op = modest_mail_operation_new (main_win ? G_OBJECT(main_win) : NULL);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_op);
	modest_mail_operation_remove_msg (mail_op, header, FALSE);
	g_object_unref (G_OBJECT (mail_op));
	
	if (main_win) { /* no need if there's no window */ 
		if (modest_window_mgr_find_registered_header (modest_runtime_get_window_mgr(),
							      header, &msg_view)) {
			if (MODEST_IS_MSG_VIEW_WINDOW (msg_view))
				modest_ui_actions_refresh_message_window_after_delete (MODEST_MSG_VIEW_WINDOW (msg_view));
		}
	}
	gdk_threads_leave (); /* CHECKED */
	
	if (header)
		g_object_unref (header);
	
	if (folder) {
		/* Trick: do a poke status in order to speed up the signaling
		   of observers.
		   A delete via the menu does this, in do_headers_action(), 
		   though I don't know why.
		 */
		tny_folder_poke_status (folder);
	
		g_object_unref (folder);
	}
	
	if (account)
		g_object_unref (account);
		
	/* Refilter the header view explicitly, to make sure that 
	 * deleted emails are really removed from view. 
	 * (They are not really deleted until contact is made with the server, 
	 * so they would appear with a strike-through until then):
	 */
	if (main_win) { /* only needed when there's a mainwindow / UI */

		/* This is a GDK lock because we are an idle callback and
		 * the code below is or does Gtk+ code */
		gdk_threads_enter (); /* CHECKED */
		ModestHeaderView *header_view = (ModestHeaderView *)
			modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(main_win),
							     MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
		if (header_view && MODEST_IS_HEADER_VIEW (header_view))
			modest_header_view_refilter (header_view);
		gdk_threads_leave ();
	}
	
	return res;
}




static gint
on_delete_message (GArray *arguments, gpointer data, osso_rpc_t *retval)
{
	/* Get the arguments: */
 	osso_rpc_t val = g_array_index (arguments,
			     osso_rpc_t,
			     MODEST_DBUS_DELETE_MESSAGE_ARG_URI);
 	gchar *uri = g_strdup (val.value.s);
 	
	/* Use g_idle to context-switch into the application's thread: */
 	g_idle_add(on_idle_delete_message, (gpointer)uri);
 	
 	return OSSO_OK;
}

static gboolean
on_idle_send_receive(gpointer user_data)
{
	gboolean auto_update;
	ModestWindow *main_win = NULL;

	main_win =
		modest_window_mgr_get_main_window (modest_runtime_get_window_mgr (),
						   FALSE); /* don't create */

	gdk_threads_enter (); /* CHECKED */

	/* Check if the autoupdate feature is on */
	auto_update = modest_conf_get_bool (modest_runtime_get_conf (), 
					    MODEST_CONF_AUTO_UPDATE, NULL);

	if (auto_update)
		/* Do send receive */
		modest_ui_actions_do_send_receive_all (main_win, FALSE);
	else
		/* Disable auto update */
		modest_platform_set_update_interval (0);

	gdk_threads_leave (); /* CHECKED */
	
	return FALSE;
}



static gint 
on_dbus_method_dump_send_queues (DBusConnection *con, DBusMessage *message)
{
	gchar *str;
	
	DBusMessage *reply;
	dbus_uint32_t serial = 0;

	GSList *account_names, *cursor;

	str = g_strdup("\nsend queues\n"
		       "===========\n");

	cursor = account_names = modest_account_mgr_account_names
		(modest_runtime_get_account_mgr(), TRUE); /* only enabled accounts */

	while (cursor) {
		TnyAccount *acc;
		gchar *tmp, *accname = (gchar*)cursor->data;
		
		tmp = g_strdup_printf ("%s", str);
		g_free (str);
		str = tmp;
		
		/* transport */
		acc = modest_tny_account_store_get_server_account (
			modest_runtime_get_account_store(), accname,
			TNY_ACCOUNT_TYPE_TRANSPORT);
		if (TNY_IS_ACCOUNT(acc)) {
			gchar *tmp, *url = tny_account_get_url_string (acc);
			ModestTnySendQueue *sendqueue =
				modest_runtime_get_send_queue (TNY_TRANSPORT_ACCOUNT(acc));
			gchar *queue_str = modest_tny_send_queue_to_string (sendqueue);
			
			tmp = g_strdup_printf ("%s[%s]: '%s': %s\n%s",
					       str, accname, tny_account_get_id (acc), url,
					       queue_str);
			g_free(queue_str);
			g_free (url);
			g_free (str);
			str = tmp;

			g_object_unref (acc);
		}
		
		cursor = g_slist_next (cursor);
	}
	modest_account_mgr_free_account_names (account_names);
							 
	g_printerr (str);
	
	reply = dbus_message_new_method_return (message);
	if (reply) {
		dbus_message_append_args (reply,
					  DBUS_TYPE_STRING, &str,
					  DBUS_TYPE_INVALID);
		dbus_connection_send (con, reply, &serial);
		dbus_connection_flush (con);
		dbus_message_unref (reply);
	}
	
	g_free (str);
	return OSSO_OK;
}


static gint 
on_dbus_method_dump_operation_queue (DBusConnection *con, DBusMessage *message)
{
	gchar *str;
	gchar *op_queue_str;
	
	DBusMessage *reply;
	dbus_uint32_t serial = 0;

	/* operations queue; */
	op_queue_str = modest_mail_operation_queue_to_string
		(modest_runtime_get_mail_operation_queue ());
		
	str = g_strdup_printf ("\noperation queue\n"
			       "===============\n"
			       "status: %s\n"
			       "%s\n",
			       tny_device_is_online (modest_runtime_get_device ()) ? "online" : "offline",
			       op_queue_str);
	g_free (op_queue_str);
	
	g_printerr (str);
	
	reply = dbus_message_new_method_return (message);
	if (reply) {
		dbus_message_append_args (reply,
					  DBUS_TYPE_STRING, &str,
					  DBUS_TYPE_INVALID);
		dbus_connection_send (con, reply, &serial);
		dbus_connection_flush (con);
		dbus_message_unref (reply);
	}
	
	g_free (str);
	return OSSO_OK;
}



static gint 
on_dbus_method_dump_accounts (DBusConnection *con, DBusMessage *message)
{
	gchar *str;
	
	DBusMessage *reply;
	dbus_uint32_t serial = 0;

	GSList *account_names, *cursor;

	str = g_strdup ("\naccounts\n========\n");

	cursor = account_names = modest_account_mgr_account_names
		(modest_runtime_get_account_mgr(), TRUE); /* only enabled accounts */

	while (cursor) {
		TnyAccount *acc;
		gchar *tmp, *accname = (gchar*)cursor->data;

		tmp = g_strdup_printf ("%s[%s]\n", str, accname);
		g_free (str);
		str = tmp;
		
		/* store */
		acc = modest_tny_account_store_get_server_account (
			modest_runtime_get_account_store(), accname,
			TNY_ACCOUNT_TYPE_STORE);
		if (TNY_IS_ACCOUNT(acc)) {
			gchar *tmp, *url = tny_account_get_url_string (acc);
			tmp = g_strdup_printf ("%sstore    : '%s': %s\n",
					       str, tny_account_get_id (acc), url);
			g_free (str);
			str = tmp;
			g_free (url);
			g_object_unref (acc);
		}
		
		/* transport */
		acc = modest_tny_account_store_get_server_account (
			modest_runtime_get_account_store(), accname,
			TNY_ACCOUNT_TYPE_TRANSPORT);
		if (TNY_IS_ACCOUNT(acc)) {
			gchar *tmp, *url = tny_account_get_url_string (acc);
			tmp = g_strdup_printf ("%stransport: '%s': %s\n",
					       str, tny_account_get_id (acc), url);
			g_free (str);
			str = tmp;
			g_free (url);
			g_object_unref (acc);
		}
		
		cursor = g_slist_next (cursor);
	}
	
	modest_account_mgr_free_account_names (account_names);
							 
	g_printerr (str);
	
	reply = dbus_message_new_method_return (message);
	if (reply) {
		dbus_message_append_args (reply,
					  DBUS_TYPE_STRING, &str,
					  DBUS_TYPE_INVALID);
		dbus_connection_send (con, reply, &serial);
		dbus_connection_flush (con);
		dbus_message_unref (reply);
	}
	
	g_free (str);
	return OSSO_OK;
}




static gint 
on_send_receive(GArray *arguments, gpointer data, osso_rpc_t * retval)
{ 	
	/* Use g_idle to context-switch into the application's thread: */
 	g_idle_add(on_idle_send_receive, NULL);
 	
 	return OSSO_OK;
}

static gboolean
on_idle_open_default_inbox(gpointer user_data)
{
	ModestWindow *main_win;
	GtkWidget *folder_view;
	
	if (!check_and_offer_account_creation ()) /* this has it's only lock already */
		return FALSE;

	/* This is a GDK lock because we are an idle callback and
	 * the code below is or does Gtk+ code */
	gdk_threads_enter (); /* CHECKED */

	main_win = modest_window_mgr_get_main_window (modest_runtime_get_window_mgr (),
						      TRUE); /* create if non-existent */
	if (!main_win) {
		g_warning ("%s: BUG: no main window", __FUNCTION__);
		gdk_threads_leave (); /* CHECKED */
		return FALSE; /* don't call me again */
	}
		
	/* Get the folder view */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (main_win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
	modest_folder_view_select_first_inbox_or_local (MODEST_FOLDER_VIEW (folder_view));
	
	gdk_threads_leave (); /* CHECKED */
	
	/* This D-Bus method is obviously meant to result in the UI being visible,
	 * so show it, by calling this idle handler directly: */
	on_idle_top_application(user_data);
	
	return FALSE; /* Do not call this callback again. */
}

static gint 
on_open_default_inbox(GArray * arguments, gpointer data, osso_rpc_t * retval)
{
	/* Use g_idle to context-switch into the application's thread: */
 	g_idle_add(on_idle_open_default_inbox, NULL);
 	
 	return OSSO_OK;
}


static gboolean 
on_idle_top_application (gpointer user_data)
{
	ModestWindow *main_win;
	
	/* This is a GDK lock because we are an idle callback and
	 * the code below is or does Gtk+ code */

	gdk_threads_enter (); /* CHECKED */
	
	main_win = modest_window_mgr_get_main_window (modest_runtime_get_window_mgr (),
						      TRUE); /* create if non-existent */
	if (main_win) {
		/* Ideally, we would just use gtk_widget_show(), 
		 * but this widget is not coded correctly to support that: */
		gtk_widget_show_all (GTK_WIDGET (main_win));
		gtk_window_present (GTK_WINDOW (main_win));
	} else
		g_warning ("%s: BUG: no main window", __FUNCTION__);

	gdk_threads_leave (); /* CHECKED */
	
	return FALSE; /* Do not call this callback again. */
}

static gint 
on_top_application(GArray * arguments, gpointer data, osso_rpc_t * retval)
{
	/* Use g_idle to context-switch into the application's thread: */
 	g_idle_add(on_idle_top_application, NULL);
 	
 	return OSSO_OK;
}
                      
/* Callback for normal D-BUS messages */
gint 
modest_dbus_req_handler(const gchar * interface, const gchar * method,
			GArray * arguments, gpointer data,
			osso_rpc_t * retval)
{
	if (g_ascii_strcasecmp (method, MODEST_DBUS_METHOD_MAIL_TO) == 0) {
		if (arguments->len != MODEST_DBUS_MAIL_TO_ARGS_COUNT)
			goto param_error;
		return on_mail_to (arguments, data, retval);		
	} else if (g_ascii_strcasecmp (method, MODEST_DBUS_METHOD_OPEN_MESSAGE) == 0) {
		if (arguments->len != MODEST_DBUS_OPEN_MESSAGE_ARGS_COUNT)
			goto param_error;
		return on_open_message (arguments, data, retval);
	} else if (g_ascii_strcasecmp (method, MODEST_DBUS_METHOD_SEND_RECEIVE) == 0) {
		if (arguments->len != 0)
			goto param_error;
		return on_send_receive (arguments, data, retval);
	} else if (g_ascii_strcasecmp (method, MODEST_DBUS_METHOD_COMPOSE_MAIL) == 0) {
		if (arguments->len != MODEST_DBUS_COMPOSE_MAIL_ARGS_COUNT)
			goto param_error;
		return on_compose_mail (arguments, data, retval);
	} else if (g_ascii_strcasecmp (method, MODEST_DBUS_METHOD_DELETE_MESSAGE) == 0) {
		if (arguments->len != MODEST_DBUS_DELETE_MESSAGE_ARGS_COUNT)
			goto param_error;
		return on_delete_message (arguments,data, retval);
	} else if (g_ascii_strcasecmp (method, MODEST_DBUS_METHOD_OPEN_DEFAULT_INBOX) == 0) {
		if (arguments->len != 0)
			goto param_error;
		return on_open_default_inbox (arguments, data, retval);
	} else if (g_ascii_strcasecmp (method, MODEST_DBUS_METHOD_TOP_APPLICATION) == 0) {
		if (arguments->len != 0)
			goto param_error;
		return on_top_application (arguments, data, retval); 
	} else { 
		/* We need to return INVALID here so
		 * libosso will return DBUS_HANDLER_RESULT_NOT_YET_HANDLED,
		 * so that our modest_dbus_req_filter will then be tried instead.
		 * */
		return OSSO_INVALID;
	}
 param_error:
	/* Notify error in D-Bus method */
	g_idle_add (notify_error_in_dbus_callback, NULL);
	return OSSO_ERROR;
}
					 
/* A complex D-Bus type (like a struct),
 * used to return various information about a search hit.
 */
#define SEARCH_HIT_DBUS_TYPE \
	DBUS_STRUCT_BEGIN_CHAR_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING /* msgid */ \
	DBUS_TYPE_STRING_AS_STRING /* subject */ \
	DBUS_TYPE_STRING_AS_STRING /* folder */ \
	DBUS_TYPE_STRING_AS_STRING /* sender */ \
	DBUS_TYPE_UINT64_AS_STRING /* msize */ \
	DBUS_TYPE_BOOLEAN_AS_STRING /* has_attachment */ \
	DBUS_TYPE_BOOLEAN_AS_STRING /* is_unread */ \
	DBUS_TYPE_INT64_AS_STRING /* timestamp */ \
	DBUS_STRUCT_END_CHAR_AS_STRING

static DBusMessage *
search_result_to_message (DBusMessage *reply,
			   GList       *hits)
{
	DBusMessageIter iter;
	DBusMessageIter array_iter;
	GList          *hit_iter;

	dbus_message_iter_init_append (reply, &iter); 
	dbus_message_iter_open_container (&iter,
					  DBUS_TYPE_ARRAY,
					  SEARCH_HIT_DBUS_TYPE,
					  &array_iter); 

	for (hit_iter = hits; hit_iter; hit_iter = hit_iter->next) {
		DBusMessageIter  struct_iter;
		ModestSearchResultHit *hit;
		char            *msg_url;
		const char      *subject;
		const char      *folder;
		const char      *sender;
		guint64          size;
		gboolean         has_attachment;
		gboolean         is_unread;
		gint64           ts;

		hit = (ModestSearchResultHit *) hit_iter->data;

		msg_url = hit->msgid;
		subject = hit->subject;
		folder  = hit->folder;
		sender  = hit->sender;
		size           = hit->msize;
		has_attachment = hit->has_attachment;
		is_unread      = hit->is_unread;
		ts             = hit->timestamp;

		g_debug ("DEBUG: %s: Adding hit: %s", __FUNCTION__, msg_url);	
		
		dbus_message_iter_open_container (&array_iter,
						  DBUS_TYPE_STRUCT,
						  NULL,
						  &struct_iter);

   		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_STRING,
						&msg_url);

		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_STRING,
						&subject); 

		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_STRING,
						&folder);

		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_STRING,
						&sender);

		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_UINT64,
						&size);

		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_BOOLEAN,
						&has_attachment);

		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_BOOLEAN,
						&is_unread);
		
		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_INT64,
						&ts);

		dbus_message_iter_close_container (&array_iter,
						   &struct_iter); 

		g_free (hit->msgid);
		g_free (hit->subject);
		g_free (hit->folder);
		g_free (hit->sender);

		g_slice_free (ModestSearchResultHit, hit);
	}

	dbus_message_iter_close_container (&iter, &array_iter);

	return reply;
}


static void
on_dbus_method_search (DBusConnection *con, DBusMessage *message)
{
	ModestDBusSearchFlags dbus_flags;
	DBusMessage  *reply = NULL;
	dbus_bool_t  res;
	dbus_int64_t sd_v;
	dbus_int64_t ed_v;
	dbus_int32_t flags_v;
	dbus_uint32_t size_v;
	const char *folder;
	const char *query;
	time_t start_date;
	time_t end_date;
	GList *hits;

	DBusError error;
	dbus_error_init (&error);

	sd_v = ed_v = 0;
	flags_v = 0;

	res = dbus_message_get_args (message,
				     &error,
				     DBUS_TYPE_STRING, &query,
				     DBUS_TYPE_STRING, &folder, /* e.g. "INBOX/drafts": TODO: Use both an ID and a display name. */
				     DBUS_TYPE_INT64, &sd_v,
				     DBUS_TYPE_INT64, &ed_v,
				     DBUS_TYPE_INT32, &flags_v,
				     DBUS_TYPE_UINT32, &size_v,
				     DBUS_TYPE_INVALID);

	dbus_flags = (ModestDBusSearchFlags) flags_v;
	start_date = (time_t) sd_v;
	end_date = (time_t) ed_v;

	ModestSearch search;
	memset (&search, 0, sizeof (search));
	
	/* Remember what folder we are searching in:
	 *
	 * Note that we don't copy the strings, 
	 * because this struct will only be used for the lifetime of this function.
	 */
	if (folder && g_str_has_prefix (folder, "MAND:")) {
		search.folder = folder + strlen ("MAND:");
	} else if (folder && g_str_has_prefix (folder, "USER:")) {
		search.folder = folder + strlen ("USER:");
	} else if (folder && g_str_has_prefix (folder, "MY:")) {
		search.folder = folder + strlen ("MY:");
	} else {
		search.folder = folder;
	}

   /* Remember the text to search for: */
#ifdef MODEST_HAVE_OGS
	search.query  = query;
#endif

	/* Other criteria: */
	search.start_date = start_date;
	search.end_date  = end_date;
	search.flags  = 0;

	/* Text to serach for in various parts of the message: */
	if (dbus_flags & MODEST_DBUS_SEARCH_SUBJECT) {
		search.flags |= MODEST_SEARCH_SUBJECT;
		search.subject = query;
	}

	if (dbus_flags & MODEST_DBUS_SEARCH_SENDER) {
		search.flags |=  MODEST_SEARCH_SENDER;
		search.from = query;
	}

	if (dbus_flags & MODEST_DBUS_SEARCH_RECIPIENT) {
		search.flags |= MODEST_SEARCH_RECIPIENT; 
		search.recipient = query;
	}

	if (dbus_flags & MODEST_DBUS_SEARCH_BODY) {
		search.flags |=  MODEST_SEARCH_BODY; 
		search.body = query;
	}

	if (sd_v > 0) {
		search.flags |= MODEST_SEARCH_BEFORE;
		search.start_date = start_date;
	}

	if (ed_v > 0) {
		search.flags |= MODEST_SEARCH_AFTER;
		search.end_date = end_date;
	}

	if (size_v > 0) {
		search.flags |= MODEST_SEARCH_SIZE;
		search.minsize = size_v;
	}

#ifdef MODEST_HAVE_OGS
	search.flags |= MODEST_SEARCH_USE_OGS;
	g_debug ("%s: Starting search for %s", __FUNCTION__, search.query);
#endif

	/* Note that this currently gets folders and messages from the servers, 
	 * which can take a long time. libmodest_dbus_client_search() can timeout, 
	 * reporting no results, if this takes a long time: */
	hits = modest_search_all_accounts (&search);

	reply = dbus_message_new_method_return (message);

	search_result_to_message (reply, hits);

	if (reply == NULL) {
		g_warning ("%s: Could not create reply.", __FUNCTION__);
	}

	if (reply) {
		dbus_uint32_t serial = 0;
		dbus_connection_send (con, reply, &serial);
    	dbus_connection_flush (con);
    	dbus_message_unref (reply);
	}

	g_list_free (hits);
}


/* A complex D-Bus type (like a struct),
 * used to return various information about a folder.
 */
#define GET_FOLDERS_RESULT_DBUS_TYPE \
	DBUS_STRUCT_BEGIN_CHAR_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING /* Folder Name */ \
	DBUS_TYPE_STRING_AS_STRING /* Folder URI */ \
	DBUS_STRUCT_END_CHAR_AS_STRING

static DBusMessage *
get_folders_result_to_message (DBusMessage *reply,
			   GList *folder_ids)
{
	DBusMessageIter iter;	
	dbus_message_iter_init_append (reply, &iter); 
	
	DBusMessageIter array_iter;
	dbus_message_iter_open_container (&iter,
					  DBUS_TYPE_ARRAY,
					  GET_FOLDERS_RESULT_DBUS_TYPE,
					  &array_iter); 

	GList *list_iter = folder_ids;
	for (list_iter = folder_ids; list_iter; list_iter = list_iter->next) {
		
		const gchar *folder_name = (const gchar*)list_iter->data;
		if (folder_name) {
			/* g_debug ("DEBUG: %s: Adding folder: %s", __FUNCTION__, folder_name);	*/
			
			DBusMessageIter struct_iter;
			dbus_message_iter_open_container (&array_iter,
							  DBUS_TYPE_STRUCT,
							  NULL,
							  &struct_iter);
	
			/* name: */
			dbus_message_iter_append_basic (&struct_iter,
							DBUS_TYPE_STRING,
							&folder_name); /* The string will be copied. */
							
			/* URI: This is maybe not needed by osso-global-search: */
			const gchar *folder_uri = "TODO:unimplemented";
			dbus_message_iter_append_basic (&struct_iter,
							DBUS_TYPE_STRING,
							&folder_uri); /* The string will be copied. */
	
			dbus_message_iter_close_container (&array_iter,
							   &struct_iter); 
		}
	}

	dbus_message_iter_close_container (&iter, &array_iter);

	return reply;
}

static void
add_single_folder_to_list (TnyFolder *folder, GList** list)
{
	if (!folder)
		return;
		
	if (TNY_IS_MERGE_FOLDER (folder)) {
		const gchar * folder_name;
		/* Ignore these because their IDs ares
		 * a) not always unique or sensible.
		 * b) not human-readable, and currently need a human-readable 
		 *    ID here, because the osso-email-interface API does not allow 
		 *    us to return both an ID and a display name.
		 * 
		 * This is actually the merged outbox folder.
		 * We could hack our D-Bus API to understand "outbox" as the merged outboxes, 
		 * but that seems unwise. murrayc.
		 */
		folder_name = tny_folder_get_name (folder);
		if (folder_name && !strcmp (folder_name, "Outbox")) {
			*list = g_list_append(*list, g_strdup ("MAND:outbox"));
		}
		return;	
	}
		
	/* Add this folder to the list: */
	/*
	const gchar * folder_name = tny_folder_get_name (folder);
	if (folder_name)
		*list = g_list_append(*list, g_strdup (folder_name));
	else {
	*/
		/* osso-global-search only uses one string,
		 * so ID is the only thing that could possibly identify a folder.
		 * TODO: osso-global search should probably be changed to 
		 * take an ID and a Name.
		 */
	const gchar * id =  tny_folder_get_id (folder);
	if (id && strlen(id)) {
		const gchar *prefix = NULL;
		TnyFolderType folder_type;
			
		/* dbus global search api expects a prefix identifying the type of
		 * folder here. Mandatory folders should have MAND: prefix, and
		 * other user created folders should have USER: prefix
		 */
		folder_type = modest_tny_folder_guess_folder_type (folder);
		switch (folder_type) {
		case TNY_FOLDER_TYPE_INBOX:
			prefix = "MY:";
			break;
		case TNY_FOLDER_TYPE_OUTBOX:
		case TNY_FOLDER_TYPE_DRAFTS:
		case TNY_FOLDER_TYPE_SENT:
		case TNY_FOLDER_TYPE_ARCHIVE:
			prefix = "MAND:";
			break;
		case TNY_FOLDER_TYPE_INVALID:
			g_warning ("%s: BUG: TNY_FOLDER_TYPE_INVALID", __FUNCTION__);
			return; /* don't add it */
		default:
			prefix = "USER:";
			
		}
		

		*list = g_list_append(*list, g_strdup_printf ("%s%s", prefix, id));
	}
}

static void
add_folders_to_list (TnyFolderStore *folder_store, GList** list)
{
	if (!folder_store)
		return;
	
	/* Add this folder to the list: */
	if (TNY_IS_FOLDER (folder_store)) {
		add_single_folder_to_list (TNY_FOLDER (folder_store), list);
	}	
		
	/* Recurse into child folders: */
		
	/* Get the folders list: */
	/*
	TnyFolderStoreQuery *query = tny_folder_store_query_new ();
	tny_folder_store_query_add_item (query, NULL, 
		TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED);
	*/
	TnyList *all_folders = tny_simple_list_new ();
	tny_folder_store_get_folders (folder_store,
				      all_folders,
				      NULL /* query */,
				      NULL /* error */);

	TnyIterator *iter = tny_list_create_iterator (all_folders);
	while (!tny_iterator_is_done (iter)) {
		
		/* Do not recurse, because the osso-global-search UI specification 
		 * does not seem to want the sub-folders, though that spec seems to 
		 * be generally unsuitable for Modest.
		 */
		TnyFolder *folder = TNY_FOLDER (tny_iterator_get_current (iter));
		if (folder) {
			add_single_folder_to_list (TNY_FOLDER (folder), list);
			 
			#if 0
			if (TNY_IS_FOLDER_STORE (folder))
				add_folders_to_list (TNY_FOLDER_STORE (folder), list);
			else {
				add_single_folder_to_list (TNY_FOLDER (folder), list);
			}
			#endif
			
			/* tny_iterator_get_current() gave us a reference. */
			g_object_unref (folder);
		}
		
		tny_iterator_next (iter);
	}
	g_object_unref (G_OBJECT (iter));
}


/* return >1 for a special folder, 0 for a user-folder */
static gint
get_rank (const gchar *folder)
{
	if (strcmp (folder, "INBOX") == 0)
		return 1;
	if (strcmp (folder, modest_local_folder_info_get_type_name(TNY_FOLDER_TYPE_SENT)) == 0)
		return 2;
	if (strcmp (folder, modest_local_folder_info_get_type_name(TNY_FOLDER_TYPE_DRAFTS)) == 0)
		return 3;
	if (strcmp (folder, modest_local_folder_info_get_type_name(TNY_FOLDER_TYPE_OUTBOX)) == 0)
		return 4;
	return 0;
}

static gint
folder_name_compare_func (const gchar* folder1, const gchar* folder2)
{
	gint r1 = get_rank (folder1);
	gint r2 = get_rank (folder2);

	if (r1 > 0 && r2 > 0)
		return r1 - r2;
	if (r1 > 0 && r2 == 0)
		return -1;
	if (r1 == 0 && r2 > 0)
		return 1;
	else
		return	modest_text_utils_utf8_strcmp (folder1, folder2, TRUE);
}

/* FIXME: */
/*   - we're still missing the outbox */
/*   - we need to take care of localization (urgh) */
/*   - what about 'All mail folders'? */
static void
on_dbus_method_get_folders (DBusConnection *con, DBusMessage *message)
{
	DBusMessage  *reply = NULL;
 	ModestAccountMgr *account_mgr = NULL;
	gchar *account_name = NULL;
	GList *folder_names = NULL;	
	TnyAccount *account_local = NULL;
	TnyAccount *account_mmc = NULL;
	
	/* Get the TnyStoreAccount so we can get the folders: */
	account_mgr = modest_runtime_get_account_mgr();
	account_name = modest_account_mgr_get_default_account (account_mgr);
	if (!account_name) {
		g_printerr ("modest: no account found\n");
	}
	
	if (account_name) {
		TnyAccount *account = NULL;
		if (account_mgr) {
			account = modest_tny_account_store_get_server_account (
				modest_runtime_get_account_store(), account_name, 
				TNY_ACCOUNT_TYPE_STORE);
		}
		
		if (!account) {
			g_printerr ("modest: failed to get tny account folder'%s'\n", account_name);
		} 
		
		printf("DEBUG: %s: Getting folders for account name=%s\n", __FUNCTION__, account_name);
		g_free (account_name);
		account_name = NULL;
		
		add_folders_to_list (TNY_FOLDER_STORE (account), &folder_names);
	
		g_object_unref (account);
		account = NULL;
	}
	
	/* Also add the folders from the local folders account,
	 * because they are (currently) used with all accounts:
	 * TODO: This is not working. It seems to get only the Merged Folder (with an ID of "" (not NULL)).
	 */
	account_local = 
		modest_tny_account_store_get_local_folders_account (modest_runtime_get_account_store());
	add_folders_to_list (TNY_FOLDER_STORE (account_local), &folder_names);

	g_object_unref (account_local);
	account_local = NULL;

	/* Obtain the mmc account */
	account_mmc = 
		modest_tny_account_store_get_mmc_folders_account (modest_runtime_get_account_store());
	if (account_mmc) {
		add_folders_to_list (TNY_FOLDER_STORE (account_mmc), &folder_names);
		g_object_unref (account_mmc);
		account_mmc = NULL;
	}

	/* specs require us to sort the folder names, although
	 * this is really not the place to do that...
	 */
	folder_names = g_list_sort (folder_names,
				    (GCompareFunc)folder_name_compare_func);

	/* Put the result in a DBus reply: */
	reply = dbus_message_new_method_return (message);

	get_folders_result_to_message (reply, folder_names);

	if (reply == NULL) {
		g_warning ("%s: Could not create reply.", __FUNCTION__);
	}

	if (reply) {
		dbus_uint32_t serial = 0;
		dbus_connection_send (con, reply, &serial);
    	dbus_connection_flush (con);
    	dbus_message_unref (reply);
	}

	g_list_foreach (folder_names, (GFunc)g_free, NULL);
	g_list_free (folder_names);
}


/** This D-Bus handler is used when the main osso-rpc 
 * D-Bus handler has not handled something.
 * We use this for D-Bus methods that need to use more complex types 
 * than osso-rpc supports.
 */
DBusHandlerResult
modest_dbus_req_filter (DBusConnection *con,
			DBusMessage    *message,
			void           *user_data)
{
	gboolean handled = FALSE;

	if (dbus_message_is_method_call (message,
					 MODEST_DBUS_IFACE,
					 MODEST_DBUS_METHOD_SEARCH)) {
		on_dbus_method_search (con, message);
		handled = TRUE;			 	
	} else if (dbus_message_is_method_call (message,
						MODEST_DBUS_IFACE,
						MODEST_DBUS_METHOD_GET_FOLDERS)) {
		on_dbus_method_get_folders (con, message);
		handled = TRUE;			 	
	} else if (dbus_message_is_method_call (message,
						MODEST_DBUS_IFACE,
						MODEST_DBUS_METHOD_DUMP_OPERATION_QUEUE)) {
		on_dbus_method_dump_operation_queue (con, message);
		handled = TRUE;
	} else if (dbus_message_is_method_call (message,
						MODEST_DBUS_IFACE,
						MODEST_DBUS_METHOD_DUMP_ACCOUNTS)) {
		on_dbus_method_dump_accounts (con, message);
		handled = TRUE;
	} else if (dbus_message_is_method_call (message,
						MODEST_DBUS_IFACE,
						MODEST_DBUS_METHOD_DUMP_SEND_QUEUES)) {
		on_dbus_method_dump_send_queues (con, message);
		handled = TRUE;
	} else {
		/* Note that this mentions methods that were already handled in modest_dbus_req_handler(). */
		/* 
		g_debug ("  debug: %s: Unexpected (maybe already handled) D-Bus method:\n   Interface=%s, Member=%s\n", 
			__FUNCTION__, dbus_message_get_interface (message),
			dbus_message_get_member(message));
		*/
	}
	
	return (handled ? 
		DBUS_HANDLER_RESULT_HANDLED :
		DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
}

static gboolean
notify_error_in_dbus_callback (gpointer user_data)
{
	ModestMailOperation *mail_op;
	ModestMailOperationQueue *mail_op_queue;

	mail_op = modest_mail_operation_new (NULL);
	mail_op_queue = modest_runtime_get_mail_operation_queue ();

	/* Issues a noop operation in order to force the queue to emit
	   the "queue-empty" signal to allow modest to quit */
	modest_mail_operation_queue_add (mail_op_queue, mail_op);
	modest_mail_operation_noop (mail_op);
	g_object_unref (mail_op);

	return FALSE;
}

static gboolean
notify_msg_not_found_in_idle (gpointer user_data)
{
	modest_platform_run_information_dialog (NULL, _("mail_ni_ui_folder_get_msg_folder_error"));

	return FALSE;
}
