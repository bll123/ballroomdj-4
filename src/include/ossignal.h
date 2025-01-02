/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_OSSIGNAL_H
#define INC_OSSIGNAL_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

void  osSetStandardSignals (void (*sigHandler)(int));
void  osCatchSignal (void (*sigHandler)(int), int signal);
void  osIgnoreSignal (int signal);
void  osDefaultSignal (int signal);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_OSSIGNAL_H */
