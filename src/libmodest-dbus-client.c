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

#include "libmodest-dbus-client.h"
#include "libmodest-dbus-api.h" /* For the API strings. */

//#define DBUS_API_SUBJECT_TO_CHANGE 1
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <string.h>



/** Get a comma-separated list of attachement URI strings, 
 * from a list of strings.
 */
static gchar* get_attachments_string (GSList *attachments)
{
	if (!attachments)
		return NULL;

	gchar *attachments_str = g_strdup("");

	GSList *iter = attachments;
	while (iter)
	{
		if (iter->data) {
			gchar *escaped;
			gchar *tmp;
			escaped = g_uri_escape_string ((const gchar *) (iter->data), NULL, TRUE);
			tmp = g_strconcat(attachments_str, ",", escaped, NULL);
			g_free(escaped);
			g_free(attachments_str);
			attachments_str = tmp;
		}
		iter = g_slist_next(iter);
	}
	return attachments_str;
}

/**
 * libmodest_dbus_client_mail_to:
 * @osso_context: a valid #osso_context_t object.
 * @mailto_uri: A mailto URI.
 * 
 * This function will try to do a remote procedure call (rpc)
 * into modest (or start it if necessary) and open a composer
 * window with the supplied parameters prefilled.
 *
 * Return value: Whether or not the rpc call to modest
 * was successfull
 **/
gboolean 
libmodest_dbus_client_mail_to (osso_context_t *osso_context, const gchar *mailto_uri)
{
	osso_rpc_t retval = { 0 };
	const osso_return_t ret = osso_rpc_run_with_defaults(osso_context, 
		   MODEST_DBUS_NAME, 
		   MODEST_DBUS_METHOD_MAIL_TO, &retval, 
		   DBUS_TYPE_STRING, mailto_uri, 
		   DBUS_TYPE_INVALID);
		
	if (ret != OSSO_OK) {
		printf("debug: %s: osso_rpc_run() failed.\n", __FUNCTION__);
		return FALSE;
	} else {
		printf("debug: %s: osso_rpc_run() succeeded.\n", __FUNCTION__);
	}
	
	osso_rpc_free_val(&retval);
	
	return TRUE;
}

/**
 * libmodest_dbus_client_compose_mail:
 * @osso_context: a valid #osso_context_t object.
 * @to: The Recipients (From: line)
 * @cc: Recipients for carbon copies
 * @bcc: Recipients for blind carbon copies
 * @subject: Subject line
 * @body: The actual body of the mail to compose.
 * @attachments: Additional list of attachments. A list of URI strings.
 * 
 * This function will try to do a remote procedure call (rpc)
 * into modest (or start it if necessary) and open a composer
 * window with the supplied parameters prefilled.
 *
 * Return value: Whether or not the rpc call to modest
 * was successfull
 **/
gboolean
libmodest_dbus_client_compose_mail (osso_context_t *osso_context, const gchar *to, const gchar *cc, 
	const gchar *bcc, const gchar* subject, const gchar* body, GSList *attachments)
{
	osso_rpc_t retval = { 0 };

	gchar *attachments_str = get_attachments_string(attachments);

	const osso_return_t ret = osso_rpc_run_with_defaults(osso_context,
		   MODEST_DBUS_NAME, 
		   MODEST_DBUS_METHOD_COMPOSE_MAIL, &retval, 
		   DBUS_TYPE_STRING, to, 
		   DBUS_TYPE_STRING, cc, 
		   DBUS_TYPE_STRING, bcc, 
		   DBUS_TYPE_STRING, subject, 
		   DBUS_TYPE_STRING, body,
		   DBUS_TYPE_STRING, attachments_str,
		   DBUS_TYPE_INVALID);

	g_free (attachments_str);

	if (ret != OSSO_OK) {
		printf("debug: %s: osso_rpc_run() failed.\n", __FUNCTION__);
		return FALSE;
	} else {
		printf("debug: %s: osso_rpc_run() succeeded.\n", __FUNCTION__);
	}

	osso_rpc_free_val(&retval);


	return TRUE;
}

