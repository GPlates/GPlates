######################################################################################################
#                                                                                                    #
# Install the ${BUILD_TARGET} target (either 'gplates' or 'pygplates').                              #
#                                                                                                    #
# This also enables the target to run on *other* machines (rather than just the *build* machine)     #
# by copying dependency artifacts into the install location on Windows and macOS (this is not        #
# typically required on Linux systems which have binary package managers that install dependencies). #
#                                                                                                    #
######################################################################################################

include(GNUInstallDirs)

#
# See the top-level BUILD-Windows.md / BUILD-macOS.md / BUILD-Linux.md for how to configure, build and
# install 'gplates' or 'pygplates' (including the GPLATES_BUILD_GPLATES and GPLATES_INSTALL_STANDALONE
# options used below).
#
# For pyGPlates, the install phase is also used by scikit-build-core to create a Python wheel (see pyproject.toml).
#

#
# Check some requirements for installing targets.
#
if (GPLATES_INSTALL_STANDALONE)
    #
    # Install GPlates or pyGPlates as a standalone bundle (by copying dependency libraries during installation).
    #

    # On Apple, warn if a code signing identity has not been specified.
    #
    # This can avoid wasted time trying to notarize a package (created via cpack) only to fail because it was not code signed.
    if (APPLE)
        # Only need to code-sign GPlates (not pyGPlates).
        #
        # NOTE: Packaging pyGPlates with CPack is no longer used (users now install pyGPlates using conda or pip).
        #       So code-signing pyGPlates is no longer required (quarantine is handled by conda and pip package managers).
        if (GPLATES_BUILD_GPLATES)  # GPlates
            # Check at *install* time thus allowing users to build without a code signing identity
            # (if they just plan to run the build locally and don't plan to deploy to other machines).
            install(
                    CODE "
                        set(CODE_SIGN_IDENTITY [[${GPLATES_APPLE_CODE_SIGN_IDENTITY}]])
                        if (NOT CODE_SIGN_IDENTITY)
                            message(WARNING [[Code signing identity not specified - please set GPLATES_APPLE_CODE_SIGN_IDENTITY before distributing to other machines]])
                        endif()
                    "
            )
        endif()
    endif()

    # On Windows, require Qt6 when installing.
    #
    # With the official pre-built Qt binaries, the default graphics driver is:
    # - Qt5: OpenGL 2.1  (see https://doc.qt.io/qt-5/windows-requirements.html#graphics-drivers)
    # - Qt6: Direct3D 11 (see https://doc.qt.io/qt-6/windows-graphics.html)
    #
    # Since Qt5 defaults to OpenGL it will look for a desktop OpenGL driver and if that does not support OpenGL 2.1 then it will
    # fallback to the ANGLE or software DLLs (because the pre-built Qt binaries are configured for 'dynamic' OpenGL).
    # This fallback relies on the ANGLE or software DLLs being present. They are dynamically loaded, rather than being dynamically linked, and
    # so are not found by file(GET_RUNTIME_DEPENDENCIES) and so are not deployed. (However 'windeployqt' will include those ANGLE and software DLLs).
    # Qt6, on the other hand, uses Direct3D by default (instead of OpenGL) and also does not use ANGLE (as a fallback when using OpenGL; although it
    # still falls back to the software DLL). So we should not get an error that 'libEGL' or 'opengl32sw' DLLs cannot be loaded (due to not being present).
    #
    # So, on Windows, we require Qt6 (instead of Qt5) when installing GPlates/pyGPlates.
    if (WIN32)
        # Check at *install* time thus allowing users to build on Windows using Qt5 instead of Qt6
        # (if they just plan to run the build locally and don't plan to deploy to other machines).
        install(
                CODE "
                    set(QT_VERSION_MAJOR [[${QT_VERSION_MAJOR}]])
                    if (QT_VERSION_MAJOR LESS 6)
                        message(FATAL_ERROR [[Installing on Windows requires Qt6 or above (not Qt5)]])
                    endif()
                "
        )
    endif()

    # Installing Qt6 plugins does not work with Qt versions 6.0 - 6.3 (according to https://bugreports.qt.io/browse/QTBUG-94066).
    # Note that intalling Qt5 plugins is fine though.
    #
    # Check at *install* time thus allowing users to build with Qt versions 6.0 - 6.3
    # (if they just plan to run the build locally and don't plan to deploy to other machines).
    #
    # This should not affect Linux platforms since they are not usually built as standalone
    # (because the Linux binary package manager is used to install dependencies on the user's system).
    # If it is an issue then an option is to use Qt5 (instead of Qt6).
    #
    # For Windows and macOS this just means Qt 6.4 (or above) should be installed if you want to deploy.
    #
    # UPDATE: We only install Qt plugins for GPlates (not pyGPlates).
    #         This is because deployment for pyGPlates involves creating wheels and using auditwheel(manylinux)/delocate(macOS)/delvewheel(Windows)
    #         to check dependencies (manylinux), copy them into the wheel and (most importantly) give them unique names (to avoid conflicts).
    #         And auditwheel/delocate/delvewheel don't copy/fix dependencies of plugins.
    #         However, fortunately pyGPlates doesn't need the Qt plugins, so we'll leave them out (until/if this changes in the future).
    #         Also, it turns out the Qt plugins are not evening loading anyway (I think) because the pyGPlates module initialisation
    #         (in 'src/api/PyGPlatesModule.cc') does not create a QApplication, which uses 'qt.conf' (via QCoreApplication) to find the plugins.
    if (GPLATES_BUILD_GPLATES)  # GPlates ...
        install(
                CODE "
                    set(QT_VERSION_MAJOR [[${QT_VERSION_MAJOR}]])
                    set(QT_VERSION_MINOR [[${QT_VERSION_MINOR}]])
                    if (QT_VERSION_MAJOR EQUAL 6 AND QT_VERSION_MINOR LESS 4)
                        message(FATAL_ERROR [[Installing Qt6 plugins requires Qt version 6.4 or above]])
                    endif()
                "
        )
    endif()
