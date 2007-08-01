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

#include <modest-platform.h>
#include <modest-tny-platform-factory.h>
#include <modest-tny-account.h>
#include <modest-tny-account-store.h>
#include <modest-tny-local-folders-account.h>
#include <modest-runtime.h>
#include <tny-simple-list.h>
#include <modest-tny-folder.h>
#include <modest-tny-outbox-account.h>
#include <modest-account-mgr-helpers.h>
#include <modest-init.h>
#include <tny-camel-transport-account.h>
#include <tny-camel-imap-store-account.h>
#include <tny-camel-pop-store-account.h>
#include <tny-folder-stats.h>
#include <string.h>
#ifdef MODEST_HAVE_HILDON0_WIDGETS
#include <hildon-widgets/hildon-file-system-info.h>
#else
#include <hildon/hildon-file-system-info.h>
#endif

TnyFolder *
modest_tny_account_get_special_folder (TnyAccount *account,
				       TnyFolderType special_type)
{
	TnyList *folders;
	TnyIterator *iter;
	TnyFolder *special_folder = NULL;

	
	g_return_val_if_fail (account, NULL);
	g_return_val_if_fail (0 <= special_type && special_type < TNY_FOLDER_TYPE_NUM,
			      NULL);
	
	TnyAccount *local_account  = NULL;
		
	/* The accounts have already been instantiated by 
	 * modest_tny_account_store_get_accounts(), which is the 
	 * TnyAccountStore::get_accounts_func() implementation,
	 * so we just get them here.
	 */
	 
	/* Per-account outbox folders are each in their own on-disk directory: */
	if (special_type == TNY_FOLDER_TYPE_OUTBOX) {
		const gchar *modest_account_name = 
			modest_tny_account_get_parent_modest_account_name_for_server_account (account);
		
		if (modest_account_name) {
			gchar *account_id = g_strdup_printf (
				MODEST_PER_ACCOUNT_LOCAL_OUTBOX_FOLDER_ACCOUNT_ID_PREFIX "%s", 
				modest_account_name);
			
			local_account = modest_tny_account_store_get_tny_account_by (modest_runtime_get_account_store(),
										     MODEST_TNY_ACCOUNT_STORE_QUERY_ID,
										     account_id);
			if (!local_account) {
				g_printerr ("modest: %s: modest_tny_account_store_get_tny_account_by(ID) returned NULL for %s\n", __FUNCTION__, account_id);
			return NULL;
			}
		
			g_free (account_id);
		} else {
			g_warning ("%s: modest_account_name was NULL.", __FUNCTION__);
		}
	} else {
		/* Other local folders are all in one on-disk directory: */
		local_account = modest_tny_account_store_get_tny_account_by (modest_runtime_get_account_store(),
									     MODEST_TNY_ACCOUNT_STORE_QUERY_ID,
									     MODEST_LOCAL_FOLDERS_ACCOUNT_ID);
	}
	
	if (!local_account) {
		g_printerr ("modest: cannot get local account\n");
		return NULL;
	}

	folders = TNY_LIST (tny_simple_list_new ());

	/* There is no need to do this _async, as these are local folders. */
	/* TODO: However, this seems to fail sometimes when the network is busy, 
	 * returning an empty list. murrayc. */	
	GError *error = NULL;
	tny_folder_store_get_folders (TNY_FOLDER_STORE (local_account),
				      folders, NULL, &error);
	if (error) {
		g_warning ("%s: tny_folder_store_get_folders() failed:\n  error=%s\n", 
			__FUNCTION__, error->message);
	}
				      
	if (tny_list_get_length (folders) == 0) {
		gchar* url_string = tny_account_get_url_string (local_account);
		g_printerr ("modest: %s: tny_folder_store_get_folders() returned an empty list for account with URL '%s'\n", 
			__FUNCTION__, url_string);
		g_free (url_string);
	}
	
	iter = tny_list_create_iterator (folders);

	while (!tny_iterator_is_done (iter)) {
		TnyFolder *folder =
			TNY_FOLDER (tny_iterator_get_current (iter));
		if (folder) {
			if (modest_tny_folder_get_local_or_mmc_folder_type (folder) == special_type) {
				special_folder = folder;
				break; /* Leaving a ref for the special_folder return value. */
			}
		
			g_object_unref (G_OBJECT(folder));
		}

		tny_iterator_next (iter);
	}
	
	g_object_unref (G_OBJECT (folders));
	g_object_unref (G_OBJECT (iter));
	g_object_unref (G_OBJECT (local_account));

	/*
	if (!special_folder) {
		g_warning ("%s: Returning NULL.", __FUNCTION__);	
	}
	*/
	
	return special_folder;
}

