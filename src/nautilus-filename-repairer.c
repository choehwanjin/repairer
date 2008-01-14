/*
 * nautilus-filename-repairer.c
 *
 *  Copyright (C) 2008 Choe Hwanjin
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libnautilus-extension/nautilus-menu-provider.h>

#include "nautilus-filename-repairer.h"
#include "nautilus-filename-repairer-i18n.h"

static GType filename_repairer_type = 0;

// from http://www.microsoft.com/globaldev/reference/wincp.mspx
// Code Pages Supported by Windows
static const char* encoding_list[] = {
    "CP874",   // Thai
    "CP932",   // Japanese Shift-JIS
    "CP936",   // Simplified Chinese GBK
    "CP949",   // Korean
    "CP950",   // Traditional Chinese Big5
    "CP1250",  // Central Europe
    "CP1251",  // Cyrillic
    "CP1252",  // Latin I
    "CP1253",  // Greek
    "CP1254",  // Turkish
    "CP1255",  // Hebrew
    "CP1256",  // Arabic
    "CP1257",  // Baltic
    "CP1258",  // Vietnam
    NULL
};

struct encoding_item {
    const char* locale;
    const char* encoding;
};

static const struct encoding_item default_encoding_list[] = {
    { "ar",    "CP1256" }, 
    { "el",    "CP1253" }, 
    { "fr",    "CP1250" }, 
    { "ja",    "CP932"  }, 
    { "ko",    "CP949"  }, 
    { "ru",    "CP1251" }, 
    { "th",    "CP874"  }, 
    { "tr",    "CP1254" }, 
    { "vi",    "CP1258" }, 
    { "zh_CN", "CP936"  }, 
    { "zh_TW", "CP950"  },
};

static int
encoding_item_cmp(const void* a, const void* b)
{
    struct encoding_item* item = (struct encoding_item*)b;
    int len = strlen(item->locale);
    return strncmp(a, item->locale, len);
}

static const char*
get_default_encoding()
{
    char* locale = setlocale(LC_CTYPE, NULL);
    if (locale != NULL) {
	struct encoding_item* res;
	res = bsearch(locale,
		      default_encoding_list,
		      G_N_ELEMENTS(default_encoding_list),
		      sizeof(default_encoding_list[0]),
		      encoding_item_cmp);
	if (res != NULL) {
	    return res->encoding;
	}
    }

    return NULL;
}

static gboolean
candidate_list_has_item(GPtrArray* array, const char* str)
{
    guint i;
    for (i = 0; i < array->len; i++) {
	if (strcmp(str, array->pdata[i]) == 0) {
	    return TRUE;
	}
    }
    return FALSE;
}


static GPtrArray*
create_candidate_list(NautilusFileInfo* info)
{
    GPtrArray* array = NULL;

    char* uri = nautilus_file_info_get_uri(info);
    if (g_str_has_prefix(uri, "file:")) {
	char* filename = gnome_vfs_get_local_path_from_uri(uri);
	if (filename != NULL) {
	    char* unescaped_filename;
	    char* basename;
	    char* displayname;
	    char* replacement;

	    unescaped_filename = gnome_vfs_unescape_string(filename, NULL);
	    if (unescaped_filename != NULL &&
		strcmp(filename, unescaped_filename) != 0) {
		basename = g_path_get_basename(unescaped_filename);
	    } else {
		basename = g_path_get_basename(filename);
	    }

	    displayname = g_filename_display_name(basename);
	    // search for U+FFFD (the Unicode replacement char)
	    // if the filename has wrong char, displayname may have U+FFFD
	    // UTF-8 encoded form of U+FFFD is "\357\277\275"
	    replacement = strstr(displayname, "\357\277\275");
	    if (replacement != NULL) {
		const char* encoding;
		char* new_name;
		int i;

		array = g_ptr_array_new();

		encoding = get_default_encoding();
		if (encoding != NULL) {
		    new_name = g_convert(basename, -1,
				"UTF-8", encoding, NULL, NULL, NULL);
		    if (new_name != NULL) {
			char* locale_filename;
			locale_filename = g_filename_from_utf8(new_name, -1, 
						       NULL, NULL, NULL);
			if (locale_filename != NULL) {
			    g_ptr_array_add(array, new_name);
			    g_free(locale_filename);
			}
		    }
		}

		for (i = 0; encoding_list[i] != NULL; i++) {
		    new_name = g_convert(basename, -1,
			"UTF-8", encoding_list[i], NULL, NULL, NULL);
		    if (new_name != NULL) {
			char* locale_filename;
			locale_filename = g_filename_from_utf8(new_name, -1, 
						       NULL, NULL, NULL);
			if (locale_filename != NULL) {
			    if (!candidate_list_has_item(array, new_name)) {
				g_ptr_array_add(array, new_name);
			    } else {
				g_free(new_name);
			    }
			    g_free(locale_filename);
			}
		    }
		}
	    }

	    g_free(displayname);
	    g_free(basename);
	    g_free(unescaped_filename);
	    g_free(filename);
	}
    }
    g_free(uri);
    
    return array;
}

typedef struct {
    GPtrArray* candidates;
    GList*     files;
    GtkWidget* parent;
} RepairCallbackArgs;

static void
filename_repairer_callback_args_free(gpointer data, GClosure* closure)
{
    if (data != NULL) {
	RepairCallbackArgs* args = (RepairCallbackArgs*)data;
	g_ptr_array_free(args->candidates, TRUE);
	nautilus_file_info_list_free(args->files);
	g_free(args);
    }
}

static void
filename_repairer_callback(NautilusMenuItem *item, RepairCallbackArgs *args)
{
    guint candidate_index;

    char* old_text_uri;
    char* new_filename;
    GnomeVFSURI* parent_uri;
    GnomeVFSURI* old_uri;
    GnomeVFSURI* new_uri;

    candidate_index = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(item), "candidate_index"));
    new_filename = g_filename_from_utf8(args->candidates->pdata[candidate_index],
					 -1, NULL, NULL, NULL);

    old_text_uri = nautilus_file_info_get_uri(args->files->data);
    old_uri = gnome_vfs_uri_new(old_text_uri);
    parent_uri = gnome_vfs_uri_get_parent(old_uri);
    new_uri = gnome_vfs_uri_append_file_name(parent_uri, new_filename);

    g_free(old_text_uri);
    g_free(new_filename);

    if (gnome_vfs_uri_exists(new_uri)) {
	gint result;
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(GTK_WINDOW(args->parent),
		    GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL, 
		    GTK_MESSAGE_QUESTION,
		    GTK_BUTTONS_OK_CANCEL,
		    _("A file named \"%s\" already exists.  Do you want to replace it?"),
		    (char*)args->candidates->pdata[candidate_index]);
	result = gtk_dialog_run(GTK_DIALOG (dialog));
	gtk_widget_destroy(dialog);

	if (result == GTK_RESPONSE_OK) {
	    gnome_vfs_move_uri(old_uri, new_uri, TRUE);
	}
    } else {
	gnome_vfs_move_uri(old_uri, new_uri, TRUE);
    }

    gnome_vfs_uri_unref(old_uri);
    gnome_vfs_uri_unref(new_uri);
    gnome_vfs_uri_unref(parent_uri);
}

static char*
get_filename_without_mnemonic(const char* filename)
{
    GString* str;

    str = g_string_new(NULL);
    while (*filename != '\0') {
	if (*filename == '_')
	    g_string_append(str, "__");
	else
	    g_string_append_c(str, *filename);
	filename++;
    }

    return g_string_free(str, FALSE);
}

static GList *
nautilus_filename_repairer_get_file_items(NautilusMenuProvider *provider,
					  GtkWidget            *window,
					  GList                *files)
{
    NautilusMenu*     submenu;
    NautilusMenuItem* item;
    GList*            items = NULL;
    GPtrArray*        array;
    char*             name;
    char*             label;
    char*             tooltip;
    char*             filename;

    if (g_list_length(files) != 1)
	return NULL;

    array = create_candidate_list(files->data);
    if (array == NULL)
	return NULL;

    if (array->len == 0) {
	g_ptr_array_free(array, TRUE);
	return NULL;
    }

    filename = get_filename_without_mnemonic(array->pdata[0]);
    name    = "NautilusFilenameRepairer::rename_as_0";
    label   = g_strdup_printf(_("Re_name as \"%s\""), filename);
    tooltip = g_strdup_printf(_("Rename as \"%s\"."), (char*)array->pdata[0]);

    RepairCallbackArgs* args;

    args = g_new(RepairCallbackArgs, 1);
    args->candidates = array;
    args->files = nautilus_file_info_list_copy(files);
    args->parent = window;

    item = nautilus_menu_item_new(name, label, tooltip, NULL);
    g_object_set_data(G_OBJECT(item), "candidate_index", GUINT_TO_POINTER(0));
    g_signal_connect_data(item, "activate",
			  G_CALLBACK(filename_repairer_callback),
			  args, filename_repairer_callback_args_free, 0);
    items = g_list_append(items, item);
    g_free(filename);
    g_free(label);
    g_free(tooltip);

    if (array->len > 1) {
	guint i = 0;

	name = "NautilusFilenameRepairer::rename_as_submenu";
	item = nautilus_menu_item_new(name,
			      _("Select a filename"),
			      _("Select a filename from sub menu items."),
			      NULL);
	submenu = nautilus_menu_new();
	nautilus_menu_item_set_submenu(item, submenu);
	items = g_list_append(items, item);

	for (i = 1 ; i < array->len; i++) {
	    name = g_strdup_printf("NautilusFilenameRepairer::rename_as_%d", i);
	    filename = get_filename_without_mnemonic(array->pdata[i]);
	    label = g_strdup_printf(_("Rename as \"%s\""), filename);

	    item = nautilus_menu_item_new(name, label, label, NULL);
	    g_object_set_data(G_OBJECT(item), "candidate_index", GUINT_TO_POINTER(i));
	    g_signal_connect(item, "activate",
			     G_CALLBACK(filename_repairer_callback),
			     args);
	    nautilus_menu_append_item(submenu, item);
	    /* this code is a workaround for a bug of nautilus
	     * See: http://bugzilla.gnome.org/show_bug.cgi?id=508878 */
	    //items = g_list_append(items, item);
	    g_free(filename);
	    g_free(label);
	    g_free(name);
	}
    }

    return items;
}

