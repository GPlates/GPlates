#!/usr/bin/env python
"""
Build the pyGPlates Python API documentation with Sphinx.

This is the single implementation of the docs build. It works against either source of the
pygplates module:

  * a CMake build tree  - pass '--module-dir' (used by the 'doc-python-api' CMake target, and by
                          CI for the 'dev' documentation, where no wheel exists)
  * an installed wheel  - omit '--module-dir' (used for released versions, so the documentation
                          describes the artifact users actually 'pip install')

Both converge on the same requirement: the directory *containing* the pygplates extension module
must be on 'sys.path'.

    WHY THAT MATTERS: importing the pygplates *package* gives every class a '__module__' of
    'pygplates.pygplates' (the extension lives inside the package - see
    'cmake/modules/Install.cmake'), and Sphinx builds each class's "Bases:" cross-reference from
    '__module__'. Those references would then point at 'pygplates.pygplates.X' targets that no
    page defines. The rendered text would be unchanged, but every "Bases:" link would silently
    stop resolving - and a missing Python cross-reference is not a warning, so even Sphinx's '-W'
    would not catch it. Importing the extension directly gives '__module__' of 'pygplates'.

    This is also why '--module-dir' here means the same thing it means to 'generate_stub.py' - the
    *parent* of the built 'pygplates' package directory - even though the two tools then put
    different directories on 'sys.path'. The stub describes the package; the documentation
    describes the flat module.

The steps mirror 'doc-python-api/CMakeLists.txt': copy the '.rst' sources, images and '_static'
into a scratch directory (so the stub '.rst' files that 'autosummary' generates never land in the
source tree), generate 'conf.py' from 'conf.py.in', then run Sphinx.

Usage:
    build_docs.py --output <html-dir> [--module-dir <dir>] [--scratch-dir <dir>] [--incremental]

    --module-dir   parent of the built 'pygplates' package directory (ie,
                   "$<TARGET_FILE_DIR:pygplates>/.."). Omit to use an installed pygplates.
    --output       where to write the HTML (default '<scratch-dir>/html')
    --scratch-dir  Sphinx source/working directory (default: a temporary directory)
    --incremental  keep the doctree and generated-stub caches (see the warning below)

By default the doctree and 'generated/' caches are deleted first. Do not pass '--incremental' for
anything you intend to publish: an incremental rebuild produces HTML identical to a clean build
except for 'searchindex.js', where it silently drops *every* index entry.
"""

import argparse
import importlib.util
import os
import re
import shutil
import sys
import tempfile
from pathlib import Path

# 'doc-python-api/' - this script lives beside 'conf.py.in' and the '.rst' sources.
SOURCE_DIR = Path(__file__).resolve().parent
REPO_ROOT = SOURCE_DIR.parent


def find_package_dir(module_dir):
    """Return the directory containing the pygplates extension module.

    With 'module_dir' (a build tree) that is its 'pygplates' sub-directory; without one, it is the
    installed package's directory. Either way the caller puts the result on 'sys.path' to get the
    flat 'import pygplates' the documentation needs.
    """
    if module_dir is not None:
        package_dir = Path(module_dir).resolve() / "pygplates"
        if not package_dir.is_dir():
            raise SystemExit(
                f"error: no 'pygplates' package directory in --module-dir '{module_dir}'.\n"
                f"       --module-dir is the *parent* of the built package (the same value\n"
                f"       'generate_stub.py' takes), ie \"$<TARGET_FILE_DIR:pygplates>/..\"."
            )
    else:
        spec = importlib.util.find_spec("pygplates")
        if spec is None or spec.origin is None:
            raise SystemExit(
                "error: pygplates is not installed in this interpreter and --module-dir was not\n"
                "       given. Either 'pip install pygplates' or pass --module-dir <build-dir>/bin."
            )
        package_dir = Path(spec.origin).resolve().parent

    # Confirm the extension really is here - if it is not, the import below would silently pick up
    # the package instead and every "Bases:" link would break without any warning.
    extensions = list(package_dir.glob("pygplates.*.so")) + \
        list(package_dir.glob("pygplates.pyd")) + list(package_dir.glob("pygplates.so"))
    if not extensions:
        raise SystemExit(
            f"error: no pygplates extension module ('pygplates.pyd' / '.so') in '{package_dir}'.\n"
            f"       Without it Sphinx would import the package and silently produce broken\n"
            f"       'Bases:' cross-references."
        )
    return package_dir


