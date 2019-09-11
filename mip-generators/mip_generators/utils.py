import tempfile, pathlib, contextlib


@contextlib.contextmanager
def temporary_directory():
    with tempfile.TemporaryDirectory() as tmpdir:
        yield pathlib.Path(tmpdir)


@contextlib.contextmanager
def temporary_model_file(ext):
    with temporary_directory() as tmpdir:
        yield tmpdir.joinpath(f"model.{ext.strip('.')}")
