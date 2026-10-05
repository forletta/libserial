import asyncio
import os

import pytest
from cobs import cobs

RX_BUFFER_LEN = int(os.getenv("DEFAULT_RX_BUFFER_LEN", "128"))


def encoded_size_to_decoded_size(encoded_size):
    constant_overhead = 2
    constant_available_size = encoded_size - constant_overhead
    variable_overhead = constant_available_size // 256
    remaining_len = constant_available_size % 256

    # The remainder is not large enough to store the vairiable overhead.
    if variable_overhead > remaining_len:
        variable_overhead -= 1

    packet_size = constant_available_size - variable_overhead

    return packet_size


@pytest.mark.asyncio
async def test_ht_size(ser):
    reader, writer = ser

    data = os.urandom(encoded_size_to_decoded_size(RX_BUFFER_LEN // 2))
    encoded = bytearray(cobs.encode(data))
    encoded.append(0)

    writer.write(encoded)

    assert await asyncio.wait_for(reader.readexactly(len(encoded)), timeout=5) == encoded


@pytest.mark.asyncio
async def test_tc_size(ser):
    reader, writer = ser

    data = os.urandom(encoded_size_to_decoded_size(RX_BUFFER_LEN))
    encoded = bytearray(cobs.encode(data))
    encoded.append(0)

    writer.write(encoded)

    assert await asyncio.wait_for(reader.readexactly(len(encoded)), timeout=10) == encoded


@pytest.mark.asyncio
async def test_idle_size(ser):
    reader, writer = ser

    data = os.urandom(RX_BUFFER_LEN // 4)
    encoded = bytearray(cobs.encode(data))
    encoded.append(0)

    writer.write(encoded)

    assert await asyncio.wait_for(reader.readexactly(len(encoded)), timeout=10) == encoded


@pytest.mark.asyncio
async def test_repeat_packets(ser):
    reader, writer = ser

    repeat = 5
    size = (RX_BUFFER_LEN // 4) - 1
    data = os.urandom(encoded_size_to_decoded_size(size))
    encoded = bytearray(cobs.encode(data))
    encoded.append(0)
    entire_encoded_stream = encoded * repeat

    read_task = asyncio.create_task(asyncio.wait_for(reader.readexactly(len(entire_encoded_stream)), timeout=10))

    for _ in range(repeat):
        writer.write(encoded)
        await asyncio.wait_for(writer.drain(), timeout=5)
        await asyncio.sleep(0.2)

    assert await read_task == entire_encoded_stream


@pytest.mark.asyncio
async def test_variable_packet_sizes(ser):
    reader, writer = ser

    encoded_packet_sizes = [
        RX_BUFFER_LEN // 6,
        (RX_BUFFER_LEN // 4) - 1,
        RX_BUFFER_LEN,
        (RX_BUFFER_LEN // 6) - 1,
        RX_BUFFER_LEN // 4,
    ]

    encoded_packets = []

    for encoded_packet_size in encoded_packet_sizes:
        packet_size = encoded_size_to_decoded_size(encoded_packet_size)

        data = os.urandom(packet_size)
        encoded = bytearray(cobs.encode(data))
        encoded.append(0)

        encoded_packets.append(encoded)

    entire_encoded_stream = b"".join(encoded_packets)
    read_task = asyncio.create_task(asyncio.wait_for(reader.readexactly(len(entire_encoded_stream)), timeout=10))

    for packet in encoded_packets:
        writer.write(packet)
        await asyncio.wait_for(writer.drain(), timeout=5)
        await asyncio.sleep(0.3)

    assert await read_task == entire_encoded_stream


@pytest.mark.asyncio
async def test_byte_transparency(ser):
    reader, writer = ser

    data = bytes(range(256))
    encoded = bytearray(cobs.encode(data))
    encoded.append(0)
    encoded *= 2

    writer.write(encoded)

    recv = await asyncio.wait_for(reader.readexactly(len(encoded)), timeout=10)

    assert recv == encoded
