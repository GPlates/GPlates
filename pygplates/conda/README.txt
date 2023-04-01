Conda Readme
============

Some notes/references on conda building and conda-forge.

meta.yaml
---------

CDT packages should be in 'build' section (instead of 'host').
For example, this fixes inability of CMake to find GL library at build time. On Ubuntu 22.04, the GL library
was then found in the sysroot (at '<build_prefix>/x86_64-conda-linux-gnu/sysroot/usr/lib/libGL.so').
Also CDT packages are not installed at runtime (only build-time).

Libraries in the 'host' section (for linking at build time) should get automatically propagated to 'run' libraries
(for linking at run time) if these libraries specified 'run_exports' in their packages.
So we shouldn't also need to specify them in our 'run' requirements - however it seems we still need to.

NumPy is a dependency since pygplates optionally uses the NumPy C API when available (to allow users to specify numpy ints/floats where Python ints/floats are accepted in pygplates).
There are build conflicts when building "locally" (eg, with "conda build ...") that are not present for automated "conda-forge" builds.
When conda-forge re-renders the pygplates feedstock it currently specifies numpy version 1.20 for Python 3.8/3.9, 1.21 for 3.10 and 1.23 for 3.11.
However "local" builds with Python 3.10, for example, result in a conflict with the default NumPy version 1.16 (at the time of writing).
The current solution (until the default version is bumped up in the conda-build tool) is provided in the local "conda_build_config.yaml" file, which contains:
    python:
    - 3.8
    - 3.9
    - 3.10
    - 3.11
    numpy:
    - 1.20
    - 1.20
    - 1.21
    - 1.23
    zip_keys:
    - python
    - numpy
This ensures the correct NumPy version is used for each Python version (to avoid build conflicts).
However, that section of "conda_build_config.yaml" should not be committed to the feedstock because conda-forge re-renders globally pinned versions (eg, NumPy) into our feedstock for us.
Specifying, eg, "--numpy 1.21" on the conda-build command-line is another option for local builds.


References
----------

Reason for CDT - https://github.com/conda/conda-build/issues/3696#issuecomment-416001186
Seems very similar to reason for manylinux.

Linking to OpenGL - https://github.com/conda-forge/pygridgen-feedstock/issues/10#issuecomment-365983589

ignore_run_exports warnings - https://github.com/conda/conda-build/issues/3494#issuecomment-489214319

Copying pygplates (standalone or non-standalone) bundle using MANIFEST.in is currently done using this - https://stackoverflow.com/a/39823635

Setuptools command classes - https://setuptools.pypa.io/en/latest/userguide/extension.html#extending-or-customizing-setuptools

How do I fix the libGL.so.1 import error?
  Traceback (most recent call last):
    File "/home/conda/staged-recipes/build_artifacts/pygplates_1677067701708/test_tmp/run_test.py", line 2, in <module>
      import pygplates
    File "/home/conda/staged-recipes/build_artifacts/pygplates_1677067701708/_test_env_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placehold_placeh/lib/python3.8/site-packages/pygplates/__init__.py", line 2, in <module>
      from .pygplates import *
  ImportError: libGL.so.1: cannot open shared object file: No such file or directory
- https://conda-forge.org/docs/maintainer/maintainer_faq.html#mfaq-libgl-so-1
- https://conda-forge.org/docs/maintainer/knowledge_base.html#libgl

Building against NumPy (need to use 'pin_compatible' when using NumPy C API) - https://conda-forge.org/docs/maintainer/knowledge_base.html#building-against-numpy

Conda-forge transfers globally pinned versions to a feedstock (eg, transferring NumPy versions) - https://conda-forge.org/docs/maintainer/pinning_deps.html#globally-pinned-packages

Python 3.10 is not compatible with NumPy 1.16 according to https://github.com/conda/conda-build/commit/daeda52d75323f92af29fb91002b1f683c5606ab, which says:
  "Python 3.10 and numpy 1.16 are not compatible so to move forward with supporting Python 3.10 and soon 3.11 we will need to also bump up the default numpy variant."
Note that they bumped it up to 1.21 in conda-build, but that was only in December 2022 (3 months ago), so hasn’t made it into the current version (3.23.3) of conda-build.

Avoid overlinking errors. Eg, on Windows we need 'mpir' instead of 'gmp'. It's used by CGAL but we apparently link directly to it (probably since CGAL is now a header-only library).
  ERROR (pygplates,Lib/site-packages/pygplates/pygplates.pyd): Needed DSO Library/bin/mpir.dll found in ['mpir']
  ERROR (pygplates,Lib/site-packages/pygplates/pygplates.pyd): .. but ['mpir'] not in reqs/run, (i.e. it is overlinking) (likely) or a missing dependency (less likely)
