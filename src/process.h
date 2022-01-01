#ifndef INC_PROCESS_H
#define INC_PROCESS_H

int processExists (pid_t);
int processStart (const char *fn, pid_t *pid, long profile);

#endif /* INC_PROCESS_H */
