
import asyncio
import contextlib
import pathlib
import tempfile

import tqdm


async def worker(evaluate, queue, *, monitor=None):
    while True:
        try:
            instance = next(
                instance for instance in queue
                if instance['result'] is None)
            instance['result'] = 'pending'
            if monitor:
                next(monitor)
            instance['result'] = await evaluate(instance['file_name'])
        except StopIteration:
            return


async def map_async_workers(evaluate, instances, *, workers):
    queue = [
        {'file_name': file_name, 'result': None}
        for file_name in instances]
    # monitor = iter(tqdm.tqdm([None for _ in queue]))
    monitor = None
    errors = await asyncio.gather(*(
        worker(evaluate, queue, monitor=monitor)
        for _ in range(workers)))
    assert all(err is None for err in errors)
    # list(monitor)
    return [item['result'] for item in queue]
