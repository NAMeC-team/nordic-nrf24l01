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
e
#include "nrf24l01/nrf24l01.h"

namespace sixtron {

NRF24L01::NRF24L01(SPI *spi, PinName com_ce, PinName irq):
		_spi(&spi), _com_ce(com_ce), _irq(irq)
{
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


} // namespace sixtron

