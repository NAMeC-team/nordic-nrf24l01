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

namespace {
#define _SPI_API_WITHOUT_CS_
#define MAX_PAYLOAD_SIZE		32 // in bytes
#define MAX_DATA_PIPE			6
#define MIN_RF_FREQUENCY    	2400 // in Hz
#define MAX_RF_FREQUENCY		2525 // in Hz
#define DEFAULT_RF_FREQUENCY	2402 // in Hz
#define HARDWARE_DELAY			4	 // in µs
}

NRF24L01::NRF24L01(SPI *spi, PinName com_ce, PinName irq):
		_com_cs(NC), _com_ce(com_ce), _irq(irq)
{
	_spi = spi;
	_spi->format(8,0);
	_com_ce = 0;
	_rf_frequency = DEFAULT_RF_FREQUENCY;
	_payload_size = MAX_PAYLOAD_SIZE;
	_mode = OperationMode::POWER_DOWN;
	_data_rate = DataRate::_2MBPS;
	_rf_output_power = RFoutputPower::_0dBm;
}

NRF24L01::NRF24L01(SPI *spi, PinName com_cs, PinName com_ce, PinName irq):
		_com_cs(com_cs), _com_ce(com_ce), _irq(irq)
{
	_spi = spi;
	_spi->format(8,0);
	_com_cs = 1;
	_com_ce = 0;
	_rf_frequency = DEFAULT_RF_FREQUENCY;
	_payload_size = MAX_PAYLOAD_SIZE;
	_mode = OperationMode::TRANSCEIVER;
	_data_rate = DataRate::_2MBPS;
	_rf_output_power = RFoutputPower::_0dBm;
}

