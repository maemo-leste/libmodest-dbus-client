/* modest-tny-msg-view.c */

/* insert (c)/licensing information) */

#include "modest-tny-msg-view.h"
#include "modest-tny-stream-gtkhtml.h"
#include "modest-tny-msg-actions.h"

#include <tny-text-buffer-stream.h>
#include <string.h>
#include <regex.h>
#include <ctype.h>
#include <glib/gi18n.h>
#include <gtkhtml/gtkhtml.h>

/* 'private'/'protected' functions */
static void     modest_tny_msg_view_class_init   (ModestTnyMsgViewClass *klass);
static void     modest_tny_msg_view_init         (ModestTnyMsgView *obj);
static void     modest_tny_msg_view_finalize     (GObject *obj);


static GSList*  get_url_matches (GString *txt);
static gboolean fill_gtkhtml_with_txt (ModestTnyMsgView *self, GtkHTML* gtkhtml, const gchar* txt, TnyMsgIface *msg);

static gboolean on_link_clicked (GtkWidget *widget, const gchar *uri,
				 ModestTnyMsgView *msg_view);
static gboolean on_url_requested (GtkWidget *widget, const gchar *uri,
				  GtkHTMLStream *stream,
				  ModestTnyMsgView *msg_view);


/*
 * we need these regexps to find URLs in plain text e-mails
 */
typedef struct _UrlMatchPattern UrlMatchPattern;
struct _UrlMatchPattern {
	gchar   *regex;
	regex_t *preg;
	gchar   *prefix;
	
};
#define MAIL_VIEWER_URL_MATCH_PATTERNS  {\
	{ "(file|http|ftp|https)://[-A-Za-z0-9_$.+!*(),;:@%&=?/~#]+[-A-Za-z0-9_$%&=?/~#]",\
	  NULL, NULL },\
	{ "www\\.[-a-z0-9.]+[-a-z0-9](:[0-9]*)?(/[-A-Za-z0-9_$.+!*(),;:@%&=?/~#]*[^]}\\),?!;:\"]?)?",\
	  NULL, "http://" },\
	{ "ftp\\.[-a-z0-9.]+[-a-z0-9](:[0-9]*)?(/[-A-Za-z0-9_$.+!*(),;:@%&=?/~#]*[^]}\\),?!;:\"]?)?",\
	  NULL, "ftp://" },\
	{ "(voipto|callto|chatto|jabberto|xmpp):[-_a-z@0-9.\\+]+", \
	   NULL, NULL},						    \
	{ "mailto:[-_a-z0-9.\\+]+@[-_a-z0-9.]+",		    \
	  NULL, NULL},\
	{ "[-_a-z0-9.\\+]+@[-_a-z0-9.]+",\
	  NULL, "mailto:"}\
	}


/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestTnyMsgViewPrivate ModestTnyMsgViewPrivate;
struct _ModestTnyMsgViewPrivate {
	GtkWidget *gtkhtml;
	TnyMsgIface *msg;
	gboolean show_attachments;
};
#define MODEST_TNY_MSG_VIEW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                 MODEST_TYPE_TNY_MSG_VIEW, \
                                                 ModestTnyMsgViewPrivate))
/* globals */
static GtkContainerClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_tny_msg_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestTnyMsgViewClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_tny_msg_view_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestTnyMsgView),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_tny_msg_view_init,
		};
		my_type = g_type_register_static (GTK_TYPE_SCROLLED_WINDOW,
		                                  "ModestTnyMsgView",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_tny_msg_view_class_init (ModestTnyMsgViewClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_tny_msg_view_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestTnyMsgViewPrivate));
}

static void
modest_tny_msg_view_init (ModestTnyMsgView *obj)
{
 	ModestTnyMsgViewPrivate *priv;
	
	priv = MODEST_TNY_MSG_VIEW_GET_PRIVATE(obj);

	priv->msg = NULL;
	
	priv->gtkhtml = gtk_html_new();
	
	/* TODO: -> conf-mgr */
	priv->show_attachments = FALSE;

	gtk_html_set_editable        (GTK_HTML(priv->gtkhtml), FALSE);
        gtk_html_allow_selection     (GTK_HTML(priv->gtkhtml), TRUE);
        gtk_html_set_caret_mode      (GTK_HTML(priv->gtkhtml), FALSE);
        gtk_html_set_blocking        (GTK_HTML(priv->gtkhtml), FALSE);
        gtk_html_set_images_blocking (GTK_HTML(priv->gtkhtml), FALSE);
	
	g_signal_connect (G_OBJECT(priv->gtkhtml), "link_clicked",
			  G_CALLBACK(on_link_clicked), obj);
	
	g_signal_connect (G_OBJECT(priv->gtkhtml), "url_requested",
			  G_CALLBACK(on_url_requested), obj);
}
	

