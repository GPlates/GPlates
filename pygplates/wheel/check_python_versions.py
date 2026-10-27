# Checks that the two lists of wheel Python versions agree:
#
#  - the 'build' list in '[tool.cibuildwheel]' in 'pyproject.toml' - the authoritative list,
#    what cibuildwheel builds wheels for;
#  - PYTHON_VERSIONS in 'pygplates/wheel/versions.sh', which repeats it for the Linux image
#    ('manylinux_2_28.dockerfile' builds a Boost.Python library per version).
#
# Nothing else keeps the two in step, and a mismatch is quiet in the worst way: the image simply
# lacks the library for a version, and the wheel build fails much later with a linker error
# naming Boost. So the check runs everywhere a stale list could do damage: in
# '.github/workflows/build-wheel-images.yml' before an image is built and published, and in
# '.github/workflows/build-wheels.yml' (the sdist job, which gates the wheel matrix).
#
# Run from the repository root; exits non-zero on a mismatch. Needs Python >= 3.11 (tomllib).

import pathlib
import sys
import tomllib

build = tomllib.loads(pathlib.Path("pyproject.toml").read_text())["tool"]["cibuildwheel"]["build"]
wheels = [b[2] + "." + b[3:].split("-")[0] for b in build]   # "cp313-*" -> "3.13"

line = next(l for l in pathlib.Path("pygplates/wheel/versions.sh").read_text().splitlines()
            if l.startswith("PYTHON_VERSIONS="))
image = line.split("=", 1)[1].strip().strip('"').split()

if wheels != image:
    print("error: the Python version lists disagree:", file=sys.stderr)
    print("  pyproject.toml [tool.cibuildwheel] build:", " ".join(wheels), file=sys.stderr)
    print("  versions.sh    PYTHON_VERSIONS         :", " ".join(image), file=sys.stderr)
    sys.exit(1)
print("Python versions:", " ".join(wheels))
