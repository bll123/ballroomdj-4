#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "check_bdj.h"
#include "progstate.h"

static bool pscb (void *udata, programstate_t state);
static int statetab [STATE_MAX];

START_TEST(progstate_chk)
{
  progstate_t     *ps;
  programstate_t  currstate = STATE_NOT_RUNNING;
  int             userdata = 1;

  for (int i = 0; i < STATE_MAX; ++i) {
    statetab [i] = 0;
  }

  ps = progstateInit ("chk-progstate");
  progstateSetCallback (ps, STATE_NOT_RUNNING, pscb, &userdata);
  progstateSetCallback (ps, STATE_INITIALIZING, pscb, &userdata);
  progstateSetCallback (ps, STATE_LISTENING, pscb, &userdata);
  progstateSetCallback (ps, STATE_CONNECTING, pscb, &userdata);
  progstateSetCallback (ps, STATE_WAIT_HANDSHAKE, pscb, &userdata);
  progstateSetCallback (ps, STATE_INITIALIZE_DATA, pscb, &userdata);
  progstateSetCallback (ps, STATE_RUNNING, pscb, &userdata);
  progstateSetCallback (ps, STATE_STOPPING, pscb, &userdata);
  progstateSetCallback (ps, STATE_STOP_WAIT, pscb, &userdata);
  progstateSetCallback (ps, STATE_CLOSING, pscb, &userdata);
  progstateSetCallback (ps, STATE_CLOSED, pscb, &userdata);

  ck_assert_int_eq (progstateCurrState (ps), currstate);

  while (! progstateIsRunning (ps)) {
    ck_assert_int_eq (statetab [currstate], 0);
    progstateProcess (ps);
    /* prior state check */
    ck_assert_int_eq (statetab [currstate], 1);
    ++currstate;
    /* current state */
    ck_assert_int_eq (progstateCurrState (ps), currstate);
    ck_assert_int_eq (statetab [currstate], 0);
  }

  ck_assert_int_eq (currstate, STATE_RUNNING);
  ck_assert_int_eq (statetab [currstate], 0);
  /* should not progress from the running state */
  /* but should call callback once */
  /* bdj4 usual processing never does this */
  progstateProcess (ps);
  ck_assert_int_eq (currstate, STATE_RUNNING);
  ck_assert_int_eq (statetab [currstate], 1);
  progstateProcess (ps);
  /* and should not call the callback again */
  ck_assert_int_eq (currstate, STATE_RUNNING);
  ck_assert_int_eq (statetab [currstate], 1);

  ck_assert_int_eq (statetab [STATE_STOPPING], 0);
  progstateShutdownProcess (ps);
  ++currstate;
  ck_assert_int_eq (progstateCurrState (ps), currstate);
  ck_assert_int_eq (currstate, STATE_STOPPING);
  /* callback has not been run as yet -- only the state has changed */
  ck_assert_int_eq (statetab [currstate], 0);

  while (progstateCurrState (ps) != STATE_CLOSED) {
    ck_assert_int_eq (statetab [currstate], 0);
    progstateProcess (ps);
    /* prior state check */
    ck_assert_int_eq (statetab [currstate], 1);
    ++currstate;
    /* current state */
    ck_assert_int_eq (progstateCurrState (ps), currstate);
    ck_assert_int_eq (statetab [currstate], 0);
  }

  ck_assert_int_eq (currstate, STATE_CLOSED);
  ck_assert_int_eq (statetab [currstate], 0);
  /* should not progress from the closed state */
  /* but should call the callback */
  /* bdj4 usual processing never does this */
  progstateProcess (ps);
  ck_assert_int_eq (currstate, STATE_CLOSED);
  ck_assert_int_eq (statetab [currstate], 1);
  progstateProcess (ps);
  ck_assert_int_eq (currstate, STATE_CLOSED);
  ck_assert_int_eq (statetab [currstate], 1);

  progstateFree (ps);
}
END_TEST

START_TEST(progstate_multi_skip)
{
  progstate_t     *ps;
  programstate_t  currstate = STATE_NOT_RUNNING;
  int             userdata = 1;

  for (int i = 0; i < STATE_MAX; ++i) {
    statetab [i] = 0;
  }

  ps = progstateInit ("chk-progstate");
  /* multiple registered callbacks are supported */
  /* unregistered states are supported */
  progstateSetCallback (ps, STATE_INITIALIZING, pscb, &userdata);
  progstateSetCallback (ps, STATE_INITIALIZING, pscb, &userdata);
  progstateSetCallback (ps, STATE_CLOSING, pscb, &userdata);
  progstateSetCallback (ps, STATE_CLOSING, pscb, &userdata);

  ck_assert_int_eq (progstateCurrState (ps), currstate);

  while (! progstateIsRunning (ps)) {
    ck_assert_int_eq (statetab [currstate], 0);
    progstateProcess (ps);
    currstate = progstateCurrState (ps);
    if (currstate == STATE_INITIALIZING) {
      ck_assert_int_eq (statetab [currstate], 2);
    } else {
      /* no callback registered */
      ck_assert_int_eq (statetab [currstate], 0);
    }
  }

  ck_assert_int_eq (progstateCurrState (ps), STATE_RUNNING);

  ck_assert_int_eq (statetab [STATE_STOPPING], 0);
  progstateShutdownProcess (ps);
  ck_assert_int_eq (progstateCurrState (ps), STATE_STOPPING);

  while (progstateCurrState (ps) != STATE_CLOSED) {
    ck_assert_int_eq (statetab [currstate], 0);
    progstateProcess (ps);
    currstate = progstateCurrState (ps);
    if (currstate == STATE_CLOSING) {
      ck_assert_int_eq (statetab [currstate], 2);
    } else {
      /* no callback registered */
      ck_assert_int_eq (statetab [currstate], 0);
    }
  }

  ck_assert_int_eq (currstate, STATE_CLOSED);
  progstateFree (ps);
}
END_TEST

Suite *
progstate_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("progstate");
  tc = tcase_create ("progstate");
  tcase_add_test (tc, progstate_chk);
  tcase_add_test (tc, progstate_multi_skip);
  suite_add_tcase (s, tc);
  return s;
}

static bool
pscb (void *udata, programstate_t state)
{
  int     *userdata;

  userdata = udata;
  ck_assert_int_eq (*userdata, 1);

  statetab [state] += 1;
  return STATE_FINISHED;
}
