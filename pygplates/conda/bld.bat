:: Build and install pyGPlates.
::
:: Pip uses the scikit-build-core build backend to compile/install pyGPlates using CMake (see pyproject.toml).
::
:: Note that Boost_ROOT helps avoid finding the Boost library using inherited env var PATH
::      (which can reference a Boost outside of conda). Also, CGAL looks for Boost too.
%PYTHON% -m pip install -vv ^
      -C cmake.define.GPLATES_MSVC_PARALLEL_BUILD=TRUE ^
      -C cmake.define.GPLATES_MSVC_PARALLEL_BUILD_PROCESSES=%CPU_COUNT% ^
      -C "cmake.define.CMAKE_PREFIX_PATH=%PREFIX%;%LIBRARY_PREFIX%" ^
      -C "cmake.define.Boost_ROOT=%LIBRARY_PREFIX%" ^
      "%SRC_DIR%"
if errorlevel 1 exit 1
