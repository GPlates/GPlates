:: Build and install pyGPlates.
::
:: Pip uses the scikit-build-core build backend to compile/install pyGPlates using CMake (see pyproject.toml).
::
:: Note: the top-level CMakeLists.txt auto-detects the conda 'host' prefix (via the PREFIX env var set
::       during 'conda build') and points CMake's Boost search there, so Boost_ROOT no longer needs to be
::       passed explicitly - this avoids finding a Boost outside of conda via an inherited BOOST_ROOT/PATH.
::       (CGAL, which also looks for Boost, benefits from the same setting.)
%PYTHON% -m pip install -vv ^
      -C cmake.define.GPLATES_MSVC_PARALLEL_BUILD=TRUE ^
      -C cmake.define.GPLATES_MSVC_PARALLEL_BUILD_PROCESSES=%CPU_COUNT% ^
      "%SRC_DIR%"
if errorlevel 1 exit 1