static void
on_connection_status_changed (TnyAccount *account, TnyConnectionStatus status, gpointer user_data)
{
	printf ("DEBUG: %s: status=%d\n", __FUNCTION__, status);
	
	if (status == TNY_CONNECTION_STATUS_DISCONNECTED) {
		/* A tinymail network operation failed, and tinymail then noticed that 
		 * the account is offline, because our TnyDevice is offline,
		 * because libconic says we are offline.
		 * So ask the user to go online again.
		 * 
		 * Note that this signal will not be emitted if the account was offline 
		 * when the network operation was first attempted. For those cases, 
		 * the application must do its own explicit checks.
		 *
		 * We make sure that this UI is shown in the main thread, to avoid races,
		 * because tinymail does not guarantee that this signal handler will be called 
		 * in the main thread.
		 */
		/* TODO: Commented out, because this causes hangs, probably related to 
		 * our use of mainloops:
		 * modest_platform_connect_and_wait (NULL);
		 */
	} else if (status == TNY_CONNECTION_STATUS_CONNECTED_BROKEN) {
		printf ("DEBUG: %s: Connection broken. Forcing TnyDevice offline.\n", 
			__FUNCTION__);
			
		/* Something went wrong during some network operation.
		 * Stop trying to use the network now,
		 * by forcing accounts into offline mode:
		 * 
		 * When libconic reconnects, it will set the device back online again,
		 * regardless of it being forced offline before.
		 */
		/* TODO: Find out when this is falsely being emitted. */
		printf ("  DEBUG: %s: Not forcing offline because tinymail is sometimes reporting false connection breaks.\n", 
			__FUNCTION__);
		/*
		TnyDevice *device = modest_runtime_get_device ();
		tny_device_force_offline (device);
		*/
	}
}



/**
 * create_tny_account:
 * @account_mgr: a valid account mgr instance
 * @session: A valid TnySessionCamel instance.
 * @account_data: the server account for which to create a corresponding tny account
 * 
 * get a tnyaccount corresponding to the server_accounts (store or transport) for this account.
 * NOTE: this function does not set the camel session or the get/forget password functions
 * 
 * Returns: a new TnyAccount or NULL in case of error.
 */
static TnyAccount*
create_tny_account (ModestAccountMgr *account_mgr,
		    TnySessionCamel *session,
		    ModestServerAccountData *account_data)
{
	g_return_val_if_fail (account_mgr, NULL);
	g_return_val_if_fail (session, NULL);
	g_return_val_if_fail (account_data, NULL);

	/* sanity checks */
	if (account_data->proto == MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN) {
		g_printerr ("modest: '%s' does not provide a protocol\n",
			    account_data->account_name);
		return NULL;
	}

	TnyAccount *tny_account = NULL;
	
	switch (account_data->proto) {
	case MODEST_PROTOCOL_TRANSPORT_SENDMAIL:
	case MODEST_PROTOCOL_TRANSPORT_SMTP:
		tny_account = TNY_ACCOUNT(tny_camel_transport_account_new ()); break;
	case MODEST_PROTOCOL_STORE_POP:
		tny_account = TNY_ACCOUNT(tny_camel_pop_store_account_new ()); break;
	case MODEST_PROTOCOL_STORE_IMAP:
		tny_account = TNY_ACCOUNT(tny_camel_imap_store_account_new ()); break;
	case MODEST_PROTOCOL_STORE_MAILDIR:
	case MODEST_PROTOCOL_STORE_MBOX:
		/* Note that this is not where we create the special local folders account.
		 * That happens in modest_tny_account_new_for_local_folders() instead.
		 */
		tny_account = TNY_ACCOUNT(tny_camel_store_account_new()); break;
	default:
		g_return_val_if_reached (NULL);
	}
	if (!tny_account) {
		g_printerr ("modest: could not create tny account for '%s'\n",
			    account_data->account_name);
		return NULL;
	}
	tny_account_set_id (tny_account, account_data->account_name);

	/* This must be set quite early, or other set() functions will fail. */
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT (tny_account), session);
    
	/* Handle connection requests:
	 * This (badly-named) signal will be called when we try to use an offline account. */
	g_signal_connect (G_OBJECT (tny_account), "connection-status-changed",
			G_CALLBACK (on_connection_status_changed), NULL);

	/* Proto */
	const gchar* proto_name =
		modest_protocol_info_get_transport_store_protocol_name(account_data->proto);
	tny_account_set_proto (tny_account, proto_name);

	return tny_account;
}



/* Camel options: */

/* These seem to be listed in 
 * libtinymail-camel/camel-lite/camel/providers/imap/camel-imap-store.c 
 */
#define MODEST_ACCOUNT_OPTION_SSL "use_ssl"
#define MODEST_ACCOUNT_OPTION_SSL_NEVER "never"
/* This is a tinymail camel-lite specific option, 
 * roughly equivalent to "always" in regular camel,
 * which is appropriate for a generic "SSL" connection option: */
#define MODEST_ACCOUNT_OPTION_SSL_WRAPPED "wrapped"
/* Not used in our UI so far: */
#define MODEST_ACCOUNT_OPTION_SSL_WHEN_POSSIBLE "when-possible"
/* This is a tinymailcamel-lite specific option that is not in regular camel. */
#define MODEST_ACCOUNT_OPTION_SSL_TLS "tls"