/**
 * libmodest_dbus_client_open_message:
 * @osso_context: a valid #osso_context_t object.
 * @msg_uri: A valid url to a mail
 *
 * This method will try to find the message supplied
 * by @msg_uri and open it for display if found. 
 * It will use remote procedure calls (rpc) over 
 * dbus to do so.
 *  
 * Return value: TRUE on successs, FALSE on error
 **/
gboolean 
libmodest_dbus_client_open_message (osso_context_t *osso_context, const gchar *mail_uri)
{
	osso_rpc_t retval = { 0 };
	const osso_return_t ret = osso_rpc_run_with_defaults(osso_context, 
		   MODEST_DBUS_NAME, 
		   MODEST_DBUS_METHOD_OPEN_MESSAGE, &retval, 
		   DBUS_TYPE_STRING, mail_uri, 
		   DBUS_TYPE_INVALID);
		
	if (ret != OSSO_OK) {
		printf("debug: %s: osso_rpc_run() failed.\n", __FUNCTION__);
		return FALSE;
	} else {
		printf("debug: %s: osso_rpc_run() succeeded.\n", __FUNCTION__);
	}
	
	osso_rpc_free_val(&retval);
	
	return TRUE;
}

gboolean 
libmodest_dbus_client_send_and_receive (osso_context_t *osso_context)
{
	osso_rpc_t retval = { 0 };
	const osso_return_t ret = osso_rpc_run_with_defaults(osso_context, 
		   MODEST_DBUS_NAME, 
		   MODEST_DBUS_METHOD_SEND_RECEIVE, &retval, 
		   DBUS_TYPE_INVALID);
		
	if (ret != OSSO_OK) {
		printf("debug: %s: osso_rpc_run() failed.\n", __FUNCTION__);
		return FALSE;
	} else {
		printf("debug: %s: osso_rpc_run() succeeded.\n", __FUNCTION__);
	}
	
	osso_rpc_free_val(&retval);
	
	return TRUE;
}

gboolean 
libmodest_dbus_client_open_default_inbox (osso_context_t *osso_context)
{
	osso_rpc_t retval = { 0 };
	const osso_return_t ret = osso_rpc_run_with_defaults(osso_context, 
		   MODEST_DBUS_NAME, 
		   MODEST_DBUS_METHOD_OPEN_DEFAULT_INBOX, &retval, 
		   DBUS_TYPE_INVALID);
		
	if (ret != OSSO_OK) {
		printf("debug: %s: osso_rpc_run() failed.\n", __FUNCTION__);
		return FALSE;
	} else {
		printf("debug: %s: osso_rpc_run() succeeded.\n", __FUNCTION__);
	}
	
	osso_rpc_free_val(&retval);
	
	return TRUE;
}

gboolean
libmodest_dbus_client_open_account (osso_context_t *osso_context,
				    const gchar *account_id)
{
	osso_rpc_t retval = { 0 };
	const osso_return_t ret =
		osso_rpc_run_with_defaults(osso_context,
					   MODEST_DBUS_NAME,
					   MODEST_DBUS_METHOD_OPEN_ACCOUNT, &retval,
					   DBUS_TYPE_STRING, account_id,
					   DBUS_TYPE_INVALID);

	if (ret != OSSO_OK) {
		printf("debug: %s: osso_rpc_run() failed.\n", __FUNCTION__);
		return FALSE;
	} else {
		printf("debug: %s: osso_rpc_run() succeeded.\n", __FUNCTION__);
	}

	osso_rpc_free_val(&retval);

	return TRUE;
}

/**
 * libmodest_dbus_client_delete_message:
 * @osso_context: a valid #osso_context_t object.
 * @msg_uri: A valid url to a mail 
 *
 * This method will try to find the message supplied
 * by @msg_uri and if found delete it. It will use
 * remote procedure calls (rpc) over dbus to do so.
 * 
 * Return value: TRUE on successs, FALSE on error
 **/
