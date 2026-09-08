# config_h.py

from dataclasses import dataclass
from textwrap import dedent

from .reflection_model import (
    ConstantModel,
    NodeModel,
    SchemaModel,
)


@dataclass
class RenderOptions:
    top_level: bool = False
    is_type: bool = False
    indent: int = 2


def render_constants(
    constants: dict[str, ConstantModel],
) -> str:
    rendered: list[str] = []

    for name, constant in constants.items():
        cpp_type = constant.cpp_type
        value = constant.value
        if isinstance(value, str):
            value = f'"{value}"'
        rendered.append(f"  static constexpr {cpp_type} {name} = {value};")

    return "\n".join(rendered)


def render_node(
    name: str,
    node: NodeModel,
    options: RenderOptions,
) -> list[str]:
    pad = " " * options.indent
    struct_name = NodeModel.cpp_name(name, True)
    instance_name = NodeModel.cpp_name(name, False)

    if node.is_struct:
        body = render_nodes(node.children, RenderOptions(indent=options.indent + 2))
        closing = (" " + instance_name) if not options.is_type else ""
        lines = [
            f"{pad}{node.cpp_type} {struct_name if not options.is_type else name} {{",
            f"{body}",
            f"{pad}}}{closing};",
        ]

    elif node.is_collection:
        if node.of is None:
            raise ValueError(f"Collection field {name!r} is missing the 'of:' field")

        alias = f"{node.of}{node.type}"
        lines = [
            f"{pad}using {alias} = {node.cpp_type_expression};",
            f"{pad}{alias} {instance_name}{node.cpp_default};",
        ]

    else:
        unit_comment = f"  // in {node.unit}" if node.unit else ""
        lines = [f"{pad}{node.cpp_type_expression} {instance_name}{node.cpp_default};{unit_comment}"]

    return lines


def render_nodes(
    nodes: dict[str, NodeModel],
    options: RenderOptions,
) -> str:
    lines: list[str] = []

    for name, node in nodes.items():
        if options.top_level or node.is_struct:
            lines.append("")

        rendered = render_node(name, node, options)
        lines.extend(rendered)

    return "\n".join(lines)


def render_header_body(
    schema: SchemaModel,
) -> str:
    return "\n".join(
        [
            # Constants
            render_constants(schema.constants),
            # Named types
            render_nodes(
                schema.types,
                RenderOptions(top_level=True, is_type=True),
            ),
            # Root structure is the Config body itself
            render_nodes(
                schema.structure.children,
                RenderOptions(top_level=True),
            ),
        ]
    )


def render_header(
    schema: SchemaModel,
) -> str:
    body = render_header_body(schema)

    header = dedent("""\
        #pragma once

        #include <bitset>
        #include <array>
        #include <cstdint>
        #include "Constants.h"
        #include "types/Collection.h"
        #include "types/FixedString.h"
        #include "types/UUID.h"
        #include "types/Frequency.h"
        #include "types/Weekdays.h"

        struct Config {{
        {body}
        }};
        """)

    return header.format(
        body=body,
    )