static void
modest_tny_msg_view_finalize (GObject *obj)
{	
	
}

GtkWidget*
modest_tny_msg_view_new (TnyMsgIface *msg)
{
	GObject *obj;
	ModestTnyMsgView* self;
	ModestTnyMsgViewPrivate *priv;
	
	obj  = G_OBJECT(g_object_new(MODEST_TYPE_TNY_MSG_VIEW, NULL));
	self = MODEST_TNY_MSG_VIEW(obj);
	priv = MODEST_TNY_MSG_VIEW_GET_PRIVATE (self);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(self),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	if (priv->gtkhtml) 
		gtk_container_add (GTK_CONTAINER(obj), priv->gtkhtml);	
	
	if (msg)
		modest_tny_msg_view_set_message (self, msg);

	return GTK_WIDGET(self);
}



static gboolean
on_link_clicked (GtkWidget *widget, const gchar *uri,
				 ModestTnyMsgView *msg_view)
{
	ModestTnyMsgViewPrivate *priv;
	
	
	if (g_str_has_prefix(uri, "attachment:")) {
		priv = MODEST_TNY_MSG_VIEW_GET_PRIVATE(msg_view);
		priv->show_attachments = !priv->show_attachments;
		modest_tny_msg_view_set_message(msg_view, priv->msg);
		return TRUE;
	}
	g_message ("link clicked: %s", uri); /* FIXME */
	return FALSE;
	
}



static TnyMsgMimePartIface *
find_cid_image (TnyMsgIface *msg, const gchar *cid)
{
	TnyMsgMimePartIface *part = NULL;
	GList *parts;

	g_return_val_if_fail (msg, NULL);
	g_return_val_if_fail (cid, NULL);
	
	parts  = (GList*) tny_msg_iface_get_parts (msg);
	while (parts && !part) {
		const gchar *part_cid;
		part = TNY_MSG_MIME_PART_IFACE(parts->data);
		part_cid = tny_msg_mime_part_iface_get_content_id (part);
		printf("CMP:%s:%s\n", cid, part_cid);
		if (part_cid && strcmp (cid, part_cid) == 0)
			return part; /* we found it! */
		
		part = NULL;
		parts = parts->next;
	}
	
	return part;
}

static TnyMsgMimePartIface *
find_attachment_by_filename (TnyMsgIface *msg, const gchar *fn)
{
	TnyMsgMimePartIface *part = NULL;
	GList *parts;

	g_return_val_if_fail (msg, NULL);
	g_return_val_if_fail (fn, NULL);
	
	parts  = (GList*) tny_msg_iface_get_parts (msg);
	while (parts && !part) {
		const gchar *part_fn;
		part = TNY_MSG_MIME_PART_IFACE(parts->data);
		part_fn = tny_msg_mime_part_iface_get_filename (part);
		if (part_fn && strcmp (fn, part_fn) == 0)
			return part; /* we found it! */
		
		part = NULL;
		parts = parts->next;
	}
	
	return part;
}

static gboolean
on_url_requested (GtkWidget *widget, const gchar *uri,
		  GtkHTMLStream *stream,
		  ModestTnyMsgView *msg_view)
{
	
	ModestTnyMsgViewPrivate *priv;
	priv = MODEST_TNY_MSG_VIEW_GET_PRIVATE (msg_view);

	g_message ("url requested: %s", uri);
	
	if (g_str_has_prefix (uri, "cid:")) {
		/* +4 ==> skip "cid:" */
		
		TnyMsgMimePartIface *part = find_cid_image (priv->msg, uri + 4);
		if (!part) {
			g_message ("%s not found", uri + 4);
			gtk_html_stream_close (stream, GTK_HTML_STREAM_ERROR);
		} else {
			TnyStreamIface *tny_stream =
				TNY_STREAM_IFACE(modest_tny_stream_gtkhtml_new(stream));
			tny_msg_mime_part_iface_decode_to_stream (part,tny_stream);
			gtk_html_stream_close (stream, GTK_HTML_STREAM_OK);
		}
	} else if (g_str_has_prefix (uri, "Attachment:")) {
		/* +11 ==> skip... */
		
		TnyMsgMimePartIface *part = find_attachment_by_filename (priv->msg, uri + 11);
		if (!part) {
			g_message ("%s not found", uri + 11);
			gtk_html_stream_close (stream, GTK_HTML_STREAM_ERROR);
		} else {
			TnyStreamIface *tny_stream =
				TNY_STREAM_IFACE(modest_tny_stream_gtkhtml_new(stream));
			tny_msg_mime_part_iface_decode_to_stream (part,tny_stream);
			gtk_html_stream_close (stream, GTK_HTML_STREAM_OK);
		}
	}
	return TRUE;
}




