import asyncio
import os
import subprocess

import pytest
import pytest_asyncio
import serial
import serial_asyncio
from dotenv import load_dotenv

load_dotenv()

SERIAL_PORT = os.getenv("SERIAL_PORT")
BAUDRATES = [int(rate) for rate in os.getenv("TEST_BAUDRATES", "9600 57600 115200").split()]
PARITIES = os.getenv("TEST_PARITIES", "USART_PARITY_NONE USART_PARITY_EVEN USART_PARITY_ODD").split()


@pytest.fixture(scope="module", params=BAUDRATES, ids=lambda b: f"baudrate:{b}")
def baudrate(request):
    return request.param


@pytest.fixture(scope="module", params=PARITIES, ids=lambda parity: f"parity:{parity}")
def parity(request):
    return request.param


@pytest.fixture(scope="module", autouse=True)
def flash_firmware(baudrate, parity):
    env = os.environ.copy()
    env["BAUDRATE"] = str(baudrate)
    env["PARITY"] = str(parity)

    subprocess.run(["make", "flash", "EXAMPLE=echo"], env=env, check=True, capture_output=True)


@pytest_asyncio.fixture
async def ser(baudrate, parity):
    parity_map = {
        "USART_PARITY_NONE": serial.PARITY_NONE,
        "USART_PARITY_EVEN": serial.PARITY_EVEN,
        "USART_PARITY_ODD": serial.PARITY_ODD,
    }

    reader, writer = await serial_asyncio.open_serial_connection(
        url=SERIAL_PORT,
        baudrate=baudrate,
        parity=parity_map[parity],
        limit=65536,
        rtscts=False,
        dsrdtr=False,
        xonxoff=False,
    )

    yield reader, writer

    writer.close()
    await writer.wait_closed()
    await asyncio.sleep(0.1)  # Settle time
