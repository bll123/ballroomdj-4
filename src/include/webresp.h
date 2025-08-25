/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#define WEB_RESP_OK         "OK"
#define WEB_RESP_NO_CONTENT "No Content"
#define WEB_RESP_BAD_REQ    "Bad Request"
#define WEB_RESP_UNAUTH     "Unauthorized"
#define WEB_RESP_FORBIDDEN  "Forbidden"
#define WEB_RESP_NOT_FOUND  "Not found"

enum {
  WEB_OK = 200,
  WEB_NO_CONTENT = 204,
  WEB_BAD_REQUEST = 400,
  WEB_UNAUTHORIZED = 401,
  WEB_FORBIDDEN = 403,
  WEB_NOT_FOUND = 404,
};

