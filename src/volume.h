#ifndef _INC_AUDIOMAIN
#define _INC_AUDIOMAIN

typedef enum {
  VOL_GET,
  VOL_SET,
  VOL_GETSINKLIST
} volaction_t;

int volumeGet (char *sinkname);
int volumeSet (char *sinkname, int vol);
int volumeGetSinkList (char *sinkname, char **sinklist);

int process (volaction_t action, char *sinkname, int *vol, char **sinklist);
void audioDisconnect (void);

#endif /* _INC_AUDIOMAIN */
