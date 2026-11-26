"""Render the canonical CLI parameter definitions without running the solver."""

import csv
from pathlib import Path
import re

from docutils import nodes
from sphinx.util.docutils import SphinxDirective


PARAMETER = re.compile(r"^PARAMETER(?:_ALIAS)?\((.*?)\)$", re.MULTILINE | re.DOTALL)


def parameter_rows(path):
    rows = {}
    for arguments in PARAMETER.findall(path.read_text(encoding="utf-8")):
        _, name, default, help_text, *aliases = next(csv.reader(
            [" ".join(arguments.splitlines())], skipinitialspace=True
        ))
        alias = aliases[0] if aliases else None
        if name in rows:
            # The schema defines this parameter in both build branches.
            if name != "numberThreads":
                raise ValueError(f"Multiple definitions for {name}")
            omp_default, _, _ = rows[name]
            rows[name] = (f"{default} (serial), {omp_default} (OpenMP)", alias, help_text)
        else:
            rows[name] = (default or '""', alias, help_text)
    if not rows:
        raise ValueError(f"No runtime parameters found in {path}")
    for name in sorted(rows, key=str.casefold):
        yield name, *rows[name]


def table_row(values, literal_columns=0):
    row = nodes.row()
    for column, value in enumerate(values):
        paragraph = nodes.paragraph()
        if value is not None:
            paragraph += nodes.literal(text=value) if column < literal_columns else nodes.Text(value)
        entry = nodes.entry()
        entry += paragraph
        row += entry
    return row


class ParameterTable(SphinxDirective):
    def run(self):
        schema = Path(self.env.srcdir).parent / "include" / "run_parameters.def"
        self.env.note_dependency(str(schema))

        table = nodes.table()
        group = nodes.tgroup(cols=4)
        for width in (24, 21, 12, 43):
            group += nodes.colspec(colwidth=width)
        header = nodes.thead()
        header += table_row(("Parameter", "Default", "Alias", "Meaning"))
        group += header
        body = nodes.tbody()
        for row in parameter_rows(schema):
            body += table_row(row, literal_columns=3)
        group += body
        table += group
        return [table]


def setup(app):
    app.add_directive("fkpt2-parameters", ParameterTable)
    return {"parallel_read_safe": True, "parallel_write_safe": True}
