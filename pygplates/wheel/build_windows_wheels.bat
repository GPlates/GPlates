@echo off

rem Make sure variables are expanded each time command using them is encountered.
setlocal EnableDelayedExpansion

rem Change to the root directory of the pyGPlates source code.
cd ..\.. || exit /B 1

for %%v in (3.8 3.9 3.10 3.11 3.12 3.13) do (

	rem Create and activate a Python virtual environment.
	rem
	rem But first remove virtual environment (if leftover from a failed run).
	if exist venv_py%%v\NUL (
		rmdir venv_py%%v /s /q || exit /B 1
	)
	py -%%v -m venv venv_py%%v || exit /B 1
	call venv_py%%v\Scripts\activate.bat || exit /B 1

	rem Upgrade pip (and wheel).
	python -m pip install --upgrade pip wheel || exit /B 1

	rem Temporary directory to store built wheel.
	set tmp_dist_dir=_dist_py%%v
	if exist !tmp_dist_dir!\NUL (
		rem Remove directory if exists (eg, due to previous cleanup error).
		rmdir !tmp_dist_dir! /s /q || exit /B 1
	)
	mkdir !tmp_dist_dir! || exit /B 1

	rem Build a wheel for the current Python version (and store the wheel in the temporary dist directory).
	rem
	rem Note: We set the CMake variable GPLATES_INSTALL_STANDALONE_SHARED_LIBRARY_DEPENDENCIES to FALSE since
	rem       we don't want to install shared library dependencies into the wheel - they will get installed
	rem       when 'delvewheel' is subsequently run to copy them into our wheel.
	rem       Note that this variable is only used if GPLATES_INSTALL_STANDALONE is TRUE, which it is
	rem       by default when building using scikit-build-core (eg, 'pip wheel ...') outside of conda.
	rem
	python -m pip wheel ^
		--wheel-dir !tmp_dist_dir! ^
		-v ^
		--config-settings cmake.define.GPLATES_INSTALL_STANDALONE_SHARED_LIBRARY_DEPENDENCIES=FALSE ^
		. ^
		|| exit /B 1

	rem Install delvewheel to copy pyGPlates shared library dependencies into the wheel.
	python -m pip install delvewheel || exit /B 1

	rem Temporary directory to store delvewheel wheel.
	set tmp_wheelhouse_dir=_wheelhouse_py%%v
	if exist !tmp_wheelhouse_dir!\NUL (
		rem Remove directory if exists (eg, due to previous cleanup error).
		rmdir !tmp_wheelhouse_dir! /s /q || exit /B 1
	)
	mkdir !tmp_wheelhouse_dir! || exit /B 1

	rem Use delvewheel to copy pyGPlates shared library dependencies into the wheel just built.
	rem
	rem Note: There's only one wheel to repair, but we can't do wildcard expansion in regular commands
	rem       (in a Windows batch file). However we can do wildcard expansion in a 'for' loop.
	for %%w in (!tmp_dist_dir!\pygplates*.whl) do (
		python -m delvewheel repair -w !tmp_wheelhouse_dir! %%w || exit /B 1
	)

	rem Install the delvewheel wheel.
	rem
	rem Note: There's only one wheel to install, but we can't do wildcard expansion in regular commands
	rem       (in a Windows batch file). However we can do wildcard expansion in a 'for' loop.
	for %%w in (!tmp_wheelhouse_dir!\pygplates*.whl) do (
		python -m pip install %%w || exit /B 1
	)

	rem Test the delvewheel wheel.
	python pygplates\test\test.py || exit /B 1

	rem Copy the delvewheel wheel to the 'wheelhouse' directory.
	if not exist wheelhouse\NUL (
		mkdir wheelhouse || exit /B 1
	)
	rem Note: There's only one wheel to copy, but we can't do wildcard expansion in regular commands
	rem       (in a Windows batch file). However we can do wildcard expansion in a 'for' loop.
	for %%w in (!tmp_wheelhouse_dir!\pygplates*.whl) do (
		copy /y /b %%w wheelhouse || exit /B 1
	)

	rem Remove the temporary dist and wheelhouse directories.
	rmdir !tmp_dist_dir! /s /q || exit /B 1
	rmdir !tmp_wheelhouse_dir! /s /q || exit /B 1

	rem Deactivate and remove the virtual environment.
	call venv_py%%v\Scripts\deactivate.bat || exit /B 1
	rmdir venv_py%%v /s /q || exit /B 1
)
