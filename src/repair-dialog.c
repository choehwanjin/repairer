#include <string.h>
#include <locale.h>
#include <gtk/gtk.h>

#include "nautilus-filename-repairer-i18n.h"

#define REPAIR_DIALOG_UI PKGDATADIR "/repair-dialog.ui"

enum {
    FILE_COLUMN_NAME,
    FILE_COLUMN_DISPLAY_NAME,
    FILE_COLUMN_NEW_NAME,
    FILE_NUM_COLUMNS
};

enum {
    ENCODING_COLUMN_LABEL,
    ENCODING_COLUMN_ENCODING,
    ENCODING_NUM_COLUMNS
};

typedef struct _UpdateContext {
    GtkDialog* dialog;
    GtkTreeView* treeview;
    GtkTreeStore* store;
    GSList* file_stack;
    GSList* iter_stack;
    GSList* enum_stack;
    char* encoding;
    gboolean include_subdir;
    gboolean success_all;
} UpdateContext;

static char* repair_dialog_get_current_encoding(GtkDialog* dialog);
static gboolean repair_dialog_get_include_subdir_flag(GtkDialog* dialog);
static void repair_dialog_set_conversion_state(GtkDialog* dialog, gboolean state);

static GtkComboBox* repair_dialog_get_encoding_combo_box(GtkDialog* dialog);
static void repair_dialog_set_encoding_combo_box(GtkDialog* dialog, GtkComboBox* combo);
static GSList* repair_dialog_get_file_list(GtkDialog* dialog);
static void repair_dialog_set_file_list(GtkDialog* dialog, GSList* list);
static GtkTreeStore* repair_dialog_get_file_list_model(GtkDialog* dialog);
static void repair_dialog_set_file_list_model(GtkDialog* dialog, GtkTreeModel* model);
static GtkTreeView* repair_dialog_get_file_list_view(GtkDialog* dialog);
static void repair_dialog_set_file_list_view(GtkDialog* dialog, GtkTreeView* view);

static void repair_dialog_update_file_list_model(GtkDialog* dialog, gboolean async);
static gboolean repair_dialog_on_idle_update(GtkDialog* dialog);


static const char* encoding_list[][2] = {
    { "Thai - CP874",                     "CP874"  },
    { "Japanese - CP932",                 "CP932"  },
    { "Chinese simplified - CP936",       "CP936"  },
    { "Korean - CP949",                   "CP949"  },
    { "Chinese traditional - CP950",      "CP950"  },
    { "Central European latin - CP1250",  "CP1250" },
    { "Cyrillic - CP1251",                "CP1251" },
    { "Western European latin - CP1252",  "CP1252" },
    { "Greek - CP1253",                   "CP1253" },
    { "Turkish - CP1254",                 "CP1254" },
    { "Hebrew - CP1255",                  "CP1255" },
    { "Arabic - CP1256",                  "CP1256" },
    { "Baltic - CP1257",                  "CP1257" },
    { "Vietnamese - CP1258",              "CP1258" },
    { NULL,                               NULL     }
};

static gboolean
local_g_file_is_directory(GFile* file)
{
    GFileType file_type;

    file_type = g_file_query_file_type(file, G_FILE_QUERY_INFO_NONE, NULL);
    return file_type == G_FILE_TYPE_DIRECTORY;
}

static char*
get_display_name(const char* name)
{
    size_t i;
    size_t len;
    GString* display_name;

    if (g_utf8_validate(name, -1, NULL))
	return g_strdup(name);

    len = strlen(name);

    display_name = g_string_sized_new(len * 3 + 1);
    for (i = 0; i < len; i++) {
	if ((guchar)name[i] < 0x80) {
	    g_string_append_c(display_name, name[i]);
	} else {
	    g_string_append_printf(display_name, "%%%2x", (guchar)name[i]);
	}
    }

    return g_string_free(display_name, FALSE);
}

static char*
get_new_name(const char* name, const char* encoding)
{
    char* new_name;

    if (encoding == NULL)
	return NULL;

    if (g_utf8_validate(name, -1, NULL)) {
	new_name = g_strdup(name);
    } else {
	new_name = g_convert(name, -1, "UTF-8", encoding, NULL, NULL, NULL);
    }

    return new_name;
}

