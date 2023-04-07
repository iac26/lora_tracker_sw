#include <radio.h>
#include <port.h>
#include <cmsis_os.h>



static SemaphoreHandle_t radio_isr_sem = NULL;
static StaticSemaphore_t radio_isr_sem_buffer;




error_t radio_init();
void radio_set_idle();
void radio_set_rx();
void radio_set_tx();
void radio_set_tx_power(int8_t power);
void radio_set_frequency(float freq);
void radio_set_preamble_len(uint16_t bytes);
void radio_set_modem_params(radio_config_t cfg);

void radio_isr_handler(void);



#define MAX_PACKET_LEN 255




error_t radio_init() {
	radio_isr_sem = xSemaphoreCreateBinaryStatic(&radio_isr_sem_buffer);
	port_register_radio_cb(radio_isr_handler);
	// set mode to lora
	spi_write_reg(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_SLEEP | RH_RF95_LONG_RANGE_MODE);

	osDelay(10);

	uint8_t data;
	spi_read_reg(RH_RF95_REG_01_OP_MODE, &data);
	if(data != (RH_RF95_MODE_SLEEP | RH_RF95_LONG_RANGE_MODE)) {
		return e_failure; // No device present?
	}

	spi_write_reg(RH_RF95_REG_0E_FIFO_TX_BASE_ADDR, 0);
	spi_write_reg(RH_RF95_REG_0F_FIFO_RX_BASE_ADDR, 0);


	//set idle mode
	spi_write_reg(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_STDBY);

	radio_config_t cfg = {
		.bw = BW_125KHZ,
		.cr = CR_4_8,
		.sf = 8
	};

	radio_set_modem_params(cfg);


	radio_set_preamble_len(8);

	radio_set_frequency(868);

	radio_set_tx_power(20);


	return e_success;
}



void radio_set_idle() {
	//set idle mode
	spi_write_reg(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_STDBY);
}

void radio_set_rx() {
	//set rx mode
	spi_write_reg(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_RXCONTINUOUS);
	spi_write_reg(RH_RF95_REG_40_DIO_MAPPING1, 0x00); // Interrupt on RxDone
}

void radio_set_tx() {
	//set tx mode
	spi_write_reg(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_TX);
	spi_write_reg(RH_RF95_REG_40_DIO_MAPPING1, 0x40); // Interrupt on TxDone
}


void radio_set_tx_power(int8_t power) {
	if (power > 20)
	    power = 20;
	if (power < 2)
	    power = 2;

	// For RH_RF95_PA_DAC_ENABLE, manual says '+20dBm on PA_BOOST when OutputPower=0xf'
	// RH_RF95_PA_DAC_ENABLE actually adds about 3dBm to all power levels. We will use it
	// for 8, 19 and 20dBm
	if (power > 17)
	{
		spi_write_reg(RH_RF95_REG_4D_PA_DAC, RH_RF95_PA_DAC_ENABLE);
	    power -= 3;
	}
	else
	{
		spi_write_reg(RH_RF95_REG_4D_PA_DAC, RH_RF95_PA_DAC_DISABLE);
	}

	// RFM95/96/97/98 does not have RFO pins connected to anything. Only PA_BOOST
	// pin is connected, so must use PA_BOOST
	// Pout = 2 + OutputPower (+3dBm if DAC enabled)
	spi_write_reg(RH_RF95_REG_09_PA_CONFIG, RH_RF95_PA_SELECT | (power-2));
}


void radio_set_frequency(float freq) {
	uint32_t frf = (freq * 1000000.0) / RH_RF95_FSTEP;
	spi_write_reg(RH_RF95_REG_06_FRF_MSB, (frf >> 16) & 0xff);
	spi_write_reg(RH_RF95_REG_07_FRF_MID, (frf >> 8) & 0xff);
	spi_write_reg(RH_RF95_REG_08_FRF_LSB, frf & 0xff);
}


void radio_set_preamble_len(uint16_t bytes) {
	spi_write_reg(RH_RF95_REG_20_PREAMBLE_MSB, bytes >> 8);
	spi_write_reg(RH_RF95_REG_21_PREAMBLE_LSB, bytes & 0xff);
}

