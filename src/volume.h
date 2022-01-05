#ifndef INC_VOLUME
#define INC_VOLUME

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  VOL_GET,
  VOL_SET,
  VOL_GETSINKLIST
} volaction_t;

typedef struct {
  int       defaultFlag;
  int       idxNumber;
  char      *name;
  char      *description;
} volsinkitem_t;

typedef struct {
  char            *defname;
  size_t          count;
  volsinkitem_t   *sinklist;
} volsinklist_t;

int     volumeGet (char *sinkname);
int     volumeSet (char *sinkname, int vol);
int     volumeGetSinkList (char *sinkname, volsinklist_t *sinklist);
void    volumeFreeSinkList (volsinklist_t *sinklist);

int     volumeProcess (volaction_t action, char *sinkname,
            int *vol, volsinklist_t *sinklist);
void    volumeDisconnect (void);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INC_VOLUME */
