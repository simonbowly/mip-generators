
import functools
import itertools
import random

import numpy as np

from .nsga2 import nsga2_elites, nsga2_order_key
from .utils import map_async_workers


def sample(random_state, population, size):
    if type(random_state) is random.Random:
        return random_state.sample(population, size)
    elif type(random_state) is np.random.RandomState:
        return random_state.choice(population, size, replace=False).tolist()
    else:
        raise TypeError("Unsupported RNG")


def select_tournament(random_state, population, func, size=2, count=2):
    ''' Tournament selection choosing the winner on lowest :order value. '''
    return [
        min(
            sample(random_state, population, size),
            key=lambda inst: func(inst['attr']))['instance']
        for _ in range(count)]


def select_combine_tournament_elite(
        population, *, random_state, fitness_func, num_elites,
        tournament_size, crossover):
    elites = sorted(
        population, key=lambda inst: fitness_func(inst['attr']),
        reverse=True)[:num_elites]
    children = list(itertools.chain(*(
        crossover(*select_tournament(random_state, population, fitness_func))
        for _ in range((len(population) - num_elites) // 2))))
    assert len(elites) + len(children) == len(population)
    return elites, children


async def attach_attributes(evaluate, population, workers):
    # Evaluate instances and attach fitness.
    data = await map_async_workers(evaluate, population, workers=workers)
    return [
        {'instance': inst, 'attr': fit}
        for inst, fit in zip(population, data)]


async def parallel_synchronous_ga(population, evaluate, progression, *,
                                  generations, workers, generation_callback=None):
    ''' Run synchronous GA starting from the given :population for the given
    number of :generations. Evaluates computationally intenstive instance
    attributes in parallel using the :evaluate coroutine, then produces
    the next generation using the given :progression algorithm. Returns the
    final generation with attributes attached. '''
    _attach_attrs = functools.partial(attach_attributes, evaluate, workers=workers)
    population = await _attach_attrs(population)
    if generation_callback:
        generation_callback(0, population)
    for i in range(generations):
        elites, children = progression(population)
        assert len(elites) + len(children) == len(population)
        population = elites + await _attach_attrs(children)
        if generation_callback:
            generation_callback(i+1, population)
    return population


def nsga2_select(random_state, population, size=2, count=2):
    return [
        min(
            sample(random_state, population, size),
            key=nsga2_order_key)['instance']
        for _ in range(count)]


async def parallel_nsga2(population, evaluate, *, random_state, objective_vector,
                         crossover, generations, workers, generation_callback=None):

    async def _attach_attrs(indivs):
        ''' Asynchronously evaluate attributes, then calculate the MO vector
        for each instance. '''
        indivs = await attach_attributes(evaluate, indivs, workers=workers)
        for individual in indivs:
            individual['objective'] = objective_vector(individual['attr'])
        return indivs

    population = await _attach_attrs(population)
    population = nsga2_elites(population, size=len(population), key='objective')
    if generation_callback:
        generation_callback(0, population)
    for i in range(generations):
        children = list(itertools.chain(*(
            crossover(*nsga2_select(random_state, population))
            for _ in range(len(population) // 2))))
        assert len(children) == len(population)
        children = await _attach_attrs(children)
        population = nsga2_elites(
            population + children, size=len(population), key='objective')
        if generation_callback:
            generation_callback(i+1, population)
    return population
