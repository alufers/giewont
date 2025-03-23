import argparse
import importlib
from ecs_codegen.cpp_types import generate_header_file
import sys
import os

def main():
    parser = argparse.ArgumentParser(description="Generate ECS boilerplate code")
    parser.add_argument("schema_path", type=str, help="Path to the python file containing the component schema.")

    args = parser.parse_args()
    

    # resolve absolute path
    abs_schema_path = os.path.abspath(args.schema_path)
    print("Adding path to sys.path: ", os.path.dirname(abs_schema_path))
    sys.path.append(os.path.dirname(abs_schema_path))
    module_name = os.path.basename(args.schema_path).split(".")[0]
    module = importlib.import_module(module_name)

    generate_header_file(sys.stdout)

if __name__ == "__main__":
    main()