typedef struct  {
	guint offset;
	guint len;
	const gchar* prefix;
} url_match_t;


static gchar *
attachments_as_html(ModestTnyMsgView *self, TnyMsgIface *msg)
{
	ModestTnyMsgViewPrivate *priv;
	gboolean attachments_found = FALSE;
	GString *appendix;
	const GList *attachment_list, *attachment;
	const gchar *content_type, *filename;
	
	if (!msg)
		return g_malloc0(1);
	
	priv = MODEST_TNY_MSG_VIEW_GET_PRIVATE (self);
	
	/* CLEANUP: starting a new HTML may be unsupported */
	appendix = g_string_new("<HTML><BODY>\n<hr><h5>Attachments:</h5>\n");
	
	attachment_list = tny_msg_iface_get_parts(msg);
	attachment = attachment_list;
	while (attachment) {
		filename = "";
		content_type = tny_msg_mime_part_iface_get_content_type(
										TNY_MSG_MIME_PART_IFACE(attachment->data));
		g_return_val_if_fail(content_type, NULL);
		if (      tny_msg_mime_part_iface_content_type_is(
										TNY_MSG_MIME_PART_IFACE(attachment->data),
										"image/jpeg")
			   || tny_msg_mime_part_iface_content_type_is(
										TNY_MSG_MIME_PART_IFACE(attachment->data),
										"image/gif")) {
			filename = tny_msg_mime_part_iface_get_filename(
										TNY_MSG_MIME_PART_IFACE(attachment->data));
			if (!filename)
				filename = "[unknown]";
			else
				attachments_found = TRUE;
			if (priv->show_attachments) {
				g_string_append_printf(appendix, "<IMG src=\"Attachment:%s\">\n<BR><A href=\"attachment:%s\">%s</A>\n", filename, filename, filename);
			} else {
				g_string_append_printf(appendix, "<A href=\"attachment:%s\">%s</A>: %s<BR>\n", filename, filename, content_type);
			}
		}
		attachment = attachment->next;
	}
	g_string_append(appendix, "</BODY></HTML>");
	if (!attachments_found)
		g_string_assign(appendix, "");
	return g_string_free(appendix, FALSE);
}

static void
hyperlinkify_plain_text (GString *txt)
{
	GSList *cursor;
	GSList *match_list = get_url_matches (txt);

	/* we will work backwards, so the offsets stay valid */
	for (cursor = match_list; cursor; cursor = cursor->next) {

		url_match_t *match = (url_match_t*) cursor->data;
		gchar *url  = g_strndup (txt->str + match->offset, match->len);
		gchar *repl = NULL; /* replacement  */

		/* the prefix is NULL: use the one that is already there */
		repl = g_strdup_printf ("<a href=\"%s%s\">%s</a>",
					match->prefix ? match->prefix : "", url, url);

		/* replace the old thing with our hyperlink
		 * replacement thing */
		g_string_erase  (txt, match->offset, match->len);
		g_string_insert (txt, match->offset, repl);
		
		g_free (url);
		g_free (repl);

		g_free (cursor->data);	
	}
	
	g_slist_free (match_list);
}



