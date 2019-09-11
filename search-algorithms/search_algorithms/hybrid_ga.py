'''
Core algorithm.

Key algorithm parameters:
    - ga_priority (p) = probability that the next task issued is a GA evaluation
    - population (n)
    - workers (w) = number of parallel workers
    - task_limit (l) = number of function evaluations to carry out

Describing the algorithm as (p, n, w, l):
    - (0, 1, 1, L)      = single sequential local search for L function evaluations
    - (0, N, N, L)      = N parallel local search runs for L total function evaluations
    - (1, N, 1, N * G)  = standard GA with G generations on population size N,
                          evaluated sequentially
    - (1, N, W, L)      = standard GA with G generations on population size N,
                          evaluated in parallel by W workers with opportunistic local
                          search
'''

import asyncio
import dataclasses
import heapq
import time
import typing


@dataclasses.dataclass(order=True)
class LSTask:
    ''' Priority queueable LS task. :solution is a neighbour of :base.
    :base_fitness is the known fitness of :base. This allows a worker
    to evaluate solution and submit a new neighbour of the better of
    ;base and :solution for subsequent LS ops. '''
    priority: typing.Any=dataclasses.field()
    base: typing.Any=dataclasses.field(compare=False)
    base_fitness: typing.Any=dataclasses.field(compare=False)
    solution: typing.Any=dataclasses.field(compare=False)


@dataclasses.dataclass
class GATask:
    ''' GA task simply requires :solution to be evaluated. '''
    solution: typing.Any=dataclasses.field(compare=False)


@dataclasses.dataclass
class Queue:
    ''' Manage both queues and record statistics. '''
    ga_queue: typing.List
    ls_queue: typing.List
    ga_results: typing.List
    best_solution: typing.Any
    ga_tasks_issued: int
    ls_tasks_issued: int
    generations: int
    total_evals: int
    record: typing.List

    def print_state(self, elapsed):
        print(
            f"Elapsed: {elapsed}  "
            f"Evals: {self.total_evals}  "
            f"Fittest: {self.best_solution[1]}  "
            f"Generation: {self.generations}  "
            f"GA Issued: {self.ga_tasks_issued}  "
            f"LS Issued: {self.ls_tasks_issued}  "
            f"Pending: {len(self.ga_queue) + len(self.ls_queue)}  "
            f"Completed: {len(self.ga_results)}")


async def worker(queue, *, rstate, evaluate, neighbour, next_population,
                 task_limit, ga_priority, population_size):
    ''' Worker coroutine. Carries out atomic queue update operations GetTask,
    LSResult, GAResult as outlined in the paper. Asynchronously evaluates
    solutions. '''
    while queue.ls_tasks_issued + queue.ga_tasks_issued < task_limit:
        # Get task from queue according to the priority rules.
        if (len(queue.ls_queue) == 0) or (len(queue.ga_queue) > 0 and queue.ga_tasks_issued <= ga_priority * (queue.ls_tasks_issued + queue.ga_tasks_issued)):
            queue.ga_tasks_issued += 1
            task = queue.ga_queue.pop()
        else:
            queue.ls_tasks_issued += 1
            task = heapq.heappop(queue.ls_queue)
        # Evaluate fitness asynchronously.
        fitness = await evaluate(task.solution)
        queue.total_evals += 1
        # Update incumbent and statistics.
        if queue.best_solution is None or fitness > queue.best_solution[1]:
            if type(task) is GATask:
                if queue.generations == 0:
                    source = 'initial'
                else:
                    source = 'genetic'
            else:
                source = 'local'
            queue.record.append({
                'source': source, 'fitness': fitness,
                'step': queue.ls_tasks_issued + queue.ga_tasks_issued})
            queue.best_solution = task.solution, fitness
        # Update the queues 
        if type(task) is GATask:
            # Update GA results and create a new population if required.
            queue.ga_results.append(dict(solution=task.solution, fitness=fitness))
            if len(queue.ga_results) >= population_size:
                queue.ga_queue = [
                    GATask(ind)
                    for ind in next_population(queue.ga_results)]
                queue.ga_results = []
                queue.generations += 1
            # Submit a local search task for a neighbour of this solution.
            heapq.heappush(queue.ls_queue, LSTask(
                priority=-fitness, solution=neighbour(task.solution),
                base=task.solution, base_fitness=fitness))
        else:
            # Submit a neighbour if this solution is better, otherwise backtrack
            # (i.e. submit a neighbour of the previous solution instead).
            if fitness > task.base_fitness:
                heapq.heappush(queue.ls_queue, LSTask(
                    priority=-fitness, solution=neighbour(task.solution),
                    base=task.solution, base_fitness=fitness))
            else:
                heapq.heappush(queue.ls_queue, LSTask(
                    priority=-task.base_fitness, solution=neighbour(task.base),
                    base=task.base, base_fitness=task.base_fitness))


async def monitor(queue, start, log_seconds):
    while True:
        await asyncio.sleep(log_seconds)
        queue.print_state(time.monotonic() - start)


async def run(*, population, workers, log_seconds, **kwargs):
    '''
    Main function to run the parallelised hybrid strategy.
    See onemax.py for an example of use.

    Create queue with initial GA population. Run workers until the evaluation
    budget (max evaluated solutions) is exhausted. Report evaluation statistics
    and return the best known solution.

        population:         List of initial candidate solutions.
        rstate:             Seeded random.Random object.
        evaluate:           Coroutine to asynchronously evaluation fitness.
        neighbour:          Function to generate a local neighbour from a solution.
        next_population:    Function to generate a new population given an
                            existing population with evaluated fitnesses.
        workers:            Number of parallel workers.
        ga_priority:        Fraction of evaluations put towards evaluating GA solutions
                            vs LS solutions.
        task_limit:         Maximum fitness evaluations before termination.

    '''
    loop = asyncio.get_event_loop()
    queue = Queue(
        ga_queue=[GATask(ind) for ind in population],
        ls_queue=[], ga_results=[], best_solution=None,
        ga_tasks_issued=0, ls_tasks_issued=0, generations=0,
        record=[], total_evals=0)
    start = time.monotonic()
    asyncio.ensure_future(monitor(queue, start, log_seconds))
    await asyncio.gather(*(
        worker(queue, population_size=len(population), **kwargs)
        for _ in range(workers)))
    queue.print_state(time.monotonic() - start)
    return queue
