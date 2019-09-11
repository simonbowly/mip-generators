from lp_generators.solution_generators import generate_beta, generate_alpha
from lp_generators.lhs_generators import generate_lhs

def generate_variable_types(variables, prob_integer, random_state, **kwargs):
    return "".join(random_state.choice(["I", "C"], variables, p=[prob_integer, 1 - prob_integer]))
