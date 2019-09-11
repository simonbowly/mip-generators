import numpy as np

from .instances import construct_canonical, construct_feasible_bounded


def lhs_repair(random_state, lhs):
    empty_columns = ((lhs != 0).sum(axis=0) == 0)
    if empty_columns.any():
        for column in np.where(empty_columns)[1]:
            # Hacky, basically just adds a loose upper bound.
            lhs[random_state.choice(lhs.shape[0]), column] = 0.0001
    return lhs


def canonical_uniform_row_crossover(random_state, parent1, parent2, bias=0.5):
    ''' bias towards parent1 '''
    assert parent1.variables == parent2.variables
    assert parent1.constraints == parent2.constraints
    choose = random_state.choice([0, 1], size=parent1.constraints, p=(bias, 1-bias))
    child1_rows = choose * parent1.constraints + np.arange(parent1.constraints)
    child2_rows = (1 - choose) * parent1.constraints + np.arange(parent1.constraints)
    lhs_merged = np.concatenate([parent1.lhs(), parent2.lhs()])
    rhs_merged = np.concatenate([parent1.rhs(), parent2.rhs()])
    return (
        construct_canonical(
            variable_types=parent1.variable_types,
            lhs=lhs_repair(random_state, lhs_merged[child1_rows, :]),
            rhs=rhs_merged[child1_rows],
            objective=parent1.objective()
        ),
        construct_canonical(
            variable_types=parent2.variable_types,
            lhs=lhs_repair(random_state, lhs_merged[child2_rows, :]),
            rhs=rhs_merged[child2_rows],
            objective=parent2.objective()
        )
    )


def feasible_bounded_uniform_row_crossover(random_state, parent1, parent2, bias=0.5):
    ''' bias towards parent1 '''
    assert parent1.variables == parent2.variables
    assert parent1.constraints == parent2.constraints
    choose = random_state.choice([0, 1], size=parent1.constraints, p=(bias, 1-bias))
    child1_rows = choose * parent1.constraints + np.arange(parent1.constraints)
    child2_rows = (1 - choose) * parent1.constraints + np.arange(parent1.constraints)
    lhs_merged = np.concatenate([parent1.lhs(), parent2.lhs()])
    rhs_merged = np.concatenate([parent1.rhs(), parent2.rhs()])
    return (
        construct_feasible_bounded(
            variable_types=parent1.variable_types,
            lhs=lhs_repair(random_state, lhs_merged[child1_rows, :]),
            alpha=parent1.alpha(),
            beta=parent1.beta()
        ),
        construct_feasible_bounded(
            variable_types=parent2.variable_types,
            lhs=lhs_repair(random_state, lhs_merged[child2_rows, :]),
            alpha=parent2.alpha(),
            beta=parent2.beta()
        )
    )