/* These seem to be listed in 
 * libtinymail-camel/camel-lite/camel/providers/imap/camel-imap-provider.c 
 */
#define MODEST_ACCOUNT_OPTION_USE_LSUB "use_lsub" /* Show only subscribed folders */
#define MODEST_ACCOUNT_OPTION_CHECK_ALL "check_all" /* Check for new messages in all folders */

/* Posssible values for tny_account_set_secure_auth_mech().
 * These might be camel-specific.
 * Really, tinymail should use an enum.
 * camel_sasl_authtype() seems to list some possible values.
 */
 
/* Note that evolution does not offer these for IMAP: */
#define MODEST_ACCOUNT_AUTH_PLAIN "PLAIN"
#define MODEST_ACCOUNT_AUTH_ANONYMOUS "ANONYMOUS"

/* Caeml's IMAP uses NULL instead for "Password".
 * Also, not that Evolution offers "Password" for IMAP, but "Login" for SMTP.*/
#define MODEST_ACCOUNT_AUTH_PASSWORD "LOGIN" 
#define MODEST_ACCOUNT_AUTH_CRAMMD5 "CRAM-MD5"

		
/**
 * update_tny_account:
 * @account_mgr: a valid account mgr instance
 * @account_data: the server account for which to create a corresponding tny account
 * 
 * get a tnyaccount corresponding to the server_accounts (store or transport) for this account.
 * NOTE: this function does not set the camel session or the get/forget password functions
 * 
 * Returns: a new TnyAccount or NULL in case of error.
 */
static gboolean
update_tny_account (TnyAccount *tny_account, ModestAccountMgr *account_mgr,
		    ModestServerAccountData *account_data)
{
	gchar *url = NULL;
	
	g_return_val_if_fail (account_mgr, FALSE);
	g_return_val_if_fail (account_data, FALSE);
	g_return_val_if_fail (tny_account, FALSE);

	tny_account_set_id (tny_account, account_data->account_name);
	       
	/* mbox and maildir accounts use a URI instead of the rest:
	 * Note that this is not where we create the special local folders account.
	 * We do that in modest_tny_account_new_for_local_folders() instead. */
	if (account_data->uri)  
		tny_account_set_url_string (TNY_ACCOUNT(tny_account), account_data->uri);
	else {
		/* Set camel-specific options: */
		
		/* Enable secure connection settings: */
		const gchar* option_security = NULL;
		switch (account_data->security) {
		case MODEST_PROTOCOL_CONNECTION_NORMAL:
			option_security = MODEST_ACCOUNT_OPTION_SSL "=" MODEST_ACCOUNT_OPTION_SSL_NEVER;
			break;
		case MODEST_PROTOCOL_CONNECTION_SSL:
			/* Apparently, use of "IMAPS" (specified in our UI spec), implies 
			 * use of the "wrapped" option: */
			option_security = MODEST_ACCOUNT_OPTION_SSL "=" MODEST_ACCOUNT_OPTION_SSL_WRAPPED;
			break;
		case MODEST_PROTOCOL_CONNECTION_TLS:
			option_security = MODEST_ACCOUNT_OPTION_SSL "=" MODEST_ACCOUNT_OPTION_SSL_TLS;
			break;
		case MODEST_PROTOCOL_CONNECTION_TLS_OP:
			/* This is not actually in our UI: */
			option_security = MODEST_ACCOUNT_OPTION_SSL "=" MODEST_ACCOUNT_OPTION_SSL_WHEN_POSSIBLE;
			break;
		default:
			break;
		}
		
		if(option_security)
			tny_camel_account_add_option (TNY_CAMEL_ACCOUNT (tny_account),
						      option_security);
		
		/* Secure authentication: */

		const gchar* auth_mech_name = NULL;
		switch (account_data->secure_auth) {
		case MODEST_PROTOCOL_AUTH_NONE:
			/* IMAP and POP need at least a password,
			 * which camel uses if we specify NULL.
			 * This setting should never happen anyway. */
			if (account_data->proto == MODEST_PROTOCOL_STORE_IMAP ||
			    account_data->proto == MODEST_PROTOCOL_STORE_POP)
				auth_mech_name = NULL;
			else if (account_data->proto == MODEST_PROTOCOL_TRANSPORT_SMTP)
				auth_mech_name = MODEST_ACCOUNT_AUTH_ANONYMOUS;
			else
				auth_mech_name = MODEST_ACCOUNT_AUTH_PLAIN;
			break;
			
		case MODEST_PROTOCOL_AUTH_PASSWORD:
			/* Camel use a password for IMAP or POP if we specify NULL,
			 * For IMAP, at least it will report an error if we use "Password", "Login" or "Plain".
			 * (POP is know to report an error for Login too. Probably Password and Plain too.) */
			if (account_data->proto == MODEST_PROTOCOL_STORE_IMAP)
				auth_mech_name = NULL;
			else if (account_data->proto == MODEST_PROTOCOL_STORE_POP)
				auth_mech_name = NULL;
			else
				auth_mech_name = MODEST_ACCOUNT_AUTH_PASSWORD;
			break;
			
		case MODEST_PROTOCOL_AUTH_CRAMMD5:
			auth_mech_name = MODEST_ACCOUNT_AUTH_CRAMMD5;
			break;
			
		default:
			g_warning ("%s: Unhandled secure authentication setting %d for "
				"account=%s (%s)", __FUNCTION__, account_data->secure_auth,
				   account_data->account_name, account_data->hostname);
			break;
		}
		
		if(auth_mech_name) 
			tny_account_set_secure_auth_mech (tny_account, auth_mech_name);
		
		if (modest_protocol_info_protocol_is_store(account_data->proto) && 
			(account_data->proto == MODEST_PROTOCOL_STORE_IMAP) ) {
			/* Other connection options, needed for IMAP. */
			tny_camel_account_add_option (TNY_CAMEL_ACCOUNT (tny_account),
						      MODEST_ACCOUNT_OPTION_USE_LSUB);
			tny_camel_account_add_option (TNY_CAMEL_ACCOUNT (tny_account),
						      MODEST_ACCOUNT_OPTION_CHECK_ALL);
		}
		
		if (account_data->username) 
			tny_account_set_user (tny_account, account_data->username);
		if (account_data->hostname)
			tny_account_set_hostname (tny_account, account_data->hostname);
		 
		/* Set the port: */
		if (account_data->port)
			tny_account_set_port (tny_account, account_data->port);
	}

	/* FIXME: for debugging. 
	 * Let's keep this because it is very useful for debugging. */
	url = tny_account_get_url_string (TNY_ACCOUNT(tny_account));
	g_debug ("%s:\n  account-url: %s\n", __FUNCTION__, url);

	g_free (url);
	return TRUE;
}

