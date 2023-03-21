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

Avoid overlinking errors. Eg, on Windows we need 'mpir' instead of 'gmp'. It's used by CGAL but we apparently link directly to it (probably since CGAL is now a header-only library).
  ERROR (pygplates,Lib/site-packages/pygplates/pygplates.pyd): Needed DSO Library/bin/mpir.dll found in ['mpir']
  ERROR (pygplates,Lib/site-packages/pygplates/pygplates.pyd): .. but ['mpir'] not in reqs/run, (i.e. it is overlinking) (likely) or a missing dependency (less likely)