def import_pygplates(package_dir):
    """Import pygplates *flat* from 'package_dir' and return the module."""
    if "pygplates" in sys.modules:
        raise SystemExit("error: pygplates was already imported before sys.path was set up.")
    sys.path.insert(0, str(package_dir))
    import pygplates  # noqa: E402  (deliberately after the sys.path insert)

    if pygplates.__file__ is None or Path(pygplates.__file__).parent != package_dir:
        raise SystemExit(
            f"error: 'import pygplates' resolved to '{pygplates.__file__}', not the extension in\n"
            f"       '{package_dir}'. Another pygplates is shadowing it on sys.path."
        )
    return pygplates


def read_copyright():
    """Extract PYGPLATES_DOCS_COPYRIGHT_STRING from 'cmake/modules/ConfigDefault.cmake'.

    Parsed rather than duplicated so the copyright has one home.
    """
    config = (REPO_ROOT / "cmake" / "modules" / "ConfigDefault.cmake").read_text(encoding="utf-8")
    match = re.search(
        r"set\(\s*PYGPLATES_DOCS_COPYRIGHT_STRING\s*\[\[(.*?)\]\]\s*\)", config, re.DOTALL
    )
    if match is None:
        raise SystemExit(
            "error: could not find PYGPLATES_DOCS_COPYRIGHT_STRING in ConfigDefault.cmake."
        )
    # A CMake bracket argument drops the single newline that immediately follows '[[' and keeps
    # everything after it - including the trailing newline, which Sphinx renders. Stripping both
    # would silently join the last copyright line to the sentence that follows it.
    text = match.group(1)
    return text[1:] if text.startswith("\n") else text


def split_version(release):
    """Split a PEP 440 version into ('major.minor', release), as 'conf.py.in' expects.

    Mirrors 'cmake/modules/Version.cmake': 'version' is the short X.Y form and 'release' is the
    full PEP 440 string.
    """
    match = re.match(r"^(\d+)\.(\d+)\.(\d+)", release)
    if match is None:
        raise SystemExit(f"error: cannot parse a PEP 440 version from '{release}'.")
    return f"{match.group(1)}.{match.group(2)}", release


def copy_rst(source, destination):
    """Copy a '.rst' file, ensuring it ends with a newline.

    CMake's configure_file() appends a trailing newline when the source lacks one (three of the
    sample-code files do). Matching that keeps this script's output identical to the CMake
    target's, including the '_sources/*.rst.txt' copies Sphinx publishes for download. The file's
    own line ending is reused, which is what configure_file writes on each platform.
    """
    data = source.read_bytes()
    if data and not data.endswith(b"\n"):
        data += b"\r\n" if b"\r\n" in data else b"\n"
    destination.write_bytes(data)


def populate_scratch_dir(scratch_dir):
    """Copy the '.rst' sources, images and '_static' into the Sphinx source directory.

    'doc-python-api/CMakeLists.txt' uses configure_file(@ONLY) for the '.rst' files, but none of
    them contain @VAR@ placeholders, so a copy plus configure_file's trailing-newline behaviour is
    equivalent.
    """
    for rst in sorted(SOURCE_DIR.glob("*.rst")):
        copy_rst(rst, scratch_dir / rst.name)

    for subdir, pattern in (("sample-code", "*.rst"), ("images", "*.png"), ("_static", "*")):
        source_subdir = SOURCE_DIR / subdir
        if not source_subdir.is_dir():
            continue
        (scratch_dir / subdir).mkdir(exist_ok=True)
        for path in sorted(source_subdir.glob(pattern)):
            if not path.is_file():
                continue
            if path.suffix == ".rst":
                copy_rst(path, scratch_dir / subdir / path.name)
            else:
                shutil.copy2(path, scratch_dir / subdir / path.name)


