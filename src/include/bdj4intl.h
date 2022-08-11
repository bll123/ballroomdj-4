#ifndef INC_BDJ4INTL_H
#define INC_BDJ4INTL_H

#include <locale.h>
#if _hdr_libintl
# include <libintl.h>
#endif

#define GETTEXT_DOMAIN "bdj4"
#define _(str) gettext (str)

#endif /* INC_BDJ4INTL_H */