static void
convert_filename(GFile* src, const char* encoding)
{
    char* src_name;
    char* dst_name;
    GFile* parent;
    GFile* dst;
    gboolean res;
    GError* error = NULL;

    src_name = g_file_get_basename(src);
    if (g_utf8_validate(src_name, -1, NULL))
	return;

    dst_name = g_convert(src_name, -1, "UTF-8", encoding, NULL, NULL, NULL);
    if (strcmp(src_name, dst_name) != 0) {
	parent = g_file_get_parent(src);
	dst = g_file_get_child(parent, dst_name);

	res = g_file_move(src, dst, G_FILE_COPY_NOFOLLOW_SYMLINKS,
		NULL, NULL, NULL, &error);

	if (res) {
	} else {
	    g_print("Error: %s\n", error->message);
	    g_error_free(error);
	}

	g_object_unref(G_OBJECT(dst));
    }

    g_free(src_name);
    g_free(dst_name);
}

static void
convert_dir(GFile* dir, const char* encoding)
{
    GFileInfo* info;

    GFileEnumerator* e = g_file_enumerate_children(dir,
	    G_FILE_ATTRIBUTE_STANDARD_NAME ","
	    G_FILE_ATTRIBUTE_STANDARD_TYPE,
	    G_FILE_QUERY_INFO_NONE,
	    NULL, NULL);
    if (e == NULL)
	return;

    info = g_file_enumerator_next_file(e, NULL, NULL);
    while (info != NULL) {
	const char* name;
	GFile* child;
	GFileType file_type;

	name = g_file_info_get_name(info);
	file_type = g_file_info_get_file_type(info);
	child = g_file_get_child(dir, name);

	if (file_type == G_FILE_TYPE_DIRECTORY) {
	    convert_dir(child, encoding);
	}

	convert_filename(child, encoding);

	g_object_unref(child);
	g_object_unref(info);
	
	info = g_file_enumerator_next_file(e, NULL, NULL);
    }
    g_object_unref(e);
}

static void
repair_filenames(GSList* files, const char* encoding, gboolean include_subdir)
{
    if (encoding == NULL)
	return;

    while (files != NULL) {
	GFile* file;

	file = files->data;

	if (include_subdir) {
	    gboolean is_dir;
	    is_dir = local_g_file_is_directory(file);
	    if (is_dir) {
		convert_dir(file, encoding);
	    }
	}

	convert_filename(file, encoding);

	files = g_slist_next(files);
    }
}

static GtkListStore*
encoding_list_model_new()
{
    int i;
    GtkListStore* store;
    GtkTreeIter iter;

    store = gtk_list_store_new(ENCODING_NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);

    i = 0;
    while (encoding_list[i][0] != NULL) {
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
		ENCODING_COLUMN_LABEL, encoding_list[i][0],
		ENCODING_COLUMN_ENCODING, encoding_list[i][1],
		-1);
	i++;
    }

    return store;
}

