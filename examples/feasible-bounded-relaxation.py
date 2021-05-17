from os import makedirs
from pathlib import Path
from random import randint

from mip_generators import formats, generators, instances

from numpy.random import RandomState


def generate(seed):
    random_state = RandomState(seed)
    common_params = dict(variables=50, constraints=100, random_state=random_state)
    return instances.construct_feasible_bounded(
        variable_types=generators.generate_variable_types(
            **common_params, prob_integer=random_state.uniform(0.5, 1.0)
        ),
        lhs=generators.generate_lhs(
            **common_params,
            density=random_state.uniform(low=0.1, high=1.0),
            pv=random_state.uniform(low=0.0, high=1.0),
            pc=random_state.uniform(low=0.0, high=1.0),
            coeff_loc=random_state.uniform(low=-2.0, high=2.0),
            coeff_scale=random_state.uniform(low=0.1, high=1.0),
        ),
        alpha=generators.generate_alpha(
            **common_params,
            frac_violations=random_state.uniform(low=0.1, high=1.0),
            beta_param=random_state.lognormal(mean=-0.2, sigma=1.8),
            mean_primal=0,
            std_primal=1,
            mean_dual=0,
            std_dual=1,
        ),
        beta=generators.generate_beta(
            **common_params, basis_split=random_state.uniform(low=0.0, high=1.0)
        ),
    )


output_dir = Path("generated")
makedirs(output_dir, exist_ok=True)
for _ in range(10):
    seed = randint(0, 2 ** 32)
    instance = generate(seed)
    target = output_dir / f"feasible-bounded-relaxation-{seed}.mps.gz"
    formats.write_mps(instance, target)
    print(f"Wrote instance for {seed = } to {target}")