TnyAccount*
modest_tny_account_new_from_server_account_name (ModestAccountMgr *account_mgr,
						 TnySessionCamel *session,
						 const gchar *server_account_name)
{
	ModestServerAccountData *account_data;
	TnyAccount *tny_account;
	
	g_return_val_if_fail (session, NULL);
	g_return_val_if_fail (server_account_name, NULL);

	account_data = 	modest_account_mgr_get_server_account_data (account_mgr, 
								    server_account_name);
	if (!account_data)
		return NULL;

	tny_account = create_tny_account (account_mgr, session, account_data);
	if (!tny_account)
		g_warning ("%s: failed to create tny_account", __FUNCTION__);
	else if (!update_tny_account (tny_account, account_mgr, account_data))
		g_warning ("%s: failed to initialize tny_account", __FUNCTION__);
	
	modest_account_mgr_free_server_account_data (account_mgr, account_data);
	
	return tny_account;
}


#if 0
gboolean
modest_tny_account_update_from_server_account_name (TnyAccount *tny_account,
						    ModestAccountMgr *account_mgr,
						    const gchar *server_account_name)
{
	ModestServerAccountData *account_data;
	gboolean valid_account_type;
	
	g_return_val_if_fail (tny_account, FALSE);
	g_return_val_if_fail (server_account_name, FALSE);
	
	account_data =	modest_account_mgr_get_server_account_data (account_mgr, 
								    server_account_name);
	if (!account_data) {
		g_warning ("%s: failed to get server account data for %s",
			   __FUNCTION__, server_account_name);
		return FALSE;
	}

	valid_account_type = FALSE;

	/* you cannot change the protocol type of an existing account;
	 * so double check we don't even try
	 */
	switch (account_data->proto) {
	case MODEST_PROTOCOL_TRANSPORT_SENDMAIL:
	case MODEST_PROTOCOL_TRANSPORT_SMTP:
		if (!TNY_IS_CAMEL_TRANSPORT_ACCOUNT(tny_account))
			g_warning ("%s: expecting transport account", __FUNCTION__);
		else
			valid_account_type = TRUE;
		break;
	case MODEST_PROTOCOL_STORE_POP:
		if (!TNY_IS_CAMEL_POP_STORE_ACCOUNT(tny_account))
			g_warning ("%s: expecting pop account", __FUNCTION__);
		else
			valid_account_type = TRUE;
		break;
	case MODEST_PROTOCOL_STORE_IMAP:
		if (!TNY_IS_CAMEL_IMAP_STORE_ACCOUNT(tny_account))
			g_warning ("%s: expecting imap account", __FUNCTION__);
		else
			valid_account_type = TRUE;
		break;
	case MODEST_PROTOCOL_STORE_MAILDIR:
	case MODEST_PROTOCOL_STORE_MBOX:
		if (!TNY_IS_CAMEL_STORE_ACCOUNT(tny_account))
			g_warning ("%s: expecting store account", __FUNCTION__);
		else
			valid_account_type = TRUE;
		break;
	default:
		g_warning ("invalid account type");
	}

	if (!valid_account_type) {
		g_warning ("%s: protocol type cannot be changed", __FUNCTION__);
		modest_account_mgr_free_server_account_data (account_mgr, account_data);
		return FALSE;
	}
	
	if (!update_tny_account (tny_account, account_mgr, account_data)) {
		g_warning ("%s: failed to update account", __FUNCTION__);
		modest_account_mgr_free_server_account_data (account_mgr, account_data);
		return FALSE;
	}

	modest_account_mgr_free_server_account_data (account_mgr, account_data);
	return TRUE;
}
#endif




