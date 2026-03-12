import asyncio
import itertools
import os

import pytest

RX_BUFFER_LEN = int(os.getenv("RX_BUFFER_LEN", "128"))


@pytest.mark.asyncio
async def test_ht_size(ser):
    reader, writer = ser

    data = os.urandom(int(RX_BUFFER_LEN / 2))

    writer.write(data)

    assert await asyncio.wait_for(reader.readexactly(len(data)), timeout=5) == data


@pytest.mark.asyncio
async def test_tc_size(ser):
    reader, writer = ser

    data = os.urandom(RX_BUFFER_LEN)

    writer.write(data)

    assert await asyncio.wait_for(reader.readexactly(len(data)), timeout=5) == data


@pytest.mark.asyncio
async def test_idle_size(ser):
    reader, writer = ser

    data = os.urandom(int(RX_BUFFER_LEN / 4))

    writer.write(data)

    assert await asyncio.wait_for(reader.readexactly(len(data)), timeout=5) == data


@pytest.mark.parametrize("packet_size", [int(RX_BUFFER_LEN / 4), int((RX_BUFFER_LEN / 2) - 1)])
@pytest.mark.asyncio
async def test_repeat_idle_size(ser, baudrate, packet_size):
    reader, writer = ser

    repeat = 5
    data = os.urandom(packet_size * repeat)
    write_deleay = 11 / baudrate

    read_task = asyncio.create_task(asyncio.wait_for(reader.readexactly(len(data)), timeout=5))

    for packet in itertools.batched(data, packet_size, strict=False):
        writer.write(bytes(packet))
        await asyncio.wait_for(writer.drain(), timeout=5)
        await asyncio.sleep(write_deleay)

    assert await read_task == data


@pytest.mark.asyncio
async def test_byte_transparency(ser):
    reader, writer = ser

    data = bytes(range(256)) * 2

    writer.write(data)

    assert await asyncio.wait_for(reader.readexactly(len(data)), timeout=5) == data


@pytest.mark.parametrize("size", [1024, 4096])
@pytest.mark.asyncio
async def test_burst(ser, size):
    reader, writer = ser

    data = os.urandom(size)

    writer.write(data)

    assert await asyncio.wait_for(reader.readexactly(len(data)), timeout=5) == data
