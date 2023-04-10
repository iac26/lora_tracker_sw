#include <radio.h>
#include <port.h>
#include <main.h>
#include <cmsis_os.h>
#include <stdlib.h>
#include <gnss.h>
#include <adc.h>
#include <stdio.h>
#include <init.h>
#include <math.h>



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
		.bw = BW_250KHZ,
		.cr = CR_4_8,
		.sf = 8
	};

	radio_set_modem_params(cfg);


	radio_set_preamble_len(8);

	radio_set_frequency(868);

	radio_set_tx_power(13);


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

error_t radio_receive(uint8_t * data, uint16_t * plen, uint16_t timeout) {
	if (*plen > MAX_PACKET_LEN) {
		return e_failure;
	}

	radio_set_rx();

	if(xSemaphoreTake( radio_isr_sem, ( TickType_t ) timeout ) == pdTRUE ) {

		uint8_t irq_flags;
		spi_read_reg(RH_RF95_REG_12_IRQ_FLAGS, &irq_flags);


		spi_write_reg(RH_RF95_REG_12_IRQ_FLAGS, 0xff); // Clear all IRQ flags

		if((irq_flags & (RH_RF95_RX_TIMEOUT | RH_RF95_PAYLOAD_CRC_ERROR))) {
			//bad packet
			radio_set_idle();
			return e_failure;
		} else if(irq_flags & RH_RF95_RX_DONE) {
			//good packet
			uint8_t packet_len;
			spi_read_reg(RH_RF95_REG_13_RX_NB_BYTES, &packet_len);

			// Reset the fifo read ptr to the beginning of the packet
			uint8_t current_fifo_address;
			spi_read_reg(RH_RF95_REG_10_FIFO_RX_CURRENT_ADDR, &current_fifo_address);
			spi_write_reg(RH_RF95_REG_0D_FIFO_ADDR_PTR, current_fifo_address);
			if(packet_len > *plen) {
				//packet too long
				radio_set_idle();
				return e_failure;
			}
			spi_read_reg_burst(RH_RF95_REG_00_FIFO, data, packet_len);
			*plen = packet_len;
			radio_set_idle();
			return e_success;
		}

	} else {
		radio_set_idle();
		return e_timeout;
	}
	radio_set_idle();
	return e_failure;
}



void radio_isr_handler(void) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(radio_isr_sem, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}




#define CALLSIGN ('R' << 0 | 'F' << 8 | 'B' << 16 | 'G' << 24)

typedef struct radio_packet {
	uint32_t callsign; //RFGB
	uint32_t packet_id; // sourceID + PacketID
	uint8_t hop_count;
	uint8_t bat_level;
	uint16_t padding;
	float longitude;
	float latitude;
	float hdop;
	float altitude;
	float bearing;
	float velocity;
}radio_packet_t;



#define PACKET_HISTORY 100

static struct tracker_data {
	uint8_t tracker_id;
	uint32_t packet_count;
	uint32_t packet_list[PACKET_HISTORY];
	uint32_t packet_pointer;

} tracker_data = {0};



void radio_store_packet(uint32_t packet_id) {
	tracker_data.packet_list[tracker_data.packet_pointer++] = packet_id; //store packet in database
	if(tracker_data.packet_pointer >= PACKET_HISTORY) {
		tracker_data.packet_pointer = 0;
	}
}

#if GROUND_STATION == 0

static uint32_t bat_level;