static const char*
get_codepage_from_current_locale()
{
    static const char* codepage_table[][2] = {
	{ "ar",    "CP1256"  },
	{ "az",    "CP1251"  },
	{ "az",    "CP1254"  },
	{ "be",    "CP1251"  },
	{ "bg",    "CP1251"  },
	{ "cs",    "CP1250"  },
	{ "cy",    "CP1253"  },
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

    int i;
    const char* locale;
    size_t len;

    locale = setlocale(LC_CTYPE, NULL);
    i = 0;
    while (codepage_table[i][0] != NULL) {
	len = strlen(codepage_table[i][0]);
	if (strncmp(codepage_table[i][0], locale, len) == 0) {
	    return codepage_table[i][1];
	}
	i++;
    }

    return "CP1252";
}

static void
select_default_encoding(GtkComboBox* combo, GtkTreeModel* model)
{
    GtkTreeIter iter;
    gboolean res;
    const char* codepage;

    codepage = get_codepage_from_current_locale();

    res = gtk_tree_model_get_iter_first(model, &iter);
    while (res) {
	char* encoding = NULL;
	gtk_tree_model_get(model, &iter, ENCODING_COLUMN_ENCODING, &encoding, -1);
	if (strcmp(encoding, codepage) == 0) {
	    gtk_combo_box_set_active_iter(combo, &iter);
	    g_free(encoding);
	    break;
	}
	g_free(encoding);
	res = gtk_tree_model_iter_next(model, &iter);
    }
}

static UpdateContext*
update_context_new()
{
    UpdateContext* context;
    context = g_new(UpdateContext, 1);
    context->dialog = NULL;
    context->store = NULL;
    context->file_stack = NULL;
    context->iter_stack = NULL;
    context->enum_stack = NULL;
    context->encoding = NULL;
    context->include_subdir = FALSE;
    context->success_all = TRUE;
    return context;
}

static void
update_context_free(UpdateContext* context)
{
    g_slist_free(context->file_stack);
    g_slist_free(context->iter_stack);
    g_slist_free(context->enum_stack);
    g_free(context->encoding);
    g_free(context);
}

static void
update_context_push(UpdateContext* context,
	GFile* file, GtkTreeIter* iter, GFileEnumerator* e)
{
    context->file_stack = g_slist_prepend(context->file_stack, file);
    context->iter_stack = g_slist_prepend(context->iter_stack, iter);
    context->enum_stack = g_slist_prepend(context->enum_stack, e);
}

static void
update_context_pop(UpdateContext* context)
{
    context->file_stack = g_slist_delete_link(context->file_stack, context->file_stack);
    context->iter_stack = g_slist_delete_link(context->iter_stack, context->iter_stack);
    context->enum_stack = g_slist_delete_link(context->enum_stack, context->enum_stack);
}

static gboolean
update_new_name_in_a_row(GtkTreeStore* store, GtkTreeIter* iter, const char* encoding)
{
    char* old_name;
    char* new_name;
    gboolean res;

    old_name = NULL;
    gtk_tree_model_get(GTK_TREE_MODEL(store), iter,
	    FILE_COLUMN_NAME, &old_name, -1);

    if (g_utf8_validate(old_name, -1, NULL)) {
	new_name = g_strdup(old_name);
    } else {
	new_name = g_convert(old_name, -1, "UTF-8", encoding, NULL, NULL, NULL);
    }
    
    if (new_name != NULL) {
	gtk_tree_store_set(store, iter, FILE_COLUMN_NEW_NAME, new_name, -1);
	g_free(new_name);
	res = TRUE;
    } else {
	gtk_tree_store_set(store, iter, FILE_COLUMN_NEW_NAME, "", -1);
	res = FALSE;
    }
    g_free(old_name);

    return res;
}

static gboolean
update_children_new_names(GtkTreeStore* store, GtkTreeIter* iter,
	 const char* encoding)
{
    GtkTreeModel* model;
    gboolean success_all;
    gboolean res;

    model = GTK_TREE_MODEL(store);

    success_all = TRUE;
    do {
	GtkTreeIter child_iter;

	res = update_new_name_in_a_row(store, iter, encoding);
	if (!res)
	    success_all = FALSE;

	res = gtk_tree_model_iter_children(model, &child_iter, iter);
	if (res) {
	    res = update_children_new_names(store, &child_iter, encoding);
	    if (!res)
		success_all = FALSE;
	}

	res = gtk_tree_model_iter_next(model, iter);
    } while (res);

    return success_all;
}

static gboolean
file_list_model_update_new_names(GtkTreeStore* store, const char* encoding)
{
    GtkTreeModel* model;
    GtkTreeIter iter;
    gboolean res;
    gboolean success_all;

    success_all = TRUE;
    model = GTK_TREE_MODEL(store);
    res = gtk_tree_model_get_iter_first(model, &iter);
    while (res) {
	GtkTreeIter child_iter;

	res = update_new_name_in_a_row(store, &iter, encoding);
	if (!res)
	    success_all = FALSE;

	res = gtk_tree_model_iter_children(model, &child_iter, &iter);
	if (res) {
	    res = update_children_new_names(store, &child_iter, encoding);
	    if (!res)
		success_all = FALSE;
	}

	res = gtk_tree_model_iter_next(model, &iter);
    }

    return success_all;
}

static GtkTreeStore*
file_list_model_new(GSList* files, gboolean include_subdir)
{
    GtkTreeStore* store;

    store = gtk_tree_store_new(FILE_NUM_COLUMNS,
	     G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    return store;
}

static void
file_list_model_append(GtkTreeStore* store,
	GtkTreeIter* iter, GtkTreeIter* parent_iter,
	const char* name, const char* display_name, const char* new_name)
{
    gtk_tree_store_append(store, iter, parent_iter);
    gtk_tree_store_set(store, iter,
	    FILE_COLUMN_NAME, name,
	    FILE_COLUMN_DISPLAY_NAME, display_name,
	    FILE_COLUMN_NEW_NAME, new_name,
	    -1);
}

static void
on_dialog_response(GtkDialog* dialog, gint response_id, gpointer data)
{
    GSList* files;
    char* encoding;
    gboolean include_subdir;

    if (response_id != GTK_RESPONSE_APPLY)
	return;

    files = repair_dialog_get_file_list(dialog);
    encoding = repair_dialog_get_current_encoding(dialog);
    include_subdir = repair_dialog_get_include_subdir_flag(dialog);

    repair_filenames(files,  encoding, include_subdir);

    g_free(encoding);
}

static void
on_dialog_destroy(GtkWidget* dialog, gpointer data)
{
    GSList* files;

    files = repair_dialog_get_file_list(GTK_DIALOG(dialog));
    repair_dialog_set_file_list(GTK_DIALOG(dialog), NULL);

    g_slist_foreach(files, (GFunc)g_object_unref, NULL);
    g_slist_free(files);
}

static void
on_encoding_changed(GtkComboBox* combo, GtkDialog* dialog)
{
    GtkTreeStore* store;
    GtkTreeModel* model;
    GtkTreeIter iter;
    char* encoding;
    gboolean res;

    model = gtk_combo_box_get_model(combo);
    res = gtk_combo_box_get_active_iter(combo, &iter);
    if (!res)
	return;

    encoding = NULL;
    gtk_tree_model_get(model, &iter, ENCODING_COLUMN_ENCODING, &encoding, -1);

    store = repair_dialog_get_file_list_model(dialog);

    res = file_list_model_update_new_names(store, encoding);
    repair_dialog_set_conversion_state(dialog, res);

    g_free(encoding);
}

static void
on_subdir_check_toggled(GtkToggleButton* button, GtkDialog* dialog)
{
    repair_dialog_update_file_list_model(dialog, TRUE);
}

GtkDialog*
repair_dialog_new(GSList* files)
{
    GObject* object;
    GtkDialog* dialog;
    GtkComboBox* combobox;
    GtkTreeView* treeview;
    GtkTreeModel* model;
    GtkTreeViewColumn* column;
    GtkCellRenderer* renderer;
    GtkBuilder* builder;
    gboolean include_subdir = FALSE;
    
    builder = gtk_builder_new();
    gtk_builder_set_translation_domain(builder, GETTEXT_PACKAGE);
    gtk_builder_add_from_file(builder, REPAIR_DIALOG_UI, NULL);

    object = gtk_builder_get_object(builder, "repair_dialog");
    if (object == NULL)
	return NULL;

    dialog = GTK_DIALOG(object);
    files = g_slist_copy(files);
    g_slist_foreach(files, (GFunc)g_object_ref, NULL);
    repair_dialog_set_file_list(dialog, files);
    g_signal_connect(G_OBJECT(dialog), "response",
		     G_CALLBACK(on_dialog_response), NULL);
    g_signal_connect(G_OBJECT(dialog), "destroy",
		     G_CALLBACK(on_dialog_destroy), NULL);

    object = gtk_builder_get_object(builder, "apply_button");
    if (object != NULL) {
	g_object_set_data(G_OBJECT(dialog), "apply_button", object);
    }

    object = gtk_builder_get_object(builder, "encoding_combo");
    if (object == NULL)
	return NULL;

    model = (GtkTreeModel*)encoding_list_model_new();
    combobox = GTK_COMBO_BOX(object);
    gtk_combo_box_set_model(combobox, model);
    g_object_unref(G_OBJECT(model));
    select_default_encoding(combobox, model);
    repair_dialog_set_encoding_combo_box(dialog, combobox);
    g_signal_connect(G_OBJECT(combobox), "changed",
		     G_CALLBACK(on_encoding_changed), dialog);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combobox), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combobox), renderer,
	    "text", ENCODING_COLUMN_LABEL, NULL);

    object = gtk_builder_get_object(builder, "subdir_check_button");
    if (object != NULL) {
	include_subdir = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(object));
	g_object_set_data(G_OBJECT(dialog), "subdir_check_button", object);
	g_signal_connect(G_OBJECT(object), "toggled",
			 G_CALLBACK(on_subdir_check_toggled), dialog);
    }

    object = gtk_builder_get_object(builder, "file_list_view");
    if (object == NULL)
	return NULL;

    treeview = GTK_TREE_VIEW(object);
    repair_dialog_set_file_list_view(dialog, treeview);

    model = (GtkTreeModel*)file_list_model_new(files, include_subdir);
    repair_dialog_set_file_list_model(dialog, model);
    repair_dialog_update_file_list_model(dialog, TRUE);
    gtk_tree_view_set_model(treeview, model);
    g_object_unref(G_OBJECT(model));

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("As is"),
	    renderer, "text", FILE_COLUMN_DISPLAY_NAME, NULL);
    gtk_tree_view_column_set_sort_column_id(column, FILE_COLUMN_NAME);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("To be"),
	    renderer, "text", FILE_COLUMN_NEW_NAME, NULL);
    gtk_tree_view_column_set_sort_column_id(column, FILE_COLUMN_NEW_NAME);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);

    g_object_unref(builder);

    return dialog;
}

