import asyncio
import os

import pytest


@pytest.mark.asyncio
async def test_byte_transparency(ser):
    reader, writer = ser

    data = bytes(range(256)) * 2

    writer.write(data)

    assert await asyncio.wait_for(reader.readexactly(len(data)), timeout=2) == data


@pytest.mark.parametrize("size", [64, 512, 1024, 4096])
@pytest.mark.asyncio
async def test_burst(ser, size):
    reader, writer = ser

    data = os.urandom(size)

    writer.write(data)

    assert await asyncio.wait_for(reader.readexactly(len(data)), timeout=5) == data
