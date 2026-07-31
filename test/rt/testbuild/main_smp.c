/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <sched.h>
#include <stdlib.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "rt_test_root.h"
#include "oslib_test_root.h"
#include "console.h"

extern bool simSmpCore1IsReady(void);
extern bool simSmpRunLockStress(void);

/*
 * SMP simulator main.
 */
int main(int argc, char *argv[]) {

  halInit();
  conInit();
  chSysInit();

  if ((port_get_core_id() != (core_id_t)0) ||
      (ch0.core_id != (core_id_t)0) ||
      (chThdGetSelfX()->owner != &ch0) ||
      (ch_system.instances[1] != &ch1)) {
    exit(1);
  }

  while (!simSmpCore1IsReady()) {
    sched_yield();
  }

  if ((argc == 2) && (strcmp(argv[1], "--startup-only") == 0)) {
    exit(0);
  }

  if (!simSmpRunLockStress()) {
    exit(1);
  }

  if ((argc == 2) && (strcmp(argv[1], "--lock-stress-only") == 0)) {
    exit(0);
  }

  test_execute((BaseSequentialStream *)&CD1, &rt_test_suite);
  test_execute((BaseSequentialStream *)&CD1, &oslib_test_suite);
  if (chtest.global_fail) {
    exit(1);
  }
  else {
    exit(0);
  }
}
