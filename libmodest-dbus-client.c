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


gboolean
libmodest_dbus_client_call_helloworld(osso_context_t *osso_context)
{
	osso_rpc_t retval;
	const osso_return_t ret = osso_rpc_run_with_defaults(osso_context, 
		   MODEST_DBUS_NAME, 
		   MODEST_DBUS_EXAMPLE_MESSAGE, &retval, 
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
libmodfest_dbus_client_send_mail (osso_context_t *osso_context, const gchar *to, const gchar *cc, 
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
libmodfest_dbus_client_mailto (osso_context_t *osso_context, const gchar *mailto_uri)
{
	return FALSE;
}

gboolean 
libmodfest_dbus_client_open_message (osso_context_t *osso_context, const gchar *mail_uri)
{
	return FALSE;
}


