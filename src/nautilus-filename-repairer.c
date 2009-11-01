/*
 * Nautilus Filename Repairer Extension
 *
 * Copyright (C) 2008 Choe Hwanjin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Author: Choe Hwajin <choe.hwanjin@gmail.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include <libnautilus-extension/nautilus-menu-provider.h>

#include "nautilus-filename-repairer.h"
#include "nautilus-filename-repairer-i18n.h"

static GType filename_repairer_type = 0;

// from http://www.microsoft.com/globaldev/reference/wincp.mspx
// Code Pages Supported by Windows
// The first item will be selected as a default encoding if the matching locale
// string is not exist
static const char* encoding_list[] = {
    "CP1252",  // Latin I  (default encoding)
    "CP936",   // Simplified Chinese GBK
    "CP1250",  // Central Europe
    "CP932",   // Japanese Shift-JIS
    "CP949",   // Korean
    "CP950",   // Traditional Chinese Big5
    "CP1251",  // Cyrillic
    "CP1253",  // Greek
    "CP1254",  // Turkish
    "CP1255",  // Hebrew
    "CP1256",  // Arabic
    "CP1257",  // Baltic
    "CP1258",  // Vietnam
    "CP874",   // Thai
    "CP737",   // Greek
    "CP850",   // "Multilingual (Latin-1)" (Western European languages)
    "CP852",   // "Slavic (Latin-2)" (Eastern European languages)
    "CP855",   // Cyrillic
    "CP857",   // Turkish
    "CP858",   // "Multilingual" with euro symbol
    "CP860",   // Portuguese
    "CP861",   // Icelandic
    "CP863",   // French Canadian
    "CP865",   // Nordic
    "CP862",   // Hebrew
    "CP866",   // Cyrillic
    "CP869",   // Greek
    "CP437",   // The original IBM PC code page
    NULL
};

struct encoding_item {
    const char* locale;
    const char* encoding;
};

static const struct encoding_item default_encoding_list[] = {
    { "ar",    "CP1256"  },
    { "az",    "CP1251"  },
    { "az",    "CP1254"  },
    { "be",    "CP1251"  },
    { "bg",    "CP1251"  },
    { "cs",    "CP1250"  },
    { "cy",    "CP28604" },
    { "el",    "CP1253"  },
    { "et",    "CP1257"  },
    { "fa",    "CP1256"  },
    { "he",    "CP1255"  },
    { "hr",    "CP1250"  },
    { "hu",    "CP1250"  },
    { "ja",    "CP932"   },
    { "kk",    "CP1251"  },
    { "ko",    "CP949"   },
    { "ky",    "CP1251"  },
    { "lt",    "CP1257"  },
    { "lv",    "CP1257"  },
    { "mk",    "CP1251"  },
    { "mn",    "CP1251"  },
    { "pl",    "CP1250"  },
    { "ro",    "CP1250"  },
    { "ru",    "CP1251"  },
    { "sk",    "CP1250"  },
    { "sl",    "CP1250"  },
    { "sq",    "CP1250"  },
    { "sr",    "CP1250"  },
    { "sr",    "CP1251"  },
    { "th",    "CP874"   },
    { "tr",    "CP1254"  },
    { "tt",    "CP1251"  },
    { "uk",    "CP1251"  },
    { "ur",    "CP1256"  },
    { "uz",    "CP1251"  },
    { "uz",    "CP1254"  },
    { "vi",    "CP1258"  },
    { "zh_CN", "CP936"   },
    { "zh_HK", "CP950"   },
    { "zh_MO", "CP950"   },
    { "zh_SG", "CP936"   },
    { "zh_TW", "CP950"   },
    { NULL,    NULL      }
};

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

static void
candidate_list_add_default_encoding(GPtrArray* array, const char* basename)
{
    char* locale = setlocale(LC_CTYPE, NULL);
    if (locale != NULL) {
	const struct encoding_item* item = default_encoding_list;
	while (item->locale != NULL) {
	    size_t len = strlen(item->locale);
	    if (strncmp(item->locale, locale, len) == 0) {
		char* new_name = g_convert(basename, -1,
				"UTF-8", item->encoding, NULL, NULL, NULL);
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
	    item++;
	}
    }
}

static GPtrArray*
create_candidate_list(NautilusFileInfo* info)
{
    gboolean islocal;
    GPtrArray* array = NULL;

    GFile* gfile = nautilus_file_info_get_location(info);
    islocal = g_file_has_uri_scheme(gfile, "file");
    if (islocal) {
	char* filename = g_file_get_basename(gfile);
	if (filename != NULL) {
	    char* unescaped_filename;
	    char* displayname;
	    char* replacement;

	    /* test for URI encoded filenames */
	    unescaped_filename = g_uri_unescape_string(filename, NULL);
	    if (unescaped_filename != NULL) {
		if (strcmp(filename, unescaped_filename) != 0) {
		    /* if the unescaped filename is valid UTF-8,
		     * then only the unescaped filename will be added
		     * but if not, unescaped filename must be in legacy
		     * encoding, so we should examine its encoding */
		    if (g_utf8_validate(unescaped_filename, -1, NULL)) {
			array = g_ptr_array_new();
			g_ptr_array_add(array, g_strdup(unescaped_filename));
		    } else {
			/* exchange the filename with unescaped name 
			 * to examine encoding of unescaped_filename */
			g_free(filename);
			filename = g_strdup(unescaped_filename);
		    }
		}
		g_free(unescaped_filename);
	    }

	    displayname = g_filename_display_name(filename);
	    // search for U+FFFD (the Unicode replacement char)
	    // if the filename has wrong char, displayname may have U+FFFD
	    // UTF-8 encoded form of U+FFFD is "\357\277\275"
	    replacement = strstr(displayname, "\357\277\275");
	    if (replacement != NULL) {
		char* new_name;
		int i;

		if (array == NULL)
		    array = g_ptr_array_new();

		candidate_list_add_default_encoding(array, filename);

		for (i = 0; encoding_list[i] != NULL; i++) {
		    new_name = g_convert(filename, -1,
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
	    g_free(filename);
	}
    }
    g_object_unref(gfile);
    
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

    char* new_filename;
    GFile* gfile_parent;
    GFile* gfile_old;
    GFile* gfile_new;

    candidate_index = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(item), "candidate_index"));
    new_filename = g_filename_from_utf8(args->candidates->pdata[candidate_index],
					 -1, NULL, NULL, NULL);

    gfile_old = nautilus_file_info_get_location(args->files->data);
    gfile_parent = g_file_get_parent(gfile_old);
    gfile_new = g_file_get_child(gfile_parent, new_filename);

    g_free(new_filename);

    if (g_file_query_exists(gfile_new, NULL)) {
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
	    g_file_move(gfile_old, gfile_new,
			G_FILE_COPY_OVERWRITE | G_FILE_COPY_NOFOLLOW_SYMLINKS,
			NULL, NULL, NULL, NULL);
	}
    } else {
	g_file_move(gfile_old, gfile_new,
		    G_FILE_COPY_OVERWRITE | G_FILE_COPY_NOFOLLOW_SYMLINKS,
		    NULL, NULL, NULL, NULL);
    }

    g_object_unref(gfile_parent);
    g_object_unref(gfile_new);
    g_object_unref(gfile_old);
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
