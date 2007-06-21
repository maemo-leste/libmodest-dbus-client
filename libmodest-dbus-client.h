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

#ifndef __LIBMODEST_DBUS_CLIENT_H__
#define __LIBMODEST_DBUS_CLIENT_H__

#include <libosso.h>
#include <glib.h>
#include <stdio.h>

gboolean
libmodest_dbus_client_send_mail (osso_context_t *osso_context, const gchar *to, 
	const gchar *cc, const gchar *bcc, const gchar* subject, const gchar* body, 
	GSList *attachments);

gboolean
libmodest_dbus_client_compose_mail (osso_context_t *osso_context, const gchar *to, 
	const gchar *cc, const gchar *bcc, const gchar* subject, const gchar* body, 
	GSList *attachments);
	
gboolean 
libmodest_dbus_client_mail_to (osso_context_t *osso_context, 
	const gchar *mailto_uri);

gboolean 
libmodest_dbus_client_open_message (osso_context_t *osso_context, 
	const gchar *mail_uri);

gboolean 
libmodest_dbus_client_send_and_receive (osso_context_t *osso_context);

gboolean 
libmodest_dbus_client_open_default_inbox (osso_context_t *osso_context);

typedef enum {

	MODEST_DBUS_SEARCH_SUBJECT   = (1 << 0),
	MODEST_DBUS_SEARCH_SENDER    = (1 << 1),
	MODEST_DBUS_SEARCH_RECIPIENT = (1 << 2),
	MODEST_DBUS_SEARCH_SIZE      = (1 << 3),
	MODEST_DBUS_SEARCH_BODY      = (1 << 6)

} ModestDBusSearchFlags;

typedef struct {
	gchar     *msgid; /* E.g. the URI of the message. */
	gchar     *subject;
	gchar     *folder; /* The name, not the URI. */
	gchar     *sender;
	guint64    msize;
	gboolean   has_attachment;
	gboolean   is_unread;
	gint64     timestamp;		 
} ModestSearchHit;


void modest_search_hit_list_free (GList *hits);


gboolean libmodest_dbus_client_search            (osso_context_t          *osso_ctx,
							const gchar             *query,
							const gchar             *folder,
							time_t                   start_date,
							time_t                   end_date,
							guint32                  min_size,
							ModestDBusSearchFlags    flags,
							GList                  **hits);

gboolean libmodest_dbus_client_delete_message   (osso_context_t   *osso_ctx,
							const char       *msg_uri);
							
							
typedef struct {
	gchar     *folder_uri;
	gchar     *folder_name;	 
} ModestFolderResult;

gboolean libmodest_dbus_client_get_folders (osso_context_t *osso_ctx, GList **folders);	

void modest_folder_result_list_free (GList *folders);

						
							
#endif /* __LIBMODEST_DBUS_CLIENT_H__ */
