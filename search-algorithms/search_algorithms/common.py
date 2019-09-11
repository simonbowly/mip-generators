''' Library of crossover, neighbourhood, selection and generation progression
operators for binary encoded problems. '''

import itertools
import operator


def create_neighbour(solution, rstate):
    ''' Apply a local search operator to create a neighbour. '''
    ind = rstate.randint(0, len(solution) - 1)
    return tuple(
        1 - elem if i == ind else elem
        for i, elem in enumerate(solution))


def select_tournament(results, tournament_size, rstate):
    ''' Select an instance using an n-tournament. '''
    candidate = rstate.choice(results)
    for _ in range(tournament_size - 1):
        alternate = rstate.choice(results)
        if alternate['fitness'] > candidate['fitness']:
            candidate = alternate
    return alternate['solution']


def one_point_crossover(a, b, rstate):
    ''' Return two children created by random one-point crossover. '''
    ind = rstate.randint(0, len(a) - 1)
    return a[:ind] + b[ind:], b[:ind] + a[ind:]


def uniform_crossover(a, b, rstate):
    ''' Return two children created by random one-point crossover. '''
    c1 = []
    c2 = []
    for e1, e2 in zip(a, b):
        if rstate.uniform(0, 1) < 0.5:
            c1.append(e1)
            c2.append(e2)
        else:
            c1.append(e2)
            c2.append(e1)
    return tuple(c1), tuple(c2)


def new_population(results, *, select, crossover, create_random):
    ''' Create a new population given :results, a list of dicts with
    'solution' and 'fitness' keys. Uses 10% elitism + 10% random +
    80% random crossover. '''
    n_elite = int(round(len(results) * 0.2))
    n_random = int(round(len(results) * 0.2))
    population = [
        entry['solution']
        for entry in itertools.islice(
            reversed(sorted(results, key=operator.itemgetter('fitness'))),
            n_elite)]
    population.extend([create_random() for _ in range(n_random)])
    while len(population) < len(results):
        a, b = crossover(select(results), select(results))
        population.extend([a, b])
    return population


def random_solution(rstate, n, p=0.5):
    return tuple(int(rstate.uniform(0, 1) < p) for _ in range(n))
