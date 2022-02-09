#ifndef INC_BDJ4INTL_H
#define INC_BDJ4INTL_H

#if _hdr_libintl
# include <libintl.h>
#endif

#define  _(str) gettext (str)
#define  N_(str) (str)
#define  NC_(context,str) (str)

#endif /* INC_BDJ4INTL_H */