void
repair_dialog_do_repair(GtkDialog* dialog)
{
    GSList* files;
    char* encoding;
    gboolean include_subdir;

    files = repair_dialog_get_file_list(dialog);
    encoding = repair_dialog_get_current_encoding(dialog);
    include_subdir = repair_dialog_get_include_subdir_flag(dialog);

    repair_filenames(files, encoding, include_subdir);

    g_free(encoding);
}

static GtkComboBox*
repair_dialog_get_encoding_combo_box(GtkDialog* dialog)
{
    return g_object_get_data(G_OBJECT(dialog), "encoding_combo_box");
}

static void
repair_dialog_set_encoding_combo_box(GtkDialog* dialog, GtkComboBox* combo)
{
    g_object_set_data(G_OBJECT(dialog), "encoding_combo_box", combo);
}

static char*
repair_dialog_get_current_encoding(GtkDialog* dialog)
{
    GtkComboBox* combo;
    GtkTreeModel* model;
    GtkTreeIter iter;
    gboolean res;
    char* encoding;

    combo = repair_dialog_get_encoding_combo_box(dialog);

    model = gtk_combo_box_get_model(combo);
    res = gtk_combo_box_get_active_iter(combo, &iter);
    if (!res)
	return NULL;

    gtk_tree_model_get(model, &iter, ENCODING_COLUMN_ENCODING, &encoding, -1);
    return encoding;
}

