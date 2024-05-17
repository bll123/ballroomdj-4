#ifndef INC_UICALLBACK_H
#define INC_UICALLBACK_H

typedef struct callback callback_t;

typedef bool  (*callbackFunc)(void *udata);
typedef bool  (*callbackFuncDouble)(void *udata, double value);
typedef bool  (*callbackFuncIntInt)(void *udata, int a, int b);
typedef bool  (*callbackFuncLong)(void *udata, long value);
typedef int   (*callbackFuncLongInt)(void *udata, long lval, int ival);
typedef long  (*callbackFuncStr)(void *udata, const char *txt);
typedef bool  (*callbackFuncStrInt)(void *udata, const char *txt, int value);

/* uicallback.c */
bool callbackIsSet (callback_t *cb);

bool callbackHandler (callback_t *cb);
bool callbackHandlerDouble (callback_t *cb, double value);
bool callbackHandlerLong (callback_t *cb, long value);
bool callbackHandlerIntInt (callback_t *cb, int a, int b);
int  callbackHandlerLongInt (callback_t *cb, long lval, int ival);
long callbackHandlerStr (callback_t *cb, const char *str);
bool callbackHandlerStrInt (callback_t *cb, const char *str, int value);

void callbackFree (callback_t *cb);
callback_t *callbackInit (callbackFunc cb, void *udata, const char *actiontext);
callback_t *callbackInitDouble (callbackFuncDouble cb, void *udata);
callback_t *callbackInitIntInt (callbackFuncIntInt cb, void *udata);
callback_t *callbackInitLong (callbackFuncLong cb, void *udata);
callback_t *callbackInitLongInt (callbackFuncLongInt cb, void *udata);
callback_t *callbackInitStr (callbackFuncStr cb, void *udata);
callback_t *callbackInitStrInt (callbackFuncStrInt cb, void *udata);

#endif /* INC_UICALLBACK_H */
