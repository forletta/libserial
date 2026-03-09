import os

import pytest


def test_byte_transparency(ser):
    data = bytes(range(256)) * 2

    assert ser.write(data) == len(data)
    assert ser.read(len(data)) == data


@pytest.mark.parametrize("size", [64, 512, 1024, 4096])
def test_burst(ser, size):
    data = os.urandom(size)

    assert ser.write(data) == size, f"Failed to write data size: {size}"
    assert ser.read(size) == data, f"Failed to read data size: {size}"