static gboolean
repair_dialog_get_include_subdir_flag(GtkDialog* dialog)
{
    GtkToggleButton* button;
    gboolean state;

    button = g_object_get_data(G_OBJECT(dialog), "subdir_check_button");
    state = gtk_toggle_button_get_active(button);
    return state;
}

static void
repair_dialog_set_conversion_state(GtkDialog* dialog, gboolean state)
{
    GtkWidget* apply_button;

    apply_button = g_object_get_data(G_OBJECT(dialog), "apply_button");
    gtk_widget_set_sensitive(apply_button, state);
}

static GSList*
repair_dialog_get_file_list(GtkDialog* dialog)
{
    return g_object_get_data(G_OBJECT(dialog), "file_list");
}

static void
repair_dialog_set_file_list(GtkDialog* dialog, GSList* list)
{
    g_object_set_data(G_OBJECT(dialog), "file_list", list);
}

static GtkTreeStore*
repair_dialog_get_file_list_model(GtkDialog* dialog)
{
    return g_object_get_data(G_OBJECT(dialog), "file_list_model");
}

static void
repair_dialog_set_file_list_model(GtkDialog* dialog, GtkTreeModel* model)
{
    g_object_set_data(G_OBJECT(dialog), "file_list_model", model);
}

static GtkTreeView*
repair_dialog_get_file_list_view(GtkDialog* dialog)
{
    return g_object_get_data(G_OBJECT(dialog), "file_list_view");
}