/* we need these dummy functions, or tinymail will complain */
static gchar*
get_pass_dummy (TnyAccount *account, const gchar *prompt, gboolean *cancel)
{
	return NULL;
}
static void
forget_pass_dummy (TnyAccount *account)
{
	/* intentionally left blank */
}



gboolean
modest_tny_account_update_from_account (TnyAccount *tny_account, ModestAccountMgr *account_mgr,
					const gchar *account_name, TnyAccountType type) 
{
	ModestAccountData *account_data = NULL;
	ModestServerAccountData *server_data = NULL;
	
	g_return_val_if_fail (tny_account, FALSE);
	g_return_val_if_fail (account_mgr, FALSE);
	g_return_val_if_fail (account_name, FALSE);
	g_return_val_if_fail (type == TNY_ACCOUNT_TYPE_STORE || type == TNY_ACCOUNT_TYPE_TRANSPORT,
			      FALSE);

	account_data = modest_account_mgr_get_account_data (account_mgr, account_name);
	if (!account_data) {
		g_printerr ("modest: %s: cannot get account data for account %s\n",
			    __FUNCTION__, account_name);
		return FALSE;
	}

	if (type == TNY_ACCOUNT_TYPE_STORE && account_data->store_account)
		server_data = account_data->store_account;
	else if (type == TNY_ACCOUNT_TYPE_TRANSPORT && account_data->transport_account)
		server_data = account_data->transport_account;
	if (!server_data) {
		g_printerr ("modest: no %s account defined for '%s'\n",
			    type == TNY_ACCOUNT_TYPE_STORE ? "store" : "transport",
			    account_data->display_name);
		modest_account_mgr_free_account_data (account_mgr, account_data);
		return FALSE;
	}
	
	update_tny_account (tny_account, account_mgr, server_data);
		
	/* This name is what shows up in the folder view -- so for some POP/IMAP/... server
 	 * account, we set its name to the account of which it is part. */
 
	if (account_data->display_name)
		tny_account_set_name (tny_account, account_data->display_name);

	modest_account_mgr_free_account_data (account_mgr, account_data);

	return TRUE;
}



TnyAccount*
modest_tny_account_new_from_account (ModestAccountMgr *account_mgr,
				     const gchar *account_name,
				     TnyAccountType type,
				     TnySessionCamel *session,
				     TnyGetPassFunc get_pass_func,
				     TnyForgetPassFunc forget_pass_func) 
{
	TnyAccount *tny_account = NULL;
	ModestAccountData *account_data = NULL;
	ModestServerAccountData *server_data = NULL;

	g_return_val_if_fail (account_mgr, NULL);
	g_return_val_if_fail (account_name, NULL);
	g_return_val_if_fail (session, NULL);
	g_return_val_if_fail (type == TNY_ACCOUNT_TYPE_STORE || type == TNY_ACCOUNT_TYPE_TRANSPORT,
			      NULL);

	account_data = modest_account_mgr_get_account_data (account_mgr, account_name);
	if (!account_data) {
		g_printerr ("modest: %s: cannot get account data for account %s\n",
			    __FUNCTION__, account_name);
		return NULL;
	}

	if (type == TNY_ACCOUNT_TYPE_STORE && account_data->store_account)
		server_data = account_data->store_account;
	else if (type == TNY_ACCOUNT_TYPE_TRANSPORT && account_data->transport_account)
		server_data = account_data->transport_account;
	if (!server_data) {
		g_printerr ("modest: no %s account defined for '%s'\n",
			    type == TNY_ACCOUNT_TYPE_STORE ? "store" : "transport",
			    account_data->display_name);
		modest_account_mgr_free_account_data (account_mgr, account_data);
		return NULL;
	}
	
	tny_account = create_tny_account (account_mgr,session, server_data);
	if (!tny_account) { 
		g_printerr ("modest: failed to create tny account for %s (%s)\n",
			    account_data->account_name, server_data->account_name);
		modest_account_mgr_free_account_data (account_mgr, account_data);
		return NULL;
	} else
		update_tny_account (tny_account, account_mgr, server_data);
		
	/* This name is what shows up in the folder view -- so for some POP/IMAP/... server
 	 * account, we set its name to the account of which it is part. */
 
	if (account_data->display_name)
		tny_account_set_name (tny_account, account_data->display_name);

	tny_account_set_forget_pass_func (tny_account,
					  forget_pass_func ? forget_pass_func : forget_pass_dummy);
	tny_account_set_pass_func (tny_account,
				   get_pass_func ? get_pass_func: get_pass_dummy);
	
        modest_tny_account_set_parent_modest_account_name_for_server_account (tny_account,
									      account_name);
        modest_account_mgr_free_account_data (account_mgr, account_data);

	return tny_account;
}

