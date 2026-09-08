# config_reflection_h.py

from dataclasses import dataclass
from dataclasses import field as dc_field
from textwrap import dedent, indent

from .reflection_model import NodeModel, SchemaModel


@dataclass
class Context:
    """
    Accumulates generated declarations.

    Declarations are emitted bottom-up because a parent FieldDescriptor
    refers to the FieldDescriptor array of its child struct.
    """

    detail_decls: list[str] = dc_field(default_factory=list)
    schema_decls: list[str] = dc_field(default_factory=list)
    counter: dict[str, int] = dc_field(default_factory=dict)

    def unique(self, base: str) -> str:
        n = self.counter.get(base, 0)
        self.counter[base] = n + 1

        if n == 0:
            return base

        return f"{base}_{n}"


def _make_ident(qualified_type: str, name: str = "") -> str:
    ident = qualified_type.replace("::", "")

    if len(name) > 0:
        ident += NodeModel.cpp_name(name, True)

    return ident


def _field_validator_expr(
    node: NodeModel,
    qualified_type: str,
    name: str,
    ctx: Context,
) -> tuple[str, str] | None:
    """Range/Length self-validator for a leaf field, from its constraint."""

    if node.constraints.range is not None:
        lo, hi = node.constraints.range
        kind = "range"
        payload = f".min = {lo}, .max = {hi}"
    elif node.constraints.length is not None:
        lo, hi = node.constraints.length
        kind = "length"
        payload = f".min = {lo}, .max = {hi}"
    else:
        return None

    ident = _make_ident(qualified_type, name)
    validator_name = ctx.unique(f"{ident}_FieldValidator")

    instance_name = NodeModel.cpp_name(name, False)

    expr = dedent(f"""\
        inline constexpr Validation::FieldValidator
          {validator_name}{{
            &Validation::{kind}Adapter<
              decltype({qualified_type}::{instance_name})>,
            Validation::FieldValidator::{kind.capitalize()}Data{{
              {payload} }},
          }};""").rstrip()

    return f"&kReflection::{validator_name}", expr


def _cross_action_expr(
    node: NodeModel,
    qualified_type: str,
    ctx: Context,
) -> tuple[str, str] | None:
    if node.cross_action is None:
        return None

    ident = _make_ident(qualified_type)
    validator_name = ctx.unique(f"{ident}_CrossValidator")

    expr = dedent(f"""\
        inline constexpr Validation::CrossAction
          {validator_name}{{
            &Validation::crossAdapter<
              {qualified_type},
              Validation::CrossFn::{node.cross_action}>
          }};""").rstrip()

    return f"&kReflection::{validator_name}", expr


def _render_make_field(
    node: NodeModel,
    qualified_type: str,
    name: str,
    field_validator: str | None = None,
) -> str:
    instance_name = NodeModel.cpp_name(name, False)

    args = [
        f"&{qualified_type}::{instance_name}",
        f'"{name}"',  # Name used to access this field in the config file format
    ]

    args.append(field_validator if field_validator is not None else "nullptr")
    args.append(f'"{node.unit}"' if node.unit is not None else '""')
    args.append("true" if node.optional else "false")

    return "makeField(\n" + ",\n".join(indent(arg, "  ") for arg in args) + ")"


def render_schema(
    node: NodeModel,
    qualified_type: str,
    ctx: Context,
):
    fields: list[str] = []
    validator_decls: list[str] = []

    cross_validator = _cross_action_expr(
        node,
        qualified_type,
        ctx,
    )

    if cross_validator is None:
        cross_validator_name = "nullptr"
    else:
        cross_validator_name, cross_validator_decl = cross_validator
        validator_decls.append(cross_validator_decl)

    for name, child in node.children.items():
        field_validator_name = None

        if not child.is_struct:
            field_validator = _field_validator_expr(
                child,
                qualified_type,
                name,
                ctx,
            )

            if field_validator is not None:
                field_validator_name, field_validator_decl = field_validator
                validator_decls.append(field_validator_decl)

        fields.append(
            _render_make_field(
                child,
                qualified_type,
                name,
                field_validator_name,
            )
        )

    if validator_decls:
        validator_decls_body = dedent("\n\n".join(validator_decls))
        ctx.detail_decls.append(
            dedent("""\
                // ============================================================
                // {qualified_type}
                // ============================================================
    
                {decl}""")
            .format(
                decl=validator_decls_body,
                qualified_type=qualified_type,
            )
            .rstrip()
        )

    fields_body = indent(",\n\n".join(fields), "    ")

    ctx.schema_decls.append(
        dedent("""\
            template<>
            struct Schema<{qualified_type}> {{
              static constexpr bool reflected = true;
    
              static constexpr auto fields = std::tuple{{
            {fields},
              }};
              
              static constexpr const Validation::CrossAction*
                crossAction = {cross_validator};
            }};""")
        .format(
            qualified_type=qualified_type,
            fields=fields_body,
            cross_validator=cross_validator_name,
        )
        .rstrip()
    )


def render_nodes(
    nodes: dict[str, NodeModel],
    qualified_type: str,
    ctx: Context,
) -> None:
    for name, node in nodes.items():
        if not node.is_struct:
            continue

        struct_name = NodeModel.cpp_name(name, True)
        child_qualified_type = f"{qualified_type}::{struct_name}"

        render_node(
            node,
            child_qualified_type,
            ctx,
        )


def render_node(
    node: NodeModel,
    qualified_type: str,
    ctx: Context,
) -> None:
    # First generate children
    render_nodes(
        node.children,
        qualified_type,
        ctx,
    )

    # Then generate fields for this node
    render_schema(
        node,
        qualified_type,
        ctx,
    )


def render_header_body(
    schema: SchemaModel,
) -> tuple[str, str]:
    ctx = Context()

    # Named reusable types:
    # Config::Zone,
    # Config::Line,
    # Config::Schedule,
    # ...
    render_nodes(
        schema.types,
        "Config",
        ctx,
    )

    # Root structure is the Config body itself
    render_node(
        schema.structure,
        "Config",
        ctx,
    )

    return (
        "\n\n".join(ctx.detail_decls),
        "\n\n".join(ctx.schema_decls),
    )


def render_header(
    schema: SchemaModel,
) -> str:
    detail_body, schema_body = render_header_body(schema)

    header = dedent("""\
        #pragma once

        #include <tuple>
        #include "config/generated/Config.h"
        #include "config/runtime/Reflection.h"
        #include "config/runtime/Validation.h"

        namespace kReflection {{

        inline constexpr uint8_t SchemaVersion = {version};

        {detail_body}

        }}  // namespace kReflection

        namespace Reflection {{

        {schema_body}

        }}  // namespace Reflection
        """)

    return header.format(
        version=schema.version,
        detail_body=detail_body,
        schema_body=schema_body,
    )
