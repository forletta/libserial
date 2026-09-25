import asyncio
import os

import pytest
from cobs import cobs


@pytest.mark.asyncio
async def test_byte_transparency(ser):
    reader, writer = ser

    data = bytes(range(256)) * 2
    encoded = bytearray(cobs.encode(data))
    encoded.append(0)

    writer.write(encoded)

    recv = await asyncio.wait_for(reader.readexactly(len(encoded)), timeout=2)

    assert recv[-1] == 0
    assert cobs.decode(recv[:-1]) == data


@pytest.mark.parametrize("size", [64, 512, 1024, 4096])
@pytest.mark.asyncio
async def test_burst(ser, size):
    reader, writer = ser

    data = bytearray(os.urandom(size))
    data[int(size / 2)] = 0
    encoded = bytearray(cobs.encode(data))
    encoded.append(0)

    writer.write(encoded)

    recv = await asyncio.wait_for(reader.readexactly(len(encoded)), timeout=10)

    assert recv[-1] == 0
    assert cobs.decode(recv[:-1]) == data
