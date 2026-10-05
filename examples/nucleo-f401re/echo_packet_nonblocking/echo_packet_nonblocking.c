#include "serial.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <stddef.h>

#define MCU_FREQ     (84000000)
#define SYSTICK_FREQ (1000)
#define LED_PORT     (GPIOA)
#define LED_PIN      (GPIO5)

#define PACKET_BUF_LEN (2048)

static volatile uint8_t rx_buffer[RX_BUFFER_LEN];
static volatile uint8_t packet_buf[PACKET_BUF_LEN];

static void rcc_setup(void) { rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]); }

static void gpio_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);

    gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);
}

static serial_dma_config_t serial_dma_config = {
    .dma = DMA1,
    .peripheral_address = (uint32_t)&USART2_DR,
    .dma_clken = RCC_DMA1,
    .usart_irq = NVIC_USART2_IRQ,
    .tx_stream = DMA_STREAM6,
    .tx_chsel = DMA_SxCR_CHSEL_4,
    .tx_irq = NVIC_DMA1_STREAM6_IRQ,
    .rx_stream = DMA_STREAM5,
    .rx_chsel = DMA_SxCR_CHSEL_4,
    .rx_irq = NVIC_DMA1_STREAM5_IRQ,
    .rx_buffer = (uint8_t *)rx_buffer,
    .rx_buffer_len = RX_BUFFER_LEN,
    .packet_buf = (uint8_t *)packet_buf,
    .packet_buf_len = PACKET_BUF_LEN,
};

static serial_config_t serial_config = {
    .usart = USART2,
    .usart_clken = RCC_USART2,
    .gpio_clken = RCC_GPIOA,
    .gpio_port = GPIOA,
    .gpio_pins = GPIO3 | GPIO2,
    .baudrate = BAUDRATE, // configured via the -DBAUDRATE=<baudrate> flag
    .databits = 8,
    .stopbits = USART_STOPBITS_1,
    .mode = USART_MODE_TX_RX,
    .parity = PARITY, // configured via the -DPARITY=<parity> flag
    .flow_control = USART_FLOWCONTROL_NONE,
    .dma_config = &serial_dma_config,
};

int main(void) {
    rcc_setup();
    gpio_setup();

    switch (PARITY) {
    case USART_PARITY_EVEN:
    case USART_PARITY_ODD:
        serial_config.databits = 9;
    default:;
    }

    serial_setup(&serial_config);

    uint8_t encode_buf[SERIAL_PACKET_ENCODE_SIZE(PACKET_BUF_LEN)];

    serial_recv_nonblocking_start();

    while (1) {
        if (serial_packet_ready()) {
            uint8_t *packet;

            while ((packet = serial_packet_ptr()) != NULL) {
                serial_send_packet_nonblocking(packet, serial_packet_len(), encode_buf, sizeof(encode_buf));

                serial_packet_pop();

                while (!serial_sent())
                    ;
            }
        }
    }

    return 0;
}

void dma1_stream6_isr(void) { serial_send_nonblocking_isr(); }

void dma1_stream5_isr(void) { serial_recv_nonblocking_isr(); }

void usart2_isr(void) { serial_recv_nonblocking_isr(); }
