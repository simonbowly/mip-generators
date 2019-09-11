import pathlib

from lp_generators import writers


def wrap_writer(writer):
    def wrapped(instance, file_name):
        if isinstance(file_name, pathlib.Path):
            file_name = str(file_name)
        writer(instance, file_name)

    return wrapped


def wrap_reader(reader):
    def wrapped(file_name):
        if isinstance(file_name, pathlib.Path):
            file_name = str(file_name)
        return reader(file_name)

    return wrapped


write_mps = wrap_writer(writers.write_mps)
write_canonical = wrap_writer(writers.write_tar_lp)
write_encoded = wrap_writer(writers.write_tar_encoded)
read_canonical = wrap_reader(writers.read_tar_lp)
read_encoded = wrap_reader(writers.read_tar_encoded)
