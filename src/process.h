#ifndef INC_PROCESS_H
#define INC_PROCESS_H

int   processExists (pid_t);
int   processStart (const char *fn, pid_t *pid, long profile, long loglvl);
void  processCatchSignals (void (*sigHandler)(int));
void  processSigChildIgnore (void);
void  processSigChildDefault (void);

#endif /* INC_PROCESS_H */
