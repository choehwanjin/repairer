#ifndef nautilus_filename_repairer_repair_dialog_h
#define nautilus_filename_repairer_repair_dialog_h

#include <gtk/gtk.h>

GtkDialog* repair_dialog_new(GSList* files);
void       repair_dialog_do_repair(GtkDialog* dialog);

#endif // nautilus_filename_repairer_repair_dialog_h
