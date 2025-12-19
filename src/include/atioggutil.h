/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

int   atioggCheckCodec (const char *ffn, int filetype);
int   atioggWriteOggFile (const char *ffn, void *newvc, int filetype);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

