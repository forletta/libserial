#include "serial.h"
#include "cobs.h"
#include "crique.h"
#include "libopencm3/cm3/nvic.h"
#include "libopencm3/stm32/dma.h"
#include "libopencm3/stm32/gpio.h"
#include "libopencm3/stm32/usart.h"
#include <stdbool.h>

static volatile uint32_t usart = 0;
static volatile serial_dma_config_t *dma_config = NULL;

static volatile bool sent = false;

static volatile bool chunk_ready = false;
static volatile size_t chunk_head = 0;
static volatile size_t chunk_tail = 0;

static volatile cobs_decode_inc_ctx_t cobs_decode_inc_ctx = {
    .state = COBS_DECODE_READ_CODE,
};

static volatile crique_t packet_queue = {};

void serial_setup(const serial_config_t *config) {
    rcc_periph_clock_enable(config->gpio_clken);
    rcc_periph_clock_enable(config->usart_clken);

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
    dma_config = config->dma_config;

    if (!dma_config)
        return;

    crique((crique_t *)&packet_queue, dma_config->packet_buf, dma_config->packet_buf_len);

    rcc_periph_clock_enable(dma_config->dma_clken);

    nvic_set_priority(dma_config->tx_irq, 0);
    nvic_enable_irq(dma_config->tx_irq);

    nvic_set_priority(dma_config->rx_irq, 0);
    nvic_enable_irq(dma_config->rx_irq);

    usart_enable_idle_interrupt(config->usart);
    nvic_set_priority(dma_config->usart_irq, 0);
    nvic_enable_irq(dma_config->usart_irq);
}

void serial_send_blocking(uint8_t *ptr, size_t len) {
    for (size_t i = 0; i < len; i++) {
        usart_send_blocking(usart, ptr[i]);
    }
}

uint8_t serial_recv_blocking(void) { return usart_recv_blocking(usart); }

