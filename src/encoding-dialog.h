#ifndef nautilus_filename_repairer_encoding_dialog_h
#define nautilus_filename_repairer_encoding_dialog_h

#include <gtk/gtk.h>

GtkDialog* encoding_dialog_new(GtkWindow* parent);
char* encoding_dialog_run(GtkDialog* dialog);

#endif /* nautilus_filename_repairer_encoding_dialog_h */
