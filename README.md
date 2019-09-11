
# Setup

    pip install -r build-requirements.txt   # numpy, cython, etc for building
    pip install -r requirements.txt         # install main packages

Tests are scattered and library uses old numpy conventions, so ...

    pytest -W ignore::PendingDeprecationWarning