static void
repair_dialog_set_file_list_view(GtkDialog* dialog, GtkTreeView* view)
{
    g_object_set_data(G_OBJECT(dialog), "file_list_view", view);
}

static UpdateContext*
repair_dialog_get_update_context(GtkDialog* dialog)
{
    return g_object_get_data(G_OBJECT(dialog), "update_context");
}

static void
repair_dialog_set_update_context(GtkDialog* dialog, UpdateContext* context)
{
    g_object_set_data(G_OBJECT(dialog), "update_context", context);
}

static gboolean
append_dir(GtkTreeStore* store, GtkTreeIter* parent_iter,
	GFile* dir, const char* encoding)
{
    GtkTreeIter iter;
    GFileInfo* info;
    GFileEnumerator* e;
    gboolean success_all = TRUE;

    e = g_file_enumerate_children(dir,
	    G_FILE_ATTRIBUTE_STANDARD_NAME ","
	    G_FILE_ATTRIBUTE_STANDARD_TYPE,
	    G_FILE_QUERY_INFO_NONE, NULL, NULL);
    if (e == NULL)
	return success_all;

    info = g_file_enumerator_next_file(e, NULL, NULL);
    while (info != NULL) {
	const char* name = g_file_info_get_name(info);
	char* display_name = get_display_name(name);
	char* new_name = get_new_name(name, encoding);
	GFileType ftype;

	file_list_model_append(store, &iter, parent_iter,
		name, display_name, new_name);

	if (new_name == NULL)
	    success_all = FALSE;

	ftype = g_file_info_get_file_type(info);
	if (ftype == G_FILE_TYPE_DIRECTORY) {
	    gboolean res;
	    GFile* child = g_file_get_child(dir, name);
	    res = append_dir(store, &iter, child, encoding);
	    g_object_unref(child);
	    if (!res)
		success_all = FALSE;
	}

	g_object_unref(info);
	g_free(display_name);
	g_free(new_name);
	
	info = g_file_enumerator_next_file(e, NULL, NULL);
    }
    g_object_unref(e);

    return success_all;
}

static void
repair_dialog_update_file_list_model(GtkDialog* dialog, gboolean async)
{
    GtkTreeStore* store;
    GtkTreeView* treeview;
    GSList* files;
    gboolean include_subdir;
    GtkComboBox* combobox;
    UpdateContext* context;
    gboolean success_all = TRUE;

    store = repair_dialog_get_file_list_model(dialog);
    files = repair_dialog_get_file_list(dialog);
    treeview = repair_dialog_get_file_list_view(dialog);
    include_subdir = repair_dialog_get_include_subdir_flag(dialog);

    combobox = repair_dialog_get_encoding_combo_box(dialog);
    gtk_widget_set_sensitive(GTK_WIDGET(combobox), FALSE);

    gtk_tree_store_clear(store);

    if (async) {
	context = update_context_new();
	context->dialog = dialog;
	context->treeview = treeview;
	context->store = store;
	context->file_stack = g_slist_copy(files);
	context->encoding = repair_dialog_get_current_encoding(dialog);
	context->include_subdir = include_subdir;

	g_slist_foreach(context->file_stack, (GFunc)g_object_ref, NULL);

	repair_dialog_set_update_context(dialog, context);

	g_idle_add((GSourceFunc)repair_dialog_on_idle_update, dialog);
    } else {
	char* encoding = repair_dialog_get_current_encoding(dialog);

	while (files != NULL) {
	    GtkTreeIter iter;
	    char* name;
	    char* display_name;
	    char* new_name;
	    GFile* file;

	    file = files->data;
	    name = g_file_get_basename(file);
	    display_name = get_display_name(name);
	    new_name = get_new_name(name, encoding);

	    file_list_model_append(store, &iter, NULL,
		    name, display_name, new_name);
	    if (new_name == NULL)
		success_all = FALSE;
	    
	    if (include_subdir) {
		GFileType file_type;
		file_type = g_file_query_file_type(file,
			G_FILE_QUERY_INFO_NONE, NULL);
		if (file_type == G_FILE_TYPE_DIRECTORY) {
		    gboolean res;
		    res = append_dir(store, &iter, file, encoding);
		    if (!res)
			success_all = FALSE;
		}
	    }

	    g_free(name);
	    g_free(display_name);
	    g_free(new_name);

	    files = g_slist_next(files);
	}

	gtk_tree_view_expand_all(treeview);
	g_free(encoding);
    }
}

