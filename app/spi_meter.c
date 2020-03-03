#include <string.h>
#include "nrfx_spim.h"
#include "app_util_platform.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "app_error.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#define NRFX_SPIM_SCK_PIN  30
#define NRFX_SPIM_MOSI_PIN 27
#define NRFX_SPIM_MISO_PIN 28
#define NRFX_SPIM_SS_PIN   29

#define SPI_INSTANCE  2                                           /**< SPI instance index. */
static const nrfx_spim_t spi = NRFX_SPIM_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */

static volatile bool spi_xfer_done;  /**< Flag used to indicate that SPI instance completed the transfer. */

#define TEST_STRING "Nordic123456789012345678901234567890\n"
static uint8_t       m_tx_buf[] = TEST_STRING;           /**< TX buffer. */
static uint8_t       m_rx_buf[sizeof(TEST_STRING) + 1];  /**< RX buffer. */
static const uint8_t m_length = sizeof(m_tx_buf);        /**< Transfer length. */

void spim_event_handler(nrfx_spim_evt_t const * p_event,
                       void *                  p_context)
{
	spi_xfer_done = true;
	NRF_LOG_INFO("Transfer completed.");
	if (m_rx_buf[0] != 0)
	{
		NRF_LOG_INFO(" Received:");
		NRF_LOG_HEXDUMP_INFO(m_rx_buf, strlen((const char *)m_rx_buf));
	}
}

void meter_thread(void * arg)
{
	UNUSED_PARAMETER(arg);

	nrfx_spim_xfer_desc_t xfer_desc = NRFX_SPIM_XFER_TRX(m_tx_buf, m_length, m_rx_buf, m_length);

	nrfx_spim_config_t spi_config = NRFX_SPIM_DEFAULT_CONFIG;
	spi_config.frequency      = NRF_SPIM_FREQ_250K;	//NRF_SPIM_FREQ_1M;
	spi_config.ss_pin         = NRFX_SPIM_SS_PIN;
	spi_config.miso_pin       = NRFX_SPIM_MISO_PIN;
	spi_config.mosi_pin       = NRFX_SPIM_MOSI_PIN;
	spi_config.sck_pin        = NRFX_SPIM_SCK_PIN;
	//    spi_config.dcx_pin        = NRFX_SPIM_DCX_PIN;
	spi_config.ss_active_high = false;
	APP_ERROR_CHECK(nrfx_spim_init(&spi, &spi_config, spim_event_handler, NULL));

	nrf_gpio_cfg_output(31);
	NRF_LOG_INFO("NRFX SPIM example started.");

	while (1)
	{
		// Reset rx buffer and transfer done flag
		memset(m_rx_buf, 0, m_length);
		spi_xfer_done = false;

		//        APP_ERROR_CHECK(nrfx_spim_xfer_dcx(&spi, &xfer_desc, 0, 15));
		APP_ERROR_CHECK(nrfx_spim_xfer(&spi, &xfer_desc, 0));
		while (!spi_xfer_done)
		{
			__WFE();
		}

		NRF_LOG_FLUSH();

		bsp_board_led_invert(BSP_BOARD_LED_0);
		nrf_gpio_pin_toggle(31);
		vTaskDelay(1000);
	}
}
