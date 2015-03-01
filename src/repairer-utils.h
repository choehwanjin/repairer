/*
 * Nautilus Filename Repairer Extension
 *
 * Copyright (C) 2015 Choe Hwanjin
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

#ifndef nautilus_filename_repairer_repairer_utils_h
#define nautilus_filename_repairer_repairer_utils_h

#include <glib.h>

void repairer_utils_set_app_path(const char* path);
gchar* repairer_utils_get_ui_path(const char* name);

#endif /* nautilus_filename_repairer_repairer_utils_h */