gboolean
libmodest_dbus_client_delete_message (osso_context_t   *osso_ctx,
				      const char       *msg_uri)
{
	osso_rpc_t    retval = { 0 };
	osso_return_t ret;
       
	ret = osso_rpc_run_with_defaults (osso_ctx, 
					  MODEST_DBUS_NAME, 
					  MODEST_DBUS_METHOD_DELETE_MESSAGE, &retval, 
					  DBUS_TYPE_STRING, msg_uri, 
					  DBUS_TYPE_INVALID);
		
	if (ret != OSSO_OK) {
		g_debug ("debug: osso_rpc_run() failed.\n");
	} else {
		g_debug ("debug: osso_rpc_run() succeeded.\n");
	}
	
	osso_rpc_free_val (&retval);

	return ret == OSSO_OK;
}

static void
modest_search_hit_free (ModestSearchHit *hit)
{
	g_free (hit->msgid);
	g_slice_free (ModestSearchHit, hit);
}

void
modest_search_hit_list_free (GList *hits)
{
	GList *iter;

	if (hits == NULL) {
		return;
	}

	for (iter = hits; iter; iter = iter->next) {
		modest_search_hit_free ((ModestSearchHit *) iter->data);
	}

	g_list_free (hits);
}

static char *
_dbus_iter_get_string_or_null (DBusMessageIter *iter)
{
	const char *string = NULL;
	char       *ret = NULL;

	dbus_message_iter_get_basic (iter, &string);
	
	if (string && strlen (string)) {
		ret = g_strdup (string);
	}

	return ret;
}

static guint64
_dbus_iter_get_uint64 (DBusMessageIter *iter)
{
	dbus_uint64_t ui64v;
	guint64       ret;

	ui64v = 0;
	dbus_message_iter_get_basic (iter, &ui64v);

	ret = (guint64) ui64v;

	return ret;
}


static gint64
_dbus_iter_get_int64 (DBusMessageIter *iter)
{
	dbus_int64_t i64v;
	gint64       ret;

	i64v = 0;
	dbus_message_iter_get_basic (iter, &i64v);

	ret = (gint64) i64v;

	return ret;
}

static gboolean
_dbus_iter_get_boolean (DBusMessageIter *iter)

{
	dbus_bool_t  val;
	gboolean     ret;

	val = FALSE;
	dbus_message_iter_get_basic (iter, &val);

	ret = (gboolean) val;

	return ret;
}

/** Get the values from the complex type (SEARCH_HIT_DBUS_TYPE)
 * in the D-Bus return message. */
