#include "serial.h"
#include "libopencm3/stm32/gpio.h"
#include "libopencm3/stm32/usart.h"

static volatile uint32_t usart = 0;

void serial_setup(const serial_config_t *config) {
    rcc_periph_clock_enable(config->clken);

    gpio_mode_setup(config->gpio_port, GPIO_MODE_AF, GPIO_PUPD_NONE, config->gpio_pins);
    gpio_set_af(config->gpio_port, GPIO_AF7, config->gpio_pins);

    usart_set_baudrate(config->usart, config->baudrate);
    usart_set_databits(config->usart, config->databits);
    usart_set_stopbits(config->usart, config->stopbits);
    usart_set_mode(config->usart, config->mode);
    usart_set_parity(config->usart, config->parity);
    usart_set_flow_control(config->usart, config->flow_control);

    usart_enable(config->usart);
    usart = config->usart;
}

void serial_send_blocking(uint8_t *ptr, size_t len) {
    for (size_t i = 0; i < len; i++) {
        usart_send_blocking(usart, ptr[i]);
    }
}

uint8_t serial_recv_blocking(void) { return usart_recv_blocking(usart); }