void NRF24L01::initialize(OperationMode mode, DataRate data_rate, uint16_t rf_frequency)
{
	// set mode to the member
	_mode = mode;

	// disable auto acknowledgement
	set_auto_acknowledgement(false);

	// set frequency channel
	set_rf_frequency(rf_frequency);

	// set date rate
	set_data_rate(data_rate);

	// enable CRC
	set_crc(CRCwidth::NONE);

	// set mode and power up
	set_power_up_and_mode(mode);

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

void NRF24L01::clear_interrupt_flags(void)
{
	spi_write_register(RegisterAddress::REG_STATUS, 0x7E);
}

void NRF24L01::set_interrupt(InterruptMode interrupt_mode)
{
	uint8_t register_value = 0xff;

	register_value = spi_read_register(RegisterAddress::REG_CONFIG);

	// disable all interrupts, force to 1 to disable
	register_value |= 0x70;

	switch (interrupt_mode) {
		case InterruptMode::NONE:
			// already configured
			break;
		case InterruptMode::RX_ONLY:
			register_value &= 0x3F;
			break;
		case InterruptMode::TX_ONLY:
			register_value &= 0x5F;
			break;
		case InterruptMode::RX_TX:
			register_value &= 0x1F;
			break;
		case InterruptMode::RETRANSMIT:
			register_value &= 0x6F;
			break;
		case InterruptMode::TX_RETRANSMIT:
			register_value &= 0x4F;
			break;
	}

	spi_write_register(RegisterAddress::REG_CONFIG, register_value);

}

void NRF24L01::start_listening(void)
{
	flush_rx();
	flush_tx();
	set_com_ce(1);
}

void NRF24L01::stop_listening(void)
{
	set_com_ce(0);
	flush_tx();
	flush_rx();
}

void NRF24L01::set_tx_address(uint8_t *tx_addr)
{
	spi_write_register(RegisterAddress::REG_TX_ADDR, (const char *)tx_addr, 5);
}

void NRF24L01::set_crc(CRCwidth crc_width)
{
	int8_t reg_config = 0;
	// read current status of CONFIG register
	reg_config = spi_read_register(RegisterAddress::REG_CONFIG);

	switch (crc_width) {
		case CRCwidth::NONE:
			// disable CRC
			reg_config &= 0xF7;
			break;
		case CRCwidth::_8bits:
			// set 0 to CRC width bits register
			reg_config = (reg_config & 0xFB);
			// enable CRC
			reg_config |= (1 << 3);
			break;
		case CRCwidth::_16bits:
			// set 1 to CRC width bits register
			reg_config |= (1 << 2);
			// enable CRC
			reg_config |= (1 << 3);
			break;
	}
	// write new value
	spi_write_register(RegisterAddress::REG_CONFIG, reg_config);
}

void NRF24L01::power_up(void)
{
	uint8_t reg_config = 0;
	// read current status of CONFIG register
	reg_config = spi_read_register(RegisterAddress::REG_CONFIG);
	// power up
	reg_config |= (1 << 1);
	// write new value config register
	spi_write_register(RegisterAddress::REG_CONFIG, reg_config);
}

void NRF24L01::power_down(void)
{
	uint8_t reg_config = 0;
	// read current status of CONFIG register
	reg_config = spi_read_register(RegisterAddress::REG_CONFIG);
	// power down
	reg_config &= 0xFD;
	// write new value config register
	spi_write_register(RegisterAddress::REG_CONFIG, reg_config);
	// set mode
	_mode = OperationMode::POWER_DOWN;
}

void NRF24L01::set_mode(OperationMode mode)
{
	uint8_t reg_config = 0;
	// read current status of CONFIG register
	reg_config = spi_read_register(RegisterAddress::REG_CONFIG);
	if (mode == OperationMode::RECEIVER) {
		// Rx control
		reg_config |= (1 << 0);
	} else {
		// Tx control
		reg_config &= 0xF7;
	}
	// write new value config register
	spi_write_register(RegisterAddress::REG_CONFIG, reg_config);

	_mode = mode;
}

NRF24L01::OperationMode NRF24L01::mode(void)
{
	return _mode;
}

void NRF24L01::set_power_up_and_mode(OperationMode mode)
{
	uint8_t reg_config = 0;
	// read current status of CONFIG register
	reg_config = spi_read_register(RegisterAddress::REG_CONFIG);

	if (mode == OperationMode::RECEIVER) {
		// Rx control
		reg_config = ((reg_config & 0xEC) | 0x03);
	} else {
		// Tx control
		reg_config = ((reg_config & 0xEC) | 0x02);
	}
	// write new value config register
	spi_write_register(RegisterAddress::REG_CONFIG, reg_config);
}

void NRF24L01::set_auto_acknowledgement(bool enable)
{
	if (enable) {
		spi_write_register(RegisterAddress::REG_EN_AA, 0x3F);
	} else {
		spi_write_register(RegisterAddress::REG_EN_AA, 0x00);
	}
}

void NRF24L01::set_auto_acknowledgement(uint8_t pipe, bool enable)
{
	uint8_t reg_en_aa = 0;

	if (pipe <= MAX_DATA_PIPE) {
		// read current value register
		reg_en_aa = spi_read_register(RegisterAddress::REG_EN_AA);
		// format new value
		if (enable) {
			reg_en_aa |= (1 << pipe);
		} else {
			reg_en_aa &= ~(1 << pipe);
		}
		//write new value register
		spi_write_register(RegisterAddress::REG_EN_AA, reg_en_aa);
	}
}

void NRF24L01::set_payload_size(RxAddressPipe rx_addr_pipe, uint8_t payload_size)
{
	if (payload_size > MAX_PAYLOAD_SIZE) {
		payload_size = MAX_PAYLOAD_SIZE;
	}

	switch(rx_addr_pipe) {
		case RxAddressPipe::RX_ADDR_P0:
			spi_write_register(RegisterAddress::REG_RX_PW_P0, payload_size);
			break;
		case RxAddressPipe::RX_ADDR_P1:
			spi_write_register(RegisterAddress::REG_RX_PW_P1, payload_size);
			break;
		case RxAddressPipe::RX_ADDR_P2:
			spi_write_register(RegisterAddress::REG_RX_PW_P2, payload_size);
			break;
		case RxAddressPipe::RX_ADDR_P3:
			spi_write_register(RegisterAddress::REG_RX_PW_P3, payload_size);
			break;
		case RxAddressPipe::RX_ADDR_P4:
			spi_write_register(RegisterAddress::REG_RX_PW_P4, payload_size);
			break;
		case RxAddressPipe::RX_ADDR_P5:
			spi_write_register(RegisterAddress::REG_RX_PW_P5, payload_size);
			break;
	}
	_payload_size = payload_size;
}

uint8_t NRF24L01::payload_size(void)
{
	return _payload_size;
}

void NRF24L01::set_channel(uint8_t channel)
{
	uint8_t max_channel = 127;

	if (channel > max_channel) {
		channel = max_channel;
	}
	spi_write_register(RegisterAddress::REG_RF_CH, channel);
}

void NRF24L01::set_com_ce(uint8_t level)
{
	_com_ce = level;

	if (level) {
		wait_us(HARDWARE_DELAY);
	}
}

void NRF24L01::set_data_rate(DataRate data_rate)
{
	uint8_t reg_rf_setup = 0;

	// read current value of RF setup register
	reg_rf_setup = spi_read_register(RegisterAddress::REG_RF_SETUP);
	// clear rf data rate value to 1 Mbps
	reg_rf_setup = (reg_rf_setup & 0xD7);
	switch(data_rate) {
		case DataRate::_250KBPS:
			reg_rf_setup |= (1 << 5);
			_data_rate = DataRate::_250KBPS;
			break;
		case DataRate::_1MBPS:
			// nothing, already updating
			_data_rate = DataRate::_1MBPS;
			break;
		case DataRate::_2MBPS:
			reg_rf_setup |= (1 << 3);
			_data_rate = DataRate::_2MBPS;
			break;
	}
	// write new data rate
	spi_write_register(RegisterAddress::REG_RF_SETUP, reg_rf_setup);
}

NRF24L01::DataRate NRF24L01::data_rate(void)
{
	uint8_t reg_rf_setup;

	reg_rf_setup = spi_read_register(RegisterAddress::REG_RF_SETUP);

	if (reg_rf_setup & 0x08) {
		_data_rate = DataRate::_2MBPS;
	} else if (reg_rf_setup & 0x20) {
		_data_rate = DataRate::_250KBPS;
	} else {
		_data_rate = DataRate::_1MBPS;
	}

	return _data_rate;
}

void NRF24L01::attach_transmitting_payload(RxAddressPipe rx_address_pipe, uint8_t *hw_addr, uint8_t payload_size)
{
	switch(rx_address_pipe) {
		case RxAddressPipe::RX_ADDR_P0:
			// set rx addr to pipe 0
			spi_write_register(RegisterAddress::REG_RX_ADDR_P0, (const char *)hw_addr, 5);
			break;
		case RxAddressPipe::RX_ADDR_P1:
			// set rx addr to pipe 1
			spi_write_register(RegisterAddress::REG_RX_ADDR_P1, (const char *)hw_addr, 5);
			break;
		case RxAddressPipe::RX_ADDR_P2:
			// set rx addr to pipe 2
			spi_write_register(RegisterAddress::REG_RX_ADDR_P2, (const char *)hw_addr, 5);
			break;
		case RxAddressPipe::RX_ADDR_P3:
			// set rx addr to pipe 3
			spi_write_register(RegisterAddress::REG_RX_ADDR_P3, (const char *)hw_addr, 5);
			break;
		case RxAddressPipe::RX_ADDR_P4:
			// set rx addr to pipe 4
			spi_write_register(RegisterAddress::REG_RX_ADDR_P4, (const char *)hw_addr, 5);
			break;
		case RxAddressPipe::RX_ADDR_P5:
			// set rx addr to pipe 5
			spi_write_register(RegisterAddress::REG_RX_ADDR_P5, (const char *)hw_addr, 5);
			break;

	}

	set_tx_address(hw_addr);

	// set payload
	set_payload_size(rx_address_pipe , payload_size);
}

void NRF24L01::attach_receive_payload(RxAddressPipe rx_address_pipe, uint8_t *hw_addr, uint8_t payload_size)
{
	// set payload
	set_payload_size(rx_address_pipe , payload_size);

	switch(rx_address_pipe) {
		case RxAddressPipe::RX_ADDR_P0:
			// set rx addr to pipe 0
			spi_write_register(RegisterAddress::REG_RX_ADDR_P0, (const char *)hw_addr, 5);
			// enable rx addr
			spi_write_register(RegisterAddress::REG_EN_RXADDR, 0x01);
			break;
		case RxAddressPipe::RX_ADDR_P1:
			// set rx addr to pipe 1
			spi_write_register(RegisterAddress::REG_RX_ADDR_P1, (const char *)hw_addr, 5);
			// enable rx addr
			spi_write_register(RegisterAddress::REG_EN_RXADDR, 0x02);
			break;
		case RxAddressPipe::RX_ADDR_P2:
			// set rx addr to pipe 2
			spi_write_register(RegisterAddress::REG_RX_ADDR_P2, (const char *)hw_addr, 5);
			// enable rx addr
			spi_write_register(RegisterAddress::REG_EN_RXADDR, 0x04);
			break;
		case RxAddressPipe::RX_ADDR_P3:
			// set rx addr to pipe 3
			spi_write_register(RegisterAddress::REG_RX_ADDR_P3, (const char *)hw_addr, 5);
			// enable rx addr
			spi_write_register(RegisterAddress::REG_EN_RXADDR, 0x08);
			break;
		case RxAddressPipe::RX_ADDR_P4:
			// set rx addr to pipe 4
			spi_write_register(RegisterAddress::REG_RX_ADDR_P4, (const char *)hw_addr, 5);
			// enable rx addr
			spi_write_register(RegisterAddress::REG_EN_RXADDR, 0x10);
			break;
		case RxAddressPipe::RX_ADDR_P5:
			// set rx addr to pipe 5
			spi_write_register(RegisterAddress::REG_RX_ADDR_P5, (const char *)hw_addr, 5);
			// enable rx addr
			spi_write_register(RegisterAddress::REG_EN_RXADDR, 0x20);
			break;
	}
}

void NRF24L01::attach_receive_address_to_pipe(RxAddressPipe rx_address_pipe, uint8_t *hw_rx_addr)
{
	switch(rx_address_pipe) {
		case RxAddressPipe::RX_ADDR_P0:
			// set rx addr to pipe 0
			spi_write_register(RegisterAddress::REG_RX_ADDR_P0, (const char *)hw_rx_addr, 5);
			// enable rx addr
			spi_write_register(RegisterAddress::REG_EN_RXADDR, 0x01);
			break;
		case RxAddressPipe::RX_ADDR_P1:
			// set rx addr to pipe 1
			spi_write_register(RegisterAddress::REG_RX_ADDR_P1, (const char *)hw_rx_addr, 5);
			// enable rx addr
			spi_write_register(RegisterAddress::REG_EN_RXADDR, 0x02);
			break;
		case RxAddressPipe::RX_ADDR_P2:
			// set rx addr to pipe 2
			spi_write_register(RegisterAddress::REG_RX_ADDR_P2, (const char *)hw_rx_addr, 5);
			// enable rx addr
			spi_write_register(RegisterAddress::REG_EN_RXADDR, 0x04);
			break;
		case RxAddressPipe::RX_ADDR_P3:
			// set rx addr to pipe 3
			spi_write_register(RegisterAddress::REG_RX_ADDR_P3, (const char *)hw_rx_addr, 5);
			// enable rx addr
			spi_write_register(RegisterAddress::REG_EN_RXADDR, 0x08);
			break;
		case RxAddressPipe::RX_ADDR_P4:
			// set rx addr to pipe 4
			spi_write_register(RegisterAddress::REG_RX_ADDR_P4, (const char *)hw_rx_addr, 5);
			// enable rx addr
			spi_write_register(RegisterAddress::REG_EN_RXADDR, 0x10);
			break;
		case RxAddressPipe::RX_ADDR_P5:
			// set rx addr to pipe 5
			spi_write_register(RegisterAddress::REG_RX_ADDR_P5, (const char *)hw_rx_addr, 5);
			// enable rx addr
			spi_write_register(RegisterAddress::REG_EN_RXADDR, 0x20);
			break;
	}
}

void NRF24L01::send_packet(const void *tx_packet, uint8_t length)
{
	set_com_ce(0);

	// manage payload length limit
	if (length > MAX_PAYLOAD_SIZE) {
		length = MAX_PAYLOAD_SIZE;
	}
	spi_write_payload((const char *)tx_packet, length);

	start_transfer();
}

void NRF24L01::start_transfer(void)
{
	//in Tx mode only
	set_com_ce(1);
	wait_us(20);
	set_com_ce(0);

}

void NRF24L01::read_packet(void *rx_packet, uint8_t length)
{
	// manage payload length limit
	if (length > MAX_PAYLOAD_SIZE) {
		length = MAX_PAYLOAD_SIZE;
	}
	spi_read_payload((char *)rx_packet, length);
}

void NRF24L01::set_rf_frequency(uint16_t rf_frequency)
{
	uint8_t channel = 0;

	if ((rf_frequency < MIN_RF_FREQUENCY) || (rf_frequency > MAX_RF_FREQUENCY)) {
		// out of the range: set default value
		_rf_frequency  = MIN_RF_FREQUENCY + 2;
	} else {
		_rf_frequency = rf_frequency;
	}

	// Frequency = 2400 + RF_CH [MHz]
	channel = _rf_frequency - MIN_RF_FREQUENCY;

	set_channel(channel);

}

uint16_t NRF24L01::rf_frequency(void)
{
	// TODO: read register ?
	// return current rf_frequency
	return _rf_frequency;
}

void NRF24L01::set_rf_output_power(RFoutputPower rf_output_power)
{
	uint8_t reg_rf_setup = 0;

	// availbale only in the Tx mode
    if (_mode == OperationMode::TRANSCEIVER) {
	    reg_rf_setup = spi_read_register(RegisterAddress::REG_RF_SETUP);
		// clear concerned bits and format new value
        reg_rf_setup = (reg_rf_setup & 0xF8);
        reg_rf_setup = (reg_rf_setup | static_cast<uint8_t>(rf_output_power));
        // set value register
        spi_write_register(RegisterAddress::REG_RF_SETUP, reg_rf_setup);
        _rf_output_power = rf_output_power;
	 }
}

NRF24L01::RFoutputPower NRF24L01::rf_output_power(void)
{
	//TODO: read register ?
	return _rf_output_power;
}

void NRF24L01::flush_rx(void)
{
	spi_single_write(static_cast<uint8_t>(RegisterOperation::OP_FLUSH_RX));
}

void NRF24L01::flush_tx(void)
{
	spi_single_write(static_cast<uint8_t>(RegisterOperation::OP_FLUSH_TX));
}

void NRF24L01::tx_address(uint8_t *tx_addr)
{
	spi_read_register(RegisterAddress::REG_TX_ADDR, tx_addr, 5);
}

void NRF24L01::rx_address(RxAddressPipe rx_address_pipe, uint8_t *rx_addr)
{
	switch(rx_address_pipe) {
			case RxAddressPipe::RX_ADDR_P0:
				spi_read_register(RegisterAddress::REG_RX_ADDR_P0, rx_addr, 5);
				break;
			case RxAddressPipe::RX_ADDR_P1:
				spi_read_register(RegisterAddress::REG_RX_ADDR_P1, rx_addr, 5);
				break;
			case RxAddressPipe::RX_ADDR_P2:
				spi_read_register(RegisterAddress::REG_RX_ADDR_P2, rx_addr, 5);
				break;
			case RxAddressPipe::RX_ADDR_P3:
				spi_read_register(RegisterAddress::REG_RX_ADDR_P3, rx_addr, 5);
				break;
			case RxAddressPipe::RX_ADDR_P4:
				spi_read_register(RegisterAddress::REG_RX_ADDR_P4, rx_addr, 5);
				break;
			case RxAddressPipe::RX_ADDR_P5:
				spi_read_register(RegisterAddress::REG_RX_ADDR_P5, rx_addr, 5);
				break;
		}
}

uint8_t NRF24L01::status_register(void)
{
	return spi_single_write(static_cast<uint8_t>(RegisterOperation::OP_NOP));
}

uint8_t NRF24L01::fifo_status_register(void)
{
	return spi_read_register(RegisterAddress::REG_FIFO_STATUS);
}

uint8_t NRF24L01::config_status_register(void)
{
	return spi_read_register(RegisterAddress::REG_CONFIG);
}

/***************************************************************************
 * transport layer
 ***************************************************************************/
void NRF24L01::spi_select(void)
{
	_com_cs = 0;
}

void NRF24L01::spi_deselect(void)
{
	_com_cs = 1;
}

void NRF24L01::spi_write_payload(const char *buffer, uint8_t length)
{
#ifdef _SPI_API_WITHOUT_CS_
	spi_select();
	_spi->write(static_cast<uint8_t>(RegisterOperation::OP_TX));
	while (length--) {
		_spi->write(*buffer++);
	}
	spi_deselect();
#else
	static char *data;
	static char resp[2];

	// create a dynamic buffer to use mbed spi API
	data = new char[length];

	// formatting data
	data[0] = static_cast<char>(RegisterAddress::OP_TX);

	for (int i = 1; i < length; i++) {
		data[i] = *buffer;
		buffer++;
	}
	spi_select();
	//TODO: ignore response?
	_spi->write(data, length, resp, sizeof(resp));
	spi_deselect();

	delete data;
#endif
}

void NRF24L01::spi_read_payload(char* buffer, uint8_t length)
{

#ifdef _SPI_API_WITHOUT_CS_
	spi_select();
	_spi->write(static_cast<uint8_t>(RegisterOperation::OP_RX));
	while (length--) {
		*buffer++ = _spi->write(static_cast<uint8_t>(RegisterOperation::OP_NOP));
	}
	spi_deselect();
#else
	static char reg;
	reg = static_cast<char>(RegisterAddress::OP_RX);
	spi_select();
	_spi->write(&reg, 1, (char *)buffer, length);
	spi_deselect();
#endif
}

void NRF24L01::spi_write_register(RegisterAddress register_address, uint8_t value)
{
#ifdef _SPI_API_WITHOUT_CS_
	spi_select();
	_spi->write((static_cast<uint8_t>(register_address) | static_cast<uint8_t>(RegisterOperation::OP_WRITE)));
	_spi->write(value);
	spi_deselect();
#else
	static char data[2];
	static char resp[2];

	// formatting data
	data[0] = (static_cast<char>(register_address) | static_cast<char>(RegisterAddress::OP_WRITE));
	data[1] = value;
	spi_select();
	//TODO: ignore response?
	_spi->write(data, sizeof(data), resp, sizeof(resp));
	spi_deselect();
#endif
}

void NRF24L01::spi_write_register(RegisterAddress register_address, const char *value, uint8_t length)
{
#ifdef _SPI_API_WITHOUT_CS_
	spi_select();
	_spi->write((static_cast<uint8_t>(register_address) | static_cast<uint8_t>(RegisterOperation::OP_WRITE)));
	while (length--) {
		//TODO: ignore response?
		_spi->write(*value++);
	}
	spi_deselect();
#else
	static char *data;
	static char resp[2];
	// create a dynamic buffer to use mbed spi API
	data = new char[length];

	// formatting data
	data[0] = (static_cast<char>(register_address) | static_cast<char>(RegisterAddress::OP_WRITE));

	for (int i = 1; i < length; i++) {
		data[i] = *value;
		value++;
	}
	spi_select();
	//TODO: ignore response?
	_spi->write(data, length, resp, sizeof(resp));
	spi_deselect();

	delete data;
#endif
}

uint8_t NRF24L01::spi_read_register(RegisterAddress register_address)
{
	static char reg;

	reg = (static_cast<char>(RegisterOperation::OP_READ) | static_cast<char>(register_address));

#ifdef _SPI_API_WITHOUT_CS_
	static uint8_t resp = 0;
	spi_select();
	_spi->write(reg);
	resp = _spi->write(static_cast<uint8_t>(RegisterOperation::OP_NOP));
	spi_deselect();
	return resp;
#else
	static char resp[2];
	spi_select();
	_spi->write(&reg, 1, resp, sizeof(resp));
	spi_deselect();
	return (uint8_t)resp[1];
#endif
}

void NRF24L01::spi_read_register(RegisterAddress register_address, uint8_t *value, uint8_t length)
{
	static char reg;

	// format register value
	reg = (static_cast<char>(RegisterOperation::OP_READ) | (static_cast<char>(register_address) & 0x1F));

#ifdef _SPI_API_WITHOUT_CS_
	spi_select();
	_spi->write(reg);
	while (length--) {
		*value++ = _spi->write(static_cast<uint8_t>(RegisterOperation::OP_NOP));
	}
	spi_deselect();
#else
	spi_select();
	// spi write sequence
	_spi->write(&reg, 1, (char *)value, length);
	spi_deselect();
#endif

}

uint8_t NRF24L01::spi_single_write(uint8_t value)
{
	uint8_t resp = 0xff;
	spi_select();
	resp = _spi->write(value);
	spi_deselect();

	return resp;
}

//void NRF24L01::spi_multiple_write(uint8_t register_value, uint8_t *resp, uint8_t length)
//{
//	spi_select();
//	_spi->write((const char *)register_value, 1, (char *)resp, length);
//	spi_deselect();
//}