static ModestSearchHit *
modest_dbus_message_iter_get_search_hit (DBusMessageIter *parent)
{
	ModestSearchHit *hit;
	DBusMessageIter  child;
	dbus_bool_t      res;
	int              arg_type;
	gboolean         error;

	error = FALSE;
	hit = g_slice_new0 (ModestSearchHit);

	arg_type = dbus_message_iter_get_arg_type (parent);

	if (arg_type != 'r') {
		return NULL;
	}

	dbus_message_iter_recurse (parent, &child);
	
	/* msgid  */
	arg_type = dbus_message_iter_get_arg_type (&child);

	if (arg_type != DBUS_TYPE_STRING) {
		error = TRUE;
		goto out;
	}

	hit->msgid = _dbus_iter_get_string_or_null (&child);

	res = dbus_message_iter_next (&child);
	if (res == FALSE) {
		error = TRUE;
		goto out;
	}

	/* subject  */
	arg_type = dbus_message_iter_get_arg_type (&child);

	if (arg_type != DBUS_TYPE_STRING) {
		error = TRUE;
		goto out;
	}

	hit->subject = _dbus_iter_get_string_or_null (&child);

	res = dbus_message_iter_next (&child);
	if (res == FALSE) {
		error = TRUE;
		goto out;
	}

	/* folder  */
	arg_type = dbus_message_iter_get_arg_type (&child);

	if (arg_type != DBUS_TYPE_STRING) {
		error = TRUE;
		goto out;
	}

	hit->folder = _dbus_iter_get_string_or_null (&child);

	res = dbus_message_iter_next (&child);
	if (res == FALSE) {
		error = TRUE;
		goto out;
	}

	/* sender  */
	arg_type = dbus_message_iter_get_arg_type (&child);

	if (arg_type != DBUS_TYPE_STRING) {
		error = TRUE;
		goto out;
	}

	hit->sender = _dbus_iter_get_string_or_null (&child);

	res = dbus_message_iter_next (&child);
	if (res == FALSE) {
		error = TRUE;
		goto out;
	}

	/* msize  */
	arg_type = dbus_message_iter_get_arg_type (&child);

	if (arg_type != DBUS_TYPE_UINT64) {
		error = TRUE;
		goto out;
	}

	hit->msize = _dbus_iter_get_uint64 (&child);

	res = dbus_message_iter_next (&child);
	if (res == FALSE) {
		error = TRUE;
		goto out;
	}

	/* has_attachment  */
	arg_type = dbus_message_iter_get_arg_type (&child);

	if (arg_type != DBUS_TYPE_BOOLEAN) {
		error = TRUE;
		goto out;
	}

	hit->has_attachment = _dbus_iter_get_boolean (&child); 

	res = dbus_message_iter_next (&child);
	if (res == FALSE) {
		error = TRUE;
		goto out;
	}

	/* is_unread  */
	arg_type = dbus_message_iter_get_arg_type (&child);

	if (arg_type != DBUS_TYPE_BOOLEAN) {
		error = TRUE;
		goto out;
	}

	hit->is_unread = _dbus_iter_get_boolean (&child);  

	res = dbus_message_iter_next (&child);
	if (res == FALSE) {
		error = TRUE;
		goto out;
	}

	/* timestamp  */
	arg_type = dbus_message_iter_get_arg_type (&child);

	if (arg_type != DBUS_TYPE_INT64) {
		error = TRUE;
		goto out;
	}

	hit->timestamp = _dbus_iter_get_int64 (&child); 

	res = dbus_message_iter_next (&child);
	if (res == TRUE) {
		error = TRUE;
		goto out;
	}	

out:
	if (error) {
		g_warning ("%s: Error during unmarshalling", __FUNCTION__);
		modest_search_hit_free (hit);
		hit = NULL;
	}

	return hit;
}

/**
 * libmodest_dbus_client_search:
 * @osso_ctx: A valid #osso_context_t object.
 * @query: The term to search for.
 * @folder: An url to specific folder or %NULL to search everywhere.
 * @start_date: Search hits before this date will be ignored.
 * @end_date: Search hits after this date will be ignored.
 * @min_size: Messagers smaller then this size will be ingored.
 * @flags: A list of flags where to search so the documentation 
 * of %ModestDBusSearchFlags for details.
 * @hits: A pointer to a valid GList pointer that will contain the search
 * hits (ModestSearchHit). The list and the items must be freed by the caller 
 * with modest_search_hit_list_free().
 *
 * This method will search the folder specified by a valid url in @folder or all
 * known accounts (local and remote) if %NULL for matches of the search term(s)
 * specified in @query. It is legal to specify 0 in @start_date, @end_date and
 * @min_size to ignore these parameters during the search otherwise those message
 * that do not meet the specifed dates or size will be ignored.
 * Where to search, be it subject, sender or the whole body can be specified by
 * the @flags parameter.
 *
 * Upon success TRUE is returned and @hits will include the search hits or the list
 * migh be empty if none of the messages matched the search criteria. The returned
 * list must be freed with modest_search_hit_list_free (). It is save to pass
 * %NULL to this function so you can call this function on the result list no matter
 * if a hit was found or not (means the list is empty - i.e. %NULL)
 * FALSE will only be return if an error during the remote procedure call (rpc) 
 * occured or if the specified folder could not be found.
 *
 * NOTE: The body of a message can only be searched if it was previously downloaded by
 * modest. This function does also not attempt do to remote searches (i.e. IMAP search).
 *
 * Example to search every account for message containing "no":
 * <informalexample><programlisting>
 * ModestDBusSearchFlags  flags;
 * osso_context_t        *osso_context;
 * GList                 *hits;
 * GList                 *iter;
 * gboolean               res;
 * 
 * [...] Initialize osso context [...]
 *
 * res = libmodest_dbus_client_search (osso_context,
 *				       "no",
 *				       NULL,
 *				       0,
 *				       0,
 *				       0,
 *				       flags,
 *				       &hits);
 * 
 * for (iter = hits; iter; iter = iter->next) {
 *	ModestSearchHit *hit = (ModestSearchHit *) iter->data;
 *   	
 *   	[...] Do something with the hit [...]
 *
 *	}
 *
 *	modest_search_hit_list_free (hits);
 * </programlisting></informalexample>
 * 
 * Return value: TRUE if the search succeded or FALSE for an error during the search
 **/
