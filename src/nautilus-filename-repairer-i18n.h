#ifndef nautils_filename_repairer_i18n_h
#define nautils_filename_repairer_i18n_h

#ifdef HAVE_CONFIG
#include <config.h>
#endif

#ifdef ENABLE_NLS
#include <glib/gi18n.h>
#else
#define _(String) (String)
#define N_(String) (String)
#define textdomain(String) (String)
#define gettext(String) (String)
#define dgettext(Domain,String) (String)
#define dcgettext(Domain,String,Type) (String)
#define bindtextdomain(Domain,Directory) (Domain)
#endif

#endif /* nautils_filename_repairer_i18n_h */
