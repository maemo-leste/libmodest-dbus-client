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
#include <dbus_api/modest-dbus-api.h> /* For the API strings. */

//#define DBUS_API_SUBJECT_TO_CHANGE 1
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <string.h>

gboolean
libmodest_dbus_client_send_mail (osso_context_t *osso_context, const gchar *to, const gchar *cc, 
	const gchar *bcc, const gchar* subject, const gchar* body, GSList *attachments)
{
	osso_rpc_t retval;
	const osso_return_t ret = osso_rpc_run_with_defaults(osso_context, 
		   MODEST_DBUS_NAME, 
		   MODEST_DBUS_METHOD_SEND_MAIL, &retval, 
		   DBUS_TYPE_STRING, to, 
		   DBUS_TYPE_STRING, cc, 
		   DBUS_TYPE_STRING, bcc, 
		   DBUS_TYPE_STRING, subject, 
		   DBUS_TYPE_STRING, body, 
		   DBUS_TYPE_INVALID);
		
	if (ret != OSSO_OK) {
		printf("debug: osso_rpc_run() failed.\n");
		return FALSE;
	} else {
		printf("debug: osso_rpc_run() succeeded.\n");
	}
	
	osso_rpc_free_val(&retval);
	
	return TRUE;
}
	
gboolean 
libmodest_dbus_client_mail_to (osso_context_t *osso_context, const gchar *mailto_uri)
{
	osso_rpc_t retval;
	const osso_return_t ret = osso_rpc_run_with_defaults(osso_context, 
		   MODEST_DBUS_NAME, 
		   MODEST_DBUS_METHOD_MAIL_TO, &retval, 
		   DBUS_TYPE_STRING, mailto_uri, 
		   DBUS_TYPE_INVALID);
		
	if (ret != OSSO_OK) {
		printf("debug: osso_rpc_run() failed.\n");
		return FALSE;
	} else {
		printf("debug: osso_rpc_run() succeeded.\n");
	}
	
	osso_rpc_free_val(&retval);
	
	return TRUE;
}

gboolean
libmodest_dbus_client_compose_mail (osso_context_t *osso_context, const gchar *to, const gchar *cc, 
	const gchar *bcc, const gchar* subject, const gchar* body, GSList *attachments)
{
	osso_rpc_t retval;
	gchar *attachments_str = NULL;
	gchar *tmp = NULL;
	GSList *next = NULL;
	
	attachments_str = g_strdup( (gchar *) attachments->data );
	
	for (next = g_slist_next(attachments); next != NULL; next = g_slist_next(next))
	{
		tmp = g_strconcat(attachments_str, ",", (gchar *) (next->data), NULL);
		g_free(attachments_str);
		attachments_str = tmp;
		if (attachments_str == NULL) {
			return OSSO_ERROR;
		}
	}

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
		
	if (ret != OSSO_OK) {
		printf("debug: osso_rpc_run() failed.\n");
		return FALSE;
	} else {
		printf("debug: osso_rpc_run() succeeded.\n");
	}
	
	osso_rpc_free_val(&retval);
	
	return TRUE;
}

gboolean 
libmodest_dbus_client_open_message (osso_context_t *osso_context, const gchar *mail_uri)
{
	osso_rpc_t retval;
	const osso_return_t ret = osso_rpc_run_with_defaults(osso_context, 
		   MODEST_DBUS_NAME, 
		   MODEST_DBUS_METHOD_OPEN_MESSAGE, &retval, 
		   DBUS_TYPE_STRING, mail_uri, 
		   DBUS_TYPE_INVALID);
		
	if (ret != OSSO_OK) {
		printf("debug: osso_rpc_run() failed.\n");
		return FALSE;
	} else {
		printf("debug: osso_rpc_run() succeeded.\n");
	}
	
	osso_rpc_free_val(&retval);
	
	return TRUE;
}

gboolean 
libmodest_dbus_client_send_and_receive (osso_context_t *osso_context)
{
	osso_rpc_t retval;
	const osso_return_t ret = osso_rpc_run_with_defaults(osso_context, 
		   MODEST_DBUS_NAME, 
		   MODEST_DBUS_METHOD_SEND_RECEIVE, &retval, 
		   DBUS_TYPE_INVALID);
		
	if (ret != OSSO_OK) {
		printf("debug: osso_rpc_run() failed.\n");
		return FALSE;
	} else {
		printf("debug: osso_rpc_run() succeeded.\n");
	}
	
	osso_rpc_free_val(&retval);
	
	return TRUE;
}

static void
modest_search_hit_free (ModestSearchHit *hit)
{
	g_free (hit->msgid);

	g_slice_free (ModestSearchHit, hit);
}


static char *
_dbus_iter_get_string_or_null (DBusMessageIter *iter)
{
	const char *string;
	char       *ret;

	dbus_message_iter_get_basic (iter, &string);
	
	ret = NULL;
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


static ModestSearchHit *
dbus_message_iter_get_search_hit (DBusMessageIter *parent)
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

	g_debug ("Umarshalling hit (%d)", dbus_message_iter_get_arg_type (parent));

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

	/* msize  */
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
		g_warning ("Error during unmarshaling");
		modest_search_hit_free (hit);
		hit = NULL;
	}

	return hit;
}


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
  	DBusError err;
	DBusConnection *con;
	DBusMessageIter iter;
	DBusMessageIter child;
        DBusMessage *reply = NULL;
	gint timeout;
	int          arg_type;
	dbus_int64_t sd_v;
	dbus_int64_t ed_v;
	dbus_int32_t flags_v;
	dbus_uint32_t size_v;


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

	sd_v = start_date;
	ed_v = end_date;
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

	dbus_error_init (&err);

	timeout = 1000; //XXX
	osso_rpc_get_timeout (osso_ctx, &timeout);

	reply = dbus_connection_send_with_reply_and_block (con,
							   msg, 
							   timeout,
							   &err);

	dbus_message_unref (msg);


	if (reply == NULL) {
            //ULOG_ERR_F("dbus_connection_send_with_reply_and_block error: %s", err.message);
	    //XXX to GError?! 
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

	g_debug ("message return");

	dbus_message_iter_init (reply, &iter);
	arg_type = dbus_message_iter_get_arg_type (&iter);
	
	g_debug ("iter type: %d", arg_type);
	dbus_message_iter_recurse (&iter, &child);
	g_debug ("recursed");

	do {
		ModestSearchHit *hit;

		hit = dbus_message_iter_get_search_hit (&child);

		if (hit) {
			*hits = g_list_prepend (*hits, hit);	
		}

	} while (dbus_message_iter_next (&child));

	dbus_message_unref (reply);

	g_debug ("Done unmarshalling message");
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

