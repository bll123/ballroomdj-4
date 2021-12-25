#ifndef INC_VOLUME
#define INC_VOLUME

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

#endif /* INC_VOLUME */
