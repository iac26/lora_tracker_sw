
#include <port.h>
#include <cmsis_os.h>
#include <spi.h>
#include <main.h>
#include <string.h>


static SemaphoreHandle_t spi_sem = NULL;
static StaticSemaphore_t spi_sem_buffer;



void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if(hspi->Instance == hspi1.Instance) {
		xSemaphoreGiveFromISR(spi_sem, &xHigherPriorityTaskWoken);
	}

	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

}


static void (*radio_cb)(void) = NULL;


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if(GPIO_Pin == GPIO_PIN_8) {
		if(radio_cb) {
			radio_cb();
		}
	}
}


void port_register_radio_cb(void (*cb)(void)) {
	radio_cb = cb;
}



void spi_init() {
	spi_sem = xSemaphoreCreateBinaryStatic(&spi_sem_buffer);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

error_t spi_write_reg(uint8_t addr, uint8_t data) {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	static uint8_t tx_data[2];
	static uint8_t rx_data[2];
	tx_data[0] = addr | 0x80; //write mode
	tx_data[1] = data;
	HAL_SPI_TransmitReceive_IT(&hspi1, tx_data, rx_data, 2);
	//wait for done
	if(xSemaphoreTake( spi_sem, ( TickType_t ) 10 ) == pdTRUE ) {
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
		return e_success;
	} else {
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
		return e_failure;
	}
}


error_t spi_read_reg(uint8_t addr, uint8_t * data) {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	static uint8_t tx_data[2];
	static uint8_t rx_data[2];
	tx_data[0] = addr;
	tx_data[1] = 0x00;
	HAL_SPI_TransmitReceive_IT(&hspi1, tx_data, rx_data, 2);
	//wait for done
	if(xSemaphoreTake( spi_sem, ( TickType_t ) 10 ) == pdTRUE ) {
		*data = rx_data[1];
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
		return e_success;
	} else {
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
		return e_failure;
	}
}

/*
 * Max data size 4 bytes
 */
error_t spi_write_reg_burst(uint8_t addr, uint8_t * data, uint8_t len) {
	static uint8_t tx_data[MAX_SPI_PACKET+1];
	static uint8_t rx_data[MAX_SPI_PACKET+1];
	tx_data[0] = addr | 0x80; //write mode
	if(len <= MAX_SPI_PACKET) {
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
		memcpy(tx_data+1, data, len);
		HAL_SPI_TransmitReceive_IT(&hspi1, tx_data, rx_data, len);
		//wait for done
		if(xSemaphoreTake( spi_sem, ( TickType_t ) 10 ) == pdTRUE ) {
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
			return e_success;
		} else {
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
			return e_failure;
		}
	} else {
		return e_failure;
	}
}


error_t spi_read_reg_burst(uint8_t addr, uint8_t * data, uint8_t len) {
	static uint8_t tx_data[MAX_SPI_PACKET+1];
	static uint8_t rx_data[MAX_SPI_PACKET+1];
	tx_data[0] = addr;
	if(len <= MAX_SPI_PACKET) {
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
		HAL_SPI_TransmitReceive_IT(&hspi1, tx_data, rx_data, len);
		//wait for done
		if(xSemaphoreTake( spi_sem, ( TickType_t ) 10 ) == pdTRUE ) {
			memcpy(data, rx_data+1, len);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
			return e_success;
		} else {
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
			return e_failure;
		}
	} else {
		return e_failure;
	}
}
