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
#ifndef CATIE_NRF24L01_H_
#define CATIE_NRF24L01_H_

class NRF24L01
{
public:

	enum class RegisterAddress : uint8_t {
		// Operations
		OP_READ             = 0x00,
		OP_WRITE            = 0x20,
		OP_RX               = 0x61,
		OP_TX               = 0xa0,
		OP_NOP              = 0xff,
		// Registers
		REG_CONFIG          = 0x00,
		REG_EN_AA           = 0x01,
		REG_EN_RXADDR       = 0x02,
		REG_SETUP_AW        = 0x03,
		REG_SETUP_RETR      = 0x04,
		REG_RF_CH           = 0x05,
		REG_RF_SETUP        = 0x06,
		REG_STATUS          = 0x07,
		REG_STATUS_ZERO     = 0x80,
		REG_STATUS_RX_DR    = 0x40,
		REG_STATUS_TX_DS    = 0x20,
		REG_OBSERVE_TX      = 0x08,
		REG_RPD             = 0x09,
		REG_RX_ADDR_P0      = 0x0a,  // 5 bytes
		REG_RX_ADDR_P1      = 0x0b,  // 5 bytes
		REG_RX_ADDR_P2      = 0x0c,
		REG_RX_ADDR_P3      = 0x0d,
		REG_RX_ADDR_P4      = 0x0e,
		REG_RX_ADDR_P5      = 0x0f,
		REG_TX_ADDR         = 0x10, // 5 bytes
		REG_RX_PW_P0        = 0x11,
		REG_RX_PW_P1        = 0x12,
		REG_RX_PW_P2        = 0x13,
		REG_RX_PW_P3        = 0x14,
		REG_RX_PW_P4        = 0x15,
		REG_RX_PW_P5        = 0x16,
		REG_FIFO_STATUS     = 0x17,
		REG_DYNPD           = 0x1c,
		REG_FEATURE         = 0x1d
	};

	NRF24L01(SPI *spi, PinName com_ce, PinName irq);

	void attach(Callback<void()> func);

	void initialize(uint8_t rf_channel);

	void set_channel(uint8_t channel);

	void set_com_ce(uint8_t level);

	void send_packet(const void *buffer, uint8_t length);

	void read_packet(void* buffer, uint8_t length);

private:
	SPI *_spi;
	DigitalOut _com_ce;
	InterruptIn _irq;
	uint8_t _channel;

	void spi_write_payload(const char *buffer, uint8_t length);

	void spi_read_payload(char* buffer, uint8_t length);

	void spi_write_register(RegisterAddress register_address, uint8_t value);

	void spi_write_register(RegisterAddress register_address, const char *value, uint8_t length);

	void spi_read_register(RegisterAddress register_address, uint8_t *value);

	void spi_read_register(RegisterAddress register_address, uint8_t *value, uint8_t length);

};


#endif // CATIE_NRF24L01_H_

