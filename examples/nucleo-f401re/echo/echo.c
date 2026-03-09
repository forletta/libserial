#include "serial.h"
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#define MCU_FREQ     (84000000)
#define SYSTICK_FREQ (1000)
#define LED_PORT     (GPIOA)
#define LED_PIN      (GPIO5)

static void rcc_setup(void) { rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]); }

static void gpio_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);

    gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);
}

static serial_config_t serial_config = {
    .usart = USART2,
    .clken = RCC_USART2,
    .gpio_port = GPIOA,
    .gpio_pins = GPIO3 | GPIO2,
    .baudrate = BAUDRATE, // configured via the -DBAUDRATE=<baudrate> flag
    .databits = 8,
    .stopbits = USART_STOPBITS_1,
    .mode = USART_MODE_TX_RX,
    .parity = PARITY, // configured via the -DPARITY=<parity> flag
    .flow_control = USART_FLOWCONTROL_NONE,
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

    while (1) {
        gpio_set(LED_PORT, LED_PIN);
        uint8_t input = serial_recv_blocking();

        gpio_clear(LED_PORT, LED_PIN);
        serial_send_blocking(&input, 1);
    }

    return 0;
}
