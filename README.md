# Mixed Integer Programming Instance Generators

This is a collection of codes used to generate challenging mixed integer
programming instances as part of my PhD thesis. It's still a work in progress,
hoping to extend the generator capabilities over time.

## Examples

Since the definition of distributions of generated instances quickly becomes
complicated with more parameters, I've intended for this package to allow
distributions to be defined by composing the various generator and constructor
functions. See the `examples` directory for scripts which automate the instance
generation process.

## Setup

Installation should be fairly easy:

    pip install -r build-requirements.txt   # numpy, cython, etc for building
    pip install -r requirements.txt         # install main packages

Note though that this installs multiple top-level packages (mip_generators,
lp_generators, scip_runner, search_algorithms) which is not great. I expect to
refine this in future.

Tests are scattered and library uses old numpy conventions, so ...

    pytest -W ignore::PendingDeprecationWarning


## Citing this work

So far this is a minor extension of the generators in the
[lp-generators](https://github.com/simonbowly/lp-generators/) package. If you
are using either package please cite our paper published in Mathematical
Programming Computation (MPC) where we describe the generator and investigate
properties of the generated instances:

```
@Article{Bowly.Smith-Miles.ea_2020_Generation-techniques,
    author      = {Bowly, Simon and Smith-Miles, Kate and Baatar, Davaatseren
                  and Mittelmann, Hans},
    title       = {Generation techniques for linear programming instances
                  with controllable properties},
    journal     = {Mathematical Programming Computation},
    year        = 2020,
    volume      = 12,
    number      = 3,
    pages       = {389--415},
    month       = sep,
}
```
