/*
 *  nautilus-filename-repairer.h
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

#ifndef nautilus_filename_repairer_h
#define nautilus_filename_repairer_h

#include <glib.h>

#define NAUTILUS_TYPE_RENAME_AS          (nautilus_filename_repairer_get_type())
#define NAUTILUS_RENAME_AS(obj)          (G_TYPE_CHECK_INSTANCE_CAST((obj), NAUTILUS_TYPE_RENAME_AS, NautilusFilenameRepairer))
#define NAUTILUS_IS_RENAME_AS(obj)       (G_TYPE_CHECK_INSTANCE_TYPE((obj), NAUTILUS_TYPE_RENAME_AS))

typedef struct _NautilusFilenameRepairer	  NautilusFilenameRepairer;
typedef struct _NautilusFilenameRepairerClass	  NautilusFilenameRepairerClass;

struct _NautilusFilenameRepairer {
    GObject parent;
};

struct _NautilusFilenameRepairerClass {
    GObjectClass parent;
};

G_BEGIN_DECLS

GType nautilus_filename_repairer_get_type     (void);
void  nautilus_filename_repairer_register_type(GTypeModule *module);

void  nautilus_filename_repairer_on_module_init(void);
void  nautilus_filename_repairer_on_module_shutdown(void);

G_END_DECLS

#endif // nautilus_filename_repairer_h
