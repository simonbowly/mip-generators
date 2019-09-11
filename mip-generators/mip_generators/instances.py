import numpy as np

from lp_generators.instance import UnsolvedInstance, EncodedInstance


def construct_canonical(*, variable_types, lhs, rhs, objective):
    return UnsolvedInstance(
        variable_types=variable_types, lhs=lhs, rhs=rhs, objective=objective
    )


def construct_feasible_bounded(*, variable_types, lhs, alpha, beta):
    return EncodedInstance(
        variable_types=variable_types, lhs=lhs, alpha=alpha, beta=beta
    )


def construct_integer_feasible(*, variable_types, A, x, s, y, r):
    m, n = A.shape
    assert len(variable_types) == n
    for vt, xv in zip(variable_types, x):
        if vt == "I":
            assert (xv - round(xv)) < 10 ** -5
    x = np.matrix(x).transpose()
    assert x.shape == (n, 1)
    s = np.matrix(s).transpose()
    assert s.shape == (m, 1)
    y = np.matrix(y).transpose()
    assert y.shape == (m, 1)
    r = np.matrix(r).transpose()
    assert r.shape == (n, 1)
    b = A * x + s
    c = A.transpose() * y - r
    return UnsolvedInstance(
        variable_types=variable_types,
        lhs=A,
        rhs=np.asarray(b.transpose(), dtype=np.float)[0],
        objective=np.asarray(c.transpose(), dtype=np.float)[0],
    )
