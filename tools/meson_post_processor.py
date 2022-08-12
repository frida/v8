import platform
import shutil


def post_process(output_dir, output_subdirs):
    output_globs = [
        "*.c",
        "*.cc",
        "*.cpp",
        "*.h",
        "*.inc",
    ]
    can_symlink = platform.system() != "Windows"
    for d, action in output_subdirs:
        for output_glob in output_globs:
            for output_file in d.glob(output_glob):
                subpath = output_file.relative_to(output_dir)
                if action == "flatten":
                    alias = output_dir / "_".join(subpath.parts)
                else:
                    alias = output_dir / output_file.name
                if can_symlink:
                    if not alias.exists():
                        alias.symlink_to(subpath)
                else:
                    if alias.exists():
                        old_contents = alias.read_bytes()
                        new_contents = output_file.read_bytes()
                        dirty = new_contents != old_contents
                    else:
                        dirty = True
                    if dirty:
                        shutil.copyfile(output_file, alias)