void radio_set_modem_params(radio_config_t cfg) {

	uint8_t reg_1d, reg_1e, reg_26;


	switch(cfg.cr) {
	case CR_4_5:
		reg_1d = RH_RF95_CODING_RATE_4_5;
		break;
	case CR_4_6:
		reg_1d = RH_RF95_CODING_RATE_4_6;
		break;
	case CR_4_7:
		reg_1d = RH_RF95_CODING_RATE_4_7;
		break;
	case CR_4_8:
		reg_1d = RH_RF95_CODING_RATE_4_8;
		break;
	default:
		reg_1d = RH_RF95_CODING_RATE_4_5;
		break;
	}

	switch(cfg.bw) {
	case BW_7_8KHZ:
		reg_1d |= RH_RF95_BW_7_8KHZ;
		break;
	case BW_10_4KHZ:
		reg_1d |= RH_RF95_BW_10_4KHZ;
		break;
	case BW_15_6KHZ:
		reg_1d |= RH_RF95_BW_15_6KHZ;
		break;
	case BW_20_8KHZ:
		reg_1d |= RH_RF95_BW_20_8KHZ;
		break;
	case BW_31_25KHZ:
		reg_1d |= RH_RF95_BW_31_25KHZ;
		break;
	case BW_41_7KHZ:
		reg_1d |= RH_RF95_BW_41_7KHZ;
		break;
	case BW_62_5KHZ:
		reg_1d |= RH_RF95_BW_62_5KHZ;
		break;
	case BW_125KHZ:
		reg_1d |= RH_RF95_BW_125KHZ;
		break;
	case BW_250KHZ:
		reg_1d |= RH_RF95_BW_250KHZ;
		break;
	case BW_500KHZ:
		reg_1d |= RH_RF95_BW_500KHZ;
		break;
	default:
		reg_1d |= RH_RF95_BW_125KHZ;
		break;
	}

	switch(cfg.sf) {
	case 6:
		reg_1e = RH_RF95_SPREADING_FACTOR_64CPS;
		break;
	case 7:
		reg_1e = RH_RF95_SPREADING_FACTOR_128CPS;
		break;
	case 8:
		reg_1e = RH_RF95_SPREADING_FACTOR_256CPS;
		break;
	case 9:
		reg_1e = RH_RF95_SPREADING_FACTOR_512CPS;
		break;
	case 10:
		reg_1e = RH_RF95_SPREADING_FACTOR_1024CPS;
		break;
	case 11:
		reg_1e = RH_RF95_SPREADING_FACTOR_2048CPS;
		break;
	case 12:
		reg_1e = RH_RF95_SPREADING_FACTOR_4096CPS;
		break;
	default:
		reg_1e = RH_RF95_SPREADING_FACTOR_64CPS;
	}

	reg_1e |= RH_RF95_PAYLOAD_CRC_ON;


	reg_26 = RH_RF95_AGC_AUTO_ON;

	spi_write_reg(RH_RF95_REG_1D_MODEM_CONFIG1, reg_1d);
	spi_write_reg(RH_RF95_REG_1E_MODEM_CONFIG2, reg_1e);
	spi_write_reg(RH_RF95_REG_26_MODEM_CONFIG3, reg_26);



}




error_t radio_transmit(uint8_t * data, uint16_t len) {
	if (len > MAX_PACKET_LEN) {
		return e_failure;
	}

	//TODO: Wait for packet to be sent

	//TODO: Wait for clear channel

	spi_write_reg(RH_RF95_REG_0D_FIFO_ADDR_PTR, 0);
	spi_write_reg_burst(RH_RF95_REG_00_FIFO, data, len);
	spi_write_reg(RH_RF95_REG_22_PAYLOAD_LENGTH, len);


	radio_set_tx();

	//wait for finished

	if(xSemaphoreTake( radio_isr_sem, ( TickType_t ) 0xffff ) == pdTRUE ) {

		uint8_t irq_flags;
		spi_read_reg(RH_RF95_REG_12_IRQ_FLAGS, &irq_flags);
		spi_write_reg(RH_RF95_REG_12_IRQ_FLAGS, 0xff); // Clear all IRQ flags

		radio_set_idle();

	}

	return e_success;


}



void radio_isr_handler(void) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(radio_isr_sem, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}





void radio_thread(void * arg) {


	radio_init();




	for(;;) {

		static uint8_t data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

		radio_transmit(data, 10);



		osDelay(5000);

	}
}






