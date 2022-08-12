#!/usr/bin/env python3

import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description="Post-process executable to prepare it for installation.")
    parser.add_argument("--input-file", dest="input_path", required=True,
                        help="executable to use as input")
    parser.add_argument("--output-file", dest="output_path", required=True,
                        help="where to write the result")
    parser.add_argument("--strip-option", dest="strip_option", required=True, choices=["true", "false"],
                        help="whether to strip the executable")
    parser.add_argument("strip_tool", nargs="+",
                        help="strip tool, including any arguments to it")
    args = parser.parse_args()

    input_path = Path(args.input_path)
    output_path = Path(args.output_path)

    strip_tool = args.strip_tool
    if strip_tool[0] == "":
        strip_tool = None

    strip_requested = args.strip_option == "true"

    intermediate_path = None
    with input_path.open(mode="rb") as input_file:
        with tempfile.NamedTemporaryFile(delete=False) as temp_file:
            shutil.copyfileobj(input_file, temp_file)
            intermediate_path = Path(temp_file.name)

    try:
        if strip_tool is not None and strip_requested:
            subprocess.run(strip_tool + [intermediate_path], check=True)
    except:
        intermediate_path.unlink()
        raise

    shutil.move(intermediate_path, output_path)


if __name__ == "__main__":
    main()
