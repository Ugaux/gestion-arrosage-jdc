# reflection_model.py

"""
- Node — anything in the configuration tree
- Node/Struct — a node containing other nodes
- Node/Field — a leaf/endpoint with a concrete type

Example:
```
NodeModel
    ├── Struct node
    │     └── children: dict[str, NodeModel]
    │
    └── Field node
          ├── type
          ├── default
          ├── constraints
          ├── optional
          └── unit
```

Complete schema:
```
Schema
  │
  ├── types
  │     ├── Zone
  │     ├── Line
  │     └── Schedule
  │
  └── structure
          │
          ├── userSettings                                 Node/Struct
          │        ├── parameters                          Node/Struct
          │        │       ├── network                     Node/Struct
          │        │       │      └── ...
          │        │       └── watering                    Node/Struct
          │        │              └── manual               Node/Struct
          │        │                    └── duration       Node/Struct
          │        │                            ├── min    Field
          │        │                            ├── max    Field
          │        │                            ├── base   Field
          │        │                            └── step   Field
          │        │
          │        └── wateringModel                       Node/Struct
          │                 ├── zones                      Field(Collection)
          │                 └── lines                      Field(Collection)
          │
          └── scheduleStorage                              Field(Collection)
```


"""

from __future__ import annotations

from typing import Any

from pydantic import BaseModel, ConfigDict, Field, model_validator

NameMap = {
    "access-point": ["ap", "AP"],
}

TypeMap = {
    "Bool": "bool",
    "UInt8": "uint8_t",
    "UUID": "UUID",
    "Bitset": "std::bitset",
    "Frequency": "Frequency",
    "WeekDays": "WeekDays",
    "Collection": "Collection",
    "String": "FixedString",
    "Struct": "struct",
}

ConstraintKeys = {
    "length",
    "range",
}


def _to_cpp_type(reflection_type):
    try:
        return TypeMap[reflection_type]
    except KeyError:
        raise ValueError(f"Unknown node type: {reflection_type!r}") from None


class FieldConstraints(BaseModel):
    length: tuple[int | str, int | str] | None = None
    range: tuple[int | str, int | str] | None = None


class NodeModel(BaseModel):
    model_config = ConfigDict(extra="forbid")

    type: str  # no type means that node is a struct

    # only if node is a struct
    cross_action: str | None = Field(default=None, alias="crossAction")
    children: dict[str, NodeModel] = Field(default_factory=dict)

    # only if node is a field
    of: str | None = None
    size: str | None = None
    default: Any = None
    optional: bool = False
    unit: str | None = None
    constraints: FieldConstraints = Field(default_factory=FieldConstraints)

    @model_validator(mode="before")
    @classmethod
    def normalize(cls, value):
        value = dict(value)

        # Has "type" → Field node
        if "type" in value:
            value["constraints"] = {key: value.pop(key) for key in ConstraintKeys if key in value}
            return value

        # No "type" → Struct node (need to extract metadata that is allowed on a struct)
        cross_action = value.pop("crossAction", None)
        return {
            "type": "Struct",
            "crossAction": cross_action,
            "children": value,
        }

    @property
    def is_struct(self) -> bool:
        return self.type == "Struct" and self.children is not None

    @property
    def is_collection(self) -> bool:
        return self.type == "Collection"

    @property
    def is_field(self) -> bool:
        return not self.is_struct

    @property
    def cpp_type(self):
        return _to_cpp_type(self.type)

    @property
    def cpp_template(self):
        match self.cpp_type:

            case "Collection":
                if self.of is None:
                    of = "MISSING_TYPE"
                else:
                    of = self.of
                if self.size is None:
                    size = "MISSING_SIZE"
                else:
                    size = self.size
                return f"<{of}, {size}>"

            case "FixedString":
                if self.constraints.length is None:
                    return "<MISSING_LENGTH>"
                return f"<{self.constraints.length[1]}>"

            case "std::bitset":
                if self.size is None:
                    return "<MISSING_SIZE>"
                return f"<{self.size}>"

        return ""

    @property
    def cpp_type_expression(self):
        return self.cpp_type + self.cpp_template

    @property
    def cpp_default(self):
        if self.default is None:
            return ""

        match self.cpp_type:
            case "bool":
                default_value = "true" if self.default else "false"
            case "FixedString":
                default_value = f'"{self.default}"'
            case _:
                default_value = str(self.default)

        return f" = {default_value}"

    @staticmethod
    def cpp_name(name: str, is_struct: bool):
        if name in NameMap:
            if is_struct:
                return NameMap[name][1]
            return NameMap[name][0]
        else:
            if is_struct:
                name = name[0].upper() + name[1:]
            return name


class ConstantModel(BaseModel):
    type: str
    value: Any

    @property
    def cpp_type(self):
        return _to_cpp_type(self.type)


class SchemaModel(BaseModel):
    model_config = ConfigDict(extra="forbid")

    version: int
    constants: dict[str, ConstantModel] = Field(default_factory=dict)
    types: dict[str, NodeModel] = Field(default_factory=dict)
    structure: NodeModel
