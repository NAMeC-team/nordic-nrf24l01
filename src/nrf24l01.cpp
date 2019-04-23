/*
 * Copyright (c) 2019, CATIE
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mbed.h"

#include "nrf24l01/nrf24l01.h"

NRF24L01::NRF24L01(SPI *spi, PinName com_ce, PinName irq):
		_com_ce(com_ce), _irq(irq)
{
	_spi = spi;
	// TODO: set spi format ?
	_spi->format(8,0);
	_com_ce = 0;
	_channel = 0;
}

void NRF24L01::initialize(uint8_t rf_channel)
{
	set_channel(rf_channel);
	// disabling auto acknowledgement
	spi_write_register(RegisterAddress::REG_EN_AA, 0x00);
	// setting the appropriate channel
	spi_write_register(RegisterAddress::REG_RF_CH, _channel);
}

void NRF24L01::attach(Callback<void()> func)
{
	 if (func) {
		 _irq.fall(func);
		 _irq.enable_irq();
	} else {
		_irq.fall(NULL);
		_irq.disable_irq();
	}
}

void NRF24L01::set_channel(uint8_t channel)
{
	uint8_t max_channel = 127;

	if (channel > max_channel) {
		channel = max_channel;
	}
	spi_write_register(RegisterAddress::REG_RF_CH, channel);
}

void NRF24L01::send_packet(const void *buffer, uint8_t length)
{
	//TODO : to be implemented
}

void NRF24L01::read_packet(void* buffer, uint8_t length)
{
	//TODO: to be implemented
}

/***************************************************************************
 * transport layer
 ***************************************************************************/
void NRF24L01::spi_write_register(RegisterAddress register_address, uint8_t value)
{
	static char data[2];
	static char resp[2];

	// formatting data
	data[0] = (static_cast<char>(register_address) | static_cast<char>(RegisterAddress::OP_WRITE));
	data[1] = value;

	//TODO: ignore response?
	_spi->write(data, sizeof(data), resp, sizeof(resp));
}

void NRF24L01::spi_write_register(RegisterAddress register_address, const char *value, uint8_t length)
{
	static uint8_t reg;
	static char *data;
	static char resp[2];

	// create a dynamic buffer to use mbed spi API
	data = new char[length];

	// formatting data
	reg = (static_cast<char>(register_address) | static_cast<char>(RegisterAddress::OP_WRITE));
	data[0] = reg;

	for (int i = 1; i < length; i++) {
		data[i] = *value;
		value++;
	}

	//TODO: ignore response?
	_spi->write(data, length, resp, sizeof(resp));

	delete data;
}

void NRF24L01::spi_read_register(RegisterAddress register_address, uint8_t *value)
{
	static char reg;
	static char resp[2];

	reg = (static_cast<char>(register_address) | static_cast<char>(RegisterAddress::OP_READ));

	_spi->write(&reg, 1, resp, sizeof(resp));

	*value = resp[1];
}

void NRF24L01::spi_read_register(RegisterAddress register_address, uint8_t *value, size_t length)
{
	static char *data;
	static char reg;

	// format register value
	reg = (static_cast<char>(register_address) | static_cast<char>(RegisterAddress::OP_READ));

	// spi write sequence
	_spi->write(&reg, 1, (char *)value, length);

	delete data;
}

