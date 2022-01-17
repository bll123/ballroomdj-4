#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>

int
main (int argc, char *argv[])
{
  unsigned int val = 1;
  int   c = 0;
  int   option_index = 0;

  /* abuse processstart and use the --profile arg to pass in the sleep time */
  static struct option bdj_options [] = {
    { "profile",    required_argument,  NULL,   'p' },
    { "debug"  ,    required_argument,  NULL,   'd' },
    { NULL,         0,                  NULL,   0 }
  };

  while ((c = getopt_long (argc, argv, "", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'd': {
        break;
      }
      case 'p': {
        if (optarg) {
          val = atol (optarg);
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  sleep (val);
  return 0;
}