typedef struct
{
	TnyStoreAccount *account;
	
	ModestTnyAccountGetMmcAccountNameCallback callback;
	gpointer user_data;
} GetMmcAccountNameData;




/* Gets the memory card name: */
static void 
on_modest_file_system_info(HildonFileSystemInfoHandle *handle,
                             HildonFileSystemInfo *info,
                             const GError *error, gpointer data)
{
	GetMmcAccountNameData *callback_data = (GetMmcAccountNameData*)data;

	if (error) {
		g_warning ("%s: error=%s", __FUNCTION__, error->message);
  	}
	
	TnyAccount *account = TNY_ACCOUNT (callback_data->account);
	
	const gchar *previous_display_name = NULL;
	
	const gchar *display_name = NULL;
	if (!error && info) {
		display_name = hildon_file_system_info_get_display_name(info);
		previous_display_name = tny_account_get_name (account);
	}
		 
	/* printf ("DEBUG: %s: display name=%s\n", __FUNCTION__,  display_name); */
	tny_account_set_name (account, display_name);
		
	/* Inform the application that the name is now ready: */
	if (callback_data->callback)
		(*(callback_data->callback)) (callback_data->account, 
			callback_data->user_data);
	
	g_object_unref (callback_data->account);
	g_slice_free (GetMmcAccountNameData, callback_data);
}

void modest_tny_account_get_mmc_account_name (TnyStoreAccount* self, ModestTnyAccountGetMmcAccountNameCallback callback, gpointer user_data)
{
	/* Just use the hard-coded path for the single memory card,
	 * rather than try to figure out the path to the specific card by 
	 * looking at the maildir URI:
	 */
	const gchar *uri_real = MODEST_MCC1_VOLUMEPATH_URI;

	/*
	gchar* uri = tny_account_get_url_string (TNY_ACCOUNT (self));
	if (!uri)
		return;

	TODO: This gets the name of the folder, but we want the name of the volume.
	gchar *uri_real = NULL;
	const gchar* prefix = "maildir://localhost/";
	if ((strstr (uri, prefix) == uri) && (strlen(uri) > strlen(prefix)) )
		uri_real = g_strconcat ("file:///", uri + strlen (prefix), NULL);
	*/

	if (uri_real) {
		//This is freed in the callback:
		GetMmcAccountNameData * callback_data = g_slice_new0(GetMmcAccountNameData);
		callback_data->account = self;
		g_object_ref (callback_data->account); /* Unrefed when we destroy the struct. */
		callback_data->callback = callback;
		callback_data->user_data = user_data;
		
		/* TODO: gnome_vfs_volume_get_display_name() does not return 
		 * the same string. But why not? Why does hildon needs its own 
		 * function for this?
		 */
		/* printf ("DEBUG: %s Calling hildon_file_system_info_async_new() with URI=%s\n", __FUNCTION__, uri_real); */
		hildon_file_system_info_async_new(uri_real, 
			on_modest_file_system_info, callback_data /* user_data */);

		/* g_free (uri_real); */
	}

	/* g_free (uri); */
}

 				

TnyAccount*
modest_tny_account_new_for_local_folders (ModestAccountMgr *account_mgr, TnySessionCamel *session,
					  const gchar* location_filepath)
{

	
	/* Make sure that the directories exist: */
	modest_init_local_folders (location_filepath);

	TnyStoreAccount *tny_account;
	CamelURL *url;
	gchar *maildir, *url_string;

	g_return_val_if_fail (account_mgr, NULL);
	g_return_val_if_fail (session, NULL);

	
	if (!location_filepath) {
		/* A NULL filepath means that this is the special local-folders maildir 
		 * account: */
		tny_account = TNY_STORE_ACCOUNT (modest_tny_local_folders_account_new ());
	}
	else {
		/* Else, for instance, a per-account outbox maildir account: */
		tny_account = TNY_STORE_ACCOUNT (tny_camel_store_account_new ());
	}
		
	if (!tny_account) {
		g_printerr ("modest: %s: cannot create account for local folders. filepath=%s", 
			__FUNCTION__, location_filepath);
		return NULL;
	}
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT(tny_account), session);
	
	/* This path contains directories for each local folder.
	 * We have created them so that TnyCamelStoreAccount can find them 
	 * and report a folder for each directory: */
	maildir = modest_local_folder_info_get_maildir_path (location_filepath);
	url = camel_url_new ("maildir:", NULL);
	camel_url_set_path (url, maildir);
	/* Needed by tinymail's DBC assertions */
 	camel_url_set_host (url, "localhost");
	url_string = camel_url_to_string (url, 0);
	
	tny_account_set_url_string (TNY_ACCOUNT(tny_account), url_string);
