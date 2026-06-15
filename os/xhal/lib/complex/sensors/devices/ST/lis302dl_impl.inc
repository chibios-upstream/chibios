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
 * @file        lis302dl_impl.inc
 * @brief       Template of LIS302DL Driver source.
 * @note        This is a template file, can be edited directly.
 *
 * @addtogroup  LIS302DL
 * @{
 */

/* This is an, automatically generated, implementation file that can be
   manually edited, it is not re-generated if already present.*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static const lis302dl_config_t *lis302dlGetConfig(lis302dl_driver_c *self) {

  return (const lis302dl_config_t *)self->config;
}

static bool lis302dlIsValidFullScale(lis302dl_acc_fs_t fs) {

  return (fs == LIS302DL_ACC_FS_2G) || (fs == LIS302DL_ACC_FS_8G);
}

static bool lis302dlIsValidODR(lis302dl_acc_odr_t odr) {

  return (odr == LIS302DL_ACC_ODR_100HZ) ||
         (odr == LIS302DL_ACC_ODR_400HZ);
}

static bool lis302dlIsValidConfig(const lis302dl_config_t *config) {
  bool valid = true;

  if ((config == NULL) || (config->spip == NULL) ||
      (config->spicfg == NULL) ||
      !lis302dlIsValidFullScale(config->accfullscale) ||
      !lis302dlIsValidODR(config->accodr)) {
    valid = false;
  }

#if LIS302DL_USE_ADVANCED == TRUE
  if (valid &&
      (config->acchighpass != LIS302DL_ACC_HP_DISABLED) &&
      (config->acchighpass != LIS302DL_ACC_HP_0) &&
      (config->acchighpass != LIS302DL_ACC_HP_1) &&
      (config->acchighpass != LIS302DL_ACC_HP_2) &&
      (config->acchighpass != LIS302DL_ACC_HP_3)) {
    valid = false;
  }
#endif

  return valid;
}

static float lis302dlFullScaleValue(lis302dl_acc_fs_t fs) {

  return (fs == LIS302DL_ACC_FS_8G) ? LIS302DL_ACC_8G : LIS302DL_ACC_2G;
}

static float lis302dlDefaultSensitivity(lis302dl_acc_fs_t fs) {

  return (fs == LIS302DL_ACC_FS_8G) ? LIS302DL_ACC_SENS_8G :
                                      LIS302DL_ACC_SENS_2G;
}

static float lis302dlCurrentDefaultSensitivity(lis302dl_driver_c *self) {

  return (self->accfullscale == LIS302DL_ACC_8G) ? LIS302DL_ACC_SENS_8G :
                                                   LIS302DL_ACC_SENS_2G;
}

static msg_t lis302dlBusAcquire(const lis302dl_config_t *config) {
  msg_t msg;

#if LIS302DL_SHARED_SPI == TRUE
  drvLock(config->spip);
#endif

  msg = drvStart(config->spip, config->spicfg);
  if (msg != HAL_RET_SUCCESS) {
#if LIS302DL_SHARED_SPI == TRUE
    drvUnlock(config->spip);
#endif
  }

  return msg;
}

static void lis302dlBusRelease(const lis302dl_config_t *config) {

#if LIS302DL_SHARED_SPI == TRUE
  drvUnlock(config->spip);
#else
  (void)config;
#endif
}

static msg_t lis302dlSPIReadRegister(const lis302dl_config_t *config,
                                     uint8_t reg, size_t n, uint8_t *bp) {
  uint8_t cmd;
  msg_t msg;

  osalDbgCheck((config != NULL) && (n > 0U) && (bp != NULL));

  cmd = (n == 1U) ? (reg | LIS302DL_RW) :
                    (reg | LIS302DL_RW | LIS302DL_MS);

  spiSelectX(config->spip);
  msg = spiSend(config->spip, 1U, &cmd);
  if (msg == HAL_RET_SUCCESS) {
    msg = spiReceive(config->spip, n, bp);
  }
  spiUnselectX(config->spip);

  return msg;
}

static msg_t lis302dlSPIWriteRegister(const lis302dl_config_t *config,
                                      uint8_t reg, size_t n,
                                      const uint8_t *bp) {
  uint8_t cmd;
  msg_t msg;

  osalDbgCheck((config != NULL) && (n > 0U) && (bp != NULL));

  cmd = (n == 1U) ? reg : (reg | LIS302DL_MS);

  spiSelectX(config->spip);
  msg = spiSend(config->spip, 1U, &cmd);
  if (msg == HAL_RET_SUCCESS) {
    msg = spiSend(config->spip, n, bp);
  }
  spiUnselectX(config->spip);

  return msg;
}

static msg_t lis302dlWriteRegister(const lis302dl_config_t *config,
                                   uint8_t reg, size_t n,
                                   const uint8_t *bp) {
  msg_t msg;

  msg = lis302dlBusAcquire(config);
  if (msg == HAL_RET_SUCCESS) {
    msg = lis302dlSPIWriteRegister(config, reg, n, bp);
    lis302dlBusRelease(config);
  }

  return msg;
}

static msg_t lis302dlApplyConfig(lis302dl_driver_c *self,
                                 const lis302dl_config_t *config) {
  float sensitivity;
  uint8_t cr[2];
  size_t i;
  msg_t msg;

  if (!lis302dlIsValidConfig(config)) {
    return HAL_RET_CONFIG_ERROR;
  }

  cr[0] = LIS302DL_CTRL_REG1_XEN | LIS302DL_CTRL_REG1_YEN |
          LIS302DL_CTRL_REG1_ZEN | LIS302DL_CTRL_REG1_PD |
          config->accodr | config->accfullscale;
  cr[1] = 0U;

#if LIS302DL_USE_ADVANCED == TRUE
  if (config->acchighpass != LIS302DL_ACC_HP_DISABLED) {
    cr[1] = config->acchighpass;
  }
#endif

  msg = lis302dlWriteRegister(config, LIS302DL_AD_CTRL_REG1, 2U, cr);
  if (msg != HAL_RET_SUCCESS) {
    return msg;
  }

  self->accfullscale = lis302dlFullScaleValue(config->accfullscale);
  sensitivity = lis302dlDefaultSensitivity(config->accfullscale);

  for (i = 0U; i < LIS302DL_ACC_NUMBER_OF_AXES; i++) {
    self->accsensitivity[i] = (config->accsensitivity != NULL) ?
                              config->accsensitivity[i] :
                              sensitivity;
    self->accbias[i] = (config->accbias != NULL) ? config->accbias[i] :
                                                   LIS302DL_ACC_BIAS;
  }

  return HAL_RET_SUCCESS;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module class "lis302dl_driver_c" methods.                                 */
/*===========================================================================*/

/**
 * @name        Interfaces implementation of lis302dl_driver_c
 * @{
 */
/**
 * @brief       Implementation of interface method @p halSensorGetChannelNumber().
 *
 * @param[in,out] ip            Pointer to the @p hal_accelerometer_i class
 *                              interface.
 * @return                      The number of channels of the sensor.
 */
static size_t __lis302dl_accl_get_channels_number_impl(void *ip) {
  lis302dl_driver_c *self = oopIfGetOwner(lis302dl_driver_c, ip);

  osalDbgCheck(ip != NULL);

  return self->accaxes;
}

/**
 * @brief       Implementation of interface method @p halSensorReadRaw().
 *
 * @param[in,out] ip            Pointer to the @p hal_accelerometer_i class
 *                              interface.
 * @param[out]    data          Pointer to the raw data array.
 * @return                      The operation status.
 */
static msg_t __lis302dl_accl_read_raw_impl(void *ip, int32_t *data) {
  lis302dl_driver_c *self = oopIfGetOwner(lis302dl_driver_c, ip);
  const lis302dl_config_t *config = lis302dlGetConfig(self);
  uint8_t tmp;
  size_t i;
  msg_t msg;

  osalDbgCheck((ip != NULL) && (data != NULL));

  osalDbgAssert(self->state == HAL_DRV_STATE_READY, "invalid state");
  if (self->state != HAL_DRV_STATE_READY) {
    return HAL_RET_INV_STATE;
  }

  msg = lis302dlBusAcquire(config);
  if (msg == HAL_RET_SUCCESS) {
    for (i = 0U; i < LIS302DL_ACC_NUMBER_OF_AXES; i++) {
      msg = lis302dlSPIReadRegister(config,
                                    LIS302DL_AD_OUT_X + (uint8_t)(i * 2U),
                                    1U, &tmp);
      if (msg != HAL_RET_SUCCESS) {
        break;
      }
      data[i] = (int32_t)((int8_t)tmp);
    }

    lis302dlBusRelease(config);
  }

  return msg;
}

/**
 * @brief       Implementation of interface method @p halSensorReadCooked().
 *
 * @param[in,out] ip            Pointer to the @p hal_accelerometer_i class
 *                              interface.
 * @param[out]    data          Pointer to the cooked data array.
 * @return                      The operation status.
 */
static msg_t __lis302dl_accl_read_cooked_impl(void *ip, float *data) {
  lis302dl_driver_c *self = oopIfGetOwner(lis302dl_driver_c, ip);
  int32_t raw[LIS302DL_ACC_NUMBER_OF_AXES];
  size_t i;
  msg_t msg;

  osalDbgCheck((ip != NULL) && (data != NULL));

  osalDbgAssert(self->state == HAL_DRV_STATE_READY, "invalid state");
  if (self->state != HAL_DRV_STATE_READY) {
    return HAL_RET_INV_STATE;
  }

  msg = __lis302dl_accl_read_raw_impl(ip, raw);
  if (msg == HAL_RET_SUCCESS) {
    for (i = 0U; i < LIS302DL_ACC_NUMBER_OF_AXES; i++) {
      data[i] = ((float)raw[i] * self->accsensitivity[i]) -
                self->accbias[i];
    }
  }

  return msg;
}

/**
 * @brief       Implementation of interface method @p
 *              halCalibratedSensorSetBias().
 *
 * @param[in,out] ip            Pointer to the @p hal_accelerometer_i class
 *                              interface.
 * @param[in]     biases        Pointer to a buffer of bias values.
 * @return                      The operation status.
 */
static msg_t __lis302dl_accl_set_bias_impl(void *ip,
                                                    const float *biases) {
  lis302dl_driver_c *self = oopIfGetOwner(lis302dl_driver_c, ip);
  size_t i;

  osalDbgCheck((ip != NULL) && (biases != NULL));

  osalDbgAssert(self->state == HAL_DRV_STATE_READY, "invalid state");
  if (self->state != HAL_DRV_STATE_READY) {
    return HAL_RET_INV_STATE;
  }

  for (i = 0U; i < LIS302DL_ACC_NUMBER_OF_AXES; i++) {
    self->accbias[i] = biases[i];
  }

  return HAL_RET_SUCCESS;
}

/**
 * @brief       Implementation of interface method @p
 *              halCalibratedSensorResetBias().
 *
 * @param[in,out] ip            Pointer to the @p hal_accelerometer_i class
 *                              interface.
 * @return                      The operation status.
 */
static msg_t __lis302dl_accl_reset_bias_impl(void *ip) {
  lis302dl_driver_c *self = oopIfGetOwner(lis302dl_driver_c, ip);
  size_t i;

  osalDbgCheck(ip != NULL);

  osalDbgAssert(self->state == HAL_DRV_STATE_READY, "invalid state");
  if (self->state != HAL_DRV_STATE_READY) {
    return HAL_RET_INV_STATE;
  }

  for (i = 0U; i < LIS302DL_ACC_NUMBER_OF_AXES; i++) {
    self->accbias[i] = LIS302DL_ACC_BIAS;
  }

  return HAL_RET_SUCCESS;
}

/**
 * @brief       Implementation of interface method @p
 *              halCalibratedSensorSetSensitivity().
 *
 * @param[in,out] ip            Pointer to the @p hal_accelerometer_i class
 *                              interface.
 * @param[in]     sensitivities Pointer to a buffer of sensitivity values.
 * @return                      The operation status.
 */
static msg_t __lis302dl_accl_set_sensitivity_impl(void *ip,
                                                           const float *sensitivities) {
  lis302dl_driver_c *self = oopIfGetOwner(lis302dl_driver_c, ip);
  size_t i;

  osalDbgCheck((ip != NULL) && (sensitivities != NULL));

  osalDbgAssert(self->state == HAL_DRV_STATE_READY, "invalid state");
  if (self->state != HAL_DRV_STATE_READY) {
    return HAL_RET_INV_STATE;
  }

  for (i = 0U; i < LIS302DL_ACC_NUMBER_OF_AXES; i++) {
    self->accsensitivity[i] = sensitivities[i];
  }

  return HAL_RET_SUCCESS;
}

/**
 * @brief       Implementation of interface method @p
 *              halCalibratedSensorResetSensitivity().
 *
 * @param[in,out] ip            Pointer to the @p hal_accelerometer_i class
 *                              interface.
 * @return                      The operation status.
 */
static msg_t __lis302dl_accl_reset_sensitivity_impl(void *ip) {
  lis302dl_driver_c *self = oopIfGetOwner(lis302dl_driver_c, ip);
  float sensitivity;
  size_t i;

  osalDbgCheck(ip != NULL);

  osalDbgAssert(self->state == HAL_DRV_STATE_READY, "invalid state");
  if (self->state != HAL_DRV_STATE_READY) {
    return HAL_RET_INV_STATE;
  }

  sensitivity = lis302dlCurrentDefaultSensitivity(self);
  for (i = 0U; i < LIS302DL_ACC_NUMBER_OF_AXES; i++) {
    self->accsensitivity[i] = sensitivity;
  }

  return HAL_RET_SUCCESS;
}
/** @} */

/**
 * @name        Methods implementations of lis302dl_driver_c
 * @{
 */
/**
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p lis302dl_driver_c instance to
 *                              be initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @return                      A new reference to the object.
 */
void *__lis302dl_objinit_impl(void *ip, const void *vmt) {
  lis302dl_driver_c *self = (lis302dl_driver_c *)ip;
  size_t i;

  /* Initialization of the ancestors-defined parts.*/
  __drv_objinit_impl(self, vmt);

  /* Initialization of interface hal_accelerometer_i.*/
  {
    static const struct hal_accelerometer_vmt lis302dl_accl_vmt = {
      .instance_offset      = offsetof(lis302dl_driver_c, accl),
      .get_channels_number  = __lis302dl_accl_get_channels_number_impl,
      .read_raw             = __lis302dl_accl_read_raw_impl,
      .read_cooked          = __lis302dl_accl_read_cooked_impl,
      .set_bias             = __lis302dl_accl_set_bias_impl,
      .reset_bias           = __lis302dl_accl_reset_bias_impl,
      .set_sensitivity      = __lis302dl_accl_set_sensitivity_impl,
      .reset_sensitivity    = __lis302dl_accl_reset_sensitivity_impl
    };
    oopIfObjectInit(&self->accl, &lis302dl_accl_vmt);
  }

  /* Initialization code.*/
  self->accaxes     = LIS302DL_ACC_NUMBER_OF_AXES;
  self->accfullscale = LIS302DL_ACC_2G;
  for (i = 0U; i < LIS302DL_ACC_NUMBER_OF_AXES; i++) {
    self->accsensitivity[i] = LIS302DL_ACC_SENS_2G;
    self->accbias[i]        = LIS302DL_ACC_BIAS;
  }

  return self;
}

/**
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p lis302dl_driver_c instance to
 *                              be disposed.
 */
void __lis302dl_dispose_impl(void *ip) {
  lis302dl_driver_c *self = (lis302dl_driver_c *)ip;

  /* No finalization code.*/
  (void)self;

  /* Finalization of the ancestors-defined parts.*/
  __drv_dispose_impl(self);
}

/**
 * @brief       Override of method @p __drv_start().
 *
 * @param[in,out] ip            Pointer to a @p lis302dl_driver_c instance.
 * @param[in]     config        Driver configuration or @p NULL.
 * @return                      The operation status.
 */
msg_t __lis302dl_start_impl(void *ip, const void *config) {
  lis302dl_driver_c *self = (lis302dl_driver_c *)ip;
  msg_t msg;

  msg = lis302dlApplyConfig(self, (const lis302dl_config_t *)config);
  if (msg == HAL_RET_SUCCESS) {
    self->config = config;
    osalThreadSleepMilliseconds(10);
  }

  return msg;
}

/**
 * @brief       Override of method @p __drv_stop().
 *
 * @param[in,out] ip            Pointer to a @p lis302dl_driver_c instance.
 */
void __lis302dl_stop_impl(void *ip) {
  lis302dl_driver_c *self = (lis302dl_driver_c *)ip;
  const lis302dl_config_t *config = lis302dlGetConfig(self);

  if (config != NULL) {
    uint8_t cr1 = 0U;

    (void) lis302dlWriteRegister(config, LIS302DL_AD_CTRL_REG1, 1U, &cr1);
#if LIS302DL_SHARED_SPI != TRUE
    drvStop(config->spip);
#endif
  }
}

/**
 * @brief       Override of method @p __drv_set_cfg().
 *
 * @param[in,out] ip            Pointer to a @p lis302dl_driver_c instance.
 * @param[in]     config        New driver configuration.
 * @return                      The configuration pointer.
 */
const void *__lis302dl_setcfg_impl(void *ip, const void *config) {
  lis302dl_driver_c *self = (lis302dl_driver_c *)ip;

  (void)self;
  (void)config;

  return NULL;
}

/**
 * @brief       Override of method @p __drv_sel_cfg().
 *
 * @param[in,out] ip            Pointer to a @p lis302dl_driver_c instance.
 * @param[in]     cfgnum        Driver configuration number.
 * @return                      The configuration pointer.
 */
const void *__lis302dl_selcfg_impl(void *ip, unsigned cfgnum) {
  lis302dl_driver_c *self = (lis302dl_driver_c *)ip;

  (void)self;
  (void)cfgnum;

  return NULL;
}
/** @} */

/**
 * @brief       VMT structure of LIS302DL driver class.
 * @note        It is public because accessed by the inlined constructor.
 */
const struct lis302dl_driver_vmt __lis302dl_driver_vmt = {
  .dispose                  = __lis302dl_dispose_impl,
  .start                    = __lis302dl_start_impl,
  .stop                     = __lis302dl_stop_impl,
  .setcfg                   = __lis302dl_setcfg_impl,
  .selcfg                   = __lis302dl_selcfg_impl
};

/**
 * @name        Regular methods of lis302dl_driver_c
 * @{
 */
/**
 * @brief       Changes the LIS302DL accelerometer full scale value.
 *
 * @param[in,out] ip            Pointer to a @p lis302dl_driver_c instance.
 * @param[in]     fs            New full scale value.
 * @return                      The operation status.
 * @retval HAL_RET_SUCCESS      If the function succeeded.
 * @retval HAL_RET_CONFIG_ERROR If the full scale is invalid.
 *
 * @api
 */
msg_t lis302dlSetFullScale(void *ip, lis302dl_acc_fs_t fs) {
  lis302dl_driver_c *self = (lis302dl_driver_c *)ip;
  const lis302dl_config_t *config = lis302dlGetConfig(self);
  float newfs;
  float scale;
  uint8_t cr;
  size_t i;
  msg_t msg;

  osalDbgCheck(ip != NULL);

  osalDbgAssert(self->state == HAL_DRV_STATE_READY, "invalid state");
  if (self->state != HAL_DRV_STATE_READY) {
    return HAL_RET_INV_STATE;
  }

  if (!lis302dlIsValidFullScale(fs)) {
    return HAL_RET_CONFIG_ERROR;
  }

  newfs = lis302dlFullScaleValue(fs);
  if (newfs == self->accfullscale) {
    return HAL_RET_SUCCESS;
  }

  msg = lis302dlBusAcquire(config);
  if (msg != HAL_RET_SUCCESS) {
    return msg;
  }

  msg = lis302dlSPIReadRegister(config, LIS302DL_AD_CTRL_REG1, 1U, &cr);
  if (msg == HAL_RET_SUCCESS) {
    cr &= (uint8_t)~LIS302DL_CTRL_REG1_FS_MASK;
    cr |= (uint8_t)fs;
    msg = lis302dlSPIWriteRegister(config, LIS302DL_AD_CTRL_REG1, 1U, &cr);
  }

  lis302dlBusRelease(config);

  if (msg == HAL_RET_SUCCESS) {
    scale = newfs / self->accfullscale;
    self->accfullscale = newfs;

    for (i = 0U; i < LIS302DL_ACC_NUMBER_OF_AXES; i++) {
      self->accsensitivity[i] *= scale;
      self->accbias[i]        *= scale;
    }
  }

  return msg;
}
/** @} */

/** @} */
