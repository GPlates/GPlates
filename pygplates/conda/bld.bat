:: NMake cannot locate the MSVC toolchain by itself (the Visual Studio generator can), so make
:: sure vcvars has run. Guarded, not unconditional: conda-build's vs2022 activation already runs
:: vcvars64.bat, and running it a second time appends the whole toolchain to PATH again - which on
:: a machine whose PATH is already long overflows cmd's 8191-character limit and kills the script
:: with nothing more than "The input line is too long". VSCMD_VER is what vcvars sets to say it ran.
::
:: There is no real fallback to put behind the guard: VSINSTALLDIR is set by the same activation
:: script that sets VSCMD_VER, so if VSCMD_VER is missing then VSINSTALLDIR almost certainly is too,
:: and calling "%VSINSTALLDIR%VC\..." would expand to a relative path that does not exist. Say so
:: instead of failing later with "The system cannot find the path specified" or a missing cl.exe.
if not defined VSCMD_VER (
    if not defined VSINSTALLDIR (
        echo ERROR: no MSVC toolchain in the environment - neither VSCMD_VER nor VSINSTALLDIR is set.
        echo        conda-build's vs2022 activation normally provides both.
        exit 1
    )
    call "%VSINSTALLDIR%VC\Auxiliary\Build\vcvarsall.bat" x64
    if errorlevel 1 exit 1
)

:: conda-build's vs2022 activation sets CMAKE_GENERATOR="Visual Studio 17 2022" (plus
:: CMAKE_GENERATOR_PLATFORM=x64 and CMAKE_GENERATOR_TOOLSET=v143). Replace it with NMake, which
:: compiles one file at a time: the parallel Visual Studio build ran out of memory
:: ("C1060: compiler is out of heap space") on conda-forge's 7 GB Windows runners.
:: GPLATES_MSVC_PARALLEL_BUILD=FALSE stops CMake adding /MP for the same reason.
set CMAKE_GENERATOR_PLATFORM=
set CMAKE_GENERATOR_TOOLSET=
set "CMAKE_GENERATOR=NMake Makefiles"

:: Build and install pyGPlates.
::
:: Pip uses the scikit-build-core build backend to compile/install pyGPlates using CMake (see pyproject.toml).
::
:: No CMAKE_PREFIX_PATH / Boost_ROOT defines are needed here: with CONDA_BUILD=1 the root
:: CMakeLists.txt prepends %PREFIX% and %PREFIX%\Library to CMAKE_PREFIX_PATH and points Boost_ROOT
:: at the latter itself (which also keeps CGAL's Boost search inside conda).
%PYTHON% -m pip install -vv ^
      -C cmake.define.GPLATES_MSVC_PARALLEL_BUILD=FALSE ^
      "%SRC_DIR%"
if errorlevel 1 exit 1