def write_conf_py(scratch_dir, package_dir, copyright_string, version, release):
    """Generate 'conf.py' from 'conf.py.in'.

    Substitutes the four @VAR@ values the CMake target substitutes, and resolves the
    '$<TARGET_FILE_DIR:pygplates>' generator expression to the real package directory.
    """
    conf = (SOURCE_DIR / "conf.py.in").read_text(encoding="utf-8")

    static_dir = scratch_dir / "_static"
    substitutions = {
        "@PYGPLATES_DOCS_COPYRIGHT_STRING@": copyright_string,
        "@PYGPLATES_VERSION_MAJOR@": version.split(".")[0],
        "@PYGPLATES_VERSION_MINOR@": version.split(".")[1],
        "@PYGPLATES_VERSION_RELEASE@": release,
        "@SPHINX_STATIC_DIR@": str(static_dir).replace("\\", "/"),
    }
    for placeholder, value in substitutions.items():
        conf = conf.replace(placeholder, value)

    # The generator expression is quoted in 'conf.py.in'; replace it quotes and all so the path is
    # emitted as a correctly-escaped Python literal.
    generator_expression = '"$<TARGET_FILE_DIR:pygplates>"'
    if generator_expression not in conf:
        raise SystemExit(
            f"error: {generator_expression} not found in conf.py.in - the sys.path line changed;\n"
            f"       this script must be updated to match."
        )
    conf = conf.replace(generator_expression, repr(str(package_dir)))

    remaining = re.findall(r"@[A-Za-z_0-9]+@", conf)
    if remaining:
        raise SystemExit(f"error: unsubstituted placeholders in conf.py: {sorted(set(remaining))}")

    (scratch_dir / "conf.py").write_text(conf, encoding="utf-8")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--module-dir", help="parent of the built 'pygplates' package directory; "
                                             "omit to use an installed pygplates")
    parser.add_argument("--output", help="directory to write the HTML into")
    parser.add_argument("--scratch-dir", help="Sphinx source/working directory")
    parser.add_argument("--incremental", action="store_true",
                        help="keep the doctree and generated-stub caches (drops search index "
                             "entries - never use for published documentation)")
    args = parser.parse_args(argv)

    package_dir = find_package_dir(args.module_dir)
    pygplates = import_pygplates(package_dir)

    release = getattr(pygplates, "__version__", None)
    if not release:
        raise SystemExit("error: the imported pygplates has no __version__.")
    version, release = split_version(release)

    temp_dir = None
    if args.scratch_dir:
        scratch_dir = Path(args.scratch_dir).resolve()
        scratch_dir.mkdir(parents=True, exist_ok=True)
    else:
        temp_dir = tempfile.mkdtemp(prefix="pygplates-docs-")
        scratch_dir = Path(temp_dir)

    html_dir = Path(args.output).resolve() if args.output else scratch_dir / "html"
    doctrees_dir = scratch_dir / "_doctrees"

    if not args.incremental:
        # An incremental rebuild silently drops every 'searchindex.js' index entry.
        for stale in (doctrees_dir, scratch_dir / "generated"):
            shutil.rmtree(stale, ignore_errors=True)

    print(f"pygplates {release} from {package_dir}")
    print(f"source    {scratch_dir}")
    print(f"output    {html_dir}")

    populate_scratch_dir(scratch_dir)
    write_conf_py(scratch_dir, package_dir, read_copyright(), version, release)

    try:
        from sphinx.cmd.build import main as sphinx_main
    except ImportError:
        raise SystemExit(
            "error: sphinx is not installed in this interpreter.\n"
            "       It must be the *same* interpreter pygplates was built against, because\n"
            "       autodoc imports the module in-process. Try:\n"
            "           conda env update -n gplates -f env.docs.yml"
        )

    # '-W' (warnings are errors) and '-j auto' match the CMake target.
    status = sphinx_main(["-q", "-b", "html", "-W", "-j", "auto",
                          "-d", str(doctrees_dir), str(scratch_dir), str(html_dir)])

    if status == 0:
        print(f"\nBuilt {html_dir / 'index.html'}")
        if temp_dir is not None:
            print(f"Note: scratch directory {temp_dir} left in place (contains conf.py and "
                  f"generated .rst).")
    return status


if __name__ == "__main__":
    sys.exit(main())
