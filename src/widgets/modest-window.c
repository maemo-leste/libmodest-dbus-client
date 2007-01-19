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

#include "modest-window.h"
#include "modest-window-priv.h"
#include "modest-tny-platform-factory.h"

/* 'private'/'protected' functions */
static void modest_window_class_init (ModestWindowClass *klass);
static void modest_window_init       (ModestWindow *obj);
static void modest_window_finalize   (GObject *obj);
/* list my signals  */
enum {
	LAST_SIGNAL
};

/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_window_get_type (void)
{
	static GType my_type = 0;
	static GType parent_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_window_init,
			NULL
		};
#if MODEST_PLATFORM_ID==1   /* gtk */
		parent_type = GTK_TYPE_WINDOW;
#elif MODEST_PLATFORM_ID==2   /* hildon (maemo) */
		parent_type = HILDON_TYPE_WINDOW;
#endif
		my_type = g_type_register_static (parent_type,
		                                  "ModestWindow",
		                                  &my_info, 
						  G_TYPE_FLAG_ABSTRACT);
	}
	return my_type;
}

static void
modest_window_class_init (ModestWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestWindowPrivate));
}

static void
modest_window_init (ModestWindow *obj)
{
	ModestWindowPrivate *priv;

	priv = MODEST_WINDOW_GET_PRIVATE(obj);

	priv->plat_factory  = modest_tny_platform_factory_get_instance ();
	priv->ui_manager    = NULL;
	priv->account_store = NULL;

	priv->toolbar       = NULL;
	priv->menubar       = NULL;
}

static void
modest_window_finalize (GObject *obj)
{
	ModestWindowPrivate *priv;	

	priv = MODEST_WINDOW_GET_PRIVATE(obj);

	if (priv->ui_manager) {
		g_object_unref (G_OBJECT(priv->ui_manager));
		priv->ui_manager = NULL;
	}
	if (priv->account_store) {
		g_object_unref (G_OBJECT(priv->account_store));
		priv->account_store = NULL;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

TnyAccountStore * 
modest_window_get_account_store (ModestWindow *window)
{
	ModestWindowPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_WINDOW (window), NULL);

	priv = MODEST_WINDOW_GET_PRIVATE (window);

	return g_object_ref (priv->account_store);
}

ModestWidgetFactory *
modest_window_get_widget_factory (ModestWindow *window)
{
	ModestWindowPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_WINDOW (window), NULL);

	priv = MODEST_WINDOW_GET_PRIVATE (window);

	return g_object_ref (priv->widget_factory);
}