gboolean
libmodest_dbus_client_search (osso_context_t          *osso_ctx,
			      const gchar             *query,
			      const gchar             *folder,
			      time_t		       start_date,
			      time_t 		       end_date,
			      guint32                  min_size,
			      ModestDBusSearchFlags    flags,
			      GList                  **hits)
{

	DBusMessage *msg;
	dbus_bool_t res;
	DBusConnection *con;
	DBusMessageIter iter;
	DBusMessageIter child;
	DBusMessage *reply = NULL;
	gint timeout;
	int arg_type;
	dbus_int64_t sd_v;
	dbus_int64_t ed_v;
	dbus_int32_t flags_v;
	dbus_uint32_t size_v;

	if (query == NULL) {
		return FALSE;
	}

	con = osso_get_dbus_connection (osso_ctx);

	if (con == NULL) {
		g_warning ("Could not get dbus connection\n");
		return FALSE;

	}


	msg = dbus_message_new_method_call (MODEST_DBUS_SERVICE,
		MODEST_DBUS_OBJECT,
		MODEST_DBUS_IFACE,
		MODEST_DBUS_METHOD_SEARCH);

    if (msg == NULL) {
       	//ULOG_ERR_F("dbus_message_new_method_call failed");
		return OSSO_ERROR;
    }

	if (folder == NULL) {
		folder = "";
	}

	sd_v = (dbus_int64_t) start_date;
	ed_v = (dbus_int64_t) end_date;
	flags_v = (dbus_int32_t) flags;
	size_v = (dbus_uint32_t) min_size;

	res  = dbus_message_append_args (msg,
					 DBUS_TYPE_STRING, &query,
					 DBUS_TYPE_STRING, &folder,
					 DBUS_TYPE_INT64, &sd_v,
					 DBUS_TYPE_INT64, &ed_v,
					 DBUS_TYPE_INT32, &flags_v,
					 DBUS_TYPE_UINT32, &size_v,
					 DBUS_TYPE_INVALID);

	dbus_message_set_auto_start (msg, TRUE);

	/* Use a long timeout (2 minutes) because the search currently 
	 * gets folders and messages from the servers. */
	timeout = 120000; //milliseconds.
	//osso_rpc_get_timeout (osso_ctx, &timeout);

    /*printf("DEBUG: %s: Before dbus_connection_send_with_reply_and_block().\n", 
		__FUNCTION__); */
	/* TODO: Detect the timeout somehow. */
	DBusError err;
	dbus_error_init (&err);
	reply = dbus_connection_send_with_reply_and_block (con,
							   msg, 
							   timeout,
							   &err);
	/* printf("DEBUG: %s: dbus_connection_send_with_reply_and_block() finished.\n", 
		__FUNCTION__); */

	dbus_message_unref (msg);

	if (!reply) {
		g_warning("%s: dbus_connection_send_with_reply_and_block() error: %s", 
			__FUNCTION__, err.message);
		return FALSE;
	}

	switch (dbus_message_get_type (reply)) {

		case DBUS_MESSAGE_TYPE_ERROR:
			dbus_set_error_from_message (&err, reply);
			//XXX to GError?!
			dbus_error_free (&err);
			dbus_message_unref (reply);
			return FALSE;

		case DBUS_MESSAGE_TYPE_METHOD_RETURN:
			/* ok we are good to go
			 * lets drop outa here and handle that */
			break;
		default:
			//ULOG_WARN_F("got unknown message type as reply");
			//retval->type = DBUS_TYPE_STRING;
			//retval->value.s = g_strdup("Invalid return value");
			//XXX to GError?! 
			dbus_message_unref (reply);
			return FALSE;
	}

	g_debug ("%s: message return", __FUNCTION__);

	dbus_message_iter_init (reply, &iter);
	arg_type = dbus_message_iter_get_arg_type (&iter);
	
	dbus_message_iter_recurse (&iter, &child);
	*hits = NULL;

	do {
		ModestSearchHit *hit;

		hit = modest_dbus_message_iter_get_search_hit (&child);

		if (hit) {
			*hits = g_list_prepend (*hits, hit);	
		}

	} while (dbus_message_iter_next (&child));

	dbus_message_unref (reply);


	/* TODO: This is from osso source, do we need it? */
#if 0
	/* Tell TaskNavigator to show "launch banner" */
	msg = dbus_message_new_method_call (TASK_NAV_SERVICE,
					    APP_LAUNCH_BANNER_METHOD_PATH,
					    APP_LAUNCH_BANNER_METHOD_INTERFACE,
					    APP_LAUNCH_BANNER_METHOD);

	if (msg == NULL) {
		g_warn ("dbus_message_new_method_call failed");
	}



	dbus_message_append_args (msg,
				  DBUS_TYPE_STRING,
				  &service,
				  DBUS_TYPE_INVALID);

	b = dbus_connection_send (conn, msg, NULL);

	if (b == NULL) {
		ULOG_WARN_F("dbus_connection_send failed");
	}

	dbus_message_unref (msg);
#endif

	return TRUE;
}


