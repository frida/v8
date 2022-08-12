#!/usr/bin/env python3

import argparse
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description="Prepare output directories for code generator tool.")
    parser.add_argument("--output-directory", dest="output_dir", required=True,
                        help="directory where output will be written")
    parser.add_argument("--prepare-subdir", dest="output_subdirs", default=[], action="append",
                        help="prepare a specific output subdir")
    parser.add_argument("--touch-stamp-file", dest="stamp_file", required=True,
                        help="stamp file to touch")
    args = parser.parse_args()

    output_dir = Path(args.output_dir)
    output_subdirs = [output_dir / Path(d) for d in args.output_subdirs]
    stamp_file = Path(args.stamp_file)

    for d in output_subdirs:
        d.mkdir(parents=True, exist_ok=True)

    stamp_file.touch()


if __name__ == "__main__":
    main()
