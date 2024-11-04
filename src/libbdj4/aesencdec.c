/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <glib.h>
#include <gcrypt.h>

#include "bdjstring.h"
#include "log.h"
#include "mdebug.h"
#include "aesencdec.h"
#include "osrandom.h"

enum {
  AES_ENCRYPT,
  AES_DECRYPT,
};

enum {
  AES_KEY_SZ = 32,
  AES_MAX_STR_SZ = 400,
  AES_SALT_SZ = 16,
  AES_RAND_SZ = 7,
};

static const char * const AES_E_PFX = "A256";

static bool initialized = false;

static void aesInit (void);
static void aesMakeKey (uint8_t *key, uint8_t *rbytes, int mode);
  // static void dumpBytes (const uint8_t *str, size_t len);

bool
aesencrypt (const char *str, char *buff, size_t sz)
{
  uint8_t           salt [AES_SALT_SZ];
  uint8_t           tmp [AES_MAX_STR_SZ];
  char              tstr [AES_MAX_STR_SZ];
  uint8_t           key [AES_KEY_SZ];
  uint8_t           rbytes [AES_RAND_SZ];
  gcry_cipher_hd_t  gch;
  size_t            len;
  size_t            tlen;
  ssize_t           rem;
  char              *data;
  int               rc;
  short             savelen;
  char              *p;

  aesInit ();

  *buff = '\0';
  len = strlen (str);
  for (int i = 0; i < AES_SALT_SZ; ++i) {
    salt [i] = (uint8_t) (dRandom () * 256.0) & 0xff;
  }

  rc = gcry_cipher_open (&gch, GCRY_CIPHER_AES256, GCRY_CIPHER_MODE_ECB, 0);
  if (rc != GPG_ERR_NO_ERROR) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "open error: %s", gcry_strerror (rc));
    return false;
  }
  aesMakeKey (key, rbytes, AES_ENCRYPT);
  rc = gcry_cipher_setkey (gch, key, AES_KEY_SZ);
  if (rc != GPG_ERR_NO_ERROR) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "set-key error: %s", gcry_strerror (rc));
    return false;
  }
  rc = gcry_cipher_setiv (gch, salt, AES_SALT_SZ);
  if (rc != GPG_ERR_NO_ERROR) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "set-iv error: %s", gcry_strerror (rc));
    return false;
  }
  gcry_cipher_final (gch);

  tlen = len;
  stpecpy (tstr, tstr + sizeof (tstr), str);
  savelen = (int16_t) len;
  rem = ((tlen / AES_KEY_SZ) * AES_KEY_SZ) - tlen;
  if (rem < (ssize_t) sizeof (int16_t)) {
    rem += AES_KEY_SZ;
  }
  rem -= sizeof (int16_t);
  memset (tstr + len, rem, rem);
  memcpy (tstr + len + rem, &savelen, sizeof (int16_t));
  tlen += rem;
  tlen += sizeof (int16_t);

  rc = gcry_cipher_encrypt (gch, tmp, sizeof (tmp), tstr, tlen);
  if (rc != GPG_ERR_NO_ERROR) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "encrypt error: %s", gcry_strerror (rc));
    return false;
  }
  memset (key, '\0', sizeof (key));

  memcpy (tmp + tlen, rbytes, AES_RAND_SZ);
  memcpy (tmp + tlen + AES_RAND_SZ, salt, AES_SALT_SZ);
  tlen += AES_RAND_SZ;
  tlen += AES_SALT_SZ;

  data = g_base64_encode ((const guchar *) tmp, tlen);
  mdextalloc (data);
  p = buff;
  p = stpecpy (p, buff + sz, AES_E_PFX);
  p = stpecpy (p, buff + sz, data);
  mdfree (data);  // allocated by glib

  gcry_cipher_close (gch);
  return true;
}

bool
aesdecrypt (const char *str, char *buff, size_t sz)
{
  uint8_t           salt [AES_SALT_SZ];
  uint8_t           tmp [AES_MAX_STR_SZ];
  uint8_t           key [AES_KEY_SZ];
  uint8_t           rbytes [AES_RAND_SZ];
  gcry_cipher_hd_t  gch;
  char              *data;
  size_t            len;
  size_t            dlen;
  int               rc;
  int16_t           savelen;

  aesInit ();

  *buff = '\0';
  len = strlen (str);
  if (strncmp (str, AES_E_PFX, strlen (AES_E_PFX)) != 0) {
    return false;
  }

  len -= strlen (AES_E_PFX);
  dlen = len;
  data = (char *) g_base64_decode (
      (const gchar *) str + strlen (AES_E_PFX), &dlen);
  mdextalloc (data);
  dlen -= AES_SALT_SZ;
  dlen -= AES_RAND_SZ;
  memcpy (rbytes, data + dlen, AES_RAND_SZ);
  memcpy (salt, data + dlen + AES_RAND_SZ, AES_SALT_SZ);

  rc = gcry_cipher_open (&gch, GCRY_CIPHER_AES256, GCRY_CIPHER_MODE_ECB, 0);
  if (rc != GPG_ERR_NO_ERROR) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "open error: %s", gcry_strerror (rc));
    return false;
  }
  aesMakeKey (key, rbytes, AES_DECRYPT);
  rc = gcry_cipher_setkey (gch, key, AES_KEY_SZ);
  if (rc != GPG_ERR_NO_ERROR) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "set-key error: %s", gcry_strerror (rc));
    return false;
  }
  rc = gcry_cipher_setiv (gch, salt, AES_SALT_SZ);
  if (rc != GPG_ERR_NO_ERROR) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "set-iv error: %s", gcry_strerror (rc));
    return false;
  }
  gcry_cipher_final (gch);
  rc = gcry_cipher_decrypt (gch, tmp, sizeof (tmp), data, dlen);
  if (rc != GPG_ERR_NO_ERROR) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "decrypt error: %s", gcry_strerror (rc));
    return false;
  }
  memset (key, '\0', sizeof (key));

  /* get the saved real length */
  memcpy (&savelen, tmp + dlen - sizeof (int16_t), sizeof (int16_t));
  memcpy (buff, tmp, savelen);
  buff [savelen] = '\0';
  mdfree (data);  // allocated by glib

  gcry_cipher_close (gch);
  return true;
}

static void
aesInit (void)
{
  if (initialized) {
    return;
  }

  (void) ! gcry_check_version (NULL);
  initialized = true;
}

static void
aesMakeKey (uint8_t *key, uint8_t *rbytes, int mode)
{
  size_t      klen;
  size_t      rbidx;

  for (size_t i = 0; i < AES_KEY_SZ; ++i) {
    key [i] = '\0';
  }
  klen = 0;
  if (mode == AES_ENCRYPT) {
    for (size_t i = 0; i < AES_RAND_SZ; ++i) {
      rbytes [i] = (uint8_t) (dRandom () * 256.0) & 0xff;
    }
  }

  rbidx = 0;
  for (size_t i = klen; i < AES_KEY_SZ; ++i) {
    key [i] = i + rbytes [0] + rbytes [rbidx];
    ++rbidx;
    if (rbidx >= AES_RAND_SZ) {
      rbidx = 0;
    }
  }
}

#if 0   /* for debugging */
static void
dumpBytes (const uint8_t *str, size_t len)
{
  for (size_t i = 0; i < len; ++i) {
    fprintf (stderr, "%02x ", str [i] & 0xff);
  }
  fprintf (stderr, "\n");
}
#endif
