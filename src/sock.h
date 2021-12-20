#ifndef INC_SOCK_H
#define INC_SOCK_H

#include <sys/types.h>

typedef struct {
  struct pollfd   *pollfds;
  int             pollcount;
} sockinfo_t;

#define SOCK_READ_TIMEOUT   2

int           sockServer (short, int *);
void          sockClose (int);
sockinfo_t *  sockAddPoll (sockinfo_t *, int);
void          sockRemovePoll (sockinfo_t *, int);
void          sockFreePoll (sockinfo_t *);
int           sockPoll (sockinfo_t *);
int           sockAccept (int, int *);
int           sockConnect (short, int *);
int           sockConnectWait (short, size_t);
char *        sockRead (int, size_t *);
char *        sockReadBuff (int, size_t *, char *data, size_t);
int           sockWriteStr (int, char *, size_t);
int           sockWriteBinary (int, char *, size_t);

#endif /* INC_SOCK_H */