static void
repair_dialog_on_update_end(GtkDialog* dialog, gboolean success_all)
{
    GtkComboBox* combobox;
    combobox = repair_dialog_get_encoding_combo_box(dialog);
    gtk_widget_set_sensitive(GTK_WIDGET(combobox), TRUE);

    repair_dialog_set_conversion_state(dialog, success_all);
}

static gboolean
repair_dialog_on_idle_update(GtkDialog* dialog)
{
    GtkTreeIter iter;
    GtkTreeIter* parent_iter;
    char* name;
    char* display_name;
    char* new_name;
    GFile* file;
    GFileType ftype;
    UpdateContext* context;
    gint n;
    int i;

    context = repair_dialog_get_update_context(dialog);

    if (context == NULL) {
	return FALSE;
    }

    for (i = 0; i < 5000; i++) {
	if (context->file_stack == NULL) {
	    repair_dialog_on_update_end(dialog, context->success_all);
	    update_context_free(context);
	    return FALSE;
	}

	file = context->file_stack->data;

	if (context->enum_stack != NULL) {
	    GFileInfo* info;
	    GFileEnumerator* e;

	    parent_iter = context->iter_stack->data;
	    e = context->enum_stack->data;

	    info = g_file_enumerator_next_file(e, NULL, NULL);
	    if (info != NULL) {
		const char* name_const;
		name_const = g_file_info_get_name(info);
		display_name = get_display_name(name_const);
		new_name = get_new_name(name_const, context->encoding);

		file_list_model_append(context->store, &iter, parent_iter,
			name_const, display_name, new_name);
		if (new_name == NULL)
		    context->success_all = FALSE;

		ftype = g_file_info_get_file_type(info);
		if (ftype == G_FILE_TYPE_DIRECTORY) {
		    GFile* child;
		    GFileEnumerator* child_e;
		    GtkTreeIter* tmp_iter;

		    tmp_iter = gtk_tree_iter_copy(&iter);
		    child = g_file_get_child(file, name_const);
		    child_e = g_file_enumerate_children(child,
			    G_FILE_ATTRIBUTE_STANDARD_NAME ","
			    G_FILE_ATTRIBUTE_STANDARD_TYPE,
			    G_FILE_QUERY_INFO_NONE, NULL, NULL);
		    
		    update_context_push(context, child, tmp_iter, child_e);
		}

		g_object_unref(info);
		g_free(display_name);
		g_free(new_name);

		n = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(context->store), parent_iter);
		if (n == 1) {
		    GtkTreePath* path;
		    path = gtk_tree_model_get_path(GTK_TREE_MODEL(context->store), parent_iter);
		    gtk_tree_view_expand_row(context->treeview, path, FALSE);
		    gtk_tree_path_free(path);
		}
	    } else {
		g_object_unref(file);
		gtk_tree_iter_free(parent_iter);
		g_object_unref(e);
		update_context_pop(context);
	    }
	} else {
	    context->file_stack = g_slist_delete_link(context->file_stack, context->file_stack);

	    name = g_file_get_basename(file);
	    display_name = get_display_name(name);
	    new_name = get_new_name(name, context->encoding);

	    file_list_model_append(context->store, &iter, NULL,
		    name, display_name, new_name);

	    if (new_name == NULL)
		context->success_all = FALSE;

	    if (context->include_subdir) {
		ftype = g_file_query_file_type(file, G_FILE_QUERY_INFO_NONE, NULL);
		if (ftype == G_FILE_TYPE_DIRECTORY) {
		    GtkTreeIter* tmp_iter;
		    GFileEnumerator* e;

		    tmp_iter = gtk_tree_iter_copy(&iter);
		    e = g_file_enumerate_children(file,
			    G_FILE_ATTRIBUTE_STANDARD_NAME ","
			    G_FILE_ATTRIBUTE_STANDARD_TYPE,
			    G_FILE_QUERY_INFO_NONE, NULL, NULL);

		    update_context_push(context, file, tmp_iter, e);
		}
	    }

	    g_free(name);
	    g_free(display_name);
	    g_free(new_name);
	}
    }

    return TRUE;
}