static void
nautilus_filename_repairer_menu_provider_iface_init(NautilusMenuProviderIface *iface)
{
    iface->get_file_items = nautilus_filename_repairer_get_file_items;
}

static void 
nautilus_filename_repairer_instance_init(NautilusFilenameRepairer *instance)
{
}

static void
nautilus_filename_repairer_class_init(NautilusFilenameRepairerClass *klass)
{
}

GType
nautilus_filename_repairer_get_type(void) 
{
    return filename_repairer_type;
}

void
nautilus_filename_repairer_register_type(GTypeModule *module)
{
    static const GTypeInfo info = {
	sizeof(NautilusFilenameRepairerClass),
	(GBaseInitFunc)NULL,
	(GBaseFinalizeFunc)NULL,
	(GClassInitFunc)nautilus_filename_repairer_class_init,
	NULL, 
	NULL,
	sizeof(NautilusFilenameRepairer),
	0,
	(GInstanceInitFunc)nautilus_filename_repairer_instance_init,
    };

    static const GInterfaceInfo menu_provider_iface_info = {
	(GInterfaceInitFunc)nautilus_filename_repairer_menu_provider_iface_init,
	NULL,
	NULL
    };

    filename_repairer_type = g_type_module_register_type(module,
						 G_TYPE_OBJECT,
						 "NautilusFilenameRepairer",
						 &info, 0);

    g_type_module_add_interface(module,
				filename_repairer_type,
				NAUTILUS_TYPE_MENU_PROVIDER,
				&menu_provider_iface_info);
}

void  nautilus_filename_repairer_on_module_init(void)
{
}

void  nautilus_filename_repairer_on_module_shutdown(void)
{
}
