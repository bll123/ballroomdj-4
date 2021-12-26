#ifndef INC_VOLUME
#define INC_VOLUME

typedef enum {
  VOL_GET,
  VOL_SET,
  VOL_GETSINKLIST
} volaction_t;

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

int volumeGet (char *sinkname);
int volumeSet (char *sinkname, int vol);
int volumeGetSinkList (char *sinkname, char **sinklist);

int process (volaction_t action, char *sinkname, int *vol, char **sinklist);
void audioDisconnect (void);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INC_VOLUME */