static void
modest_folder_result_free (ModestFolderResult *item)
{
	g_free (item->folder_name);
	g_free (item->folder_uri);
	g_slice_free (ModestFolderResult, item);
}

void
modest_folder_result_list_free (GList *list)
{
	GList *iter;

	if (list == NULL) {
		return;
	}

	for (iter = list; iter; iter = iter->next) {
		modest_folder_result_free ((ModestFolderResult *) iter->data);
	}

	g_list_free (list);
}


/** Get the values from the complex type (GET_FOLDERS_RESULT_DBUS_TYPE)
 * in the D-Bus return message. */
static ModestFolderResult *
modest_dbus_message_iter_get_folder_item (DBusMessageIter *parent)
{
	gboolean error = FALSE;
	ModestFolderResult *item = g_slice_new0 (ModestFolderResult);

	int arg_type = dbus_message_iter_get_arg_type (parent);

	if (arg_type != 'r') {
		return NULL;
	}

	DBusMessageIter  child;
	dbus_message_iter_recurse (parent, &child);
	
	/* folder name: */
	arg_type = dbus_message_iter_get_arg_type (&child);

	if (arg_type != DBUS_TYPE_STRING) {
		error = TRUE;
		goto out;
	}

	item->folder_name = _dbus_iter_get_string_or_null (&child);
	
	
	dbus_bool_t res = dbus_message_iter_next (&child);
	if (res == FALSE) {
		error = TRUE;
		goto out;
	}

	/* folder URI:  */
	arg_type = dbus_message_iter_get_arg_type (&child);

	if (arg_type != DBUS_TYPE_STRING) {
		error = TRUE;
		goto out;
	}

	item->folder_uri = _dbus_iter_get_string_or_null (&child);


out:
	if (error) {
		g_warning ("%s: Error during unmarshalling", __FUNCTION__);
		modest_folder_result_free (item);
		item = NULL;
	}

	return item;
}

