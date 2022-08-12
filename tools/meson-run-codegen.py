#!/usr/bin/env python3

import argparse
from meson_post_processor import post_process
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description="Wrap code generator tool.")
    parser.add_argument("--output-directory", dest="output_dir", required=True,
                        help="directory where output will be written")
    parser.add_argument("--flatten-subdir", dest="flatten_dirs", default=[], action="append",
                        help="declare a single output directory to be flattened")
    parser.add_argument("--link-subdir", dest="link_dirs", default=[], action="append",
                        help="declare a single output directory to be linked/copied")
    parser.add_argument("generator_args", metavar="command", nargs="*",
                        help="generator and arguments")
    args = parser.parse_args()

    output_dir = Path(args.output_dir)
    output_subdirs = [(output_dir / Path(d), "flatten") for d in args.flatten_dirs]
    output_subdirs += [(output_dir / Path(d), "link") for d in args.link_dirs]

    generator_args = args.generator_args
    if len(generator_args) == 0:
        parser.error("missing generator args")

    for d, _ in output_subdirs:
        d.mkdir(parents=True, exist_ok=True)

    subprocess.run(generator_args, check=True)

    post_process(output_dir, output_subdirs)


if __name__ == "__main__":
    main()
