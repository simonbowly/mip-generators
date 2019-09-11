import asyncio
import pathlib
import time

import pytest

from scip_runner.solve import solve_async


model_easy = pathlib.Path(__file__).parent.joinpath("inst_1897027209.mps")
model_hard = pathlib.Path(__file__).parent.joinpath("inst_2083253852.mps")

def test_solve_async():
    asyncio.run(solve_async(model_easy))

def test_solve_async_timeout():
    start = time.monotonic()
    asyncio.run(solve_async(model_hard, proc_timeout=0.1))
    assert (time.monotonic() - start) < 1
