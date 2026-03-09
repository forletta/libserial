#include "libopencm3/stm32/rcc.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t usart;
    enum rcc_periph_clken clken;
    uint32_t gpio_port;
    uint16_t gpio_pins;
    uint32_t baudrate;
    uint32_t databits;
    uint32_t stopbits;
    uint32_t mode;
    uint32_t parity;
    uint32_t flow_control;
} serial_config_t;

void serial_setup(const serial_config_t *config);
void serial_send_blocking(uint8_t *ptr, size_t len);
uint8_t serial_recv_blocking(void);