void serial_send_nonblocking(uint8_t *ptr, size_t len) {
    // DMA STREAM CONTROL REGISTER:
    // Stream reset:
    dma_stream_reset(dma_config->dma, dma_config->tx_stream);
    // Interupts:
    dma_enable_transfer_complete_interrupt(dma_config->dma, dma_config->tx_stream);
    // Transfer:
    dma_set_transfer_mode(dma_config->dma, dma_config->tx_stream, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
    dma_enable_direct_mode(dma_config->dma, dma_config->tx_stream);
    dma_enable_memory_increment_mode(dma_config->dma, dma_config->tx_stream);
    // Sizing:
    dma_set_peripheral_size(dma_config->dma, dma_config->tx_stream, DMA_SxCR_PSIZE_8BIT);
    dma_set_memory_size(dma_config->dma, dma_config->tx_stream, DMA_SxCR_MSIZE_8BIT);
    // Priority:
    dma_set_priority(dma_config->dma, dma_config->tx_stream, DMA_SxCR_PL_VERY_HIGH);
    // Channel select:
    dma_channel_select(dma_config->dma, dma_config->tx_stream, dma_config->tx_chsel);

    // DMA STREAM PERIPHERAL ADDRESS, MEMORY ADRESS, AND NUMBER 0F DATA REGISTERS:
    dma_set_peripheral_address(dma_config->dma, dma_config->tx_stream, dma_config->peripheral_address);
    dma_set_memory_address(dma_config->dma, dma_config->tx_stream, (uint32_t)ptr);
    dma_set_number_of_data(dma_config->dma, dma_config->tx_stream, len);

    // ENABLE:
    dma_enable_stream(dma_config->dma, dma_config->tx_stream);
    usart_enable_tx_dma(usart);

    sent = false;
}

bool serial_sent(void) { return sent; }

void serial_send_nonblocking_isr(void) {
    if (dma_config && dma_get_interrupt_flag(dma_config->dma, dma_config->tx_stream, DMA_TCIF)) {
        dma_clear_interrupt_flags(dma_config->dma, dma_config->tx_stream, DMA_TCIF);
        dma_disable_transfer_complete_interrupt(dma_config->dma, dma_config->tx_stream);

        usart_disable_tx_dma(usart);
        dma_disable_stream(dma_config->dma, dma_config->tx_stream);

        sent = true;
    }
}

static void update_chunk(void) {
    uint32_t remaining = dma_get_number_of_data(dma_config->dma, dma_config->rx_stream);
    uint32_t dma_tail = dma_config->rx_buffer_len - remaining;

    if (dma_tail == 0) // On TC NDTR is most likely wrapped to rx_buffer_len rather than 0.
        dma_tail = dma_config->rx_buffer_len;

    if (chunk_tail == dma_tail) // Chunk already proccessed.
        return;

    chunk_head = dma_tail < chunk_tail ? 0 : chunk_tail;
    chunk_tail = dma_tail;

    chunk_ready = true;
}

void serial_recv_nonblocking_start() {
    // DMA STREAM CONTROL REGISTER:
    // Stream reset:
    dma_stream_reset(dma_config->dma, dma_config->rx_stream);
    // Interupts:
    dma_enable_half_transfer_interrupt(dma_config->dma, dma_config->rx_stream);
    dma_enable_transfer_complete_interrupt(dma_config->dma, dma_config->rx_stream);
    // Transfer:
    dma_set_transfer_mode(dma_config->dma, dma_config->rx_stream, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
    dma_enable_circular_mode(dma_config->dma, dma_config->rx_stream);
    dma_enable_memory_increment_mode(dma_config->dma, dma_config->rx_stream);
    // Sizing:
    dma_set_peripheral_size(dma_config->dma, dma_config->rx_stream, DMA_SxCR_PSIZE_8BIT);
    dma_set_memory_size(dma_config->dma, dma_config->rx_stream, DMA_SxCR_MSIZE_8BIT);
    // Priority:
    dma_set_priority(dma_config->dma, dma_config->rx_stream, DMA_SxCR_PL_VERY_HIGH);
    // Channel select:
    dma_channel_select(dma_config->dma, dma_config->rx_stream, dma_config->rx_chsel);

    // DMA STREAM PERIPHERAL ADDRESS, MEMORY ADRESS, AND NUMBER 0F DATA REGISTERS:
    dma_set_peripheral_address(dma_config->dma, dma_config->rx_stream, dma_config->peripheral_address);
    dma_set_memory_address(dma_config->dma, dma_config->rx_stream, (uint32_t)dma_config->rx_buffer);
    dma_set_number_of_data(dma_config->dma, dma_config->rx_stream, dma_config->rx_buffer_len);

    // ENABLE:
    dma_enable_stream(dma_config->dma, dma_config->rx_stream);
    usart_enable_rx_dma(usart);
}

void serial_recv_nonblocking_isr(void) {
    // Half-Transfer & Transfer-Complete:
    if (dma_get_interrupt_flag(dma_config->dma, dma_config->rx_stream, DMA_HTIF | DMA_TCIF)) {
        dma_clear_interrupt_flags(dma_config->dma, dma_config->rx_stream, DMA_HTIF | DMA_TCIF);
        update_chunk();
    }

    // USART Idle:
    if (usart_get_flag(usart, USART_SR_IDLE)) {
        (void)USART_DR(USART2); // Clear SR_IDLE Flag
        update_chunk();
    }
}

bool serial_chunk_ready(void) {
    if (!chunk_ready)
        return false;

    chunk_ready = false;
    return true;
}

uint8_t *serial_chunk_ptr(void) { return &dma_config->rx_buffer[chunk_head]; }

size_t serial_chunk_len(void) { return chunk_tail - chunk_head; }

void serial_send_packet_blocking(uint8_t *packet_buf, size_t packet_len, uint8_t *encode_buf, size_t encode_buf_len) {
    size_t encoded_len;

    cobs_encode(packet_buf, packet_len, encode_buf, encode_buf_len, &encoded_len);

    serial_send_blocking(encode_buf, encoded_len);
}

size_t serial_recv_packet_blocking(uint8_t *packet_buf, size_t packet_buf_len) {
    cobs_decode_inc_ctx_t ctx;
    cobs_decode_inc_begin(&ctx);

    bool completed = false;
    size_t decoded_len = 0;

    while (!completed) {
        uint8_t byte = serial_recv_blocking();

        size_t chunck_decoded_len, chunk_consumed_len;

        cobs_decode_inc_args_t args = {
            .dec_dst = packet_buf + decoded_len,
            .dec_dst_max = packet_buf_len - decoded_len,
            .enc_src = &byte,
            .enc_src_max = 1,
        };

        // TODO: Make these packet functions fallible
        cobs_ret_t r = cobs_decode_inc(&ctx, &args, &chunk_consumed_len, &chunck_decoded_len, &completed);

        decoded_len += chunck_decoded_len;
    }

    return decoded_len;
}

void serial_send_packet_nonblocking(uint8_t *packet_buf, size_t packet_len, uint8_t *encode_buf,
                                    size_t encode_buf_len) {
    size_t encoded_len;

    cobs_encode(packet_buf, packet_len, encode_buf, encode_buf_len, &encoded_len);

    serial_send_nonblocking(encode_buf, encoded_len);
}

bool serial_packet_ready(void) {
    // discard volatile
    crique_t *cq = (crique_t *)&packet_queue;
    cobs_decode_inc_ctx_t *ctx = (cobs_decode_inc_ctx_t *)&cobs_decode_inc_ctx;

    if (crique_peek(cq) != NULL)
        return true;

    if (!serial_chunk_ready())
        return false;

    uint8_t *enc_src = serial_chunk_ptr();
    size_t enc_src_len = serial_chunk_len();

    size_t total_src_consumed = 0;

    while (total_src_consumed < enc_src_len) {
        bool complete = false;

        while (!complete) {
            if (total_src_consumed == enc_src_len)
                return false;

            uint8_t *enc_src_head = enc_src + total_src_consumed;
            size_t enc_src_remaining = enc_src_len - total_src_consumed;

            cobs_decode_inc_args_t args = {
                .enc_src = enc_src_head,
                .enc_src_max = enc_src_remaining,
                .dec_dst = crique_write_ensure_remaining(cq, 4),
                .dec_dst_max = crique_write_remaining(cq),
            };

            size_t src_consumed = 0, dst_written = 0;
            cobs_decode_inc(ctx, &args, &src_consumed, &dst_written, &complete);

            total_src_consumed += src_consumed;
            crique_written(cq, dst_written);
        }

        total_src_consumed += 1; // 0 seperator

        crique_push(cq);
        cobs_decode_inc_begin(ctx);
    }

    return true;
}
uint8_t *serial_packet_ptr(void) { return crique_peek((crique_t *)&packet_queue); }

size_t serial_packet_len(void) { return crique_peek_len((crique_t *)&packet_queue); }

void serial_packet_pop(void) { crique_pop((crique_t *)&packet_queue); }
