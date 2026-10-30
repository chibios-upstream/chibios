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

/**
 * @file    fpu_irq_test.h
 * @brief   FPU registers across interrupts test header.
 */

#ifndef FPU_IRQ_TEST_H
#define FPU_IRQ_TEST_H

/**
 * @brief   Test results.
 */
typedef struct {
  /**
   * @brief   Iterations executed.
   */
  uint32_t                  iterations;
  /**
   * @brief   Iterations where the interrupt was not taken.
   */
  uint32_t                  missed;
  /**
   * @brief   Errors on the interrupt return path without preemption.
   */
  uint32_t                  errors_no_preemption;
  /**
   * @brief   Errors on the interrupt return path with preemption.
   */
  uint32_t                  errors_preemption;
} fpu_irq_test_result_t;

#ifdef __cplusplus
extern "C" {
#endif
  bool fpu_irq_test_execute(fpu_irq_test_result_t *rp);
#ifdef __cplusplus
}
#endif

#endif /* FPU_IRQ_TEST_H */
