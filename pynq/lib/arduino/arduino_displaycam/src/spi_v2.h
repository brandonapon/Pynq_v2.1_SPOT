/*
 * spi_v2.h
 *
 *  Created on: Apr 16, 2018
 *      Author: Brandon
 */

#ifndef SRC_SPI_V2_H_
#define SRC_SPI_V2_H_

#include <xparameters.h>
#include <stdint.h>

#ifdef XPAR_XSPI_NUM_INSTANCES

/*
 * SPI API
 */
typedef int spi;

spi spi_open_device(unsigned int device);
spi spi_open(unsigned int spiclk, unsigned int miso,
             unsigned int mosi, unsigned int ss);
spi spi_configure(spi dev_id, unsigned int clk_phase,
                   unsigned int clk_polarity);
void spi_transfer(spi dev_id, const uint8_t* write_data, uint8_t* read_data,
                  unsigned int length);

void spi_transfer_mod(spi dev_id, const char* write_data, char* read_data, unsigned int length_write, unsigned int length_read);
void spi_close(spi dev_id);
unsigned int spi_get_num_devices(void);

#endif


#endif /* SRC_SPI_V2_H_ */