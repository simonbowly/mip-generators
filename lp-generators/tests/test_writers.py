
import os

import pytest

from lp_generators.instance import EncodedInstance, UnsolvedInstance
from lp_generators.writers import (
    write_mps,
    write_tar_encoded, read_tar_encoded,
    write_tar_lp, read_tar_lp)
from lp_generators.utils import temp_file_path
from .testing import random_encoded, random_encoded_mip, assert_approx_equal


@pytest.mark.parametrize('instance', [
    random_encoded(3, 5),
    random_encoded(5, 3),
    random_encoded_mip(5, 3, "IIICI"),
    ])
def test_write_mps(instance):
    with temp_file_path('.mps.gz') as file_path:
        write_mps(instance, file_path)
        assert os.path.exists(file_path)


@pytest.mark.parametrize('instance', [
    random_encoded(3, 5),
    random_encoded(5, 3),
    random_encoded_mip(5, 3, "IIICI"),
    ])
def test_read_write_tar_encoded(instance):
    with temp_file_path() as file_path:
        write_tar_encoded(instance, file_path)
        assert os.path.exists(file_path)
        read_instance = read_tar_encoded(file_path)
    assert isinstance(read_instance, EncodedInstance)
    assert instance.variables == read_instance.variables
    assert instance.constraints == read_instance.constraints
    assert instance.variable_types == read_instance.variable_types
    assert_approx_equal(instance.lhs(), read_instance.lhs())
    assert_approx_equal(instance.alpha(), read_instance.alpha())
    assert_approx_equal(instance.beta(), read_instance.beta())


@pytest.mark.parametrize('instance', [
    random_encoded(3, 5),
    random_encoded(5, 3),
    random_encoded_mip(5, 3, "IIICI"),
    ])
def test_read_write_tar_lp(instance):
    with temp_file_path() as file_path:
        write_tar_lp(instance, file_path)
        assert os.path.exists(file_path)
        read_instance = read_tar_lp(file_path)
    assert isinstance(read_instance, UnsolvedInstance)
    assert instance.variables == read_instance.variables
    assert instance.constraints == read_instance.constraints
    assert instance.variable_types == read_instance.variable_types
    assert_approx_equal(instance.lhs(), read_instance.lhs())
    assert_approx_equal(instance.rhs(), read_instance.rhs())
    assert_approx_equal(instance.objective(), read_instance.objective())
