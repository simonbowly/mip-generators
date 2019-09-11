'''
Implements a simple asynchronous GA - as soon as the queue of evaluation
tasks becomes empty, the next population is calculated on the basis of
currently available results. No local search is implemented.
'''

import asyncio
import dataclasses
import time
import typing


@dataclasses.dataclass
class Queue:
    ga_queue: typing.List
    ga_results: typing.List
    best_solution: typing.Any
    tasks_issued: int
    total_evals: int

    def print_state(self, elapsed):
        print(
            f"Elapsed: {elapsed}  "
            f"Evals: {self.total_evals}  "
            f"Fittest: {self.best_solution[1]}  "
            f"Issued: {self.tasks_issued}  "
            f"Pending: {len(self.ga_queue)}  "
            f"Completed: {len(self.ga_results)}")


async def worker(queue, *, evaluate, next_population, task_limit):
    while queue.tasks_issued < task_limit:
        # Get task to do. Asynchronously update population first if necessary.
        if len(queue.ga_queue) == 0:
            queue.ga_queue = next_population(queue.ga_results)
            queue.ga_results = []
        solution = queue.ga_queue.pop()
        # Evaluate fitness on worker.
        fitness = await evaluate(solution)
        queue.total_evals += 1
        # Update the incumbent.
        if queue.best_solution is None or fitness > queue.best_solution[1]:
            queue.best_solution = solution, fitness
        queue.ga_results.append(dict(solution=solution, fitness=fitness))
        queue.tasks_issued += 1


async def monitor(queue, start, log_seconds):
    while True:
        await asyncio.sleep(log_seconds)
        queue.print_state(time.monotonic() - start)


async def run(*, population, workers, log_seconds, **kwargs):
    queue = Queue(
        ga_queue=population, ga_results=[],
        best_solution=None, tasks_issued=0, total_evals=0)
    start = time.monotonic()
    asyncio.ensure_future(monitor(queue, start, log_seconds))
    await asyncio.gather(*(
        worker(queue, **kwargs)
        for _ in range(workers)))
    queue.print_state(time.monotonic() - start)
    return queue
