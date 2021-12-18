#include <stdlib.h>
#include <check.h>

#include "check_bdj.h"

int
main (void)
{
  int     number_failed;
  Suite   *s;
  SRunner *sr;

  s = list_suite();
  sr = srunner_create (s);
  s = rafile_suite();
  srunner_add_suite (sr, s);
  srunner_run_all (sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
