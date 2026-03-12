#include "libopencm3/stm32/rcc.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t dma;
    uint32_t peripheral_address;
    enum rcc_periph_clken clken;
    uint8_t usart_irq;
    uint8_t tx_stream;
    uint32_t tx_chsel;
    uint8_t tx_irq;
    uint8_t rx_stream;
    uint32_t rx_chsel;
    uint8_t rx_irq;
    uint8_t *rx_buffer;
    size_t rx_buffer_len;
} serial_dma_config_t;

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
    serial_dma_config_t *dma_config;
} serial_config_t;

void serial_setup(const serial_config_t *config);

void serial_send_blocking(uint8_t *ptr, size_t len);
uint8_t serial_recv_blocking(void);

void serial_send_nonblocking(uint8_t *ptr, size_t len);
void serial_send_nonblocking_isr(void);

void serial_recv_nonblocking_start(void);
void serial_recv_nonblocking_isr(void);

bool serial_chunk_ready(void);
uint8_t *serial_chunk_ptr(void);
size_t serial_chunk_len(void);
