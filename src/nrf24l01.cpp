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

void NRF24L01::send_packet(uint8_t *packet, size_t packet_length, uint8_t *response, size_t response_length)
{
	//TODO : to be implemented
}

void NRF24L01::spi_set_register(RegisterAddress register_address, uint8_t value)
{
	static char data[2];
	static char resp[2];
//	register_address |= RegisterAddress::OP_WRITE;

	// formatting data
	data[0] = (static_cast<char>(register_address) | static_cast<char>(RegisterAddress::OP_WRITE));
	data[1] = value;

	_spi->write(data, sizeof(data), resp, sizeof(resp));

	//ignore response

}

void NRF24L01::spi_get_register(RegisterAddress register_address, uint8_t *value)
{
	static char data;
	static char resp[2];

	data = (static_cast<char>(register_address) | static_cast<char>(RegisterAddress::OP_READ));

	_spi->write(&data, 1, resp, sizeof(resp));

	*value = resp[1];

}


