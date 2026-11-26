#!/usr/bin/env python3
"""Exercise the installed wheel, including its native programs, without build tools."""

import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--smoke-source", type=Path, default=Path(__file__).with_name("test_packaging.py"))
    args = parser.parse_args()
    # All commands used by the test are absolute. Hide the compiler, Make and
    # source checkout so an accidental runtime build cannot make this test pass.
    with tempfile.TemporaryDirectory(prefix="fkpt2-wheel-runtime-") as directory:
        root = Path(directory)
        env = os.environ.copy()
        for name in list(env):
            if name == "PYTHONPATH" or name.startswith("FKPT2_"):
                env.pop(name)
        env["PATH"] = str(root / "no-build-tools")
        env["XDG_STATE_HOME"] = str(root / "state")
        # The published sdist contains the same broad API/CLI smoke test.
        # Read only its literal constant; do not import a source-tree extension.
        import ast
        tree = ast.parse(args.smoke_source.read_text())
        body = next(ast.literal_eval(node.value) for node in tree.body
                    if isinstance(node, ast.Assign)
                    and any(isinstance(target, ast.Name) and target.id == "SMOKE" for target in node.targets))
        check = "import fkpt2,sys; from pathlib import Path; " \
                "assert Path(fkpt2.__file__).resolve().is_relative_to(Path(sys.prefix).resolve())\n"
        subprocess.run([sys.executable, "-c", check + body], cwd=root, env=env, check=True)


if __name__ == "__main__":
    main()
