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
# The following shows how to configure CMake for either 'gplates' or 'pygplates', build it and then
# install it into a 'staging' sub-directory. It assumes the source code is in a directory 'gplates-src'
# and that you are creating a sibling directory 'gplates-build' or 'pygplates-build' (or both).
#
# For 'gplates':
#
#   mkdir gplates-build  # You should now see 'gplates-src/' and 'gplates-build/' side-by-side
#   cd gplates-build
#   cmake -D GPLATES_BUILD_GPLATES:BOOL=TRUE ../gplates-src  # Note the TRUE for building gplates
#   cmake --build .
#   cmake --install . --prefix staging  # Should now have a 'gplates-build/staging/' directory
#
# For 'pygplates':
#
#   mkdir pygplates-build  # You should now see 'gplates-src/' and 'pygplates-build/' side-by-side
#   cd pygplates-build
#   cmake -D GPLATES_BUILD_GPLATES:BOOL=FALSE ../gplates-src  # Note the FALSE for building pygplates
#   cmake --build .
#   cmake --install . --prefix staging  # Should now have a 'pygplates-build/staging/' directory
#
# For GPlates, in most cases you wouldn't typically install directly like this. More likely you'd create a package
# using CPack (see Package.cmake) which will, in turn, install to its own staging area prior to creating a package.
# However for pyGPlates, we use the install phase to setup our staging area for creating a Python package using setuptools.
#

