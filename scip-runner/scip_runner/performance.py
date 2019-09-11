
import asyncio
import os
import pathlib
import shutil
import uuid

from .logs import parse_logs
from .solve import solve_async



def handle_failure(model_file, settings_file, logs):
    directory = pathlib.Path("log-failures").joinpath(str(uuid.uuid4()))
    os.makedirs(directory)
    if model_file:
        shutil.copy(model_file, directory.joinpath("model.mps.gz"))
    if settings_file:
        shutil.copy(settings_file, directory.joinpath("scip.set"))
    if logs:
        with open(directory.joinpath("scip.log"), "w") as outfile:
            outfile.write(logs)
    print(f"Failure written to {directory}")


def summary(parsed_logs):
    return {
        'status': parsed_logs['solve_status'],
        'solve_time': parsed_logs['timing']['solving'],
        'tree_nodes': parsed_logs['tree']['nodes'],
        'simplex_iterations': sum(
            parsed_logs['lp'][kind]['Iterations']
            for kind in ["primal LP", "dual LP", "strong branching"]),
        'primal_dual_integral': parsed_logs['solution']['primal_dual_integral'],
    }


async def compare_branching(model_file, configs, time_limit=None):
    ''' Compare branching methods by first solving the model with default
    settings, then providing the solution to SCIP before solving the model
    with each custom configuration. '''
    data = {}
    solution_file = str(model_file) + ".sol"
    logs = await solve_async(
        model_file, write_solution_file=solution_file,
        time_limit=time_limit)
    try:
        data['default_logs'] = logs
        data["default"] = parse_logs(logs)
    except:
        handle_failure(model_file, None, logs)
        raise
    if not pathlib.Path(solution_file).exists():
        raise ValueError("No solution file written.")
    for name, settings_file in configs.items():
        logs = await solve_async(
            model_file, settings_file=settings_file,
            read_solution_file=solution_file,
            time_limit=time_limit)
        data[f'{name}_logs'] = logs
        try:
            data[name] = parse_logs(logs)
        except:
            handle_failure(model_file, settings_file, logs)
            raise
    compare = {
        name: summary(data[name])
        for name in configs
    }
    return compare, data


async def compare_heuristics(model_file, configs, time_limit=None):
    ''' Compare primal heuristic settings by solving with each custom
    configuration and reporting the primal integral. '''
    data = {}
    for name, settings_file in configs.items():
        logs = await solve_async(
            model_file, settings_file=settings_file,
            time_limit=time_limit)
        data[f'{name}_logs'] = logs
        data[name] = parse_logs(logs)
    compare = {
        name: summary(data[name])
        for name in configs
    }
    return compare, data
