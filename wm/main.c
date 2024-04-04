#include "wm.h"

#include <stdlib.h>

int main() {
  window_manager wm = wm_create();

  wm_run(&wm);

  XCloseDisplay(wm.display);
  return EXIT_SUCCESS;
}
