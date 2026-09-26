/* No generation bookkeeping is required in this minimal external-style LLD. */
#ifndef TEST_SPI_HLD_LLD_H
#define TEST_SPI_HLD_LLD_H

/* Define capabilities here, not before the HLD includes the LLD header. */
#define SPI_SUPPORTS_CIRCULAR TEST_CIRCULAR
#define SPI_SUPPORTS_SLAVE_MODE FALSE
#define spi_lld_driver_fields unsigned lld_field
#define spi_lld_config_fields unsigned lld_field

void spi_lld_init(void);
msg_t spi_lld_start(hal_spi_driver_c *spip);
void spi_lld_stop(hal_spi_driver_c *spip);
const hal_spi_config_t *spi_lld_setcfg(hal_spi_driver_c *spip,
                                     const hal_spi_config_t *config);
const hal_spi_config_t *spi_lld_selcfg(hal_spi_driver_c *spip, unsigned cfgnum);
msg_t spi_lld_ignore(hal_spi_driver_c *spip, size_t n);
msg_t spi_lld_exchange(hal_spi_driver_c *spip, size_t n,
                       const void *txbuf, void *rxbuf);
msg_t spi_lld_send(hal_spi_driver_c *spip, size_t n, const void *txbuf);
msg_t spi_lld_receive(hal_spi_driver_c *spip, size_t n, void *rxbuf);
msg_t spi_lld_stop_transfer(hal_spi_driver_c *spip, size_t *np);
#define spi_lld_select(spip) ((void)(spip))
#define spi_lld_unselect(spip) ((void)(spip))

#endif
