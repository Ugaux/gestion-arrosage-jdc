# generate_config.py

from os.path import join as join_path
from pathlib import Path

import yaml
from config_module import (
    SchemaModel,
    generate_config_h,
    generate_config_reflection_h,
)

SCRIPT_FOLDER = Path(__file__).parent

VERSION = "1.0.0"
INPUT_DIR = join_path(SCRIPT_FOLDER, "schema.yaml")
OUTPUT_DIR = join_path(SCRIPT_FOLDER.parent.parent, "src", "config", "generated")


if __name__ == "__main__":
    output_path = Path(OUTPUT_DIR)
    output_path.mkdir(parents=True, exist_ok=True)

    with open(INPUT_DIR, "r") as f:
        schema = yaml.safe_load(f)
    print(f"YAML input file parsed")

    model = SchemaModel.model_validate(schema)
    print(f"Schema model validated")

    fileGenerators = [
        ["Config.h", generate_config_h(model)],
        ["ConfigReflection.h", generate_config_reflection_h(model)],
    ]
    for gen in fileGenerators:
        filename = gen[0]
        file_content = gen[1]
        (output_path / filename).write_text(file_content)
        print(f"File '{output_path / filename}' generated")