/* 	printf("DEBUG: %s:\n  url=%s\n", __FUNCTION__, url_string); */

	/* TODO: Use a more generic way of identifying memory card paths, 
	 * and of marking accounts as memory card accounts, maybe
	 * via a derived TnyCamelStoreAccount ? */
	const gboolean is_mmc = 
		location_filepath && 
		(strcmp (location_filepath, MODEST_MCC1_VOLUMEPATH) == 0);
		
	/* The name of memory card locations will be updated asynchronously.
	 * This is just a default: */
	const gchar *name = is_mmc ? _("Memory Card") : 
		MODEST_LOCAL_FOLDERS_DEFAULT_DISPLAY_NAME;
	tny_account_set_name (TNY_ACCOUNT(tny_account), name); 
	
 	/* Get the correct display name for memory cards, asynchronously: */
 	if (location_filepath) {
 		GError *error = NULL;
 		gchar *uri = g_filename_to_uri(location_filepath, NULL, &error);
 		if (error) {
 			g_warning ("%s: g_filename_to_uri(%s) failed: %s", __FUNCTION__, 
 				location_filepath, error->message);
 			g_error_free (error);
 			error = NULL;	
 		} else if (uri) {
 			/* Get the account name asynchronously:
 			 * This might not happen soon enough, so some UI code might 
 			 * need to call this again, specifying a callback.
 			 */
 			modest_tny_account_get_mmc_account_name (tny_account, NULL, NULL);
 				
 			g_free (uri);
 			uri = NULL;
 		}
 	}
 	
	
	const gchar* id = is_mmc ? MODEST_MMC_ACCOUNT_ID :
		MODEST_LOCAL_FOLDERS_ACCOUNT_ID;
	tny_account_set_id (TNY_ACCOUNT(tny_account), id);
	
	tny_account_set_forget_pass_func (TNY_ACCOUNT(tny_account), forget_pass_dummy);
	tny_account_set_pass_func (TNY_ACCOUNT(tny_account), get_pass_dummy);
	
	modest_tny_account_set_parent_modest_account_name_for_server_account (
		TNY_ACCOUNT (tny_account), id);
	
	camel_url_free (url);
	g_free (maildir);
	g_free (url_string);

	return TNY_ACCOUNT(tny_account);
}


TnyAccount*
modest_tny_account_new_for_per_account_local_outbox_folder (ModestAccountMgr *account_mgr,
							    const gchar* account_name,
							    TnySessionCamel *session)
{
	g_return_val_if_fail (account_mgr, NULL);
	g_return_val_if_fail (account_name, NULL);
	g_return_val_if_fail (session, NULL);
	
	/* Notice that we create a ModestTnyOutboxAccount here, 
	 * instead of just a TnyCamelStoreAccount,
	 * so that we can later identify this as a special account for internal use only.
	 */
	TnyStoreAccount *tny_account = TNY_STORE_ACCOUNT (modest_tny_outbox_account_new ());
	if (!tny_account) {
		g_printerr ("modest: cannot create account for per-account local outbox folder.");
		return NULL;
	}
	
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT(tny_account), session);
	
	/* Make sure that the paths exists on-disk so that TnyCamelStoreAccount can 
	 * find it to create a TnyFolder for it: */
	gchar *folder_dir = modest_per_account_local_outbox_folder_info_get_maildir_path_to_outbox_folder (account_name); 
	modest_init_one_local_folder(folder_dir);
	g_free (folder_dir);
	folder_dir = NULL;

	/* This path should contain just one directory - "outbox": */
	gchar *maildir = 
		modest_per_account_local_outbox_folder_info_get_maildir_path (account_name);
			
	CamelURL *url = camel_url_new ("maildir:", NULL);
	camel_url_set_path (url, maildir);
	g_free (maildir);
	
	/* Needed by tinymail's DBC assertions */
 	camel_url_set_host (url, "localhost");
	gchar *url_string = camel_url_to_string (url, 0);
	camel_url_free (url);
	
	tny_account_set_url_string (TNY_ACCOUNT(tny_account), url_string);
/* 	printf("DEBUG: %s:\n  url=%s\n", __FUNCTION__, url_string); */
	g_free (url_string);

	/* This text should never been seen,
	 * because the per-account outbox accounts are not seen directly by the user.
	 * Their folders are merged and shown as one folder. */ 
	tny_account_set_name (TNY_ACCOUNT(tny_account), "Per-Account Outbox"); 
	
	gchar *account_id = g_strdup_printf (
		MODEST_PER_ACCOUNT_LOCAL_OUTBOX_FOLDER_ACCOUNT_ID_PREFIX "%s", 
		account_name);
	tny_account_set_id (TNY_ACCOUNT(tny_account), account_id);
	g_free (account_id);
	
	tny_account_set_forget_pass_func (TNY_ACCOUNT(tny_account), forget_pass_dummy);
	tny_account_set_pass_func (TNY_ACCOUNT(tny_account), get_pass_dummy);
	
	/* Make this think that it belongs to the modest local-folders parent account: */
	modest_tny_account_set_parent_modest_account_name_for_server_account (
		TNY_ACCOUNT (tny_account), MODEST_LOCAL_FOLDERS_ACCOUNT_ID);

	return TNY_ACCOUNT(tny_account);
}