static gchar *
convert_to_html (const gchar *data)
{
	int		 i;
	gboolean	 first_space = TRUE;
	GString		*html;	    
	gsize           len;

	if (!data)
		return NULL;

	len = strlen (data);
	html = g_string_sized_new (len + 100);	/* just a  guess... */
	
	g_string_append_printf (html,
				"<html>"
				"<head>"
				"<meta http-equiv=\"content-type\""
				" content=\"text/html; charset=utf8\">"
				"</head>"
				"<body><tt>");
	
	/* replace with special html chars where needed*/
	for (i = 0; i != len; ++i)  {
		char	kar = data[i]; 
		switch (kar) {
			
		case 0:  break; /* ignore embedded \0s */	
		case '<' : g_string_append   (html, "&lt;"); break;
		case '>' : g_string_append   (html, "&gt;"); break;
		case '&' : g_string_append   (html, "&quot;"); break;
		case '\n': g_string_append   (html, "<br>\n"); break;
		default:
			if (kar == ' ') {
				g_string_append (html, first_space ? " " : "&nbsp;");
				first_space = FALSE;
			} else	if (kar == '\t')
				g_string_append (html, "&nbsp; &nbsp;&nbsp;");
			else {
				int charnum = 0;
				first_space = TRUE;
				/* optimization trick: accumulate 'normal' chars, then copy */
				do {
					kar = data [++charnum + i];
					
				} while ((i + charnum < len) &&
					 (kar > '>' || (kar != '<' && kar != '>'
							&& kar != '&' && kar !=  ' '
							&& kar != '\n' && kar != '\t')));
				g_string_append_len (html, &data[i], charnum);
				i += (charnum  - 1);
			}
		}
	}
	
	g_string_append (html, "</tt></body></html>");
	hyperlinkify_plain_text (html);

	return g_string_free (html, FALSE);
}




static gint 
cmp_offsets_reverse (const url_match_t *match1, const url_match_t *match2)
{
	return match2->offset - match1->offset;
}



/*
 * check if the match is inside an existing match... */
static void
chk_partial_match (const url_match_t *match, int* offset)
{
	if (*offset >= match->offset && *offset < match->offset + match->len)
		*offset = -1;
}

static GSList*
get_url_matches (GString *txt)
{
	regmatch_t rm;
        int rv, i, offset = 0;
        GSList *match_list = NULL;

	static UrlMatchPattern patterns[] = MAIL_VIEWER_URL_MATCH_PATTERNS;
	const size_t pattern_num = sizeof(patterns)/sizeof(UrlMatchPattern);

	/* initalize the regexps */
	for (i = 0; i != pattern_num; ++i) {
		patterns[i].preg = g_new0 (regex_t,1);
		g_assert(regcomp (patterns[i].preg, patterns[i].regex,
				  REG_ICASE|REG_EXTENDED|REG_NEWLINE) == 0);
	}
        /* find all the matches */
	for (i = 0; i != pattern_num; ++i) {
		offset     = 0;	
		while (1) {
			int test_offset;
			if ((rv = regexec (patterns[i].preg, txt->str + offset, 1, &rm, 0)) != 0) {
				g_assert (rv == REG_NOMATCH); /* this should not happen */
				break; /* try next regexp */ 
			}
			if (rm.rm_so == -1)
				break;

			/* FIXME: optimize this */
			/* to avoid partial matches on something that was already found... */
			/* check_partial_match will put -1 in the data ptr if that is the case */
			test_offset = offset + rm.rm_so;
			g_slist_foreach (match_list, (GFunc)chk_partial_match, &test_offset);
			
			/* make a list of our matches (<offset, len, prefix> tupels)*/
			if (test_offset != -1) {
				url_match_t *match = g_new (url_match_t,1);
				match->offset = offset + rm.rm_so;
				match->len    = rm.rm_eo - rm.rm_so;
				match->prefix = patterns[i].prefix;
				match_list = g_slist_prepend (match_list, match);
			}
			offset += rm.rm_eo;
		}
	}

	for (i = 0; i != pattern_num; ++i) {
		regfree (patterns[i].preg);
		g_free  (patterns[i].preg);
	} /* don't free patterns itself -- it's static */
	
	/* now sort the list, so the matches are in reverse order of occurence.
	 * that way, we can do the replacements starting from the end, so we don't need
	 * to recalculate the offsets
	 */
	match_list = g_slist_sort (match_list,
				   (GCompareFunc)cmp_offsets_reverse); 
	return match_list;	
}

static gboolean
fill_gtkhtml_with_txt (ModestTnyMsgView *self, GtkHTML* gtkhtml, const gchar* txt, TnyMsgIface *msg)
{
	GString *html;
	gchar *html_attachments;
	
	g_return_val_if_fail (gtkhtml, FALSE);
	g_return_val_if_fail (txt, FALSE);

	html = g_string_new(convert_to_html (txt));
	html_attachments = attachments_as_html(self, msg);
	g_string_append(html, html_attachments);

	gtk_html_load_from_string (gtkhtml, html->str, html->len);
	g_string_free (html, TRUE);
	g_free(html_attachments);

	return TRUE;
}



