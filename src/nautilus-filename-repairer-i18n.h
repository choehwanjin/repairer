#ifndef nautils_filename_repairer_i18n_h
#define nautils_filename_repairer_i18n_h

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* remove warning */
#undef _
#undef N_

#ifdef ENABLE_NLS

#include <libintl.h>
#define _(String) dgettext(GETTEXT_PACKAGE, String)
#define N_(String) gettext_noop (String)

#else /* ENABLE_NLS */

#define _(String) (String)
#define N_(String) (String)
#define textdomain(String) (String)
#define gettext(String) (String)
#define dgettext(Domain,String) (String)
#define dcgettext(Domain,String,Type) (String)
#define bindtextdomain(Domain,Directory) (Domain)
#define bind_textdomain_codeset(Domain,Encoding) (Domain)

#endif /* ENABLE_NLS */

#endif /* nautils_filename_repairer_i18n_h */
