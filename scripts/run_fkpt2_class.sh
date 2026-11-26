#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
default_class="$project_dir/../xother_files/github_codes_to-study/class_public_2026-11-25/class"
exec "${PYTHON:-python3}" "$project_dir/scripts/run_fkpt2_class.py" \
    --class-bin "${CLASS_BIN:-$default_class}" --fkpt2-bin "${FKPT2_BIN:-$project_dir/fkpt2}" "$@"