static gboolean
set_html_message (ModestTnyMsgView *self, TnyMsgMimePartIface *tny_body, TnyMsgIface *msg)
{
	gchar *html_attachments;
	TnyStreamIface *gtkhtml_stream;	
	ModestTnyMsgViewPrivate *priv;
	
	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (tny_body, FALSE);
	
	priv = MODEST_TNY_MSG_VIEW_GET_PRIVATE(self);

	gtkhtml_stream =
		TNY_STREAM_IFACE(modest_tny_stream_gtkhtml_new
				 (gtk_html_begin(GTK_HTML(priv->gtkhtml))));
	
	tny_stream_iface_reset (gtkhtml_stream);
	tny_msg_mime_part_iface_decode_to_stream (tny_body, gtkhtml_stream);
	html_attachments = attachments_as_html(self, msg);
	/* is this clean? */
	gtkhtml_write(gtkhtml_stream, html_attachments, strlen(html_attachments));
	tny_stream_iface_reset (gtkhtml_stream);

	g_object_unref (G_OBJECT(gtkhtml_stream));
	g_free (html_attachments);
	
	return TRUE;
}


/* this is a hack --> we use the tny_text_buffer_stream to
 * get the message text, then write to gtkhtml 'by hand' */
static gboolean
set_text_message (ModestTnyMsgView *self, TnyMsgMimePartIface *tny_body, TnyMsgIface *msg)
{
	GtkTextBuffer *buf;
	GtkTextIter begin, end;
	TnyStreamIface* txt_stream;
	gchar *txt;
	ModestTnyMsgViewPrivate *priv;
		
	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (tny_body, FALSE);

	priv           = MODEST_TNY_MSG_VIEW_GET_PRIVATE(self);
	
	buf            = gtk_text_buffer_new (NULL);
	txt_stream     = TNY_STREAM_IFACE(tny_text_buffer_stream_new (buf));
		
	tny_stream_iface_reset (txt_stream);
	tny_msg_mime_part_iface_decode_to_stream (tny_body, txt_stream);
	tny_stream_iface_reset (txt_stream);		
	
	gtk_text_buffer_get_bounds (buf, &begin, &end);
	txt = gtk_text_buffer_get_text (buf, &begin, &end, FALSE);
	
	fill_gtkhtml_with_txt (self, GTK_HTML(priv->gtkhtml), txt, msg);

	g_object_unref (G_OBJECT(txt_stream));
	g_object_unref (G_OBJECT(buf));

	g_free (txt);
	return TRUE;
}

gchar *
modest_tny_msg_view_get_selected_text (ModestTnyMsgView *self)
{
	ModestTnyMsgViewPrivate *priv;
	gchar *sel;
	GtkWidget *html;
	int len;
	GtkClipboard *clip;

	g_return_val_if_fail (self, NULL);
	priv = MODEST_TNY_MSG_VIEW_GET_PRIVATE(self);
	html = priv->gtkhtml;
	
	/* I'm sure there is a better way to check for selected text */
	sel = gtk_html_get_selection_html(GTK_HTML(html), &len);
	if (!sel)
		return NULL;
	
	g_free(sel);
	
	clip = gtk_widget_get_clipboard(html, GDK_SELECTION_PRIMARY);
	return gtk_clipboard_wait_for_text(clip);
}

void
modest_tny_msg_view_set_message (ModestTnyMsgView *self, TnyMsgIface *msg)
{
	TnyMsgMimePartIface *body;
	ModestTnyMsgViewPrivate *priv;

	g_return_if_fail (self);
	
	priv = MODEST_TNY_MSG_VIEW_GET_PRIVATE(self);

	priv->msg = msg;
	
	fill_gtkhtml_with_txt (self, GTK_HTML(priv->gtkhtml), "", msg);

	if (!msg) 
		return;
	
	body = modest_tny_msg_actions_find_body_part (msg, "text/html");
	if (body) {
		set_html_message (self, body, msg);
		return;
	}
	
	body = modest_tny_msg_actions_find_body_part (msg, "text/plain");
	if (body) {
		set_text_message (self, body, msg);
		return;
	}

	/* hmmmmm */
	fill_gtkhtml_with_txt (self, GTK_HTML(priv->gtkhtml),
				_("Unsupported message type"), msg);
}
