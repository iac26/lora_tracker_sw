
#include <port.h>
#include <cmsis_os.h>
#include <spi.h>
#include <usart.h>
#include <main.h>
#include <string.h>



/* SPI */

static SemaphoreHandle_t spi_sem = NULL;
static StaticSemaphore_t spi_sem_buffer;



void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if(hspi->Instance == hspi1.Instance) {
		xSemaphoreGiveFromISR(spi_sem, &xHigherPriorityTaskWoken);
	}

	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

}


void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if(hspi->Instance == hspi1.Instance) {
		xSemaphoreGiveFromISR(spi_sem, &xHigherPriorityTaskWoken);
	}

	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if(hspi->Instance == hspi1.Instance) {
		xSemaphoreGiveFromISR(spi_sem, &xHigherPriorityTaskWoken);
	}

	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

}



void spi_init() {
	spi_sem = xSemaphoreCreateBinaryStatic(&spi_sem_buffer);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

error_t spi_write_reg(uint8_t addr, uint8_t data) {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	static uint8_t tx_data[2];
	tx_data[0] = addr | 0x80; //write mode
	tx_data[1] = data;
	HAL_SPI_Transmit_IT(&hspi1, tx_data, 2);
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
	HAL_SPI_Transmit_IT(&hspi1, &addr, 1);
	//wait for tx done
	if(xSemaphoreTake( spi_sem, ( TickType_t ) 10 ) == pdTRUE ) {
		HAL_SPI_Receive_IT(&hspi1, data, 1);
	} else {
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
		return e_failure;
	}
	//wait for rx done
	if(xSemaphoreTake( spi_sem, ( TickType_t ) 10 ) == pdTRUE ) {
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
	if(len <= MAX_SPI_PACKET) {
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
		uint8_t wr_addr = addr | 0x80; //write mode
		HAL_SPI_Transmit_IT(&hspi1, &wr_addr, 1);
		//wait for tx addr done
		if(xSemaphoreTake( spi_sem, ( TickType_t ) 10 ) == pdTRUE ) {
			HAL_SPI_Transmit_DMA(&hspi1, data, len);
		} else {
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
			return e_failure;
		}
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
	if(len <= MAX_SPI_PACKET) {
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
		HAL_SPI_Transmit_IT(&hspi1, &addr, 1);
		//wait for tx done
		if(xSemaphoreTake( spi_sem, ( TickType_t ) 10 ) == pdTRUE ) {
			HAL_SPI_Receive_DMA(&hspi1, data, len);
		} else {
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
			return e_failure;
		}
		//wait for rx done
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




/* UART */
/*
 * DMA ping pong reception
 */


static SemaphoreHandle_t uart_sem = NULL;
static StaticSemaphore_t uart_sem_buffer;

#define UART_PING_PONG_SIZE 256


static uint8_t rx_buffer[256];
static uint8_t rx_offset;




void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if(huart->Instance == huart2.Instance) {
		rx_offset = 0;
		xSemaphoreGiveFromISR(uart_sem, &xHigherPriorityTaskWoken);
	}
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if(huart->Instance == huart2.Instance) {
		rx_offset = UART_PING_PONG_SIZE/2;
		xSemaphoreGiveFromISR(uart_sem, &xHigherPriorityTaskWoken);
	}
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}




void uart_init(void) {
	uart_sem = xSemaphoreCreateBinaryStatic(&uart_sem_buffer);

	//DMA is configured circular
	HAL_UART_Receive_DMA(&huart2, rx_buffer, UART_PING_PONG_SIZE);
}


error_t uart_wait(uint16_t timeout) {
	if(xSemaphoreTake( uart_sem, ( TickType_t ) timeout ) == pdTRUE ) {
		return e_success;
	} else {
		return e_failure;
	}
}

uint16_t uart_get_buffer(uint8_t ** buffer) {
	*buffer = rx_buffer + rx_offset;
	return UART_PING_PONG_SIZE/2;
}









/* EXTI */

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


