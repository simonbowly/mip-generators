import pytest
import numpy as np

from mip_generators import formats, instances


def test_construct_integer_feasible():
    ''' Needed a quick compatibilty test with LP encoding code. '''
    instance = instances.construct_integer_feasible(
        variable_types="IIICCC",
        A=np.matrix([
            [1, 0, 1, 1, 1, 1],
            [-1, 1, 0, -1, 1, -1],
            [0, 1, -1, 0, 0, -1],
        ]),
        x=[1, 1, 3, 0.1, 0.3, 0.5],
        s=[0.1, 0.1, 1],
        y=[1, 2, 3],
        r=[0.2, 0.4, 0.5, 1.2, 0.1, 0.5]
    )
    assert instance.solution() is not None
    formats.write_canonical(instance, "/tmp/model.mip")
    formats.write_encoded(instance, "/tmp/model.mipenc")
    formats.write_mps(instance, "/tmp/model.mps.gz")
