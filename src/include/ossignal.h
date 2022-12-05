/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_OSSIGNAL_H
#define INC_OSSIGNAL_H

void  osSetStandardSignals (void (*sigHandler)(int));
void  osCatchSignal (void (*sigHandler)(int), int signal);
void  osIgnoreSignal (int signal);
void  osDefaultSignal (int signal);

#endif /* INC_OSSIGNAL_H */
