#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "bdjmsg.h"
#include "bdjstring.h"

#define LSZ         4
#define MSG_RS      '~'

size_t
msgEncode (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, char *msgbuff, size_t mlen)
{
  if (args == NULL) {
    args = "";
  }
  size_t len = (size_t) snprintf (msgbuff, mlen, "%0*d%c%0*d%c%0*d%c%s",
      LSZ, routefrom, MSG_RS, LSZ, route, MSG_RS, LSZ, msg, MSG_RS, args);
  ++len;
  return len;
}

void
msgDecode (char *msgbuff, bdjmsgroute_t *routefrom, bdjmsgroute_t *route,
    bdjmsgmsg_t *msg, char *args, size_t alen)
{
  char        *p = NULL;

  p = msgbuff;
  *routefrom = atol (p);
  p += LSZ + 1;
  *route = atol (p);
  p += LSZ + 1;
  *msg = atol (p);
  p += LSZ + 1;
  if (args != NULL) {
    *args = '\0';
    strlcpy (args, p, alen);
  }
}