/**
 * libmodest_dbus_client_get_folders:
 * @osso_ctx: A valid #osso_context_t object.
 * @folders: A pointer to a valid GList pointer that will contain the folder items
 * (ModestFolderResult). The list and the items must be freed by the caller 
 * with modest_folder_result_list_free().
 *
 * This method will obtain a list of folders in the default account.
 *
 * Upon success TRUE is returned and @folders will include the folders or the list
 * might be empty if there are no folders. The returned
 * list must be freed with modest_folder_result_list_free ().
 *
 * NOTE: A folder will only be retrieved if it was previously downloaded by
 * modest. This function does also not attempt do to remote refreshes (i.e. IMAP).
 * 
 * Return value: TRUE if the request succeded or FALSE for an error.
 **/
gboolean
libmodest_dbus_client_get_folders (osso_context_t          *osso_ctx,
			      GList                  **folders)
{
	/* Initialize output argument: */
	if (folders)
		*folders = NULL;
	else
		return FALSE;

	DBusConnection *con = osso_get_dbus_connection (osso_ctx);

	if (con == NULL) {
		g_warning ("Could not get dbus connection\n");
		return FALSE;

	}

	DBusMessage *msg = dbus_message_new_method_call (MODEST_DBUS_SERVICE,
		MODEST_DBUS_OBJECT,
		MODEST_DBUS_IFACE,
		MODEST_DBUS_METHOD_GET_FOLDERS);

    if (msg == NULL) {
       	//ULOG_ERR_F("dbus_message_new_method_call failed");
		return OSSO_ERROR;
    }

	dbus_message_set_auto_start (msg, TRUE);

	/* Use a long timeout (2 minutes) because the search currently 
	 * gets folders from the servers. */
	gint timeout = 120000;
	//osso_rpc_get_timeout (osso_ctx, &timeout);

  	DBusError err;
  	dbus_error_init (&err);
	DBusMessage *reply = dbus_connection_send_with_reply_and_block (con,
							   msg, 
							   timeout,
							   &err);

	dbus_message_unref (msg);
	msg = NULL;

	if (reply == NULL) {
		g_warning("%s: dbus_connection_send_with_reply_and_block() error:\n   %s", 
			__FUNCTION__, err.message);
		return FALSE;
	}

	switch (dbus_message_get_type (reply)) {

		case DBUS_MESSAGE_TYPE_ERROR:
			dbus_set_error_from_message (&err, reply);
			//XXX to GError?!
			dbus_error_free (&err);
			dbus_message_unref (reply);
			return FALSE;

		case DBUS_MESSAGE_TYPE_METHOD_RETURN:
			/* ok we are good to go
			 * lets drop outa here and handle that */
			break;
		default:
			//ULOG_WARN_F("got unknown message type as reply");
			//retval->type = DBUS_TYPE_STRING;
			//retval->value.s = g_strdup("Invalid return value");
			//XXX to GError?! 
			dbus_message_unref (reply);
			return FALSE;
	}

	g_debug ("%s: message return", __FUNCTION__);

	DBusMessageIter iter;
	dbus_message_iter_init (reply, &iter);
	/* int arg_type = dbus_message_iter_get_arg_type (&iter); */
	
	DBusMessageIter child;
	dbus_message_iter_recurse (&iter, &child);

	do {
		ModestFolderResult *item = modest_dbus_message_iter_get_folder_item (&child);

		if (item) {
			*folders = g_list_append (*folders, item);	
		}

	} while (dbus_message_iter_next (&child));

	dbus_message_unref (reply);


	/* TODO: This is from osso source, do we need it? */
#if 0
	/* Tell TaskNavigator to show "launch banner" */
	msg = dbus_message_new_method_call (TASK_NAV_SERVICE,
					    APP_LAUNCH_BANNER_METHOD_PATH,
					    APP_LAUNCH_BANNER_METHOD_INTERFACE,
					    APP_LAUNCH_BANNER_METHOD);

	if (msg == NULL) {
		g_warn ("dbus_message_new_method_call failed");
	}



	dbus_message_append_args (msg,
				  DBUS_TYPE_STRING,
				  &service,
				  DBUS_TYPE_INVALID);

	b = dbus_connection_send (conn, msg, NULL);

	if (b == NULL) {
		ULOG_WARN_F("dbus_connection_send failed");
	}

	dbus_message_unref (msg);
#endif

	return TRUE;
}


