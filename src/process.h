#ifndef INC_PROCESS_H
#define INC_PROCESS_H

int   processExists (pid_t);
int   processStart (const char *fn, pid_t *pid, ssize_t profile, ssize_t loglvl);
int   processKill (pid_t);
void  processCatchSignal (void (*sigHandler)(int), int signal);
void  processIgnoreSignal (int signal);
void  processDefaultSignal (int signal);

#endif /* INC_PROCESS_H */
