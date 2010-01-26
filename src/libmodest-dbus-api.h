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


#ifndef __MODEST_DBUS_API__
#define __MODEST_DBUS_API__


/* Note that the com.nokia service name and /com/nokia object name 
 * are what is assumed by the, bizarrely named, osso_rpc_run_with_defaults() function, 
 * so they are probably a good choice. */
#define MODEST_DBUS_NAME    "modest"
#define MODEST_DBUS_SERVICE "com.nokia."MODEST_DBUS_NAME
#define MODEST_DBUS_OBJECT  "/com/nokia/"MODEST_DBUS_NAME /* Also known as a D-Bus Path. */
#define MODEST_DBUS_IFACE   "com.nokia."MODEST_DBUS_NAME


#define MODEST_DBUS_METHOD_MAIL_TO "MailTo"
enum ModestDbusMailToArguments
{
	MODEST_DBUS_MAIL_TO_ARG_URI,
	MODEST_DBUS_MAIL_TO_ARGS_COUNT
};

#define MODEST_DBUS_METHOD_OPEN_MESSAGE "OpenMessage"
enum ModestDbusOpenMessageArguments
{
	MODEST_DBUS_OPEN_MESSAGE_ARG_URI,
	MODEST_DBUS_OPEN_MESSAGE_ARGS_COUNT
};

#define MODEST_DBUS_METHOD_OPEN_ACCOUNT "OpenAccount"
enum ModestDbusOpenAccountArguments
{
	MODEST_DBUS_OPEN_ACCOUNT_ARG_ID,
	MODEST_DBUS_OPEN_ACCOUNT_ARGS_COUNT
};

#define MODEST_DBUS_METHOD_SEND_RECEIVE "SendReceive"

#define MODEST_DBUS_METHOD_SEND_RECEIVE_FULL "SendReceiveFull"
enum ModestDbusSendReceiveFullArguments
{
	MODEST_DBUS_SEND_RECEIVE_FULL_ARG_ACCOUNT_ID,
	MODEST_DBUS_SEND_RECEIVE_FULL_ARG_MANUAL,
	MODEST_DBUS_SEND_RECEIVE_FULL_ARGS_COUNT
};

#define MODEST_DBUS_METHOD_COMPOSE_MAIL "ComposeMail"
enum ModestDbusComposeMailArguments
{
	MODEST_DBUS_COMPOSE_MAIL_ARG_TO,
	MODEST_DBUS_COMPOSE_MAIL_ARG_CC,
	MODEST_DBUS_COMPOSE_MAIL_ARG_BCC,
	MODEST_DBUS_COMPOSE_MAIL_ARG_SUBJECT,
	MODEST_DBUS_COMPOSE_MAIL_ARG_BODY,
	MODEST_DBUS_COMPOSE_MAIL_ARG_ATTACHMENTS,
	MODEST_DBUS_COMPOSE_MAIL_ARGS_COUNT
};

#define MODEST_DBUS_METHOD_DELETE_MESSAGE "DeleteMessage"
enum ModestDbusDeleteMessageArguments
{
	MODEST_DBUS_DELETE_MESSAGE_ARG_URI,
	MODEST_DBUS_DELETE_MESSAGE_ARGS_COUNT
};

#define MODEST_DBUS_METHOD_OPEN_DEFAULT_INBOX "OpenDefaultInbox"

#define MODEST_DBUS_METHOD_OPEN_EDIT_ACCOUNTS_DIALOG "OpenEditAccountsDialog"

#define MODEST_DBUS_METHOD_GET_UNREAD_MESSAGES "GetUnreadMessages"
enum ModestDbusGetUnreadMessagesArguments
{
	MODEST_DBUS_GET_UNREAD_MESSAGES_ARG_MSGS_PER_ACCOUNT,
	MODEST_DBUS_GET_UNREAD_MESSAGES_ARGS_COUNT
};

/*
 * these methods are for debugging only, and should _not_ be
 * exported through libmodest-dbus-client
 */
#define MODEST_DBUS_METHOD_DUMP_OPERATION_QUEUE   "DumpOperationQueue"
#define MODEST_DBUS_METHOD_DUMP_ACCOUNTS          "DumpAccounts"
#define MODEST_DBUS_METHOD_DUMP_SEND_QUEUES       "DumpSendQueues"



/* These are handle via normal D-Bus instead of osso-rpc: */
#define MODEST_DBUS_METHOD_SEARCH "Search"
#define MODEST_DBUS_METHOD_GET_FOLDERS "GetFolders"

/** This is an undocumented hildon-desktop method that is 
 * sent to applications when they are started from the menu,
 * but not when started from D-Bus activation, so that 
 * applications can be started without visible UI.
 * At least, I think so. murrayc.
 **/
#define MODEST_DBUS_METHOD_TOP_APPLICATION "top_application"

#define MODEST_DBUS_METHOD_UPDATE_FOLDER_COUNTS "update_folder_counts"
enum ModestDbusUpdateFolderCountsArguments
{
	MODEST_DBUS_UPDATE_FOLDER_COUNTS_ARG_ACCOUNT_ID,
	MODEST_DBUS_UPDATE_FOLDER_COUNTS_ARGS_COUNT
};

/* signal emitted when an account has been created */
#define MODEST_DBUS_SIGNAL_ACCOUNT_CREATED "account_created"
enum ModestDbusSignalAccountCreatedArguments
{
	MODEST_DBUS_SIGNAL_ACCOUNT_CREATED_ARG_ACCOUNT_ID,
	MODEST_DBUS_SIGNAL_ACCOUNT_CREATED_ARGS_COUNT
};

/* signal emitted when an account has been removed */
#define MODEST_DBUS_SIGNAL_ACCOUNT_REMOVED "account_removed"
enum ModestDbusSignalAccountRemovedArguments
{
	MODEST_DBUS_SIGNAL_ACCOUNT_REMOVED_ARG_ACCOUNT_ID,
	MODEST_DBUS_SIGNAL_ACCOUNT_REMOVED_ARGS_COUNT
};

/* signal emitted when a folder is updated */
#define MODEST_DBUS_SIGNAL_FOLDER_UPDATED "folder_updated"
enum ModestDbusSignalFolderUpdatedArguments
{
	MODEST_DBUS_SIGNAL_FOLDER_UPDATED_ARG_ACCOUNT_ID,
	MODEST_DBUS_SIGNAL_FOLDER_UPDATED_ARG_FOLDER_ID,
	MODEST_DBUS_SIGNAL_FOLDER_UPDATED_ARGS_COUNT
};

/* signal emitted when a message read/unread flag is changed */
#define MODEST_DBUS_SIGNAL_MSG_READ_CHANGED "msg_read_changed"
enum ModestDbusSignalMsgReadChangedArguments
{
	MODEST_DBUS_SIGNAL_MSG_READ_CHANGED_ARG_MSG_ID,
	MODEST_DBUS_SIGNAL_MSG_READ_CHANGED_ARG_READ,
	MODEST_DBUS_SIGNAL_MSG_READ_CHANGED_ARGS_COUNT
};

#endif /* __MODEST_DBUS_API__ */
