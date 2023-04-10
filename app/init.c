
#include <cmsis_os.h>
#include <init.h>
#include <radio.h>
#include <gnss.h>
#include <port.h>


/**
 * @brief 	macro to declare a static thread in FreeRTOS
 * @details	This macros make the necessary funtion calls to setup a stack and
 * 			working area for the declaration of a static FreeRTOS thread.
 *
 * @param	handle	A @p TaskHandle_t object to reference the created Thread.
 * @param	name	A name for thread.
 * @param 	func	The entry point for the thread.
 * @param 	cont	The context for the thread.
 * @param 	sz		The desired size for the thread stack.
 * @param	prio	The priority for the thread.
 */
#define INIT_THREAD_CREATE(handle, name, func, cont, sz, prio) \
	static StaticTask_t name##_buffer; \
	static StackType_t name##_stack[ sz ]; \
	handle = xTaskCreateStatic( \
			func, \
	        #name, \
			sz, \
			( void * ) cont, \
			prio, \
			name##_stack, \
			&name##_buffer)



#define RADIO_SZ	256
#define RADIO_PRIO	(6)

#define GNSS_SZ		256
#define GNSS_PRIO	(5)

static TaskHandle_t radio_handle = NULL;
static TaskHandle_t gnss_handle = NULL;


void init(void) {

#if GROUND_STATION == 0
	spi_init();

	uart_init();



	INIT_THREAD_CREATE(radio_handle, radio, radio_thread, NULL, RADIO_SZ, RADIO_PRIO);

	INIT_THREAD_CREATE(gnss_handle, gnss, gnss_thread, NULL, GNSS_SZ, GNSS_PRIO);

#else
	spi_init();


	INIT_THREAD_CREATE(radio_handle, radio, radio_gs_thread, NULL, RADIO_SZ, RADIO_PRIO);

#endif

}