typedef gint (*TnyStatsFunc) (TnyFolderStats *stats);
#define TASK_GET_ALL_COUNT	0
#define TASK_GET_LOCAL_SIZE	1
#define TASK_GET_FOLDER_COUNT	2

typedef struct _RecurseFoldersHelper {
	gint task;
	guint sum;
	guint folders;
} RecurseFoldersHelper;

static void
recurse_folders (TnyFolderStore *store, 
		 TnyFolderStoreQuery *query, 
		 RecurseFoldersHelper *helper)
{
	TnyIterator *iter;
	TnyList *folders = tny_simple_list_new ();

	tny_folder_store_get_folders (store, folders, query, NULL);
	iter = tny_list_create_iterator (folders);

	helper->folders += tny_list_get_length (folders);

	while (!tny_iterator_is_done (iter)) {
		TnyFolder *folder;

		folder = TNY_FOLDER (tny_iterator_get_current (iter));
		if (folder) {
			if (helper->task == TASK_GET_ALL_COUNT)
				helper->sum += tny_folder_get_all_count (folder);

			if (helper->task == TASK_GET_LOCAL_SIZE)
				helper->sum += tny_folder_get_local_size (folder);

			if (TNY_IS_FOLDER_STORE (folder))
				recurse_folders (TNY_FOLDER_STORE (folder), query, helper);

 			g_object_unref (folder);
		}

		tny_iterator_next (iter);
	}
	 g_object_unref (G_OBJECT (iter));
	 g_object_unref (G_OBJECT (folders));
}

gint 
modest_tny_folder_store_get_folder_count (TnyFolderStore *self)
{
	RecurseFoldersHelper *helper;
	gint retval;

	g_return_val_if_fail (TNY_IS_FOLDER_STORE (self), -1);

	/* Create helper */
	helper = g_malloc0 (sizeof (RecurseFoldersHelper));
	helper->task = TASK_GET_FOLDER_COUNT;
	helper->sum = 0;
	helper->folders = 0;

	recurse_folders (self, NULL, helper);

	retval = helper->folders;

	g_free (helper);

	return retval;
}

gint
modest_tny_folder_store_get_message_count (TnyFolderStore *self)
{
	RecurseFoldersHelper *helper;
	gint retval;

	g_return_val_if_fail (TNY_IS_FOLDER_STORE (self), -1);
	
	/* Create helper */
	helper = g_malloc0 (sizeof (RecurseFoldersHelper));
	helper->task = TASK_GET_ALL_COUNT;
	helper->sum = 0;

	recurse_folders (self, NULL, helper);

	retval = helper->sum;

	g_free (helper);

	return retval;
}

gint 
modest_tny_folder_store_get_local_size (TnyFolderStore *self)
{
	RecurseFoldersHelper *helper;
	gint retval;

	g_return_val_if_fail (TNY_IS_FOLDER_STORE (self), -1);

	/* Create helper */
	helper = g_malloc0 (sizeof (RecurseFoldersHelper));
	helper->task = TASK_GET_LOCAL_SIZE;
	helper->sum = 0;

	recurse_folders (self, NULL, helper);

	retval = helper->sum;

	g_free (helper);

	return retval;
}

const gchar* 
modest_tny_account_get_parent_modest_account_name_for_server_account (TnyAccount *self)
{
	return (const gchar *)g_object_get_data (G_OBJECT (self), "modest_account");
}

void 
modest_tny_account_set_parent_modest_account_name_for_server_account (TnyAccount *self, 
								      const gchar* parent_modest_account_name)
{
	g_object_set_data_full (G_OBJECT(self), "modest_account",
				(gpointer) g_strdup (parent_modest_account_name), g_free);
}

gboolean
modest_tny_account_is_virtual_local_folders (TnyAccount *self)
{
	/* We should make this more sophisticated if we ever use ModestTnyLocalFoldersAccount 
	 * for anything else. */
	return MODEST_IS_TNY_LOCAL_FOLDERS_ACCOUNT (self);
}


gboolean
modest_tny_account_is_memory_card_account (TnyAccount *self)
{
	const gchar* account_id = NULL;

	g_return_val_if_fail (TNY_ACCOUNT (self), FALSE);

	if (!self)
		return FALSE;

	account_id = tny_account_get_id (self);

	if (!account_id)
		return FALSE;
	else	
		return (strcmp (account_id, MODEST_MMC_ACCOUNT_ID) == 0);
}
