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

	enum class RegisterOperation : uint8_t {
		// Operations
		OP_READ             = 0x00,
		OP_WRITE            = 0x20,
		OP_RX               = 0x61,
		OP_TX               = 0xa0,
		OP_NOP              = 0xff,
		OP_FLUSH_TX			= 0xE1,
		OP_FLUSH_RX			= 0xE2,

        // in RX mode, write payload ACK to pipe P
        // use (val | pipe) to set pipe (between 0b000 and 0b101)
        OP_ACK_TX_P0      = 0xa8
	};

	enum class RegisterAddress : uint8_t {
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

	enum class OperationMode : uint8_t {
		TRANSCEIVER			= 0x00,
		RECEIVER			= 0x01,
		POWER_DOWN			= 0x02
	};

	enum class DataRate : uint16_t {
		_250KBPS			= 250,
		_1MBPS				= 1000,
		_2MBPS				= 2000
	};

	enum class RxAddressPipe : uint8_t {
		RX_ADDR_P0			= 0,
		RX_ADDR_P1			= 1,
		RX_ADDR_P2			= 2,
		RX_ADDR_P3			= 3,
		RX_ADDR_P4			= 4,
		RX_ADDR_P5			= 5
	};

	enum class RFoutputPower : uint8_t {
		_18dBm				= 18,
		_12dBm				= 12,
		_6dBm				= 6,
		_0dBm				= 0
 	};

	enum class CRCwidth : uint8_t {
		NONE				= 0,
		_8bits				= 8,
		_16bits				= 16
	};

	enum class InterruptMode : uint8_t {
		NONE				= 0,
		RX_ONLY				= 1,
		TX_ONLY				= 2,
		RX_TX				= 3,
		RETRANSMIT			= 4,
		TX_RETRANSMIT		= 5
	};

	NRF24L01(SPI *spi, PinName com_ce, PinName irq);

	NRF24L01(SPI *spi, PinName com_cs, PinName com_ce, PinName irq);

	void initialize(OperationMode mode, DataRate data_rate, uint16_t rf_frequency);

	void attach(Callback<void()> func);

	void clear_interrupt_flags(void);

	void set_interrupt(InterruptMode interrupt_mode);

	void start_listening(void);

	void stop_listening(void);

	void set_tx_address(uint8_t *tx_addr);

	void set_crc(CRCwidth crc_width);

	void power_up(void);

	void power_down(void);

	void set_mode(OperationMode mode);

	OperationMode mode(void);

	void set_power_up_and_mode(OperationMode mode);

	void set_auto_acknowledgement(bool enable);

	void set_auto_acknowledgement(uint8_t pipe, bool enable);

    void enable_dynamic_payload(bool enable);

    void enable_payload_ack_mode(bool enable);

	void set_payload_size(RxAddressPipe rx_addr_pipe, uint8_t payload_size);

	uint8_t payload_size(void);

	void set_channel(uint8_t channel);

	void set_com_ce(uint8_t level);

	void set_data_rate(DataRate data_rate);

	DataRate data_rate(void);

	void attach_receive_payload(RxAddressPipe rx_address_pipe, uint8_t *hw_addr, uint8_t payload_size);

	void attach_transmitting_payload(RxAddressPipe rx_address_pipe, uint8_t *hw_addr, uint8_t payload_size);

	void attach_receive_address_to_pipe(RxAddressPipe rx_address_pipe, uint8_t *hw_rx_addr);

	void send_packet(const void *buffer, uint8_t length);

	void start_transfer(void);

	void read_packet(void* buffer, uint8_t length);

	void set_rf_frequency(uint16_t rf_frequency);

	uint16_t rf_frequency(void);

	void set_rf_output_power(RFoutputPower rf_output_power);

	RFoutputPower rf_output_power(void);

	void load_auto_ack_payload(const char *buffer, uint8_t length, RxAddressPipe pipe);

	void flush_rx(void);

	void flush_tx(void);

	void tx_address(uint8_t *tx_addr);

	void rx_address(RxAddressPipe rx_address_pipe, uint8_t *rx_addr);

	uint8_t status_register(void);

	uint8_t fifo_status_register(void);

	uint8_t config_status_register(void);

private:
	SPI *_spi;
	DigitalOut _com_cs;
	DigitalOut _com_ce;
	InterruptIn _irq;
	uint16_t _rf_frequency;
	uint8_t _payload_size;
	OperationMode _mode;
	DataRate _data_rate;
	RFoutputPower _rf_output_power;

	void spi_select(void);

	void spi_deselect(void);

	void spi_write_payload(const char *buffer, uint8_t length);

    void spi_write_ack_payload(const char *buffer, uint8_t length, uint8_t pipe);

	void spi_read_payload(char* buffer, uint8_t length);

	void spi_write_register(RegisterAddress register_address, uint8_t value);

	void spi_write_register(RegisterAddress register_address, const char *value, uint8_t length);

	uint8_t spi_read_register(RegisterAddress register_address);

	void spi_read_register(RegisterAddress register_address, uint8_t *value, uint8_t length);

	uint8_t spi_single_write(uint8_t value);

};


#endif // CATIE_NRF24L01_H_

