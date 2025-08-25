/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

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

