
#include <stdio.h>
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

Cmd cmd = {0};

int main(int argc, char *argv[]) {
  NOB_GO_REBUILD_URSELF(argc, argv);

  nob_cc(&cmd);
  nob_cc_flags(&cmd);
  nob_cc_output(&cmd, "swaystatus");
  nob_cc_inputs(&cmd, "swaystatus.c");

  if (!cmd_run(&cmd)) {
    fprintf(stderr, "ERROR: Failed to compile swaystatus");
    return 1;
  }

  // cmd_append(&cmd, "./swaystatus");

  // if (!cmd_run(&cmd)) {
  //   fprintf(stderr, "ERROR: Failed to run swaystatus");
  //   return 1;
  // }

  return 0;
}
