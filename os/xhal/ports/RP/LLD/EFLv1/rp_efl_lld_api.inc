/**
 * @brief   Hook invoked immediately before the RP EFL driver disables XIP.
 * @details Applications may override this weak hook in order to coordinate
 *          other cores, park threads, or perform any other preparation
 *          required before flash becomes temporarily unavailable through XIP.
 * @note    This hook is called in thread context while XIP is still enabled.
 */
void rpEflBeforeXipOff(void);

/**
 * @brief   Hook invoked immediately after the RP EFL driver restores XIP.
 * @details Applications may override this weak hook in order to resume the
 *          coordination established by @p rpEflBeforeXipOff().
 * @note    This hook is called in thread context after XIP has been restored.
 */
void rpEflAfterXipOn(void);

void efl_lld_init(void);
msg_t efl_lld_start(hal_efl_driver_c *self);
void efl_lld_stop(hal_efl_driver_c *self);
flash_error_t efl_lld_read(hal_efl_driver_c *self, flash_offset_t offset,
                           size_t n, uint8_t *rp);
flash_error_t efl_lld_program(hal_efl_driver_c *self, flash_offset_t offset,
                              size_t n, const uint8_t *pp);
flash_error_t efl_lld_start_erase_all(hal_efl_driver_c *self);
flash_error_t efl_lld_start_erase_sector(hal_efl_driver_c *self,
                                         flash_sector_t sector);
flash_error_t efl_lld_query_erase(hal_efl_driver_c *self, unsigned *msec);
flash_error_t efl_lld_verify_erase(hal_efl_driver_c *self,
                                   flash_sector_t sector);

/* RP-local extensions, the standard flash interface has no slots for
   block erases with explicit JEDEC commands or unique ID reads.  Unlike
   the standard operations, which follow the shared flsAcquireExclusive()
   convention, these extensions self-lock on the driver mutex and must
   not be called with the driver already acquired.*/
flash_error_t rpEflStartEraseBlock(hal_efl_driver_c *self,
                                   uint8_t cmd,
                                   uint32_t block);
flash_error_t rpEflReadUniqueId(hal_efl_driver_c *self, uint8_t *uid);
