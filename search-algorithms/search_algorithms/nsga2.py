
import itertools

import numpy as np


def dominates(self, other):
    ''' dominates(self, other) should return true if <= for all objectives
    and < for at least one. Not efficient implementation (loops twice). '''
    # FIXME variable names
    if len(self) != len(other):
        raise ValueError('Dominance check requires vector lengths to match.')
    return (
        all(a <= b for a, b in zip(self, other)) and
        any(a < b for a, b in zip(self, other)))


def split_dominated(population, key):
    ''' Split the given population into dominated and nondominated parts. '''
    dominated = list()
    nondominated = list()
    for individual in population:
        if any(
                dominates(other[key], individual[key])
                for other in population):
            dominated.append(individual)
        else:
            nondominated.append(individual)
    return dominated, nondominated


def nondominated_sort(population, key):
    ''' Return the population as a list of nondominated frontiers.
    Inefficient O(MN^3) algorithm sorts into ranks by splitting out the
    nondominated points iteratively. '''
    ranks = []
    remaining = list(population)
    while remaining:
        remaining, nondominated = split_dominated(remaining, key)
        ranks.append(nondominated)
    return ranks


def assign_nondominated_rank(population, key):
    ''' Input dicts with vector at given key. Output dicts with updated
    :pareto_rank key. Sort into ranks, unroll to single list with rank
    indices assigned (population is reordered). '''
    ranks = nondominated_sort(population, key=key)
    return list(itertools.chain(*(
        [dict(individual, pareto_rank=i) for individual in rank]
        for i, rank in enumerate(ranks))))


def assign_crowding_distance(population, key):
    ''' Input dicts with vector at given key. Output dicts with updated
    :crowding_distance key.
    Return NSGA-II crowding distance estimates along the given dimension
    for all individuals in the population. Calculated as the distance between
    left and right neighbours (in sorted order) of each individual, scaled
    by the range of the objective. '''
    population = [dict(ind, crowding_distance=0) for ind in population]
    dimensions = len(population[0][key])
    for dim in range(dimensions):
        # Break ties by minimising other objectives, to ensure infinite
        # crowding value is assigned to the pareto dominant individual.
        population.sort(key=lambda ind: (ind[key][dim], ind[key]))
        vrange = population[-1][key][dim] - population[0][key][dim]
        if vrange == 0:
            continue
        population[0]['crowding_distance'] = np.inf
        population[-1]['crowding_distance'] = np.inf
        for a, b, c in zip(population, population[1:], population[2:]):
            b['crowding_distance'] = (
                b['crowding_distance'] +
                (c[key][dim] - a[key][dim]) / vrange)
    return population


def nsga2_order_key(ind):
    ''' Minimise pareto rank, break ties by maximising crowding distance. '''
    return ind['pareto_rank'], -ind['crowding_distance']


def nsga2_elites(population, size, key):
    ''' Calculate NSGA-II ranking and return the :size set of elite
    candidates. '''
    population = assign_nondominated_rank(population, key=key)
    population = assign_crowding_distance(population, key=key)
    return sorted(population, key=nsga2_order_key)[:size]