void radio_thread(void * arg) {


	radio_init();

	//read hardware board ID
	tracker_data.tracker_id = GPIOB->IDR & 0x0F;

	srand(tracker_data.tracker_id);

	for(;;) {

		static radio_packet_t tx_packet = {
				.callsign = CALLSIGN,
				.hop_count = 0
		};

		gnss_data_t gnss_data = gnss_get_data();

		tx_packet.longitude = gnss_data.longitude;
		tx_packet.latitude = gnss_data.latitude;
		tx_packet.hdop = gnss_data.hdop;
		tx_packet.velocity = gnss_data.speed;
		tx_packet.bat_level = (uint32_t)bat_level * 60 / 4096;

		tx_packet.packet_id = tracker_data.tracker_id << 24 | tracker_data.packet_count++;

		radio_store_packet(tx_packet.packet_id);

		radio_transmit((uint8_t *) &tx_packet, sizeof(radio_packet_t));

		 //timer between 2 and 5 minutes
		int32_t rx_timer = (int64_t)rand()*18000 / RAND_MAX + 12000;
		HAL_ADC_Start_DMA(&hadc, &bat_level, 1);
		while(rx_timer > 0) {
			uint32_t time = HAL_GetTick();
			static radio_packet_t rx_packet;
			uint16_t size = sizeof(radio_packet_t);
			if(radio_receive((uint8_t *)&rx_packet, &size, rx_timer) == e_success) {
				//packet received
				if(rx_packet.callsign == CALLSIGN) {
					//valid packet
					uint8_t known_packet = 0;
					//check if packet is known
					for (uint32_t i = 0; i < PACKET_HISTORY; i++) {
						if(rx_packet.packet_id == tracker_data.packet_list[i]) {
							//known packet -> discard
							known_packet = 1;
							break;
						}
					}
					if(known_packet == 0) {
						//unknown packet -> repeat
						rx_packet.hop_count += 1;
						radio_store_packet(rx_packet.packet_id);
						osDelay(rx_timer/10000); //wait 100ms
						radio_transmit((uint8_t *)&rx_packet, sizeof(radio_packet_t));
					}
				}
			} else {
				//some other condition (stray packet, timeout, error)
			}
			time = HAL_GetTick() - time;
			rx_timer -= time;
		}
	}
}

#else

/*
	uint32_t callsign; //RFGB
	uint32_t packet_id; // sourceID + PacketID
	uint8_t hop_count;
	uint8_t bat_level;
	uint16_t padding;
	float longitude;
	float latitude;
	float hdop;
	float altitude;
	float bearing;
	float velocity;
*/

/* GS stuff */

void reverse(char* str, int len)
{
    int i = 0, j = len - 1, temp;
    while (i < j) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}


int intToStr(int x, char str[], int d)
{
    int i = 0;
    while (x) {
        str[i++] = (x % 10) + '0';
        x = x / 10;
    }
    while (i < d)
        str[i++] = '0';

    reverse(str, i);
    str[i] = '\0';
    return i;
}


uint16_t ftoa(float n, char* res, int afterpoint)
{
    int ipart = (int)n;
    float fpart = n - (float)ipart;
    int i = intToStr(ipart, res, 0);

    if (afterpoint != 0) {
        res[i] = '.';

        fpart = fpart * pow(10, afterpoint);

        i += intToStr((int)fpart, res + i + 1, afterpoint);
    }

    return i;
}

void radio_gs_thread(void * arg) {
	radio_init();

	static char msg1[64];

	uint16_t len1 = sprintf(msg1, "#BS Tracker GS, Iacopo Sprenger\r\n");
	lpuart_send(msg1, len1);

	for(;;) {
		static radio_packet_t rx_packet;
		uint16_t size = sizeof(radio_packet_t);
		if(radio_receive((uint8_t *)&rx_packet, &size, 0xffff) == e_success) {
				//packet received
				if(rx_packet.callsign == CALLSIGN) {
					static char msg[128];
					static char num[32];
					uint16_t len = sprintf(msg, "$p,%ld,%ld,%d,%d,",
							rx_packet.packet_id >> 24, rx_packet.packet_id&0xffffff, rx_packet.hop_count, rx_packet.bat_level);
					lpuart_send(msg, len);
					ftoa(rx_packet.latitude, num, 6);
					len = sprintf(msg, "%s,", num);
					lpuart_send(msg, len);
					ftoa(rx_packet.longitude, num, 6);
					len = sprintf(msg, "%s,", num);
					lpuart_send(msg, len);
					ftoa(rx_packet.hdop, num, 3);
					len = sprintf(msg, "%s,", num);
					lpuart_send(msg, len);
					ftoa(rx_packet.altitude, num, 3);
					len = sprintf(msg, "%s,", num);
					lpuart_send(msg, len);
					ftoa(rx_packet.bearing, num, 3);
					len = sprintf(msg, "%s,", num);
					lpuart_send(msg, len);
					ftoa(rx_packet.velocity, num, 3);
					len = sprintf(msg, "%s,\r\n", num);
					lpuart_send(msg, len);


			}
		}
	}

}

#endif



