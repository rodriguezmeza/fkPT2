"""Build the Cython interface against the same native library as the CLI."""

import os
from pathlib import Path
import shlex
import subprocess
import sys
import sysconfig

from Cython.Build import cythonize
from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext


ROOT = Path(__file__).resolve().parent


class BuildNativeExtension(build_ext):
    def build_extensions(self):
        workdir = Path(self.build_temp).resolve() / "fkpt2-native"
        library = workdir / "libfkpt2.a"
        platform_options = []
        if sys.platform == "darwin":
            target = (os.environ.get("MACOSX_DEPLOYMENT_TARGET")
                      or sysconfig.get_config_var("MACOSX_DEPLOYMENT_TARGET"))
            if target:
                platform_options.append(f"MACOSX_DEPLOYMENT_TARGET={target}")
        command = [
            *shlex.split(os.environ.get("MAKE", "make")),
            "-s", "--no-print-directory", *platform_options,
            *shlex.split(os.environ.get("FKPT2_MAKE_ARGS", "")),
            f"WRKDIR={workdir}", "FKPT2_PYTHON_BUILD=1",
            "python-build-env",
        ]
        # The nested make can use the parent make's jobserver when present.
        proc = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, text=True,
                              close_fds=False)
        if proc.returncode != 0:
            print(proc.stdout)
            proc.check_returncode()

        marker = "FKPT2_LINK_FLAGS="
        link_flags = next(line[len(marker):] for line in proc.stdout.splitlines()
                          if line.startswith(marker))
        for extension in self.extensions:
            extension.extra_objects = [str(library)]
            extension.extra_link_args = shlex.split(link_flags)
            extension.depends = [*extension.depends, str(library)]

        super().build_extensions()


setup(
    package_dir={"": "python"},
    py_modules=["fkpt2_class"],
    ext_modules=cythonize(
        [Extension("fkpt2", ["python/fkpt2.pyx"], include_dirs=["include"],
                   depends=["include/fkpt2.h", "include/fkpt2_class_provider.h"])],
        build_dir="build/cython",
        compiler_directives={"language_level": 3},
    ),
    cmdclass={"build_ext": BuildNativeExtension},
)
