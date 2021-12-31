#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bdjmsg.h"

size_t
msgEncode (long route, long msg, char *args, char *msgbuff, size_t mlen)
{
  // ### FIX TODO handle args
  size_t len = snprintf (msgbuff, mlen, "%08ld%c%08ld%c%s",
      route, '\0', msg, '\0', "");
  return len;
}

void
msgDecode (char *msgbuff, long *route, long *msg, char *args)
{
  *route = atol (msgbuff);
  char *p = msgbuff + strlen (msgbuff) + 1;
  *msg = atol (p);
  // ### FIX TODO handle args
}
