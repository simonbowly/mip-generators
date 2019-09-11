
import asyncio
import subprocess
from pathlib import Path

PROC_TIMEOUT_MUL = 1.5


def _command(model_file, *,  settings_file=None,
             read_solution_file=None,
             write_solution_file=None,
             time_limit=None):
    ''' Produce a subprocess command to solve the given model with SCIP
    using the given settings. '''
    if not Path(model_file).exists():
        raise ValueError(f"Model file {model_file} not found.")
    cmd = f'read {model_file}'
    if settings_file:
        cmd += f' set load {settings_file}'
    if read_solution_file:
        if not Path(read_solution_file).exists():
            raise ValueError(f"Solution file {read_solution_file} not found.")
        cmd += f' read {read_solution_file}'
    if time_limit:
        cmd += f' set limits time {time_limit}'
    cmd += ' set timing clocktype 1'
    cmd += ' optimize'
    if write_solution_file:
        cmd += f' write solution {write_solution_file}'
    cmd += ' display statistics'
    cmd += ' quit'
    return f"scip -c '{cmd}'"


def _handle_process_output(retcode, stdout):
    logs = stdout.decode()
    if logs.strip().endswith("SCIP>"):
    # if "SCIP Status" not in logs:
        header, mid, message = logs.partition("by T. Koch (zimpl.zib.de)")
        assert mid
        message = "; ".join(
            line for line in
            message.strip().split("\n")
            if line)
        raise ValueError("SCIP waiting for input.", message)
    return logs


def solve(*args, **kwargs):
    ''' Run SCIP with given inputs and return the logs. '''
    proc = subprocess.Popen(
        _command(*args, **kwargs), shell=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        stdin=subprocess.DEVNULL)
    stdout, stderr = proc.communicate()
    return _handle_process_output(proc.returncode, stdout)


async def solve_async(*args, proc_timeout=None, **kwargs):
    if proc_timeout is None:
        if "time_limit" in kwargs and kwargs["time_limit"] is not None:
            proc_timeout = kwargs["time_limit"] * PROC_TIMEOUT_MUL
        else:
            proc_timeout = 3600.0
    proc = await asyncio.create_subprocess_shell(
        "exec " + _command(*args, **kwargs),
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE,
        stdin=asyncio.subprocess.DEVNULL)
    try:
        stdout, stderr = await asyncio.wait_for(
            proc.communicate(),
            timeout=proc_timeout)
    except asyncio.TimeoutError:
        proc.kill()
        stdout, stderr = await proc.communicate()
    return _handle_process_output(proc.returncode, stdout)
