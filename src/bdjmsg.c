#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bdjmsg.h"
#include "bdjstring.h"

#define LSZ 4

size_t
msgEncode (long route, long msg, char *args, char *msgbuff, size_t mlen)
{
  if (args == NULL) {
    args = "";
  }
  size_t len = snprintf (msgbuff, mlen, "%0*ld%c%0*ld%c%s",
      LSZ, route, '~', LSZ, msg, '~', args);
  ++len;
  return len;
}

void
msgDecode (char *msgbuff, long *route, long *msg, char *args)
{
  *route = atol (msgbuff);
  char *p = msgbuff + LSZ + 1;
  *msg = atol (p);
  if (args != NULL) {
    *args = '\0';
    p += LSZ + 1;
    strlcpy (args, p, BDJMSG_MAX);
  }
}