endif()


#
# Install the gplates or pygplates target.
#
# Support all target configurations. Ie, no need for the CONFIGURATIONS option in install(TARGETS ...), which is equivalent to...
#
#   install(TARGETS gplates
#       CONFIGURATIONS Release RelWithDebInfo MinSizeRel Debug
#       RUNTIME ...
#       BUNDLE ...
#       LIBRARY ...)
#
# This applies to all install(TARGETS), not just this one.
# Note that if we only supported Release then we'd have to specify 'CONFIGURATIONS Release' for every install() command (not just TARGETS).
#
# Note: GPlates uses RUNTIME and BUNDLE entity types.
#       PyGPlates is a module library which always uses the LIBRARY entity type (according to CMake: "Module libraries are always treated as LIBRARY targets").
#
# Note: For standalone we want to bundle everything together so it's relocatable, and it's easier to place gplates or pygplates
#       in the base install directory (along with 'qt.conf', which has to be in the same directory).
#
if (GPLATES_BUILD_GPLATES)  # GPlates ...

    if (GPLATES_INSTALL_STANDALONE)
        #
        # For a standalone installation bundle everything together in the base install directory.
        #
        # For Windows this means 'gplates.exe' ultimately gets installed into, for example, "C:\Program Files\GPlates\GPlates 2.3.0"
        # instead of "C:\Program Files\GPlates\GPlates 2.3.0\bin". And we copy the dependency DLLs into the same directory as gplates (so it can find them).
        # For macOS this means you immediately see the app bundle in the base directory (rather than in a 'bin' sub-directory).
        # For Linux the standalone version is typically packaged as an archive (not a '.deb') and the extracted gplates executable will be immediately visible (in base directory).
        set(STANDALONE_BASE_INSTALL_DIR .)

        # Install the gplates target.
        install(TARGETS gplates
            RUNTIME # Windows and Linux
                DESTINATION ${STANDALONE_BASE_INSTALL_DIR}
            BUNDLE # Apple
                DESTINATION ${STANDALONE_BASE_INSTALL_DIR})
    else() # not standalone
        #
        # When not a standalone installation just use the standard install location ('bin').
        # For example, this would end up in '/usr/bin/' if no install prefix is specified (when running "cmake --install .")
        # since the default install prefix is '/usr'.
        #
        install(TARGETS gplates
            RUNTIME # Windows and Linux
                DESTINATION ${CMAKE_INSTALL_BINDIR}
            BUNDLE # Apple
                DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()

else()  # pyGPlates ...

    #
    # For 'pygplates' we install the pygplates module library into a 'pygplates/' sub-directory of the base directory since we are making
    # pygplates a "Python package" (with the pygplates module library in a 'pygplates/' directory as well as an '__init__.py').
    #
    # For a standalone installation this enables the pygplates module library, via code in '__init__.py', to find its runtime location
    # (needed to locate the GDAL/PROJ data bundled with pygplates).
    #
    # When not a standalone installation, GDAL/PROJ are installed in a standard location and so GDAL/PROJ are able to find their own data directories, which means
    # we don't need to bundle them up with pygplates. But we'll still retain the 'pygplates/' package directory (and '__init__.py') rather than leaving it as a
    # single pygplates shared library file (such as 'pygplates.so' or 'pygplates.pyd') in the base directory (ie, not in a 'pygplates/' sub-directory).
    #
    set(PYGPLATES_PYTHON_PACKAGE_DIR pygplates)
    #
    # When NOT building using scikit-build-core we install the 'pygplates' package into the 'lib/' sub-directory of the install prefix directory.
    # For example, this would end up in '/usr/lib/' if no install prefix is specified (when running "cmake --install .") since the default install prefix is '/usr'.
    #
    # Building using scikit-build-core happens when building wheels with pip (eg, 'pip wheel ...' or 'pip install ...').
    # And conda also builds using scikit-build-core (because conda relies on 'pip install').
    # In these cases we want to install the 'pygplates' package into the *base* directory (not 'lib/' sub-directory).
    # This ensures the 'pygplates' package ends up in the 'site-packages' directory of the Python installation (rather than a 'lib/' sub-directory).
    if (NOT SKBUILD)
        set(PYGPLATES_PYTHON_PACKAGE_DIR ${CMAKE_INSTALL_LIBDIR}/${PYGPLATES_PYTHON_PACKAGE_DIR})
    endif()

    if (GPLATES_INSTALL_STANDALONE)
        set(STANDALONE_BASE_INSTALL_DIR ${PYGPLATES_PYTHON_PACKAGE_DIR})
    endif()

    # Install the pygplates target.
    install(TARGETS pygplates
        LIBRARY # Windows, Apple and Linux
            DESTINATION ${PYGPLATES_PYTHON_PACKAGE_DIR})

    ########################################################################################################################
    # Create and install an "__init__.py" file for pygplates in same directory as the pygplates library (on all platforms) #
    ########################################################################################################################
    #
    # This is because pygplates is a "Python package" where the pygplates module library is in the *base* 'pygplates/' directory as well as '__init__.py'.
    set(PYGPLATES_INIT_PY "${CMAKE_CURRENT_BINARY_DIR}/__init__.py")
    #
    # Notes for the "__init__.py" source code:
    #
    # Previously, once we imported symbols from the pygplates shared library (C++) into the namespace of this package (also called pygplates),
    # we used to delete 'pygplates.pygplates' (after renaming it to something more private with a leading underscore).
    # However we no longer do this because the '__module__' attribute of objects in pygplates remain as 'pygplates.pygplates'
    # (since 'pygplates.{pyd,so}' is in 'pygplates/' package). And the '__module__' attribute is used during pickling
    # (at least 'dill' appears to use it), so deleting what it references interferes with pickling (at least for 'dill').
    #
    # This does mean we have both pygplates.<symbol> and pygplates.pygplates.<symbol>. The former being the public API.
    #
    # We could change the name of the module to '_pygplates' (so that we have pygplates.<symbol> and pygplates._pygplates.<symbol>) but it would
    # require a lot of potentially confusing changes. For example, pygplates tests run on the build target (rather than the installed Python package),
    # which would be '_pygplates'.{pyd,so}, and therefore the tests would need to 'import _pygplates' instead of 'import pygplates'.
    # Also GPlates embeds 'pygplates' (not '_pygplates') and so we'd need to use a module name of 'pygplates' when building GPlates
    # and '_pygplates' when building pyGPlates. So it's easier just to keep it as 'pygplates' (instead of '_pygplates').
    #
    # Note that we allow no indentation in the file content to avoid Python 'unexpected indent' errors.
    file(WRITE "${PYGPLATES_INIT_PY}" [[
# Import the pygplates shared library (C++).
from .pygplates import *
# Import any private symbols (with leading underscore).
from .pygplates import __version__
from .pygplates import __doc__

# Let the pygplates shared library (C++) know of its imported location.
import os.path
pygplates._post_import(os.path.dirname(__file__))
]])
    install(FILES "${PYGPLATES_INIT_PY}" DESTINATION ${PYGPLATES_PYTHON_PACKAGE_DIR})

endif()


#
# Install Python scripts (but only for the gplates target).
#
if (GPLATES_BUILD_GPLATES)  # GPlates ...
    foreach (_script )  # currently empty but we can add scripts here in the future if needed
        if (EXISTS "${PROJECT_SOURCE_DIR}/scripts/${_script}")
            if (GPLATES_INSTALL_STANDALONE)
                # For standalone we want to bundle everything together so it's relocatable.
                if (APPLE)
                    install(FILES "${PROJECT_SOURCE_DIR}/scripts/${_script}" DESTINATION ${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/Resources/scripts)
                else()
                    install(FILES "${PROJECT_SOURCE_DIR}/scripts/${_script}" DESTINATION ${STANDALONE_BASE_INSTALL_DIR}/scripts)
                endif()
            else()
                install(FILES "${PROJECT_SOURCE_DIR}/scripts/${_script}" DESTINATION share/gplates/scripts)
            endif()
        endif()
    endforeach()
endif()

# Install geodata if requested (but only for the gplates target).
#
# The variables GPLATES_INSTALL_GEO_DATA and GPLATES_INSTALL_GEO_DATA_DIR are cache variables that the user can set to control this.
#
if (GPLATES_BUILD_GPLATES)  # GPlates ...
    if (GPLATES_INSTALL_GEO_DATA)
        # Remove the trailing '/', if there is one, so that we can then
        # append a '/' in CMake's 'install(DIRECTORY ...)' which tells us:
        #
        #   "The last component of each directory name is appended to the destination directory but
        #    a trailing slash may be used to avoid this because it leaves the last component empty"
        #
        string(REGEX REPLACE "/+$" "" _SOURCE_GEO_DATA_DIR "${GPLATES_INSTALL_GEO_DATA_DIR}")

        #
        # Note: Depending on the installation location ${CMAKE_INSTALL_PREFIX} a path length limit might be
        #       exceeded since some of the geodata paths can be quite long, and combined with ${CMAKE_INSTALL_PREFIX}
        #       could, for example, exceed 260 characters (MAX_PATH) on Windows (eg, when creating an NSIS package).
        #       This can even happen on the latest Windows 10 with long paths opted in.
        #       For example, when packaging with NSIS you can get a geodata file with a path like the following:
        #           <build_dir>\_CPack_Packages\win64\NSIS\gplates_2.3.0_win64\GeoData\<geo_data_file>
        #       ...and currently <geo_data_file> can reach 160 chars, which when added to the middle part
        #       '\_CPack_Packages\...' of ~60 chars becomes ~220 chars leaving only 40 chars for <build_dir>.
        #
        #       Which means you'll need a build directory path that's under 40 characters long (which is pretty short).
        #       Something like "C:\gplates\build\trunk-py37\" (which is already 28 characters).
        #
        if (GPLATES_INSTALL_STANDALONE)
            # For standalone we want to bundle everything together so it's relocatable.
            install(DIRECTORY ${_SOURCE_GEO_DATA_DIR}/ DESTINATION ${STANDALONE_BASE_INSTALL_DIR}/GeoData)
        else()
            install(DIRECTORY ${_SOURCE_GEO_DATA_DIR}/ DESTINATION share/gplates/GeoData)
        endif()
    endif()
endif()

# Install Linux man page (but only for the gplates target).
if (GPLATES_BUILD_GPLATES)  # GPlates ...
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")  # Linux
        if (EXISTS "${PROJECT_SOURCE_DIR}/doc/gplates.1.gz")
            if (GPLATES_INSTALL_STANDALONE)
                # For standalone we want to bundle everything together so it's relocatable.
                install(FILES  "${PROJECT_SOURCE_DIR}/doc/gplates.1.gz" DESTINATION ${STANDALONE_BASE_INSTALL_DIR}/man1)
            else()
                install(FILES  "${PROJECT_SOURCE_DIR}/doc/gplates.1.gz" DESTINATION share/man/man1)
            endif()
        endif()
    endif()
endif()


#
# Whether to install GPlates (or pyGPlates) as a standalone bundle (by copying dependency libraries during installation).
#
# When GPLATES_INSTALL_STANDALONE is true then we install code to fix up GPlates (or pyGPlates) for deployment to another machine
# (which mainly involves copying dependency libraries into the install location, which subsequently gets packaged).
# When this is false then we don't install dependencies, instead only installing the GPlates executable (or pyGPlates library) and a few non-dependency items.
#
if (GPLATES_INSTALL_STANDALONE)
    #
    # Configure install code to fix up GPlates (or pyGPlates) for deployment to another machine.
    #
    # Note that we don't get Qt to deploy its libraries/plugins to our install location (using windeployqt/macdeployqt).
    # Instead we find the Qt library dependencies ourself and we explicitly list the Qt plugins we expect to use.
    # On macOS:   The Qt deployment tool 'macdeployqt' actually deploys more than just the Qt libraries/plugins (and the libraries they depend on).
    #             It also deploys all libraries that GPlates depends on (eg, Boost, GDAL, etc). But we're already doing this via file(GET_RUNTIME_DEPENDENCIES)
    #             and we're explicitly listing the Qt plugins. So we don't really need to use 'macdeployqt'.
    #
    
    #
    # Run the deployment code in the install prefix location.
    #
    # This consists of a bunch of "install(CODE ...)" commands to install code into "cmake_install.cmake" that CMake in turn
    # executes at 'install' time into the install prefix location ${CMAKE_INSTALL_PREFIX} (evaluated at 'install' time).
    #
    # Note: The command "install(CODE)" requires CMake 3.14 since we're using generator expressions in the code.
    #       And using FOLLOW_SYMLINK_CHAIN in file(INSTALL) requires CMake 3.15.
    #
    # Note: When using CODE with double quotes, as with install(CODE "<code>"), variable subsitution is *enabled*.
    #       So we use this when transferring variables.
    #       However when using square brackets, as with install(CODE [[<code>]]), variable subitition is *disabled* (as is escaping).
    #       So we use this for the bulk of code to avoid unwanted variable transfer (instead using variables defined at *install* time)
    #       and to avoid having to escape characters (like double quotes).
    #       An example of this is ${CMAKE_INSTALL_PREFIX} where we use square brackets (CODE [[<code>]]) to ensure it is expanded only at
    #       install time (not at configure time). This is important because it can be different. For example, at configure time it might
    #       default to "c:/Program Files/${PROJECT_NAME}", but when packaging, the install prefix will instead be the staging area.
    #


    if (GPLATES_BUILD_GPLATES)  # GPlates ...

        #######################################
        # Install the Python standard library #
        #######################################
        #
        # Note: The Python standard library is only installed for the 'gplates' target which has an embedded Python interpreter
        #       (not 'pygplates' which is imported into a Python interpreter on the user's system via 'import pygplates').
        #

        # Find the relative path from the Python prefix directory to the standard library directory.
        # We'll use this as the standard library install location relative to our install prefix.
        if (APPLE)
            # On Apple, Python may either be a framework (eg, MacPorts) or a normal prefix layout (eg, conda).
            if (GPLATES_PYTHON_STDLIB_DIR MATCHES "/Python\\.framework/")
                # Framework Python (eg, MacPorts). Later on, if we're also installing shared library dependencies,
                # we will also install the Python framework library itself (and its Resources directory).
                # Convert, for example, '/opt/local/Library/Frameworks/Python.framework/Versions/3.8/lib/python3.8' to
                # 'gplates.app/Contents/Frameworks/Python.framework/Versions/3.8/lib/python3.8'.
                string(REGEX REPLACE "^.*/(Python\\.framework/.*)$" "gplates.app/Contents/Frameworks/\\1" GPLATES_PYTHON_STDLIB_INSTALL_PREFIX ${GPLATES_PYTHON_STDLIB_DIR})
            else()
                # Non-framework Python (eg, conda). Install the standard library under 'gplates.app/Contents/Frameworks/'
                # using its path relative to the Python prefix (eg, 'gplates.app/Contents/Frameworks/lib/python3.14').
                # This must match GPLATES_STANDALONE_PYTHON_STDLIB_DIR in Config_h.cmake and the runtime location in
                # 'src/file-io/StandaloneBundle.cc'. The non-framework libpython itself is a regular '.dylib' and is
                # copied into 'Contents/MacOS/' via the shared-library-dependency install below.
                file(RELATIVE_PATH _python_stdlib_relative_to_prefix ${GPLATES_PYTHON_PREFIX_DIR} ${GPLATES_PYTHON_STDLIB_DIR})
                set(GPLATES_PYTHON_STDLIB_INSTALL_PREFIX gplates.app/Contents/Frameworks/${_python_stdlib_relative_to_prefix})
            endif()
        else() # Windows or Linux
            file(RELATIVE_PATH GPLATES_PYTHON_STDLIB_INSTALL_PREFIX ${GPLATES_PYTHON_PREFIX_DIR} ${GPLATES_PYTHON_STDLIB_DIR})
        endif()

        # Remove the trailing '/', if there is one, so that we can then
        # append a '/' in CMake's 'install(DIRECTORY ...)' which tells us:
        #
        #   "The last component of each directory name is appended to the destination directory but
        #    a trailing slash may be used to avoid this because it leaves the last component empty"
        #
        string(REGEX REPLACE "/+$" "" _PYTHON_STDLIB_DIR "${GPLATES_PYTHON_STDLIB_DIR}")
        # Install the Python standard library.
        install(DIRECTORY "${_PYTHON_STDLIB_DIR}/" DESTINATION ${STANDALONE_BASE_INSTALL_DIR}/${GPLATES_PYTHON_STDLIB_INSTALL_PREFIX})

        # On Windows there's also a 'DLLs/' sibling directory of the 'Lib/' directory.
        if (WIN32)
            get_filename_component(_PYTHON_DLLS_DIR "${_PYTHON_STDLIB_DIR}" DIRECTORY)
            set(_PYTHON_DLLS_DIR "${_PYTHON_DLLS_DIR}/DLLs")
            if (EXISTS "${_PYTHON_DLLS_DIR}")
                install(DIRECTORY "${_PYTHON_DLLS_DIR}/" DESTINATION ${STANDALONE_BASE_INSTALL_DIR}/DLLs)
            endif()
        endif()

    endif()

    #############################################################################################
    # Copy the Proj library data into standalone bundle (to avoid Proj error finding 'proj.db') #
    #############################################################################################
    #
    # Find the 'projinfo' command.
    find_program(PROJINFO_COMMAND "projinfo" PATHS ${PROJ_BINARY_DIRS})
    if (PROJINFO_COMMAND)
        # Run 'projinfo --searchpaths' to get a list of directories that Proj will look for resources in.
        # Note that 'projinfo' is new in Proj version 6.0 and the '--searchpaths' option is new in version 7.0.
        execute_process(COMMAND ${PROJINFO_COMMAND} --searchpaths
            RESULT_VARIABLE _projinfo_result
            OUTPUT_VARIABLE _projinfo_output
            ERROR_QUIET)
        if (NOT _projinfo_result)  # success
            # Convert 'projinfo' output to a list of lines - we do this by converting newlines to the list separator character ';'.
            string(REPLACE "\n" ";" _projinfo_search_paths "${_projinfo_output}")
            # Search each path for 'proj.db'.
            foreach(_projinfo_search_path ${_projinfo_search_paths})
                file(TO_CMAKE_PATH ${_projinfo_search_path} _projinfo_search_path)
                if (EXISTS "${_projinfo_search_path}/proj.db")
                    set(_proj_data_dir ${_projinfo_search_path})
                    break()
                endif()
            endforeach()
            if (NOT _proj_data_dir)
                message(WARNING "Found proj resource dirs but did not find 'proj.db' - proj library data will not be included in standalone bundle.")
            endif()
        else()
            message(WARNING "'projinfo' does not support '--searchpaths' option - likely using Proj version older than 7.0 - proj library data will not be included in standalone bundle.")
        endif()
    else()
        message(WARNING "Unable to find 'projinfo' command - likely using Proj version older than 6.0 - proj library data will not be included in standalone bundle.")
    endif()
    #
    # Install the Proj data.
    if (_proj_data_dir)
        # Remove the trailing '/', if there is one, so that we can then append a '/' in CMake's 'install(DIRECTORY ...)' which tells us:
        #   "The last component of each directory name is appended to the destination directory but
        #    a trailing slash may be used to avoid this because it leaves the last component empty"
        string(REGEX REPLACE "/+$" "" _proj_data_dir "${_proj_data_dir}")
        if (GPLATES_BUILD_GPLATES)  # GPlates ...
            if (APPLE)
                set(_proj_data_rel_base gplates.app/Contents/Resources/${GPLATES_STANDALONE_PROJ_DATA_DIR})
            else()
                set(_proj_data_rel_base ${GPLATES_STANDALONE_PROJ_DATA_DIR})
            endif()
        else()  # pyGPlates ...
            set(_proj_data_rel_base ${GPLATES_STANDALONE_PROJ_DATA_DIR})
        endif()
        install(DIRECTORY "${_proj_data_dir}/" DESTINATION ${STANDALONE_BASE_INSTALL_DIR}/${_proj_data_rel_base})
    endif()

    #################################################################################################
    # Copy the GDAL library data into standalone bundle to avoid GDAL error finding 'gcs.csv'       #
    # (which was moved into 'proj.db' for GDAL >= 2.5, but there's other GDAL data files to bundle) #
    #################################################################################################
    #
    if (WIN32)
        # The 'gdal-config' command is not available on Windows. Instead we're expected to use the GDAL_DATA environment variable.
        set(_gdal_data_dir $ENV{GDAL_DATA})
        if (NOT _gdal_data_dir)
            message(WARNING "GDAL_DATA environment variable not set - GDAL library data will not be included in standalone bundle.")
        endif()
    else() # Apple or Linux
        # Find the 'gdal-config' command (should be able to find via PATH environment variable).
        find_program(GDAL_CONFIG_COMMAND "gdal-config")
        if (GDAL_CONFIG_COMMAND)
            # Run 'gdal-config --datadir' to get directory that GDAL will look for resources in.
            execute_process(COMMAND ${GDAL_CONFIG_COMMAND} --datadir
                RESULT_VARIABLE _gdal_config_result
                OUTPUT_VARIABLE _gdal_config_output
                ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
            if (NOT _gdal_config_result)  # success
                set(_gdal_data_dir ${_gdal_config_output})
            else()
                message(WARNING "'gdal-config --datadir' failed - GDAL library data will not be included in standalone bundle.")
            endif()
        else()
            message(WARNING "Unable to find 'gdal-config' command - GDAL library data will not be included in standalone bundle.")
        endif()
    endif()
    #
    # Install the GDAL data.
    if (_gdal_data_dir)
        file(TO_CMAKE_PATH ${_gdal_data_dir} _gdal_data_dir)
        if (EXISTS "${_gdal_data_dir}")
            # Remove the trailing '/', if there is one, so that we can then append a '/' in CMake's 'install(DIRECTORY ...)' which tells us:
            #   "The last component of each directory name is appended to the destination directory but
            #    a trailing slash may be used to avoid this because it leaves the last component empty"
            string(REGEX REPLACE "/+$" "" _gdal_data_dir "${_gdal_data_dir}")
            if (GPLATES_BUILD_GPLATES)  # GPlates ...
                if (APPLE)
                    set(_gdal_data_rel_base gplates.app/Contents/Resources/${GPLATES_STANDALONE_GDAL_DATA_DIR})
                else()
                    set(_gdal_data_rel_base ${GPLATES_STANDALONE_GDAL_DATA_DIR})
                endif()
            else()  # pyGPlates ...
                set(_gdal_data_rel_base ${GPLATES_STANDALONE_GDAL_DATA_DIR})
            endif()
            install(DIRECTORY "${_gdal_data_dir}/" DESTINATION ${STANDALONE_BASE_INSTALL_DIR}/${_gdal_data_rel_base})
        else()
            message(WARNING "GDAL data directory \"${_gdal_data_dir}\" does not exist - GDAL library data will not be included in standalone bundle.")
        endif()
    endif()

    ##########################################################################################################
    # Copy the GDAL library plugins (eg, NetCDF) into standalone bundle (unless compiled into core library). #
    ##########################################################################################################
    #
    # Find the GDAL library plugins directory.
    if (WIN32)
        # The 'gdal-config' command is not available on Windows. Instead we're expected to use the GDAL_DRIVER_PATH environment variable.
        set(_gdal_plugins_dir $ENV{GDAL_DRIVER_PATH})
        if (NOT _gdal_plugins_dir)
            message(WARNING "GDAL_DRIVER_PATH environment variable not set - any GDAL library plugins not compiled into core library will not be included in standalone bundle.")
        endif()
    else() # Apple or Linux
        # Find the 'gdal-config' command (should be able to find via PATH environment variable).
        find_program(GDAL_CONFIG_COMMAND "gdal-config")
        if (GDAL_CONFIG_COMMAND)
            # Run 'gdal-config --prefix' to get directory that GDAL was install into.
            execute_process(COMMAND ${GDAL_CONFIG_COMMAND} --prefix
                RESULT_VARIABLE _gdal_config_result
                OUTPUT_VARIABLE _gdal_config_output
                ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
            if (NOT _gdal_config_result)  # success
                set(_gdal_home_dir ${_gdal_config_output})
            else()
                message(WARNING "'gdal-config --prefix' failed - any GDAL library plugins not compiled into core library will not be included in standalone bundle.")
            endif()
        else()
            message(WARNING "Unable to find 'gdal-config' command - any GDAL library plugins not compiled into core library will not be included in standalone bundle.")
        endif()

        # The GDAL plugins directory is a sub-directory of the GDAL home directory.
        # The exact location depends on the platform and how GDAL is installed.
        if (APPLE)
            # If the GDAL home directory is inside a framework then it has a different plugin path.
            if (_gdal_home_dir MATCHES "[^/]+\\.framework/")
                set(_gdal_plugins_dir ${_gdal_home_dir}/PlugIns)
            else()
                set(_gdal_plugins_dir ${_gdal_home_dir}/lib/gdalplugins)
            endif()
        else()  # Linux
            set(_gdal_plugins_dir ${_gdal_home_dir}/lib/gdalplugins)
        endif()
    endif()
    #
    if (_gdal_plugins_dir)
        file(TO_CMAKE_PATH ${_gdal_plugins_dir} _gdal_plugins_dir)
        if (EXISTS "${_gdal_plugins_dir}")
            # Remove the trailing '/' if there is one.
            string(REGEX REPLACE "/+$" "" _gdal_plugins_dir "${_gdal_plugins_dir}")
        else()
            # The GDAL plugins directory does not exist. It's possible the plugins were compiled into core GDAL library though.
            # For each specific plugin we later attempt to install we'll emit a message indicating it will not be included in standalone bundle.
        endif()
    endif()
    #
    # The GDAL 'plugins' install directory (relative to base install location).
    #
    if (GPLATES_BUILD_GPLATES)  # GPlates ...
        if (APPLE)
            # On macOS place in 'gplates.app/Contents/Resources/gdal_plugins/'.
            set(GDAL_PLUGINS_INSTALL_PREFIX "gplates.app/Contents/Resources/${GPLATES_STANDALONE_GDAL_PLUGINS_DIR}")
        else() # Windows or Linux
            # On Windows, and Linux, place in the 'gdal_plugins/' sub-directory of the directory containing the executable.
            set(GDAL_PLUGINS_INSTALL_PREFIX "${GPLATES_STANDALONE_GDAL_PLUGINS_DIR}")
        endif()
    else()  # pyGPlates ...
        set(GDAL_PLUGINS_INSTALL_PREFIX "${GPLATES_STANDALONE_GDAL_PLUGINS_DIR}")
    endif()
    #
    # Function to install a GDAL plugin. Call as...
    #
    #   install_gdal_plugin(gdal_plugin_short_name)
    #
    # ...and the full path to installed plugin file will be added to 'GDAL_PLUGINS_INSTALLED'.
    function(install_gdal_plugin gdal_plugin_short_name)
        # Get the source file location of the GDAL plugin.
        set(_gdal_plugin_path ${_gdal_plugins_dir}/gdal_${gdal_plugin_short_name})
        if (WIN32)
            set(_gdal_plugin_path ${_gdal_plugin_path}.dll)
        elseif (APPLE)
            set(_gdal_plugin_path ${_gdal_plugin_path}.dylib)
        else()  # Linux
            set(_gdal_plugin_path ${_gdal_plugin_path}.so)
        endif()

        if (NOT EXISTS "${_gdal_plugin_path}")
            # Report message at install time since it's not an error if plugin is compiled into core GDAL library.
            #
            # Report a message even though it looks like an error because it's provides a debug clue if GDAL plugins exist
            # (ie, are *not* compiled into the core GDAL library) but were nevertheless not found.
            install(CODE "message(\"GDAL plugin ${_gdal_plugin_path} not found, so not installed (might be compiled into core GDAL library though)\")")
            return()
        endif()

        # The plugin install directory (relative to ${CMAKE_INSTALL_PREFIX}).
        set(_install_gdal_plugin_dir ${STANDALONE_BASE_INSTALL_DIR}/${GDAL_PLUGINS_INSTALL_PREFIX})

        # Install the GDAL plugin.
        install(FILES "${_gdal_plugin_path}" DESTINATION ${_install_gdal_plugin_dir})

        # Extract plugin filename.
        get_filename_component(_gdal_plugin_file "${_gdal_plugin_path}" NAME)

        # Use square brackets to avoid evaluating ${CMAKE_INSTALL_PREFIX} at configure time (should be done at install time).
        string(CONCAT _installed_gdal_plugin
            [[${CMAKE_INSTALL_PREFIX}/]]
            ${_install_gdal_plugin_dir}/
            ${_gdal_plugin_file})

        # Add full path to installed plugin file to the plugin list.
        set(_installed_gdal_plugin_list ${GDAL_PLUGINS_INSTALLED})
        list(APPEND _installed_gdal_plugin_list "${_installed_gdal_plugin}")
        # Set caller's plugin list.
        set(GDAL_PLUGINS_INSTALLED ${_installed_gdal_plugin_list} PARENT_SCOPE)

        # Also record the *source* plugin file. We scan the source plugins (not the installed copies)
        # for their runtime dependencies, because some libraries (eg, from conda) use relative rpaths
        # (eg, '@loader_path/...') that only resolve at the source location, not the install location.
        set(_source_gdal_plugin_list ${GDAL_PLUGINS_SOURCE})
        list(APPEND _source_gdal_plugin_list "${_gdal_plugin_path}")
        set(GDAL_PLUGINS_SOURCE ${_source_gdal_plugin_list} PARENT_SCOPE)
    endfunction()
    #
    # Install the GDAL plugins (if not already compiled into the core GDAL library).
    #
    # Each installed plugin (full installed path) is added to GDAL_PLUGINS_INSTALLED (which is a list variable).
    # And each installed path has ${CMAKE_INSTALL_PREFIX} in it (to be evaluated at install time).
    # Later we will pass GDAL_PLUGINS_INSTALLED to file(GET_RUNTIME_DEPENDENCIES) to find its dependencies and install them also.
    #
    # UPDATE: We only install GDAL plugins for GPlates (not pyGPlates).
    #         This is because deployment for pyGPlates involves creating wheels and using auditwheel(manylinux)/delocate(macOS)/delvewheel(Windows)
    #         to check dependencies (manylinux), copy them into the wheel and (most importantly) give them unique names (to avoid conflicts).
    #         And auditwheel/delocate/delvewheel don't copy/fix dependencies of plugins.
    #         However, fortunately pyGPlates doesn't need the following GDAL drivers (plugins), so we'll leave them out (until/if this changes in the future).
    if (GPLATES_BUILD_GPLATES)  # GPlates ...
        # NetCDF plugin.
        install_gdal_plugin(netCDF)
    endif()


    #####################
    # Install "qt.conf" #
    #####################

    # Create the "qt.conf" file.
    set(QT_CONF_FILE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf")
    # We'll place the Qt plugins in the 'plugins/' sub-directory of the directory containing 'qt.conf'.
    set(QT_PLUGIN_DIR_BASENAME "plugins")
    file(WRITE "${QT_CONF_FILE}" "[Paths]\nPlugins = ${QT_PLUGIN_DIR_BASENAME}\n")

    # The "qt.conf" file currently only specifies location of Qt plugins.
    #
    # UPDATE: We only install Qt plugins for GPlates (not pyGPlates).
    #         This is because deployment for pyGPlates involves creating wheels and using auditwheel(manylinux)/delocate(macOS)/delvewheel(Windows)
    #         to check dependencies (manylinux), copy them into the wheel and (most importantly) give them unique names (to avoid conflicts).
    #         And auditwheel/delocate/delvewheel don't copy/fix dependencies of plugins.
    #         However, fortunately pyGPlates doesn't need the Qt plugins, so we'll leave them out (until/if this changes in the future).
    #         Also, it turns out the Qt plugins are not evening loading anyway (I think) because the pyGPlates module initialisation
    #         (in 'src/api/PyGPlatesModule.cc') does not create a QApplication, which is required to parse 'qt.conf' (via QCoreApplication).
    if (GPLATES_BUILD_GPLATES)  # GPlates ...
        # Install the "qt.conf" file for gplates.
        if (APPLE)
            # On macOS install into the bundle 'Resources' directory.
            install(FILES "${QT_CONF_FILE}" DESTINATION ${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/Resources)
        else() # Windows or Linux
            # On Windows and Linux install into same directory as executable.
            install(FILES "${QT_CONF_FILE}" DESTINATION ${STANDALONE_BASE_INSTALL_DIR})
        endif()
    endif()

    ######################
    # Install Qt plugins #
    ######################

    # The 'plugins' directory (relative to base install location).
    #
    if (GPLATES_BUILD_GPLATES)  # GPlates ...
        if (APPLE)
            # On macOS relative paths inside 'qt.conf' are relative to 'gplates.app/Contents/'.
            set(QT_PLUGINS_INSTALL_PREFIX "gplates.app/Contents/${QT_PLUGIN_DIR_BASENAME}")
        else() # Windows or Linux
            # On Windows, and Linux, relative paths inside 'qt.conf' are relative to the directory containing the executable.
            set(QT_PLUGINS_INSTALL_PREFIX "${QT_PLUGIN_DIR_BASENAME}")
        endif()
    else()  # pyGPlates ...
        set(QT_PLUGINS_INSTALL_PREFIX "${QT_PLUGIN_DIR_BASENAME}")
    endif()

    # Function to install a Qt plugin target. Call as...
    #
    #   install_qt_plugin(qt_plugin_target)
    #
    # ...and the full path to installed plugin file will be added to 'QT_PLUGINS_INSTALLED'.
    function(install_qt_plugin qt_plugin_target)
        # Get the target file location of the Qt plugin target.
        #
        # Note that we have access to Qt imported targets (like Qt${QT_VERSION_MAJOR}::QJpegPlugin) because we
        # (the file containing this code) gets included by 'src/CMakeLists.txt' (which has found
        # Qt and imported its targets) and so the Qt imported targets are visible to us.
        get_target_property(_qt_plugin_path "${qt_plugin_target}" LOCATION)

        if(EXISTS "${_qt_plugin_path}")
            # Extract plugin type (eg, 'imageformats') and filename.
            get_filename_component(_qt_plugin_file "${_qt_plugin_path}" NAME)
            get_filename_component(_qt_plugin_type "${_qt_plugin_path}" DIRECTORY)
            get_filename_component(_qt_plugin_type "${_qt_plugin_type}" NAME)

            # The plugin install directory (relative to ${CMAKE_INSTALL_PREFIX}).
            set(_install_qt_plugin_dir ${STANDALONE_BASE_INSTALL_DIR}/${QT_PLUGINS_INSTALL_PREFIX}/${_qt_plugin_type})

            # Install the Qt plugin.
            install(FILES "${_qt_plugin_path}" DESTINATION ${_install_qt_plugin_dir})

            # Use square brackets to avoid evaluating ${CMAKE_INSTALL_PREFIX} at configure time (should be done at install time).
            string(CONCAT _installed_qt_plugin
                [[${CMAKE_INSTALL_PREFIX}/]]
                ${_install_qt_plugin_dir}/
                ${_qt_plugin_file})

            # Add full path to installed plugin file to the plugin list.
            set(_installed_qt_plugin_list ${QT_PLUGINS_INSTALLED})
            list(APPEND _installed_qt_plugin_list "${_installed_qt_plugin}")
            # Set caller's plugin list.
            set(QT_PLUGINS_INSTALLED ${_installed_qt_plugin_list} PARENT_SCOPE)

            # Also record the *source* plugin file. We scan the source plugins (not the installed copies)
            # for their runtime dependencies, because some libraries (eg, from conda) use relative rpaths
            # (eg, '@loader_path/...') that only resolve at the source location, not the install location.
            set(_source_qt_plugin_list ${QT_PLUGINS_SOURCE})
            list(APPEND _source_qt_plugin_list "${_qt_plugin_path}")
            set(QT_PLUGINS_SOURCE ${_source_qt_plugin_list} PARENT_SCOPE)
        else()
            message(FATAL_ERROR "Qt plugin ${qt_plugin_target} not found")
        endif()
    endfunction()

    # Install Qt plugins.
    #
    # Each installed plugin (full installed path) is added to QT_PLUGINS_INSTALLED (which is a list variable).
    # And each installed path has ${CMAKE_INSTALL_PREFIX} in it (to be evaluated at install time).
    # Later we will pass QT_PLUGINS_INSTALLED to file(GET_RUNTIME_DEPENDENCIES) to find its dependencies and install them also.
    #
    # UPDATE: We only install Qt plugins for GPlates (not pyGPlates).
    #         This is because deployment for pyGPlates involves creating wheels and using auditwheel(manylinux)/delocate(macOS)/delvewheel(Windows)
    #         to check dependencies (manylinux), copy them into the wheel and (most importantly) give them unique names (to avoid conflicts).
    #         And auditwheel/delocate/delvewheel don't copy/fix dependencies of plugins.
    #         However, fortunately pyGPlates doesn't need the Qt plugins, so we'll leave them out (until/if this changes in the future).
    #         Also, it turns out the Qt plugins are not evening loading anyway (I think) because the pyGPlates module initialisation
    #         (in 'src/api/PyGPlatesModule.cc') does not create a QApplication, which uses 'qt.conf' (via QCoreApplication) to find the plugins.
    if (GPLATES_BUILD_GPLATES)  # GPlates ...
        # This works on Qt5.
        # But only works on Qt6 for versions 6.4 and above (according to https://bugreports.qt.io/browse/QTBUG-94066).
        #
        # This should not affect Linux platforms since they are not usually built as standalone
        # (because the Linux binary package manager is used to install dependencies on the user's system).
        # If it is an issue then an option is to use Qt5 (instead of Qt6).
        #
        # For Windows and macOS this just means Qt 6.4 (or above) should be installed if you want to deploy.
        if (QT_VERSION_MAJOR EQUAL 5 OR
            (QT_VERSION_MAJOR EQUAL 6 AND QT_VERSION_MINOR GREATER_EQUAL 4))
            # Install common platform *independent* plugins used by GPlates.
            # Note: This list was obtained by running the Qt deployment tool (windeployqt/macdeployqt) on GPlates (to see which plugins it deployed).
            install_qt_plugin(Qt${QT_VERSION_MAJOR}::QSvgIconPlugin)
            install_qt_plugin(Qt${QT_VERSION_MAJOR}::QGifPlugin)
            install_qt_plugin(Qt${QT_VERSION_MAJOR}::QICOPlugin)
            install_qt_plugin(Qt${QT_VERSION_MAJOR}::QJpegPlugin)
            install_qt_plugin(Qt${QT_VERSION_MAJOR}::QSvgPlugin)

            # Install platform *dependent* plugins used by GPlates.
            # Note: This list was obtained by running the Qt deployment tool (windeployqt/macdeployqt) on GPlates (to see which plugins it deployed).
            if (WIN32)
                install_qt_plugin(Qt${QT_VERSION_MAJOR}::QWindowsIntegrationPlugin)
                # The Windows Vista style plugin ('qwindowsvistastyle', target 'QWindowsVistaStylePlugin')
                # was replaced in Qt 6.7 by the modern Windows style plugin ('qmodernwindowsstyle',
                # target 'QModernWindowsStylePlugin'), which provides both the 'windowsvista' and
                # 'windows11' styles. The old target no longer exists in Qt 6.7+, so install whichever
                # style plugin target is available.
                if (TARGET Qt${QT_VERSION_MAJOR}::QModernWindowsStylePlugin)
                    install_qt_plugin(Qt${QT_VERSION_MAJOR}::QModernWindowsStylePlugin)
                else()
                    install_qt_plugin(Qt${QT_VERSION_MAJOR}::QWindowsVistaStylePlugin)
                endif()
            elseif (APPLE)
                install_qt_plugin(Qt${QT_VERSION_MAJOR}::QCocoaIntegrationPlugin)
                install_qt_plugin(Qt${QT_VERSION_MAJOR}::QMacStylePlugin)
            else() # Linux
                install_qt_plugin(Qt${QT_VERSION_MAJOR}::QXcbIntegrationPlugin)
                # The following plugins are needed otherwise GPlates generates the following error and then seg. faults:
                #  "QXcbIntegration: Cannot create platform OpenGL context, neither GLX nor EGL are enabled"
                # Actually installing only the Glx plugin solved the issue (on Ubuntu 20.04), but we'll also install Egl in case.
                install_qt_plugin(Qt${QT_VERSION_MAJOR}::QXcbGlxIntegrationPlugin)
                install_qt_plugin(Qt${QT_VERSION_MAJOR}::QXcbEglIntegrationPlugin)
            endif()
        endif()
    endif()


    ###################################################
    # Install dynamically linked dependency libraries #
    ###################################################
    # Only install shared library dependencies if requested.
    if (GPLATES_INSTALL_STANDALONE_SHARED_LIBRARY_DEPENDENCIES)
        include(InstallSharedLibraryDependencies)
    endif()

endif()  # GPLATES_INSTALL_STANDALONE