#
# Check some requirments for installing targets (such as minimum CMake version required).
#
if (GPLATES_INSTALL_STANDALONE)
    #
    # Install GPlates or pyGPlates as a standalone bundle (by copying dependency libraries during installation).
    #

    # On Apple, warn if a code signing identity has not been specified.
    #
    # This can avoid wasted time trying to notarize a package (created via cpack) only to fail because it was not code signed.
    if (APPLE)
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
        # When not a standalone installation just use the standard install locations ('bin' and 'lib').
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
    # For a standalone installation this enables the pygplates module library to find its runtime location (needed to locate the GDAL/PROJ data bundled with pygplates).
    #
    # When not a standalone installation, GDAL/PROJ are installed in a standard location and so GDAL/PROJ are able to find their own data directories, which means
    # we don't need to bundle them up with pygplates. But we'll still retain the 'pygplates/' package directory (and '__init__.py') rather than leaving it as a
    # single pygplates shared library file (such as 'pygplates.so' or 'pygplates.pyd') in the base directory (ie, not in a 'pygplates/' sub-directory).
    #
    set(PYGPLATES_PYTHON_PACKAGE_DIR pygplates)
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
    # Note that we allow no indentation in the file content to avoid Python 'unexpected indent' errors.
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
    foreach (_script hellinger.py hellinger_maths.py)
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
# On Windows and Apple this is *enabled* by default since we typically distribute a self-contained package to users on those systems.
# However this can be *disabled* for use cases such as creating a conda package (since conda manages dependency installation itself).
#
# On Linux this is *disabled* by default since we rely on the Linux binary package manager to install dependencies on the user's system
# (for example, we create a '.deb' package that only *lists* the dependencies, which are then installed on the target system if not already there).
# However this can be *enabled* for use cases such as creating a standalone bundle for upload to a cloud service (where it is simply extracted).
#
if (GPLATES_INSTALL_STANDALONE)
    #
    # Configure install code to fix up GPlates (or pyGPlates) for deployment to another machine.
    #
    # Note that we don't get Qt to deploy its libraries/plugins to our install location (using windeployqt/macdeployqt).
    # Instead we find the Qt library dependencies ourself and we explicitly list the Qt plugins we expect to use.
    # On Windows: The official pre-built Qt binaries are configured for 'dynamic' OpenGL (see https://doc.qt.io/qt-5/windows-requirements.html).
    #             This means the normal desktop OpenGL drivers will be used where sufficient, otherwise Qt falls back to ANGLE or software OpenGL.
    #             This fallback relies on the ANGLE or software DLLs being present. They are dynamically loaded, rather than being dynamically linked, and
    #             so are not found by file(GET_RUNTIME_DEPENDENCIES) and so are not deployed. However 'windeployqt' will include those ANGLE and software DLLs.
    #             But the fact that we're not using 'windeployqt' is fine because GPlates uses OpenGL 3.3 which is above what ANGLE and software supports
    #             and so Qt cannot fallback. Hence not deploying the ANGLE and software DLLs is not a problem. GPlates also tells Qt not to fall back by
    #             specifying the Qt::AA_UseDesktopOpenGL attribute (in the C++ code).
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

        #
        # Function to install the Python standard library.
        #
        # Note: The Python standard library is only installed for the 'gplates' target which has an embedded Python interpreter
        #       (not 'pygplates' which is imported into a Python interpreter on the user's system via 'import pygplates').
        #
        function(install_python_standard_library)
            if (APPLE)
                # On Apple we're expecting Python to be a framework. Ideally we should already have installed the Python framework
                # (ie, this function should be called after the frameworks have been installed), but this is not absolutely necessary
                # (since we could install the Python standard library first and then install the Python framework library/resources second).
                if (GPLATES_PYTHON_STDLIB_DIR MATCHES "/Python\\.framework/")
                    # Convert, for example, '/opt/local/Library/Frameworks/Python.framework/Versions/3.8/lib/python3.8' to
                    # 'gplates.app/Contents/Frameworks/Python.framework/Versions/3.8/lib/python3.8'.
                    string(REGEX REPLACE "^.*/(Python\\.framework/.*)$" "gplates.app/Contents/Frameworks/\\1" _PYTHON_STDLIB_INSTALL_DIR ${GPLATES_PYTHON_STDLIB_DIR})
                else()
                    message(FATAL_ERROR "Expected Python to be a framework")
                endif()
            else() # Windows or Linux
                # Find the relative path from the Python prefix directory to the standard library directory.
                # We'll use this as the standard library install location relative to our install prefix.
                file(RELATIVE_PATH _PYTHON_STDLIB_INSTALL_DIR ${GPLATES_PYTHON_PREFIX_DIR} ${GPLATES_PYTHON_STDLIB_DIR})
            endif()

            # Remove the trailing '/', if there is one, so that we can then
            # append a '/' in CMake's 'install(DIRECTORY ...)' which tells us:
            #
            #   "The last component of each directory name is appended to the destination directory but
            #    a trailing slash may be used to avoid this because it leaves the last component empty"
            #
            string(REGEX REPLACE "/+$" "" _PYTHON_STDLIB_DIR "${GPLATES_PYTHON_STDLIB_DIR}")
            # Install the Python standard library.
            install(DIRECTORY "${_PYTHON_STDLIB_DIR}/" DESTINATION ${STANDALONE_BASE_INSTALL_DIR}/${_PYTHON_STDLIB_INSTALL_DIR})

            # On Windows there's also a 'DLLs/' sibling directory of the 'Lib/' directory.
            if (WIN32)
                get_filename_component(_PYTHON_DLLS_DIR "${_PYTHON_STDLIB_DIR}" DIRECTORY)
                set(_PYTHON_DLLS_DIR "${_PYTHON_DLLS_DIR}/DLLs")
                if (EXISTS "${_PYTHON_DLLS_DIR}")
                    install(DIRECTORY "${_PYTHON_DLLS_DIR}/" DESTINATION ${STANDALONE_BASE_INSTALL_DIR}/DLLs)
                endif()
            endif()

            # On Apple there are some shared '.so' libraries in Python framework that need code signing.
            #
            # For example, there's a directory called, , 'Python.framework/Versions/3.8/lib/python3.8/lib-dynload/' that is in 'sys.path' and contains '.so' libraries.
            # There's also site packages (eg, in 'Python.framework/Versions/3.8/lib/python3.8/site-packages/' like NumPy that contain '.so' libraries.
            if (APPLE)
                # We need to codesign and secure timestamp these (otherwise Apple notarization fails).
                # So we use our codesign() function - it is defined later but that's fine since this code is not executed until after codesign() has been defined.
                install(
                    CODE "set(STANDALONE_BASE_INSTALL_DIR [[${STANDALONE_BASE_INSTALL_DIR}]])"
                    CODE "set(_PYTHON_STDLIB_INSTALL_DIR [[${_PYTHON_STDLIB_INSTALL_DIR}]])"
                    CODE [[
                        # Recursively search for '.so' files within the Python standard library.
                        file(GLOB_RECURSE _python_shared_libs "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${_PYTHON_STDLIB_INSTALL_DIR}/*.so")
                        foreach(_python_shared_lib ${_python_shared_libs})
                            codesign(${_python_shared_lib})
                        endforeach()
                    ]]
                )
            endif()
        endfunction()

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
    # Find the GDAL home directory (the GDAL plugins will be in a sub-directory depending on the platform).
    if (WIN32)
        # The 'gdal-config' command is not available on Windows. Instead we'll use the GDAL_HOME environment variable (that we asked the user to set).
        set(_gdal_home_dir $ENV{GDAL_HOME})
        if (NOT _gdal_home_dir)
            message(WARNING "GDAL_HOME environment variable not set - any GDAL library plugins not compiled into core library will not be included in standalone bundle.")
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
    endif()
    #
    if (_gdal_home_dir)
        file(TO_CMAKE_PATH ${_gdal_home_dir} _gdal_home_dir)
        if (EXISTS "${_gdal_home_dir}")
            # Remove the trailing '/' if there is one.
            string(REGEX REPLACE "/+$" "" _gdal_home_dir "${_gdal_home_dir}")
        else()
            message(WARNING "GDAL home directory \"${_gdal_home_dir}\" does not exist - any GDAL library plugins not compiled into core library will not be included in standalone bundle.")
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
        if (NOT EXISTS "${_gdal_home_dir}")
            return()
        endif()

        # Get the source file location of the GDAL plugin.
        if (WIN32)
            set(_gdal_plugin_path ${_gdal_home_dir}/bin/gdalplugins/gdal_${gdal_plugin_short_name}.dll)
        elseif (APPLE)
            # If the GDAL home directory is inside a framework then it has a different plugin path.
            if (_gdal_home_dir MATCHES "[^/]+\\.framework/")
                set(_gdal_plugin_path ${_gdal_home_dir}/PlugIns/gdal_${gdal_plugin_short_name}.dylib)
            else()
                set(_gdal_plugin_path ${_gdal_home_dir}/lib/gdalplugins/gdal_${gdal_plugin_short_name}.dylib)
            endif()
        else()  # Linux
            set(_gdal_plugin_path ${_gdal_home_dir}/lib/gdalplugins/gdal_${gdal_plugin_short_name}.so)
        endif()

        if (NOT EXISTS "${_gdal_plugin_path}")
            # Report message at install time since it's not an error if plugin is compiled into core GDAL library.
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
    endfunction()
    #
    # Install the GDAL plugins (if not already compiled into the core GDAL library).
    #
    # Each installed plugin (full installed path) is added to GDAL_PLUGINS_INSTALLED (which is a list variable).
    # And each installed path has ${CMAKE_INSTALL_PREFIX} in it (to be evaluated at install time).
    # Later we will pass GDAL_PLUGINS_INSTALLED to file(GET_RUNTIME_DEPENDENCIES) to find its dependencies and install them also.
    #
    # NetCDF plugin.
    install_gdal_plugin(netCDF)


    ###############################################
    # Install the Visual Studio runtime libraries #
    ###############################################
    #
    # Note that Qt5 should also be using the same runtime libraries because it should be using the same compiler
    # since, when installing Qt, we selected the pre-built components appropriate for our compiler.
    # For example, "MSVC 2015 64-bit" when compiling 64-bit using Visual Studio 2015.
    #
    if (MSVC)
        # CMake tells us for Visual Studio 2015 (and higher) this will: "install the Windows Universal CRT libraries for app-local deployment (e.g. to Windows XP)".
        #
        # I've verified that this copies all the DLLs in the "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt" directory of the Windows SDK
        # (see https://devblogs.microsoft.com/cppblog/introducing-the-universal-crt/).
        set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)

        set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
        include(InstallRequiredSystemLibraries)
        # Install the runtime libraries in same location as gplates.exe (or pygplates.pyd) so they can be found when executing gplates (or importing pygplates).
        install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION ${STANDALONE_BASE_INSTALL_DIR})
    endif()

    ##################################
    # Enable Windows DLL redirection #
    ##################################
    #
    # Ensure our installed dependency DLLs get loaded, not any DLL with the same module name that is already loaded into memory
    # (due to the DLL search order including something called "Loaded-module list").
    # See https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order
    #
    # We avoid this by using DLL redirection (which is higher in the DLL search order).
    # This involves installing an empty file called "gplate.exe.local" in the same folder as "gplate.exe".
    # See https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-redirection
    #
    # Note: It appears that simply copying "gplate.exe.local" to an existing GPlates installation *after* the installed "gplates.exe"
    #       has been run will not work (according to https://stackoverflow.com/a/52756644). This was verified by a user on the
    #       GPlates forum (see https://discourse.gplates.org/t/mouse-cursor-offset-in-gplates/735/9).
    #
    if (WIN32)
        if (GPLATES_BUILD_GPLATES)  # GPlates ...
            # Create an empty "gplates.exe.local" file.
            set(GPLATES_EXE_LOCAL_FILE "${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:${BUILD_TARGET}>.local")
            # Note: All configurations generate the same content (empty file) so can have the same output filename.
            file(GENERATE OUTPUT "${GPLATES_EXE_LOCAL_FILE}" CONTENT "")

            # Install "gplates.exe.local" into same directory as "gplates.exe".
            install(FILES "${GPLATES_EXE_LOCAL_FILE}" DESTINATION ${STANDALONE_BASE_INSTALL_DIR})
        endif()
    endif()

    #####################
    # Install "qt.conf" #
    #####################

    # Create the "qt.conf" file.
    set(QT_CONF_FILE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf")
    # We'll place the Qt plugins in the 'plugins/' sub-directory of the directory containing 'qt.conf'.
    set(QT_PLUGIN_DIR_BASENAME "plugins")
    file(WRITE "${QT_CONF_FILE}" "[Paths]\nPlugins = ${QT_PLUGIN_DIR_BASENAME}\n")

    if (GPLATES_BUILD_GPLATES)  # GPlates ...
        # Install the "qt.conf" file for gplates.
        if (APPLE)
            # On macOS install into the bundle 'Resources' directory.
            install(FILES "${QT_CONF_FILE}" DESTINATION ${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/Resources)
        else() # Windows or Linux
            # On Windows and Linux install into same directory as executable.
            install(FILES "${QT_CONF_FILE}" DESTINATION ${STANDALONE_BASE_INSTALL_DIR})
        endif()
    else()  # pyGPlates ...
        # Install the "qt.conf" file for pygplates in same directory as pygplates library (on all platforms).
        install(FILES "${QT_CONF_FILE}" DESTINATION ${STANDALONE_BASE_INSTALL_DIR})
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
    #   install_qt5_plugin(qt_plugin_target)
    #
    # ...and the full path to installed plugin file will be added to 'QT_PLUGINS_INSTALLED'.
    function(install_qt5_plugin qt_plugin_target)
        # Get the target file location of the Qt plugin target.
        #
        # Note that we have access to Qt imported targets (like Qt5::QJpegPlugin) because we
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
        else()
            message(FATAL_ERROR "Qt plugin ${qt_plugin_target} not found")
        endif()
    endfunction()

    # Each installed plugin (full installed path) is added to QT_PLUGINS_INSTALLED (which is a list variable).
    # And each installed path has ${CMAKE_INSTALL_PREFIX} in it (to be evaluated at install time).
    # Later we will pass QT_PLUGINS_INSTALLED to file(GET_RUNTIME_DEPENDENCIES) to find its dependencies and install them also.

    # Install common platform *independent* plugins (used by GPlates and pyGPlates).
    # Note: This list was obtained by running the Qt deployment tool (windeployqt/macdeployqt) on GPlates (to see which plugins it deployed).
    install_qt5_plugin(Qt5::QGenericEnginePlugin)
    install_qt5_plugin(Qt5::QSvgIconPlugin)
    install_qt5_plugin(Qt5::QGifPlugin)
    install_qt5_plugin(Qt5::QICOPlugin)
    install_qt5_plugin(Qt5::QJpegPlugin)
    install_qt5_plugin(Qt5::QSvgPlugin)
    # These are common to Windows and macOS only...
    if (WIN32 OR APPLE)
        install_qt5_plugin(Qt5::QICNSPlugin)
        install_qt5_plugin(Qt5::QTgaPlugin)
        install_qt5_plugin(Qt5::QTiffPlugin)
        install_qt5_plugin(Qt5::QWbmpPlugin)
        install_qt5_plugin(Qt5::QWebpPlugin)
    endif()

    # Install platform *dependent* plugins used by GPlates.
    if (GPLATES_BUILD_GPLATES)  # GPlates ...
        # Note: This list was obtained by running the Qt deployment tool (windeployqt/macdeployqt) on GPlates (to see which plugins it deployed).
        if (WIN32)
            install_qt5_plugin(Qt5::QWindowsIntegrationPlugin)
            install_qt5_plugin(Qt5::QWindowsVistaStylePlugin)
        elseif (APPLE)
            install_qt5_plugin(Qt5::QCocoaIntegrationPlugin)
            install_qt5_plugin(Qt5::QMacStylePlugin)
        else() # Linux
            install_qt5_plugin(Qt5::QXcbIntegrationPlugin)
            # The following plugins are needed otherwise GPlates generates the following error and then seg. faults:
            #  "QXcbIntegration: Cannot create platform OpenGL context, neither GLX nor EGL are enabled"
            # Actually installing only the Glx plugin solved the issue (on Ubuntu 20.04), but we'll also install Egl in case.
            install_qt5_plugin(Qt5::QXcbGlxIntegrationPlugin)
            install_qt5_plugin(Qt5::QXcbEglIntegrationPlugin)
        endif()
    endif()

    #######################################################
    # Install all dynamically linked dependency libraries #
    #######################################################

    # List platform-specific parameters to pass to 'file(GET_RUNTIME_DEPENDENCIES ...)'.
    #
    # Examples include...
    # List of regular expressions to exclude when searching runtime dependencies.
    # List of directories when searching.
    if (WIN32)
        # On Windows exclude 'api-ms-', 'System32' and 'SysWOW64'.
        # The necessary 'api-ms-win-*' get installed when installing the Windows Universal CRT libraries (using InstallRequiredSystemLibraries).
        list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[.*api-ms-win-.*]])
        list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[.*[/\\][Ss]ystem32[/\\].*]])
        list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[.*[/\\][Ss]ys[Ww][Oo][Ww]64[/\\].*]])
        # On Windows search for DLLs using the PATH environment variable.
        set(GET_RUNTIME_DEPENDENCIES_DIRECTORIES $ENV{PATH})
    elseif (APPLE)
        # On macOS exclude '/usr/lib' and '/System/Library'.
        # These should only contain system libraries (ie, should not contain any of our dependency libraries).
        list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[/usr/lib.*]])
        list(APPEND GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[/System/Library.*]])
    else() # Linux
        # On Linux don't exclude the standard library locations (eg, '/lib[64]' or '/usr/lib').
        # Our dependency libraries get installed there (by the binary package manager).
    endif()

    #
    # Find the dependency libraries.
    #
    # Note: file(GET_RUNTIME_DEPENDENCIES) requires CMake 3.16.
    #

    # The *build* target: executable (for gplates) or module library (for pygplates).
    install(CODE "set(_target_file \"$<TARGET_FILE:${BUILD_TARGET}>\")")

    install(
            CODE "set(GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES [[${GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES}]])"
            CODE "set(GET_RUNTIME_DEPENDENCIES_DIRECTORIES [[${GET_RUNTIME_DEPENDENCIES_DIRECTORIES}]])"
            # Note: Using \"${QT_PLUGINS_INSTALLED}\"" instead of [[${QT_PLUGINS_INSTALLED}]] because install code needs to evaluate
            #       ${CMAKE_INSTALL_PREFIX} (inside QT_PLUGINS_INSTALLED). And a side note, it does this at install time...
            CODE "set(QT_PLUGINS_INSTALLED \"${QT_PLUGINS_INSTALLED}\")"
            CODE "set(GDAL_PLUGINS_INSTALLED \"${GDAL_PLUGINS_INSTALLED}\")"
            CODE "set(GPLATES_BUILD_GPLATES [[${GPLATES_BUILD_GPLATES}]])"
            CODE [[
                unset(ARGUMENT_EXECUTABLES)
                unset(ARGUMENT_BUNDLE_EXECUTABLE)
                # Search the Qt and GDAL plugins regardless of whether installing gplates or pygplates.
                set(ARGUMENT_MODULES MODULES ${QT_PLUGINS_INSTALLED} ${GDAL_PLUGINS_INSTALLED})
                # Target 'gplates' is an executable and target 'pygplates' is a module.
                if (GPLATES_BUILD_GPLATES)  # GPlates ...
                    # Add gplates to the list of executables to search.
                    set(ARGUMENT_BUNDLE_EXECUTABLE BUNDLE_EXECUTABLE "${_target_file}")  # gplates
                    set(ARGUMENT_EXECUTABLES EXECUTABLES "${_target_file}")  # gplates
                else()  # pyGPlates ...
                    # Add pygplates to the list of modules to search.
                    set(ARGUMENT_MODULES ${ARGUMENT_MODULES} "${_target_file}")  # pygplates
                endif()
                
                # Only specify arguments to file(GET_RUNTIME_DEPENDENCIES) if we have them.
                # The arguments that might be empty are DIRECTORIES, PRE_EXCLUDE_REGEXES and POST_EXCLUDE_REGEXES.
                unset(ARGUMENT_DIRECTORIES)
                unset(ARGUMENT_PRE_EXCLUDE_REGEXES)
                unset(ARGUMENT_POST_EXCLUDE_REGEXES)
                if (GET_RUNTIME_DEPENDENCIES_DIRECTORIES)
                    set(ARGUMENT_DIRECTORIES DIRECTORIES ${GET_RUNTIME_DEPENDENCIES_DIRECTORIES})
                endif()
                if (GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES)
                    set(ARGUMENT_PRE_EXCLUDE_REGEXES PRE_EXCLUDE_REGEXES ${GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES})
                    set(ARGUMENT_POST_EXCLUDE_REGEXES POST_EXCLUDE_REGEXES ${GET_RUNTIME_DEPENDENCIES_EXCLUDE_REGEXES})
                endif()

                #
                # Search the *build* target, but we'll later install its dependencies into the *install* location.
                #
                file(GET_RUNTIME_DEPENDENCIES
                    ${ARGUMENT_EXECUTABLES}  # Evaluates to empty for pygplates.
                    # Also search the Qt plugins (since they're not discoverable because not dynamically linked)...
                    ${ARGUMENT_MODULES}  # Also includes pygplates (when installing pygplates).
                    ${ARGUMENT_BUNDLE_EXECUTABLE}  # Ignored on non-Apple platforms, and evaluates to empty for pygplates.
                    RESOLVED_DEPENDENCIES_VAR _resolved_dependencies
                    UNRESOLVED_DEPENDENCIES_VAR _unresolved_dependencies
                    CONFLICTING_DEPENDENCIES_PREFIX _conflicting_dependencies
                    ${ARGUMENT_DIRECTORIES}  # Can evaluate to empty.
                    ${ARGUMENT_PRE_EXCLUDE_REGEXES}  # Can evaluate to empty.
                    ${ARGUMENT_POST_EXCLUDE_REGEXES})  # Can evaluate to empty.

                # Fail if any unresolved/conflicting dependencies.
                if (_unresolved_dependencies)
                    message(FATAL_ERROR "There were unresolved dependencies of \"${_target_file}\":
                        ${_unresolved_dependencies}")
                endif()
                if (_conflicting_dependencies)
                    message(FATAL_ERROR "There were conflicting dependencies of \"${_target_file}\":
                        ${_conflicting_dependencies}")
                endif()
            ]]
    )


    #
    # Install the dependency libraries.
    #
    if (WIN32)

        # On Windows we simply copy the dependency DLLs to the install prefix location (where 'gplates.exe', or 'pygplates.pyd', is)
        # so that they will get found at runtime by virtue of being in the same directory.
        install(
                CODE "set(STANDALONE_BASE_INSTALL_DIR [[${STANDALONE_BASE_INSTALL_DIR}]])"
                CODE [[
                    # Install the dependency libraries in the *install* location.
                    foreach(_resolved_dependency ${_resolved_dependencies})
                        file(INSTALL "${_resolved_dependency}" DESTINATION "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}")
                    endforeach()
                ]]
        )

        # Install the Python standard library.
        #
        # Note: The Python standard library is only installed for the 'gplates' target which has an embedded Python interpreter
        #       (not 'pygplates' which is imported into a Python interpreter on the user's system via 'import pygplates').
        if (GPLATES_BUILD_GPLATES)  # GPlates ...
            install_python_standard_library()
        endif()

    elseif (APPLE)

        #
        # On macOS we need to:
        #   1 - Copy each resolved direct and indirect dependency of GPlates (or pyGPlates) and its Qt plugins into the appropriate location inside the
        #       installed GPlates application bundle (or base install directory of pyGPlates) depending of whether a regular '.dylib' or a framework.
        #   2 - Fix up the path to each *direct* dependency of GPlates (or pyGPlates), its Qt plugins and their resolved dependencies
        #       (ie, each dependency will depend on other dependencies in turn and must point to their location within the installation).
        #   3 - Code sign GPlates (or pyGPlates), its Qt plugins and their resolved dependencies with a valid Developer ID certificate.
        #       For GPlates we also then code sign the entire application *bundle* (for pyGPlates, the 'pygplates.so' library has already been signed).
        #

        # Find the 'codesign' command.
        find_program(CODESIGN "codesign")
        if (NOT CODESIGN)
            message(FATAL_ERROR "Unable to find 'codesign' command - cannot sign installed bundle with a Developer ID cerficate")
        endif()

        # Create an entitlements file for code signing.
        #
        # Apple notarization of Developer ID signed app bundles requires hardened runtime to succeed.
        # However that also seems to prevent using relative paths inside our bundle (such as "@executable_path/../Frameworks/...").
        # Currently we get around that by enabling the 'disable-library-validation' entitlement - although it would be better if we
        # didn't (because that enables unsigned libraries inside bundle to be loaded, although we do sign all ours).
        # Still, it's a bit of a security loophole.
        set(ENTITLEMENTS_FILE "${CMAKE_CURRENT_BINARY_DIR}/gplates.entitlements")
        file(WRITE "${ENTITLEMENTS_FILE}" [[
                <?xml version="1.0" encoding="UTF-8"?>
                <!DOCTYPE plist PUBLIC "-//Apple Computer/DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
                <plist version="1.0">
                <dict>
                    <key>com.apple.security.cs.disable-library-validation</key>
                    <true/>
                </dict>
                </plist>
            ]])
        
        #
        # Function to code sign an installed file/directory using a code signing identity (typically a Developer ID).
        install(
            CODE "set(CODESIGN [[${CODESIGN}]])"
            CODE "set(ENTITLEMENTS_FILE [[${ENTITLEMENTS_FILE}]])"
            # The CMake cache variable configured by the user to specify their code signing identity on macOS...
            CODE "set(CODE_SIGN_IDENTITY [[${GPLATES_APPLE_CODE_SIGN_IDENTITY}]])"
            CODE [[
                function(codesign installed_file)
                    # Only sign if a signing identity was provided.
                    if (CODE_SIGN_IDENTITY)
                        # Run 'codesign' to sign installed file/directory with a Developer ID certificate.
                        # Note that we need "--timestamp" to provide a secure timestamp, otherwise notarization will fail.
                        execute_process(
                            COMMAND ${CODESIGN} --timestamp --force --verify --options runtime --sign ${CODE_SIGN_IDENTITY}
                                    --entitlements ${ENTITLEMENTS_FILE} ${installed_file}
                            RESULT_VARIABLE _codesign_result
                            OUTPUT_VARIABLE _codesign_output
                            ERROR_VARIABLE _codesign_error)
                        if (_codesign_result)
                            message(FATAL_ERROR "${CODESIGN} failed: ${_codesign_error}")
                        endif()
                    endif()
                endfunction()
            ]]
        )

        # Copy each resolved dependency of GPlates (or pyGPlates) and its Qt plugins into the appropriate location inside the installed GPlates application bundle
        # (or base install directory of pygplates) depending of whether a regular '.dylib' or a framework.
        install(
                #
                # Function to install/copy the *framework* of a resolved dependency library into the installed bundle.
                #
                # A framework should look like the following:
                #
                #   Dependency.framework/
                #                        Dependency -> Versions/Current/Dependency
                #                        Resources  -> Versions/Current/Resources
                #                        Versions/
                #                                 Current -> 2
                #                                 2/
                #                                         Dependency
                #                                         Resources/
                #                                                       Info.plist
                #
                # We only copy the resolved dependency library itself (eg, 'Dependency.framework/Versions/2/Dependency') and the framework 'Resources' directory
                # (eg, 'Dependency.framework/Versions/2/Resources/'), while also setting up the symbolic links shown above if the framework is versioned
                # (eg, 'Dependency.framework/Dependency', 'Dependency.framework/Resources' and 'Dependency.framework/Versions/Current').
                #
                # The code in this function was inspired by the CMake 3.19 implementation of BundleUtilities.
                #
                CODE [[
                    function(install_framework resolved_dependency install_framework_prefix installed_dependency)
                        # Get the *install* dependency directory (within the install framework) to copy the resolved dependency library to.
                        # For example, convert '/.../Dependency.framework/Versions/2/Dependency' to '${install_framework_prefix}/Dependency.framework/Versions/2'.
                        string(REGEX REPLACE "^.*/([^/]+\\.framework/.+)/[^/]+$" "${install_framework_prefix}/\\1" _install_dependency_dir "${resolved_dependency}")

                        # Copy the resolved dependency library into the install dependency directory.
                        # For example, copy '/.../Dependency.framework/Versions/2/Dependency' to '${install_framework_prefix}/Dependency.framework/Versions/2'.
                        file(INSTALL "${resolved_dependency}" DESTINATION "${_install_dependency_dir}" FOLLOW_SYMLINK_CHAIN)

                        # Get the directory of the resolved dependency library.
                        # For example, convert '/.../Dependency.framework/Versions/2/Dependency' to '/.../Dependency.framework/Versions/2'.
                        string(REGEX REPLACE "^(.*)/[^/]+$" "\\1" _resolved_dependency_dir "${resolved_dependency}")

                        # If there's a 'Resources/' directory (in same directory as resolved dependency library) then
                        # copy it to the equivalent directory in the installed framework.
                        # For example, if there's a '/.../Dependency.framework/Versions/2/Resources' directory.
                        if (EXISTS "${_resolved_dependency_dir}/Resources")
                            # For example, copy '/.../Dependency.framework/Versions/2/Resources' to '${install_framework_prefix}/Dependency.framework/Versions/2'.
                            file(INSTALL "${_resolved_dependency_dir}/Resources" DESTINATION "${_install_dependency_dir}" FOLLOW_SYMLINK_CHAIN
                                    # Exclude any app bundles inside the 'Resources/' directory since otherwise these would also need to be code signed and
                                    # have a hardened runtime (for notarization) and have a secure timestamp (also for notarization).
                                    REGEX "[^/]+\\.app" EXCLUDE)
                        endif()

                        # See if there's a "Versions" directory in the framework.
                        # For example, convert '/.../Dependency.framework/Versions/2/Dependency' to 'Versions'.
                        string(REGEX REPLACE "^.*/([^/]+)/[^/]+/[^/]+$" "\\1" _versions_dir_basename "${resolved_dependency}")
                        if (_versions_dir_basename STREQUAL "Versions")
                            # _install_versions_dir = '${install_framework_prefix}/Dependency.framework/Versions'
                            string(REGEX REPLACE "^(.*)/[^/]+$" "\\1" _install_versions_dir "${_install_dependency_dir}")

                            # Create symbolic link (eg, '${install_framework_prefix}/Dependency.framework/Versions/Current' -> '2').
                            if (NOT EXISTS "${_install_versions_dir}/Current")
                                # Get the version directory (eg, "2" from '${install_framework_prefix}/Dependency.framework/Versions/2').
                                string(REGEX REPLACE "^.*/([^/]+)$" "\\1" _install_version_dir_basename "${_install_dependency_dir}")

                                # Create symbolic link.
                                # Note: file(CREATE_LINK) requires CMake 3.14.
                                file(CREATE_LINK "${_install_version_dir_basename}" "${_install_versions_dir}/Current" SYMBOLIC)
                            endif()

                            # Get '${install_framework_prefix}/Dependency.framework' from '${install_framework_prefix}/Dependency.framework/Versions/2'.
                            string(REGEX REPLACE "^(.*)/[^/]+/[^/]+$" "\\1" _install_framework_dir "${_install_dependency_dir}")

                            # Create symbolic link (eg, '${install_framework_prefix}/Dependency.framework/Resources' -> 'Versions/Current/Resources').
                            if (NOT EXISTS "${_install_framework_dir}/Resources")
                                # Create symbolic link.
                                # Note: file(CREATE_LINK) requires CMake 3.14.
                                file(CREATE_LINK "Versions/Current/Resources" "${_install_framework_dir}/Resources" SYMBOLIC)
                            endif()

                            # Get 'Dependency' from '/.../Dependency.framework/Versions/2/Dependency'.
                            string(REGEX REPLACE "^.*/([^/]+)$" "\\1" _dependency_basename "${resolved_dependency}")

                            # Create symbolic link (eg, '${install_framework_prefix}/Dependency.framework/Dependency' -> 'Versions/Current/Dependency').
                            if (NOT EXISTS "${_install_framework_dir}/${_dependency_basename}")
                                # Create symbolic link.
                                # Note: file(CREATE_LINK) requires CMake 3.14.
                                file(CREATE_LINK "Versions/Current/${_dependency_basename}" "${_install_framework_dir}/${_dependency_basename}" SYMBOLIC)
                            endif()
                        endif()

                        # Get '${install_framework_prefix}/Dependency.framework/Versions/2/Dependency' from '/.../Dependency.framework/Versions/2/Dependency'.
                        string(REGEX REPLACE "^.*/([^/]+\\.framework/.*)$" "${install_framework_prefix}/\\1" _installed_dependency "${resolved_dependency}")
                        # Set caller's 'installed_dependency'.
                        set(${installed_dependency} "${_installed_dependency}" PARENT_SCOPE)
                    endfunction()
                ]]
                #
                # Copy each resolved dependency of GPlates (or pyGPlates) and its Qt plugins into the appropriate location inside the installed GPlates application bundle
                # (or base install directory of pygplates) depending of whether a regular '.dylib' or a framework.
                #
                CODE "set(STANDALONE_BASE_INSTALL_DIR ${STANDALONE_BASE_INSTALL_DIR})"
                CODE "set(GPLATES_BUILD_GPLATES [[${GPLATES_BUILD_GPLATES}]])"
                CODE [[
                    set(_installed_dependencies)
                    foreach(_resolved_dependency ${_resolved_dependencies})
                        # If the resolved dependency is inside a framework then copy the framework into the GPlates bundle or pyGPlates install location
                        # (but only copy the resolved dependency library and the framework 'Resources/' directory).
                        if (_resolved_dependency MATCHES "[^/]+\\.framework/")
                            if (GPLATES_BUILD_GPLATES)  # GPlates ...
                                # Install in the 'Contents/Frameworks/' sub-directory of the 'gplates' app bundle.
                                install_framework(${_resolved_dependency} "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/Frameworks" _installed_dependency)
                            else()  # pyGPlates ...
                                # Install in the 'Frameworks/' sub-directory of base install directory of pygplates.
                                install_framework(${_resolved_dependency} "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/Frameworks" _installed_dependency)
                            endif()
                        else()  # regular '.dylib' ...
                            # Ensure we copy symlinks (using FOLLOW_SYMLINK_CHAIN). For example, with 'libCGAL.13.dylib -> libCGAL.13.0.3.dylib' both the symlink
                            # 'libCGAL.13.dylib' and the dereferenced library 'libCGAL.13.0.3.dylib' are copied, otherwise just the symlink would be copied.
                            if (GPLATES_BUILD_GPLATES)  # GPlates ...
                                # Install in the 'Contents/MacOS/' sub-directory of 'gplates' app bundle.
                                file(INSTALL "${_resolved_dependency}" DESTINATION "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/MacOS" FOLLOW_SYMLINK_CHAIN)
                            else()  # pyGPlates ...
                                # Install in the 'lib/' sub-directory of base install directory of pygplates.
                                file(INSTALL "${_resolved_dependency}" DESTINATION "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/lib" FOLLOW_SYMLINK_CHAIN)
                            endif()

                            if (GPLATES_BUILD_GPLATES)  # GPlates ...
                                # Get '${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/MacOS/dependency.dylib' from resolved dependency.
                                string(REGEX REPLACE "^.*/([^/]+)$" "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/MacOS/\\1" _installed_dependency "${_resolved_dependency}")
                            else()  # pyGPlates ...
                                # Get '${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/lib/dependency.dylib' from resolved dependency.
                                string(REGEX REPLACE "^.*/([^/]+)$" "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/lib/\\1" _installed_dependency "${_resolved_dependency}")
                            endif()
                        endif()

                        # Add installed dependency to the list.
                        list(APPEND _installed_dependencies "${_installed_dependency}")
                    endforeach()
                ]]
        )

        # Install the Python standard library.
        #
        # Note: The Python standard library is only installed for the 'gplates' target which has an embedded Python interpreter
        #       (not 'pygplates' which is imported into a Python interpreter on the user's system via 'import pygplates').
        if (GPLATES_BUILD_GPLATES)  # GPlates ...
            install_python_standard_library()
        endif()

        # Find the 'otool' command.
        find_program(OTOOL "otool")
        if (NOT OTOOL)
            message(FATAL_ERROR "Unable to find 'otool' command - cannot fix dependency paths to reference inside installation")
        endif()

        # Find the 'install_name_tool' command.
        find_program(INSTALL_NAME_TOOL "install_name_tool")
        if (NOT INSTALL_NAME_TOOL)
            message(FATAL_ERROR "Unable to find 'install_name_tool' command - cannot fix dependency paths to reference inside installation")
        endif()

        # Fix up the path to each *direct* dependency of GPlates (or pyGPlates), its Qt plugins and their installed dependencies.
        # Each dependency will depend on other dependencies in turn and must point to their location within the installation.
        # At the same time code sign GPlates (or pyGPlates), its Qt plugins and their installed dependencies with a valid Developer ID certificate.
        install(
                #
                # Function to find the relative path from the directory of the installed file to the specified installed dependency library.
                #
                # This is only needed for installing pyGPlates because it uses @loader_path which is different between the pygplates library,
                # its Qt plugins and dependency frameworks/libraries (whereas GPlates uses @executable_path which is fixed for all).
                # The returned relative path can then be used as '@loader_path/<relative_path>'.
                #
                CODE [[
                    function(get_relative_path_to_installed_dependency installed_file installed_dependency installed_dependency_relative_path)

                        get_filename_component(_installed_file_dir ${installed_file} DIRECTORY)

                        # Need to optionally convert relative paths to absolute paths (required by file(RELATIVE_PATH)) because it's possible that
                        # CMAKE_INSTALL_PREFIX (embedded in install paths) is a relative path (eg, 'staging' if installing with
                        # 'cmake --install . --prefix staging').
                        #
                        # Note that both the installed file and installed dependency will have paths starting with CMAKE_INSTALL_PREFIX so the
                        # relative path will be unaffected by whatever absolute prefix we use, so we don't need to specify BASE_DIR
                        # (it will default to 'CMAKE_CURRENT_SOURCE_DIR' which defaults to the current working directory when this
                        # install code is finally run in cmake script mode '-P' but, as mentioned, it doesn't matter what this is).
                        get_filename_component(_installed_file_dir ${_installed_file_dir} ABSOLUTE)
                        get_filename_component(installed_dependency ${installed_dependency} ABSOLUTE)

                        # Get the relative path.
                        file(RELATIVE_PATH _installed_dependency_relative_path ${_installed_file_dir} ${installed_dependency})

                        # Set caller's relative path.
                        set(${installed_dependency_relative_path} ${_installed_dependency_relative_path} PARENT_SCOPE)
                    endfunction()
                ]]

                CODE "set(OTOOL [[${OTOOL}]])"
                CODE "set(INSTALL_NAME_TOOL [[${INSTALL_NAME_TOOL}]])"
                CODE "set(STANDALONE_BASE_INSTALL_DIR ${STANDALONE_BASE_INSTALL_DIR})"
                CODE "set(GPLATES_BUILD_GPLATES [[${GPLATES_BUILD_GPLATES}]])"
                #
                # Function to change the dependency install names in the specified install file so that it references the
                # installed dependency file's location relative to the GPlates executable (or pyGPlates library).
                #
                CODE [[
                    function(fix_dependency_install_names installed_file)
                        # Run 'otool -L <installed-file>' to get a list of dependencies.
                        execute_process(
                            COMMAND ${OTOOL} -L ${installed_file}
                            RESULT_VARIABLE _otool_result
                            OUTPUT_VARIABLE _otool_output
                            ERROR_VARIABLE _otool_error)
                        if (_otool_result)
                            message(FATAL_ERROR "${OTOOL} failed: ${_otool_error}")
                        endif()

                        # Convert 'otool' output to a list of lines.
                        # We do this by converting newlines to the list separator character ';' but
                        # only after escaping any existing ';' characters in the output.
                        string(REPLACE ";" "\\;" _otool_output_lines "${_otool_output}")
                        string(REPLACE "\n" ";" _otool_output_lines "${_otool_output_lines}")
                    
                        # Extract a dependency from each line in the output.
                        set(_dependency_file_install_names)
                        foreach(_otool_output_line ${_otool_output_lines})
                            # Dependency lines follow the format " <install-name> (<versioning-info>)".
                            # Extract the <install-name> part.
                            # Lines containing dependencies look like:
                            #   dependency.dylib (compatibility version 0.0.0, current version 0.0.0)
                            # ...so we pattern match using the parentheses.
                            if (_otool_output_line MATCHES ".*\\(.*\\)")
                                string(REGEX REPLACE "^(.*)\\(.*\\).*$" "\\1" _dependency_file_install_name "${_otool_output_line}")
                                string(STRIP ${_dependency_file_install_name} _dependency_file_install_name)
                                if (_dependency_file_install_name)  # Might be that the last line is empty for some reason
                                    list(APPEND _dependency_file_install_names "${_dependency_file_install_name}")
                                endif()
                            endif()
                        endforeach()
                    
                        # Accumulate 'install_name_tool' options '-change <old> <new>' for each dependency.
                        set(_change_installed_dependency_file_install_names_options)
                        foreach(_dependency_file_install_name ${_dependency_file_install_names})

                            # Skip system libraries since these are also on the deployed system and
                            # hence their install name will still apply (on the deployed system).
                            if ((_dependency_file_install_name MATCHES "^/usr/lib") OR
                                (_dependency_file_install_name MATCHES "^/System"))
                                continue()
                            endif()

                            # Get the install name for the installed dependency (ie, the dependency location in the installed bundle).
                            #
                            # See if it's a framework.
                            if (_dependency_file_install_name MATCHES "[^/]+\\.framework/")
                                if (GPLATES_BUILD_GPLATES)  # GPlates ...
                                    # For example, "@executable_path/../Frameworks/Dependency.framework/Versions/2/Dependency".
                                    string(REGEX REPLACE "^.*/([^/]+\\.framework/.*)$" "@executable_path/../Frameworks/\\1"
                                            _installed_dependency_file_install_name "${_dependency_file_install_name}")
                                else()  # pyGPlates ...
                                    # For example, "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/Frameworks/Dependency.framework/Versions/2/Dependency".
                                    string(REGEX REPLACE "^.*/([^/]+\\.framework/.*)$" "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/Frameworks/\\1"
                                            _installed_dependency_file_name "${_dependency_file_install_name}")
                                    # Get the relative path from the installed file to its installed dependency.
                                    if (_installed_dependency_file_name STREQUAL _dependency_file_install_name)
                                        # string(REGEX REPLACE) did not find a match.
                                        # This can happen when the installed file references itself as a dependency.
                                        # In this case just use "@loader_path".
                                        set(_installed_dependency_file_install_name "@loader_path")
                                    else()
                                        # Get the path of installed dependency relative to the referencing installed file.
                                        get_relative_path_to_installed_dependency(
                                                ${installed_file} ${_installed_dependency_file_name} _relative_path_to_installed_dependency)
                                        set(_installed_dependency_file_install_name "@loader_path/${_relative_path_to_installed_dependency}")
                                    endif()
                                endif()
                            else()  # it's a regular shared library...
                                if (GPLATES_BUILD_GPLATES)  # GPlates ...
                                    # Non-framework librares are installed in same directory as executable.
                                    # For example, "@executable_path/../MacOS/dependency.dylib".
                                    string(REGEX REPLACE "^.*/([^/]+)$" "@executable_path/../MacOS/\\1"
                                            _installed_dependency_file_install_name "${_dependency_file_install_name}")
                                else()  # pyGPlates ...
                                    # Non-framework librares are installed in 'lib/' sub-directory of directory containing pygplates library.
                                    # For example, "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/lib/dependency.dylib".
                                    string(REGEX REPLACE "^.*/([^/]+)$" "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/lib/\\1"
                                            _installed_dependency_file_name "${_dependency_file_install_name}")
                                    # Get the relative path from the installed file to its installed dependency.
                                    if (_installed_dependency_file_name STREQUAL _dependency_file_install_name)
                                        # string(REGEX REPLACE) did not find a match.
                                        # This can happen when the installed file references itself as a dependency.
                                        # In this case just use "@loader_path".
                                        set(_installed_dependency_file_install_name "@loader_path")
                                    else()
                                        # Get the path of installed dependency relative to the referencing installed file.
                                        get_relative_path_to_installed_dependency(
                                                ${installed_file} ${_installed_dependency_file_name} _relative_path_to_installed_dependency)
                                        set(_installed_dependency_file_install_name "@loader_path/${_relative_path_to_installed_dependency}")
                                    endif()
                                endif()
                            endif()

                            # Add '-change <old> <new>' to the list of 'install_name_tool' options.
                            set(_change_installed_dependency_file_install_names_options ${_change_installed_dependency_file_install_names_options}
                                    -change "${_dependency_file_install_name}" "${_installed_dependency_file_install_name}")
                        endforeach()

                        # Run 'install_name_tool -change <installed-dependency-file-install-name> <installed-dependency-file> ... <installed-file>' .
                        execute_process(
                            COMMAND ${INSTALL_NAME_TOOL} ${_change_installed_dependency_file_install_names_options} ${installed_file}
                            RESULT_VARIABLE _install_name_tool_result
                            ERROR_VARIABLE _install_name_tool_error)
                        if (_install_name_tool_result)
                            message(FATAL_ERROR "${INSTALL_NAME_TOOL} failed: ${_install_name_tool_error}")
                        endif()

                        # Get the install name for the installed file itself (as opposed to its dependencies).
                        # For this we'll just use the basename since it's not needed at runtime to find dependency libraries.
                        string(REGEX REPLACE "^.*/([^/]+)$" "\\1" _installed_file_install_name "${installed_file}")

                        # Run 'install_name_tool -id <installed-file-install-name> <installed-file>'.
                        #
                        # Note: This does nothing for executables (eg, the 'gplates' executable).
                        #       For example, 'install_name_tool -id gplates ${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/MacOS/gplates'.
                        execute_process(
                            COMMAND ${INSTALL_NAME_TOOL} -id ${_installed_file_install_name} ${installed_file}
                            RESULT_VARIABLE _install_name_tool_result
                            ERROR_VARIABLE _install_name_tool_error)
                        if (_install_name_tool_result)
                            message(FATAL_ERROR "${INSTALL_NAME_TOOL} failed: ${_install_name_tool_error}")
                        endif()
                    endfunction()
                ]]

                # Note: Using \"${QT_PLUGINS_INSTALLED}\"" instead of [[${QT_PLUGINS_INSTALLED}]] because install code needs to evaluate
                #       ${CMAKE_INSTALL_PREFIX} (inside QT_PLUGINS_INSTALLED). And a side note, it does this at install time...
                CODE "set(QT_PLUGINS_INSTALLED \"${QT_PLUGINS_INSTALLED}\")"
                CODE "set(GDAL_PLUGINS_INSTALLED \"${GDAL_PLUGINS_INSTALLED}\")"
                CODE "set(GPLATES_BUILD_GPLATES [[${GPLATES_BUILD_GPLATES}]])"
                # The *build* target filename: executable (for gplates) or module library (for pygplates).
                CODE "set(_target_file_name \"$<TARGET_FILE_NAME:${BUILD_TARGET}>\")"
                #
                # Fix up the path to each *direct* dependency of GPlates (or pyGPlates), its Qt plugins and their installed dependencies.
                #
                # At the same time code sign GPlates (or pyGPlates), its Qt plugins and their installed dependencies with a valid Developer ID certificate (if available).
                #
                CODE [[
                    # Fix the dependency install names in each installed dependency.
                    foreach(_installed_dependency ${_installed_dependencies})
                        fix_dependency_install_names(${_installed_dependency})
                        # Sign *after* fixing dependencies (since we cannot modify after signing).
                        codesign(${_installed_dependency})
                    endforeach()

                    # Fix the dependency install names in each installed Qt plugin.
                    foreach(_qt_plugin ${QT_PLUGINS_INSTALLED} ${GDAL_PLUGINS_INSTALLED})
                        fix_dependency_install_names(${_qt_plugin})
                        # Sign *after* fixing dependencies (since we cannot modify after signing).
                        codesign(${_qt_plugin})
                    endforeach()

                    # And finally fix dependencies and code sign the GPlates application bundle (or the pyGPlates library).
                    #
                    if (GPLATES_BUILD_GPLATES)  # GPlates ...
                        # Fix the dependency install names in the installed gplates executable.
                        fix_dependency_install_names(${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app/Contents/MacOS/gplates)
                        # Sign *after* fixing dependencies (since we cannot modify after signing).
                        #
                        # NOTE: We sign the entire installed bundle (not just the 'gplates.app/Contents/MacOS/gplates' executable).
                        #       And this must be done as the last step.
                        codesign(${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/gplates.app)
                    else()  # pyGPlates ...
                        # Fix the dependency install names in the installed pygplates library.
                        fix_dependency_install_names(${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${_target_file_name})
                        # Sign *after* fixing dependencies (since we cannot modify after signing).
                        codesign(${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${_target_file_name})
                    endif()
                ]]
        )

    else()  # Linux

        #
        # On Linux we need to:
        #   1 - copy each resolved direct and indirect dependency library of GPlates (or pyGPlates) and its Qt plugins into the 'lib/' sub-directory of base install directory,
        #   2 - specify an appropriate RPATH for GPlates (or pyGPlates), its Qt plugins and their resolved dependencies
        #       (ie, each dependency will depend on other dependencies in turn and must point to their location, ie, in 'lib/').
        #

        install(
                #
                # Copy each resolved dependency of GPlates (or pyGPlates) and its Qt plugins into the 'lib/' sub-directory of base install directory.
                #
                # On Linux we simply copy the dependency shared libraries to the 'lib/' sub-directory of the
                # install prefix location so that they will get found at runtime from an RPATH of '$ORIGIN/lib' where $ORIGIN is
                # the location of the gplates executable (or pyGPlates library) in the base install directory.
                #
                CODE "set(STANDALONE_BASE_INSTALL_DIR ${STANDALONE_BASE_INSTALL_DIR})"
                CODE "set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})"
                CODE [[
                    set(_installed_dependencies)
                    foreach(_resolved_dependency ${_resolved_dependencies})
                        # Install into the 'lib/' sub-directory of base install directory.
                        # Ensure we copy symlinks (using FOLLOW_SYMLINK_CHAIN). For example, with 'libCGAL.13.so -> libCGAL.13.0.3.so' both the symlink
                        # 'libCGAL.13.so' and the dereferenced library 'libCGAL.13.0.3.so' are copied, otherwise just the symlink would be copied.
                        file(INSTALL "${_resolved_dependency}" DESTINATION "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/" FOLLOW_SYMLINK_CHAIN)

                        # Get '${CMAKE_INSTALL_PREFIX}/<base-install-dir>/lib/dependency.so' from resolved dependency.
                        string(REGEX REPLACE
                            "^.*/([^/]+)$"
                            "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/\\1"
                            _installed_dependency
                            "${_resolved_dependency}")

                        # Add installed dependency to the list.
                        list(APPEND _installed_dependencies "${_installed_dependency}")
                    endforeach()
                ]]
        )

        # Install the Python standard library.
        #
        # Note: The Python standard library is only installed for the 'gplates' target which has an embedded Python interpreter
        #       (not 'pygplates' which is imported into a Python interpreter on the user's system via 'import pygplates').
        if (GPLATES_BUILD_GPLATES)  # GPlates ...
            install_python_standard_library()
        endif()

        # Find the 'patchelf' command.
        find_program(PATCHELF "patchelf")
        if (NOT PATCHELF)
            message(FATAL_ERROR "Unable to find 'patchelf' command - cannot set RPATH - please install 'patchelf', for example 'sudo apt install patchelf' on Ubuntu")
        endif()

        install(
                CODE "set(PATCHELF [[${PATCHELF}]])"
                CODE "set(STANDALONE_BASE_INSTALL_DIR ${STANDALONE_BASE_INSTALL_DIR})"
                CODE "set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})"
                #
                # Function to set the RPATH of the specified installed file to '$ORIGIN/<relative-path-to-libs-dir>' so that it can
                # find its direct dependency libraries (in the 'lib/' sub-directory of the base install directory).
                #
                CODE [[
                    function(set_rpath installed_file)
                        #
                        # Find the relative path from the directory of the installed file to the directory where all the dependency libraries are installed.
                        #

                        get_filename_component(_installed_file_dir ${installed_file} DIRECTORY)

                        # Need to optionally convert relative paths to absolute paths (required by file(RELATIVE_PATH)) because it's possible that
                        # CMAKE_INSTALL_PREFIX is a relative path (eg, 'staging' if installing with 'cmake --install . --prefix staging').
                        #
                        # Note that both the installed file and installed libs will have paths starting with "${CMAKE_INSTALL_PREFIX}/<base-install-dir>"
                        # so the relative path will be unaffected by whatever absolute prefix we use, so we don't need to specify BASE_DIR
                        # (it will default to 'CMAKE_CURRENT_SOURCE_DIR' which defaults to the current working directory when this
                        # install code is finally run in cmake script mode '-P' but, as mentioned, it doesn't matter what this is).
                        get_filename_component(_installed_file_dir ${_installed_file_dir} ABSOLUTE)
                        get_filename_component(_installed_libs_dir "${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}" ABSOLUTE)

                        # Get the relative path to the 'libs' sub-directory of base install directory.
                        file(RELATIVE_PATH _relative_path_to_libs_dir "${_installed_file_dir}" "${_installed_libs_dir}")

                        #
                        # Run 'patchelf --set-rpath <rpath> <installed-file>' to set the required RPATH.
                        #
                        execute_process(
                            COMMAND ${PATCHELF} --set-rpath $ORIGIN/${_relative_path_to_libs_dir} ${installed_file}
                            RESULT_VARIABLE _patchelf_result
                            ERROR_VARIABLE _patchelf_error)
                        if (_patchelf_result)
                            message(FATAL_ERROR "${PATCHELF} failed: ${_patchelf_error}")
                        endif()
                    endfunction()
                ]]

                # Note: Using \"${QT_PLUGINS_INSTALLED}\"" instead of [[${QT_PLUGINS_INSTALLED}]] because install code needs to evaluate
                #       ${CMAKE_INSTALL_PREFIX} (inside QT_PLUGINS_INSTALLED). And a side note, it does this at install time...
                CODE "set(QT_PLUGINS_INSTALLED \"${QT_PLUGINS_INSTALLED}\")"
                CODE "set(GDAL_PLUGINS_INSTALLED \"${GDAL_PLUGINS_INSTALLED}\")"
                # The *build* target filename: executable (for gplates) or module library (for pygplates).
                CODE "set(_target_file_name \"$<TARGET_FILE_NAME:${BUILD_TARGET}>\")"
                #
                # Set the RPATH of gplates (or pygplates), its Qt plugins and their installed dependencies so that they all can find their direct dependencies.
                #
                # For example, gplates (or pygplates) needs to find its *direct* dependency libraries in 'lib/' and those dependencies need to find their *direct*
                # dependencies (also in 'lib/').
                #
                CODE [[
                    # Set the RPATH in each installed dependency.
                    foreach(_installed_dependency ${_installed_dependencies})
                        set_rpath(${_installed_dependency})
                    endforeach()

                    # Set the RPATH in each installed Qt plugin.
                    foreach(_qt_plugin ${QT_PLUGINS_INSTALLED} ${GDAL_PLUGINS_INSTALLED})
                        set_rpath(${_qt_plugin})
                    endforeach()

                    # Set the RPATH in the installed gplates executable (or pygplates library).
                    set_rpath(${CMAKE_INSTALL_PREFIX}/${STANDALONE_BASE_INSTALL_DIR}/${_target_file_name})
                ]]
        )
    endif()  # if (WIN32) ... elif (APPLE) ... else ...
endif()  # GPLATES_INSTALL_STANDALONE
