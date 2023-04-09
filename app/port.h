#ifndef PORT_H
#define PORT_H

#include <stdint.h>
#include <util.h>

#define MAX_SPI_PACKET 256

void spi_init();

error_t spi_write_reg(uint8_t addr, uint8_t data);
error_t spi_read_reg(uint8_t addr, uint8_t * data);
error_t spi_write_reg_burst(uint8_t addr, uint8_t * data, uint8_t len);
error_t spi_read_reg_burst(uint8_t addr, uint8_t * data, uint8_t len);

void uart_init(void);
error_t uart_wait(void);
uint16_t uart_get_buffer(uint8_t ** buffer);

void port_register_radio_cb(void (*cb)(void));


#endif /* PORT_H */
