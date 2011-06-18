#include <gtk/gtk.h>

#include "nautilus-filename-repairer-i18n.h"
#include "repair-dialog.h"

int main(int argc, char** argv)
{
    int i;
    GtkDialog* dialog;
    GSList* files;
    gint res;

#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
#endif

    gtk_init(&argc, &argv);

    files = NULL;
    if (argc == 1) {
	dialog = GTK_DIALOG(gtk_file_chooser_dialog_new(
			_("Filename Repairer: Select File"),
			NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL));
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
	res = gtk_dialog_run(dialog);
	if (res == GTK_RESPONSE_ACCEPT) {
	    files = gtk_file_chooser_get_files(GTK_FILE_CHOOSER(dialog));
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
    } else {
	for (i = 1; i < argc; i++) {
	    GFile* file = g_file_new_for_path(argv[i]);
	    files = g_slist_prepend(files, file);
	}
	files = g_slist_reverse(files);
    }

    if (files == NULL)
	return 0;

    dialog = repair_dialog_new(files);
    res = gtk_dialog_run(dialog);
    gtk_widget_hide(GTK_WIDGET(dialog));

    if (res == GTK_RESPONSE_APPLY) {
	repair_dialog_do_repair(dialog);
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));

    g_slist_foreach(files, (GFunc)g_object_unref, NULL);
    g_slist_free(files);

    return 0;
}