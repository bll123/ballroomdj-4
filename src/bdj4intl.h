#ifndef INC_BDJ4INTL_H
#define INC_BDJ4INTL_H

#include <locale.h>

// do this before including libintl, as libintl on msys64/windows
// defines LC_MESSAGES

#if defined (LC_MESSAGES)
# define LOC_LC_MESSAGES LC_MESSAGES
#else
# define LOC_LC_MESSAGES LC_ALL
#endif

#if _hdr_libintl
# include <libintl.h>
#endif

#define GETTEXT_DOMAIN "bdj4"
#define _(str) gettext (str)

#endif /* INC_BDJ4INTL_H */
